#pragma once

#include "SettingsDialog.h"
#include "StreamEngine.h"
#include "ThemeManager.h"

#include <QMainWindow>

class QPushButton;
class QPlainTextEdit;
class QStackedWidget;
class QLabel;

namespace lumen {

class PreviewWidget;
class AccentBadge;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(ThemeManager* theme, QWidget* parent = nullptr);

private slots:
    void onGoLiveClicked();
    void onSettingsClicked();
    void onStreamStarted();
    void onStreamStopped(int exitCode, QProcess::ExitStatus status);
    void onStreamLog(const QString& line);
    void onStreamError(const QString& message);

private:
    void buildSidebar();
    void buildStreamPage();
    void buildAboutPage();
    void selectNav(int index);
    void refreshStreamSummary();

    ThemeManager*  m_theme;
    StreamEngine*  m_engine;
    Settings       m_settings;

    QStackedWidget* m_pages = nullptr;
    QPushButton*    m_navStream  = nullptr;
    QPushButton*    m_navSettings = nullptr;
    QPushButton*    m_navAbout   = nullptr;

    PreviewWidget*  m_preview = nullptr;
    AccentBadge*    m_badge = nullptr;
    QLabel*         m_statusPill = nullptr;
    QLabel*         m_presetLabel = nullptr;
    QLabel*         m_bitrateLabel = nullptr;
    QPushButton*    m_goLiveButton = nullptr;
    QPlainTextEdit* m_logView = nullptr;
};

} // namespace lumen
