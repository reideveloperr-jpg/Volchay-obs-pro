#pragma once

#include "PresetManager.h"

#include <QObject>
#include <QProcess>
#include <QString>
#include <QTimer>

namespace lumen {

// What kind of pixels the encoder consumes. Renamed from the original
// VideoSource so the new "Window" mode is part of a single enum.
enum class VideoSourceMode {
    Screen,        // gdigrab (Win), x11grab (Linux), avfoundation (macOS)
    Window,        // gdigrab title=<window-title> (Windows only)
    TestPattern,   // ffmpeg lavfi testsrc — useful to verify RTMP without real capture
};

// Aggregates the selectable inputs for a stream. Splitting microphone
// and desktop audio means the user can run with one, both, or neither.
// When both are enabled the StreamEngine mixes them with amix=inputs=2.
struct SourceConfig {
    // ---- Video ----
    VideoSourceMode videoMode    = VideoSourceMode::Screen;
    int             screenIndex  = 0;       // index into QGuiApplication::screens()
    QString         windowTitle;             // Window mode only

    // ---- Audio: microphone ----
    bool            micEnabled        = false;
    QString         micDeviceId;             // platform-specific device handle
    QString         micDeviceLabel;          // for UI / logging only

    // ---- Audio: desktop / system ----
    bool            desktopAudioEnabled  = false;
    QString         desktopAudioDeviceId;
    QString         desktopAudioDeviceLabel;
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
                                       const SourceConfig& sources);

public slots:
    void start(const StreamConfig& cfg,
               const StreamTarget& target,
               const SourceConfig& sources);
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
