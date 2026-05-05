#include "StreamEngine.h"

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
            // h264_qsv: cbr / vbr / icq (quality target).
            return rc == RateControl::CBR ? QStringLiteral("cbr") :
                   rc == RateControl::VBR ? QStringLiteral("vbr") :
                                            QStringLiteral("icq");
        case Encoder::AMF_H264:
            // h264_amf: cbr / vbr_peak / cqp.
            return rc == RateControl::CBR ? QStringLiteral("cbr") :
                   rc == RateControl::VBR ? QStringLiteral("vbr_peak") :
                                            QStringLiteral("cqp");
    }
    return QStringLiteral("cbr");
}

QStringList platformScreenCaptureInputArgs(int fps, int w, int h) {
    QStringList args;
#if defined(Q_OS_WIN)
    args << "-f" << "gdigrab"
         << "-framerate" << QString::number(fps)
         << "-video_size" << QString("%1x%2").arg(w).arg(h)
         << "-i" << "desktop";
#elif defined(Q_OS_MACOS)
    args << "-f" << "avfoundation"
         << "-framerate" << QString::number(fps)
         << "-video_size" << QString("%1x%2").arg(w).arg(h)
         << "-i" << "1:0"; // first display, default audio
#else
    // Linux / X11
    const QString display = qEnvironmentVariable("DISPLAY", ":0.0");
    args << "-f" << "x11grab"
         << "-framerate" << QString::number(fps)
         << "-video_size" << QString("%1x%2").arg(w).arg(h)
         << "-i" << display;
#endif
    return args;
}

QStringList platformAudioCaptureInputArgs() {
    QStringList args;
#if defined(Q_OS_WIN)
    // dshow virtual audio device. The user is expected to install/select
    // a desktop-audio capture device; if not, the stream is video-only.
    args << "-f" << "dshow" << "-i" << "audio=virtual-audio-capturer";
#elif defined(Q_OS_MACOS)
    // Audio is already captured as part of avfoundation "1:0" above.
#else
    // Linux / PulseAudio (default monitor source).
    args << "-f" << "pulse" << "-i" << "default";
#endif
    return args;
}

} // namespace

StreamEngine::StreamEngine(QObject* parent)
    : QObject(parent), m_proc(new QProcess(this)) {
    m_proc->setProgram(QStringLiteral("ffmpeg"));
    m_proc->setProcessChannelMode(QProcess::SeparateChannels);
    connect(m_proc, &QProcess::readyReadStandardError,
            this, &StreamEngine::onReadyReadStandardError);
    connect(m_proc, &QProcess::finished,
            this, &StreamEngine::onFinished);
    connect(m_proc, &QProcess::errorOccurred,
            this, &StreamEngine::onErrorOccurred);
}

StreamEngine::~StreamEngine() {
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
                                          VideoSource source) {
    QStringList args;
    args << "-hide_banner" << "-loglevel" << "info";

    // ---- Input ----
    if (source == VideoSource::TestPattern) {
        args << "-re"
             << "-f" << "lavfi"
             << "-i" << QString("testsrc2=size=%1x%2:rate=%3")
                            .arg(cfg.widthPx).arg(cfg.heightPx).arg(cfg.fps)
             << "-f" << "lavfi"
             << "-i" << "sine=frequency=440:sample_rate="
                          + QString::number(cfg.audioSampleRateHz);
    } else {
        args << platformScreenCaptureInputArgs(cfg.fps, cfg.widthPx, cfg.heightPx);
        args << platformAudioCaptureInputArgs();
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
        if (cfg.rateControl == RateControl::CBR ||
            cfg.rateControl == RateControl::VBR) {
            args << "-maxrate" << QString("%1k").arg(cfg.videoBitrateKbps)
                 << "-bufsize" << QString("%1k").arg(cfg.videoBitrateKbps * 2);
        }
    }

    // ---- Audio encoder ----
    args << "-c:a" << "aac"
         << "-b:a" << QString("%1k").arg(cfg.audioBitrateKbps)
         << "-ar" << QString::number(cfg.audioSampleRateHz)
         << "-ac" << "2";

    // ---- Output (RTMP) ----
    QString rtmpFull = target.rtmpUrl;
    if (!rtmpFull.endsWith('/')) rtmpFull += '/';
    rtmpFull += target.streamKey;
    args << "-f" << "flv" << rtmpFull;

    return args;
}

void StreamEngine::start(const StreamConfig& cfg,
                         const StreamTarget& target,
                         VideoSource source) {
    if (isRunning()) {
        emit errorOccurred(tr("Поток уже запущен."));
        return;
    }
    if (target.streamKey.trimmed().isEmpty()) {
        emit errorOccurred(tr("Не задан Stream Key. Открой настройки и вставь ключ из dashboard.twitch.tv."));
        return;
    }

    const QStringList args = buildFfmpegArgs(cfg, target, source);
    // Mask the RTMP URL (last argument) before logging — it contains the
    // stream key and the log view is visible on screen.
    QStringList loggable = args;
    if (!loggable.isEmpty()) {
        loggable.last() = QStringLiteral("<rtmp-url-with-stream-key-hidden>");
    }
    emit logLine(QStringLiteral("$ ffmpeg ") + loggable.join(' '));
    m_proc->setArguments(args);
    m_proc->start();
    if (!m_proc->waitForStarted(3000)) {
        emit errorOccurred(tr("Не удалось запустить ffmpeg. Убедись что ffmpeg установлен и доступен в PATH."));
        return;
    }
    emit started();
}

void StreamEngine::stop() {
    if (!isRunning()) return;
    // ffmpeg traps 'q' on stdin to gracefully finalize the FLV/RTMP stream.
    m_proc->write("q\n");
    if (!m_proc->waitForFinished(3000)) {
        m_proc->terminate();
        if (!m_proc->waitForFinished(2000)) {
            m_proc->kill();
        }
    }
}

void StreamEngine::onReadyReadStandardError() {
    const QByteArray chunk = m_proc->readAllStandardError();
    for (const QByteArray& line : chunk.split('\n')) {
        const QString s = QString::fromUtf8(line).trimmed();
        if (!s.isEmpty()) emit logLine(s);
    }
}

void StreamEngine::onFinished(int exitCode, QProcess::ExitStatus status) {
    emit stopped(exitCode, status);
}

void StreamEngine::onErrorOccurred(QProcess::ProcessError err) {
    QString msg;
    switch (err) {
        case QProcess::FailedToStart:
            msg = tr("ffmpeg не найден. Установи ffmpeg и добавь в PATH.");
            break;
        case QProcess::Crashed:
            msg = tr("ffmpeg упал.");
            break;
        case QProcess::Timedout:
            msg = tr("ffmpeg не отвечает.");
            break;
        default:
            msg = tr("Ошибка ffmpeg: %1").arg(int(err));
    }
    emit errorOccurred(msg);
}

} // namespace lumen
