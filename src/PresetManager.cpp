#include "PresetManager.h"

#include <QCoreApplication>
#include <QObject>

namespace lumen {

namespace {

QVector<Preset> makePresets() {
    QVector<Preset> v;

    auto add = [&](const char* id,
                   const QString& name,
                   const QString& desc,
                   int w, int h, int fps, int vbr) {
        Preset p;
        p.id = QString::fromUtf8(id);
        p.displayName = name;
        p.description = desc;
        p.config.widthPx = w;
        p.config.heightPx = h;
        p.config.fps = fps;
        p.config.videoBitrateKbps = vbr;
        p.config.audioBitrateKbps = 160;
        p.config.audioSampleRateHz = 48000;
        p.config.keyframeIntervalSec = 2;
        p.config.encoder = Encoder::X264;
        p.config.rateControl = RateControl::CBR;
        p.config.x264Preset = QStringLiteral("veryfast");
        p.config.profile = QStringLiteral("high");
        v.push_back(p);
    };

    // Bitrates per Twitch broadcasting guidelines (CBR, x264, keyframe 2s).
    // Source: https://help.twitch.tv/s/article/broadcasting-guidelines
    // Only 720p and 1080p — Twitch resamples anything higher down to 1080p
    // unless Enhanced Broadcasting (HEVC/AV1) is enabled.
    add("twitch-720p30",  QObject::tr("Твич 720p30"),
        QObject::tr("1280×720, 30 fps, 3000 kbps — облегчённый поток"),
        1280, 720,  30, 3000);
    add("twitch-720p60",  QObject::tr("Твич 720p60"),
        QObject::tr("1280×720, 60 fps, 4500 kbps — официальный 720p60"),
        1280, 720,  60, 4500);
    add("twitch-1080p30", QObject::tr("Твич 1080p30"),
        QObject::tr("1920×1080, 30 fps, 4500 kbps — экономичный 1080p"),
        1920, 1080, 30, 4500);
    add("twitch-1080p60", QObject::tr("Твич 1080p60"),
        QObject::tr("1920×1080, 60 fps, 6000 kbps — рекомендуемый Twitch"),
        1920, 1080, 60, 6000);

    return v;
}

} // namespace

const QVector<Preset>& PresetManager::builtinPresets() {
    static const QVector<Preset> kPresets = makePresets();
    return kPresets;
}

const Preset* PresetManager::findById(const QString& id) {
    for (const auto& p : builtinPresets()) {
        if (p.id == id) return &p;
    }
    return nullptr;
}

const Preset* PresetManager::defaultPreset() {
    return findById(QStringLiteral("twitch-1080p60"));
}

} // namespace lumen
