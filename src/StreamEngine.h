#pragma once

#include "PresetManager.h"

#include <QObject>
#include <QProcess>
#include <QString>

namespace lumen {

// Source of the video frames to stream.
enum class VideoSource {
    Screen,        // Full screen capture (gdigrab on Win, x11grab on Linux, avfoundation on macOS)
    TestPattern,   // ffmpeg lavfi testsrc — useful for verifying RTMP without real capture
};

struct StreamTarget {
    // Twitch ingest URL — see https://help.twitch.tv/s/twitch-ingest-recommendation
    QString rtmpUrl  = QStringLiteral("rtmp://live.twitch.tv/app");
    QString streamKey;
};

// Wraps a single ffmpeg child process that pushes RTMP to Twitch.
// All ffmpeg argv generation, process lifecycle, log scraping, and
// status signalling lives here so the UI layer never touches QProcess.
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
                                       VideoSource source);

public slots:
    void start(const StreamConfig& cfg,
               const StreamTarget& target,
               VideoSource source);
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
};

} // namespace lumen
