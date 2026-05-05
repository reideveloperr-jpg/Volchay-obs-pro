#include "Devices.h"

#include <QGuiApplication>
#include <QProcess>
#include <QRect>
#include <QRegularExpression>
#include <QScreen>

namespace lumen {

namespace {

// Run a short-lived helper, capture its merged stdout+stderr, and return
// the text. Bounded by a 3 second wait so a misbehaving binary can't hang
// the GUI when the user just opened the Settings dialog.
QString runAndCapture(const QString& program, const QStringList& args) {
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start(program, args);
    if (!p.waitForStarted(1500)) return QString();
    p.waitForFinished(3000);
    return QString::fromUtf8(p.readAll());
}

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
// ffmpeg writes its device list to stderr in a free-form format. We parse
// the lines our backend cares about and ignore the rest. The exact regex
// targets the audio device blocks: lines that look like
//     [dshow @ ...]  "Microphone (Realtek)"
//     [AVFoundation indev @ ...] [0] Built-in Microphone
QStringList parseFfmpegDshowAudio(const QString& text) {
    QStringList out;
    bool audioBlock = false;
    const QStringList lines = text.split('\n');
    static const QRegularExpression nameRe(R"(\"([^\"]+)\")");
    for (const QString& raw : lines) {
        const QString l = raw.trimmed();
        if (l.contains("DirectShow audio devices", Qt::CaseInsensitive)) {
            audioBlock = true;
            continue;
        }
        if (l.contains("DirectShow video devices", Qt::CaseInsensitive)) {
            audioBlock = false;
            continue;
        }
        if (!audioBlock) continue;
        const auto m = nameRe.match(l);
        if (m.hasMatch() && !l.contains("Alternative name", Qt::CaseInsensitive)) {
            out << m.captured(1);
        }
    }
    return out;
}

QStringList parseFfmpegAvfoundationAudio(const QString& text) {
    QStringList out;
    bool audioBlock = false;
    const QStringList lines = text.split('\n');
    // Matches: "[AVFoundation indev @ 0x...] [0] Built-in Microphone"
    static const QRegularExpression entryRe(
        R"(\[\d+\]\s+(.+))");
    for (const QString& raw : lines) {
        const QString l = raw.trimmed();
        if (l.contains("AVFoundation audio devices", Qt::CaseInsensitive)) {
            audioBlock = true;
            continue;
        }
        if (l.contains("AVFoundation video devices", Qt::CaseInsensitive)) {
            audioBlock = false;
            continue;
        }
        if (!audioBlock) continue;
        const auto m = entryRe.match(l);
        if (m.hasMatch()) out << m.captured(1).trimmed();
    }
    return out;
}
#endif

} // namespace

QList<ScreenInfo> enumerateScreens() {
    QList<ScreenInfo> out;
    const auto screens = QGuiApplication::screens();
    for (int i = 0; i < screens.size(); ++i) {
        QScreen* s = screens[i];
        const QRect g = s->geometry();
        ScreenInfo info;
        info.index = i;
        info.x = g.x();
        info.y = g.y();
        info.w = g.width();
        info.h = g.height();
        QString name = s->name();
        if (name.isEmpty()) name = QStringLiteral("Display %1").arg(i + 1);
        info.label = QStringLiteral("%1 — %2×%3").arg(name).arg(g.width()).arg(g.height());
        out.push_back(info);
    }
    return out;
}

#if defined(Q_OS_WIN)

QList<AudioDevice> enumerateMicrophones() {
    QList<AudioDevice> out;
    const QString text = runAndCapture(
        QStringLiteral("ffmpeg"),
        {"-hide_banner", "-list_devices", "true", "-f", "dshow", "-i", "dummy"});
    for (const QString& name : parseFfmpegDshowAudio(text)) {
        // Heuristic: skip the loopback device used for desktop audio so the
        // mic list doesn't double-list it.
        if (name.compare(QStringLiteral("virtual-audio-capturer"),
                         Qt::CaseInsensitive) == 0) continue;
        AudioDevice d;
        d.id = name;
        d.label = name;
        out.push_back(d);
    }
    return out;
}

QList<AudioDevice> enumerateDesktopAudio() {
    QList<AudioDevice> out;
    // The OBS-bundled "virtual-audio-capturer" or the screen-capture
    // recorder's loopback device is the canonical Windows option. We list
    // it whether or not the user has it installed; if they don't, ffmpeg
    // will fail at start time with a clear error.
    AudioDevice d;
    d.id = QStringLiteral("virtual-audio-capturer");
    d.label = QStringLiteral("virtual-audio-capturer (Screen Capture Recorder)");
    out.push_back(d);

    // Also surface any other dshow audio devices the user might want to
    // designate as desktop audio (e.g. a Voicemeeter virtual cable).
    const QString text = runAndCapture(
        QStringLiteral("ffmpeg"),
        {"-hide_banner", "-list_devices", "true", "-f", "dshow", "-i", "dummy"});
    for (const QString& name : parseFfmpegDshowAudio(text)) {
        if (name.compare(QStringLiteral("virtual-audio-capturer"),
                         Qt::CaseInsensitive) == 0) continue;
        AudioDevice extra;
        extra.id = name;
        extra.label = name;
        out.push_back(extra);
    }
    return out;
}

#elif defined(Q_OS_MACOS)

QList<AudioDevice> enumerateMicrophones() {
    QList<AudioDevice> out;
    const QString text = runAndCapture(
        QStringLiteral("ffmpeg"),
        {"-hide_banner", "-f", "avfoundation", "-list_devices", "true", "-i", ""});
    int idx = 0;
    for (const QString& name : parseFfmpegAvfoundationAudio(text)) {
        AudioDevice d;
        d.id = QString::number(idx++);
        d.label = name;
        out.push_back(d);
    }
    return out;
}

QList<AudioDevice> enumerateDesktopAudio() {
    // macOS has no first-class loopback API; users typically install
    // BlackHole or Soundflower, which then appear as regular audio
    // devices. List the same set as microphones with a note.
    QList<AudioDevice> out = enumerateMicrophones();
    for (auto& d : out) {
        d.label = QStringLiteral("(loopback) %1").arg(d.label);
    }
    return out;
}

#else // Linux / X11 + PulseAudio

QList<AudioDevice> enumerateMicrophones() {
    QList<AudioDevice> out;
    const QString text = runAndCapture(QStringLiteral("pactl"),
                                       {"list", "sources", "short"});
    if (text.isEmpty()) {
        // pactl missing or PulseAudio not running — give the user the
        // implicit "default" so the dialog is still usable.
        AudioDevice d;
        d.id = QStringLiteral("default");
        d.label = QStringLiteral("default (PulseAudio default source)");
        out.push_back(d);
        return out;
    }
    const QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        const QStringList cols = line.split('\t');
        if (cols.size() < 2) continue;
        const QString name = cols[1];
        // Monitor sources are desktop-audio loopbacks, not real mics.
        if (name.endsWith(QStringLiteral(".monitor"))) continue;
        AudioDevice d;
        d.id = name;
        d.label = name;
        out.push_back(d);
    }
    return out;
}

QList<AudioDevice> enumerateDesktopAudio() {
    QList<AudioDevice> out;
    const QString text = runAndCapture(QStringLiteral("pactl"),
                                       {"list", "sources", "short"});
    const QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        const QStringList cols = line.split('\t');
        if (cols.size() < 2) continue;
        const QString name = cols[1];
        if (!name.endsWith(QStringLiteral(".monitor"))) continue;
        AudioDevice d;
        d.id = name;
        d.label = name;
        out.push_back(d);
    }
    return out;
}

#endif

} // namespace lumen
