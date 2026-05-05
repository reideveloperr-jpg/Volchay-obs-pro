#include "StreamEngine.h"

#include "Devices.h"

#include <QStandardPaths>
#include <QStringBuilder>

namespace lumen {

namespace {

QString encoderName(Encoder e) {
    switch (e) {
        case Encoder::X264:       return QStringLiteral("libx264");
        case Encoder::NVENC_H264: return QStringLiteral("h264_nvenc");
        case Encoder::QSV_H264:   return QStringLiteral("h264_qsv");
        case Encoder::AMF_H264:   return QStringLiteral("h264_amf");
    }
    return QStringLiteral("libx264");
}

QString rateControlMode(RateControl rc, Encoder e) {
    // Map our enum to encoder-specific ffmpeg `-rc` flag values.
    switch (e) {
        case Encoder::X264:
            return rc == RateControl::CBR ? QStringLiteral("cbr") :
                   rc == RateControl::VBR ? QStringLiteral("vbr") :
                                            QStringLiteral("crf");
        case Encoder::NVENC_H264:
            return rc == RateControl::CBR ? QStringLiteral("cbr") :
                   rc == RateControl::VBR ? QStringLiteral("vbr_hq") :
                                            QStringLiteral("constqp");
        case Encoder::QSV_H264:
            return rc == RateControl::CBR ? QStringLiteral("cbr") :
                   rc == RateControl::VBR ? QStringLiteral("vbr") :
                                            QStringLiteral("icq");
        case Encoder::AMF_H264:
            return rc == RateControl::CBR ? QStringLiteral("cbr") :
                   rc == RateControl::VBR ? QStringLiteral("vbr_peak") :
                                            QStringLiteral("cqp");
    }
    return QStringLiteral("cbr");
}

// Build the ffmpeg arg list for the chosen video source. Returns an
// empty list and sets *failureReason if the source can't be expressed
// for the host OS (e.g. Window mode on Linux).
QStringList buildVideoInputArgs(const SourceConfig& sc, const StreamConfig& cfg,
                                QString* failureReason) {
    QStringList args;
    const QString fps = QString::number(cfg.fps);
    const QString sz  = QString("%1x%2").arg(cfg.widthPx).arg(cfg.heightPx);

    if (sc.videoMode == VideoSourceMode::TestPattern) {
        args << "-re"
             << "-f" << "lavfi"
             << "-i" << QString("testsrc2=size=%1:rate=%2").arg(sz, fps);
        return args;
    }

#if defined(Q_OS_WIN)
    if (sc.videoMode == VideoSourceMode::Window) {
        if (sc.windowTitle.trimmed().isEmpty()) {
            if (failureReason) *failureReason =
                QObject::tr("Не указан заголовок окна для захвата.");
            return {};
        }
        args << "-f" << "gdigrab"
             << "-framerate" << fps
             << "-i" << QString("title=%1").arg(sc.windowTitle);
        return args;
    }
    // Screen mode. gdigrab can target an arbitrary rectangle on the
    // virtual desktop using -offset_x/-offset_y; we use the geometry of
    // the chosen QScreen so multi-monitor selection works.
    {
        const auto screens = enumerateScreens();
        QStringList input;
        input << "-f" << "gdigrab" << "-framerate" << fps;
        if (sc.screenIndex >= 0 && sc.screenIndex < screens.size()) {
            const ScreenInfo& s = screens[sc.screenIndex];
            input << "-offset_x" << QString::number(s.x)
                  << "-offset_y" << QString::number(s.y)
                  << "-video_size" << QString("%1x%2").arg(s.w).arg(s.h);
        } else {
            input << "-video_size" << sz;
        }
        input << "-i" << "desktop";
        args << input;
        return args;
    }
#elif defined(Q_OS_MACOS)
    if (sc.videoMode == VideoSourceMode::Window) {
        if (failureReason) *failureReason =
            QObject::tr("Захват по заголовку окна на macOS пока не поддерживается.");
        return {};
    }
    // avfoundation indexes screens after the cameras; map screenIndex to
    // a positive offset from "1:" (first display). Users with weird
    // setups can still fall back to "Screen 0" via the default.
    args << "-f" << "avfoundation"
         << "-framerate" << fps
         << "-video_size" << sz
         << "-i" << QString::number(qMax(0, sc.screenIndex)) + ":";
    return args;
#else
    if (sc.videoMode == VideoSourceMode::Window) {
        if (failureReason) *failureReason =
            QObject::tr("Захват по заголовку окна на Linux/X11 не поддерживается.");
        return {};
    }
    // x11grab: aim at the geometry of the selected QScreen so multi-head
    // setups stream the right monitor instead of always grabbing :0.0+0,0.
    QString display = qEnvironmentVariable("DISPLAY", ":0.0");
    int x = 0, y = 0, w = cfg.widthPx, h = cfg.heightPx;
    const auto screens = enumerateScreens();
    if (sc.screenIndex >= 0 && sc.screenIndex < screens.size()) {
        const ScreenInfo& s = screens[sc.screenIndex];
        x = s.x; y = s.y; w = s.w; h = s.h;
    }
    args << "-f" << "x11grab"
         << "-framerate" << fps
         << "-video_size" << QString("%1x%2").arg(w).arg(h)
         << "-i" << QString("%1+%2,%3").arg(display).arg(x).arg(y);
    return args;
#endif
}

// Build the ffmpeg arg list for a single audio device. The SourceConfig
// fields tell us which platform backend to use.
QStringList buildSingleAudioInput(const QString& deviceId) {
    QStringList args;
#if defined(Q_OS_WIN)
    args << "-f" << "dshow" << "-i" << QString("audio=%1").arg(deviceId);
#elif defined(Q_OS_MACOS)
    // avfoundation address ":<idx>" means audio-only at index <idx>.
    args << "-f" << "avfoundation" << "-i" << QString(":%1").arg(deviceId);
#else
    args << "-f" << "pulse" << "-i" << deviceId;
#endif
    return args;
}

} // namespace

StreamEngine::StreamEngine(QObject* parent)
    : QObject(parent),
      m_proc(new QProcess(this)),
      m_terminateTimer(new QTimer(this)),
      m_killTimer(new QTimer(this)) {
    m_proc->setProgram(QStringLiteral("ffmpeg"));
    m_proc->setProcessChannelMode(QProcess::SeparateChannels);
    connect(m_proc, &QProcess::readyReadStandardError,
            this, &StreamEngine::onReadyReadStandardError);
    connect(m_proc, &QProcess::finished,
            this, &StreamEngine::onFinished);
    connect(m_proc, &QProcess::errorOccurred,
            this, &StreamEngine::onErrorOccurred);

    m_terminateTimer->setSingleShot(true);
    m_killTimer->setSingleShot(true);
    connect(m_terminateTimer, &QTimer::timeout, this, [this] {
        // ffmpeg ignored 'q'; ask it to exit politely.
        if (m_proc->state() == QProcess::Running) {
            m_proc->terminate();
            m_killTimer->start(2000);
        }
    });
    connect(m_killTimer, &QTimer::timeout, this, [this] {
        // Still alive after terminate(); fall back to SIGKILL.
        if (m_proc->state() == QProcess::Running) {
            m_proc->kill();
        }
    });
}

StreamEngine::~StreamEngine() {
    // Destructor must be synchronous so the QProcess doesn't outlive us;
    // a brief block here (only at app shutdown) is acceptable.
    m_terminateTimer->stop();
    m_killTimer->stop();
    if (m_proc->state() != QProcess::NotRunning) {
        m_proc->kill();
        m_proc->waitForFinished(2000);
    }
}

bool StreamEngine::isRunning() const {
    return m_proc->state() == QProcess::Running;
}

QStringList StreamEngine::buildFfmpegArgs(const StreamConfig& cfg,
                                          const StreamTarget& target,
                                          const SourceConfig& sources) {
    QStringList args;
    args << "-hide_banner" << "-loglevel" << "info";

    // ---- Inputs ----
    // Video first so it's stream index 0 for -map purposes.
    QStringList videoArgs = buildVideoInputArgs(sources, cfg, /*failureReason*/ nullptr);
    args << videoArgs;

    // Audio sources. Video is always input 0 so the first audio input is
    // always at index 1; later audio inputs follow at 2, 3, etc.
    int audioInputCount = 0;
    constexpr int firstAudioInputIndex = 1;
    auto addAudio = [&](const QStringList& inputArgs) {
        args << inputArgs;
        ++audioInputCount;
    };

    if (sources.videoMode == VideoSourceMode::TestPattern &&
        !sources.micEnabled && !sources.desktopAudioEnabled) {
        // Test pattern with no real audio source — supply a sine tone so
        // the FLV stream still has an audio track Twitch can chew on.
        QStringList toneArgs;
        toneArgs << "-f" << "lavfi"
                 << "-i" << QString("sine=frequency=440:sample_rate=%1")
                                 .arg(cfg.audioSampleRateHz);
        addAudio(toneArgs);
    }
    if (sources.micEnabled && !sources.micDeviceId.isEmpty()) {
        addAudio(buildSingleAudioInput(sources.micDeviceId));
    }
    if (sources.desktopAudioEnabled && !sources.desktopAudioDeviceId.isEmpty()) {
        addAudio(buildSingleAudioInput(sources.desktopAudioDeviceId));
    }

    // ---- Audio routing ----
    // 0 audio inputs → no audio in the output.
    // 1 audio input  → straight pass-through, AAC encode below.
    // 2+ audio inputs → mix them all into one track via amix.
    QString audioMap;  // what `-map` should reference for audio output
    if (audioInputCount == 0) {
        // no-op; we'll drop audio encoder args below
    } else if (audioInputCount == 1) {
        audioMap = QString::number(firstAudioInputIndex) + QStringLiteral(":a");
    } else {
        QString filter;
        for (int i = 0; i < audioInputCount; ++i) {
            filter += QString("[%1:a]").arg(firstAudioInputIndex + i);
        }
        filter += QString("amix=inputs=%1:duration=longest:dropout_transition=0[aout]")
                      .arg(audioInputCount);
        args << "-filter_complex" << filter;
        audioMap = QStringLiteral("[aout]");
    }

    // Always map the video stream from input 0.
    args << "-map" << QStringLiteral("0:v");
    if (!audioMap.isEmpty()) {
        args << "-map" << audioMap;
    }

    // ---- Video encoder ----
    args << "-c:v" << encoderName(cfg.encoder);
    if (cfg.encoder == Encoder::X264) {
        args << "-preset" << cfg.x264Preset
             << "-profile:v" << cfg.profile
             << "-tune" << "zerolatency";
    }
    args << "-pix_fmt" << "yuv420p"
         << "-b:v" << QString("%1k").arg(cfg.videoBitrateKbps)
         << "-g" << QString::number(cfg.fps * cfg.keyframeIntervalSec)
         << "-keyint_min" << QString::number(cfg.fps * cfg.keyframeIntervalSec)
         << "-r" << QString::number(cfg.fps);

    // Rate control. CBR pins maxrate=bitrate so the upload stays predictable
    // (Twitch's recommendation). VBR lets maxrate float a bit. CQP/CRF means
    // quality-target mode and we don't pin maxrate at all.
    const QString rcMode = rateControlMode(cfg.rateControl, cfg.encoder);
    if (cfg.encoder == Encoder::X264) {
        if (cfg.rateControl == RateControl::CBR) {
            // libx264 has no CBR flag — emulate via nal-hrd=cbr + maxrate=bitrate.
            args << "-maxrate" << QString("%1k").arg(cfg.videoBitrateKbps)
                 << "-bufsize" << QString("%1k").arg(cfg.videoBitrateKbps * 2)
                 << "-x264-params"
                 << QString("nal-hrd=cbr:keyint=%1:min-keyint=%1")
                        .arg(cfg.fps * cfg.keyframeIntervalSec);
        } else if (cfg.rateControl == RateControl::VBR) {
            args << "-maxrate" << QString("%1k").arg(cfg.videoBitrateKbps * 3 / 2)
                 << "-bufsize" << QString("%1k").arg(cfg.videoBitrateKbps * 2);
        } else { // CQP -> CRF for libx264
            args << "-crf" << QStringLiteral("23");
        }
    } else {
        // Hardware encoders take an explicit -rc flag.
        args << "-rc" << rcMode;
        if (cfg.rateControl == RateControl::CBR) {
            args << "-maxrate" << QString("%1k").arg(cfg.videoBitrateKbps)
                 << "-bufsize" << QString("%1k").arg(cfg.videoBitrateKbps * 2);
        } else if (cfg.rateControl == RateControl::VBR) {
            args << "-maxrate" << QString("%1k").arg(cfg.videoBitrateKbps * 3 / 2)
                 << "-bufsize" << QString("%1k").arg(cfg.videoBitrateKbps * 2);
        }
    }

    // ---- Audio encoder (only if we have an audio track) ----
    if (audioInputCount > 0) {
        args << "-c:a" << "aac"
             << "-b:a" << QString("%1k").arg(cfg.audioBitrateKbps)
             << "-ar" << QString::number(cfg.audioSampleRateHz)
             << "-ac" << "2";
    }

    // ---- Output (RTMP) ----
    QString rtmpFull = target.rtmpUrl;
    if (!rtmpFull.endsWith('/')) rtmpFull += '/';
    rtmpFull += target.streamKey;
    args << "-f" << "flv" << rtmpFull;

    return args;
}

void StreamEngine::start(const StreamConfig& cfg,
                         const StreamTarget& target,
                         const SourceConfig& sources) {
    if (isRunning()) {
        emit errorOccurred(tr("Поток уже запущен."));
        return;
    }
    if (target.streamKey.trimmed().isEmpty()) {
        emit errorOccurred(tr("Не задан Stream Key. Открой настройки и вставь ключ из dashboard.twitch.tv."));
        return;
    }
    // Surface platform mismatches (e.g. window-title capture on Linux)
    // before we hand argv to ffmpeg, so the user sees a useful message
    // instead of an opaque ffmpeg syntax error.
    QString reason;
    if (buildVideoInputArgs(sources, cfg, &reason).isEmpty()) {
        emit errorOccurred(reason.isEmpty()
            ? tr("Видео-источник не настроен.") : reason);
        return;
    }

    const QStringList args = buildFfmpegArgs(cfg, target, sources);
    // Mask the RTMP URL (last argument) before logging — it contains the
    // stream key and the log view is visible on screen.
    QStringList loggable = args;
    if (!loggable.isEmpty()) {
        loggable.last() = QStringLiteral("<rtmp-url-with-stream-key-hidden>");
    }
    emit logLine(QStringLiteral("$ ffmpeg ") + loggable.join(' '));
    m_proc->setArguments(args);
    // Suppress the QProcess::errorOccurred handler while waitForStarted
    // pumps the local event loop — otherwise a FailedToStart signal would
    // emit a generic "ffmpeg не найден" message AND we'd emit a second one
    // when waitForStarted returns false.
    m_swallowProcessErrors = true;
    m_proc->start();
    const bool ok = m_proc->waitForStarted(3000);
    m_swallowProcessErrors = false;
    if (!ok) {
        emit errorOccurred(tr("Не удалось запустить ffmpeg. Убедись что ffmpeg установлен и доступен в PATH."));
        return;
    }
    emit started();
}

void StreamEngine::stop() {
    if (!isRunning()) return;
    // ffmpeg traps 'q' on stdin to gracefully finalize the FLV/RTMP stream.
    // Drive the shutdown ladder via timers so we don't block the GUI thread.
    // onFinished() will cancel any pending timer once ffmpeg actually exits.
    m_proc->write("q\n");
    m_terminateTimer->start(3000);
}

void StreamEngine::onReadyReadStandardError() {
    const QByteArray chunk = m_proc->readAllStandardError();
    for (const QByteArray& line : chunk.split('\n')) {
        const QString s = QString::fromUtf8(line).trimmed();
        if (!s.isEmpty()) emit logLine(s);
    }
}

void StreamEngine::onFinished(int exitCode, QProcess::ExitStatus status) {
    m_terminateTimer->stop();
    m_killTimer->stop();
    emit stopped(exitCode, status);
}

void StreamEngine::onErrorOccurred(QProcess::ProcessError err) {
    if (m_swallowProcessErrors) return;
    QString msg;
    switch (err) {
        case QProcess::FailedToStart:
            msg = tr("ffmpeg не найден. Установи ffmpeg и добавь в PATH.");
            break;
        case QProcess::Crashed:
            msg = tr("ffmpeg аварийно завершился.");
            break;
        case QProcess::WriteError:
            msg = tr("Ошибка записи в stdin ffmpeg.");
            break;
        case QProcess::ReadError:
            msg = tr("Ошибка чтения stderr ffmpeg.");
            break;
        default:
            msg = tr("Ошибка процесса ffmpeg.");
            break;
    }
    emit errorOccurred(msg);
}

} // namespace lumen
