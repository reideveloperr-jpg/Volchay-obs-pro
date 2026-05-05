#pragma once

#include "PresetManager.h"
#include "Source.h"

#include <QObject>
#include <QProcess>
#include <QString>
#include <QTimer>

namespace lumen {

struct StreamTarget {
    // Twitch ingest URL — see https://help.twitch.tv/s/twitch-ingest-recommendation
    QString rtmpUrl  = QStringLiteral("rtmp://live.twitch.tv/app");
    QString streamKey;
};

// Wraps a single ffmpeg child process that pushes RTMP to Twitch.
// All ffmpeg argv generation, process lifecycle, log scraping, and
// status signalling lives here so the UI layer never touches QProcess.
//
// In Phase 1 the engine streams a single video source at a time: the
// first enabled video entry in `SourceList` is sent over the wire, and
// every enabled audio entry is mixed into one track via amix.
class StreamEngine : public QObject {
    Q_OBJECT
public:
    explicit StreamEngine(QObject* parent = nullptr);
    ~StreamEngine() override;

    bool isRunning() const;

    // Builds the argv that will be passed to ffmpeg. Public so the UI
    // can show a "preview command" and so it can be unit-tested.
    static QStringList buildFfmpegArgs(const StreamConfig& cfg,
                                       const StreamTarget& target,
                                       const SourceList& sources,
                                       QString* failureReason = nullptr);

public slots:
    void start(const StreamConfig& cfg,
               const StreamTarget& target,
               const SourceList& sources);
    void stop();

signals:
    void started();
    void stopped(int exitCode, QProcess::ExitStatus status);
    void logLine(const QString& line);
    void errorOccurred(const QString& message);

private slots:
    void onReadyReadStandardError();
    void onFinished(int exitCode, QProcess::ExitStatus status);
    void onErrorOccurred(QProcess::ProcessError err);

private:
    QProcess* m_proc;
    // Async shutdown ladder: q\n -> terminate -> kill, driven by these
    // single-shot timers so the GUI thread never blocks on waitForFinished.
    QTimer* m_terminateTimer;
    QTimer* m_killTimer;
    // Set while waitForStarted() is running. onErrorOccurred() suppresses
    // its own emission during this window so start() can produce a single,
    // deduplicated error message.
    bool m_swallowProcessErrors = false;
};

} // namespace lumen
