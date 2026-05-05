#pragma once

#include "PresetManager.h"
#include "StreamEngine.h"
#include "ThemeManager.h"

#include <QDialog>

class QLineEdit;
class QSpinBox;
class QComboBox;
class QCheckBox;
class QRadioButton;
class QButtonGroup;
class QStackedWidget;

namespace lumen {

// Settings holds everything the streamer persists between launches and
// the StreamEngine needs at run time. It's deliberately a flat struct so
// QSettings serialization stays trivial and the same value can be passed
// across thread boundaries without ownership concerns.
struct Settings {
    StreamConfig  config;
    StreamTarget  target;
    SourceConfig  sources;
    QString       presetId = QStringLiteral("twitch-1080p60");
    Theme         theme    = Theme::Blackout;
    QColor        accent   = QColor(255, 153, 0); // amber-orange
    bool          rememberStreamKey = true;
};

// Modal preferences dialog. Tabbed: "Стрим" (preset + stream key),
// "Источники" (display/window selection + mic + desktop audio),
// "Кодировщик" (manual overrides), "Внешний вид" (theme + accent).
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(const Settings& initial,
                            ThemeManager* theme,
                            QWidget* parent = nullptr);

    Settings result() const { return m_settings; }

private slots:
    void onPresetChanged(int index);
    void onApplyPreset();
    void onPickAccent();
    void onRefreshDevices();
    void onVideoModeChanged();
    void accept() override;

private:
    void buildStreamTab(QWidget* tab);
    void buildSourcesTab(QWidget* tab);
    void buildEncoderTab(QWidget* tab);
    void buildAppearanceTab(QWidget* tab);
    void writeBackFromUi();
    void loadFromSettings();
    void applyConfigToInputs(const StreamConfig& cfg);
    void populateDeviceCombos();

    Settings        m_settings;
    ThemeManager*   m_theme;

    // Stream tab
    QComboBox*  m_presetCombo  = nullptr;
    QLineEdit*  m_rtmpEdit     = nullptr;
    QLineEdit*  m_streamKeyEdit = nullptr;
    QCheckBox*  m_rememberKey  = nullptr;

    // Sources tab — video
    QRadioButton* m_videoScreenRadio  = nullptr;
    QRadioButton* m_videoWindowRadio  = nullptr;
    QRadioButton* m_videoTestRadio    = nullptr;
    QButtonGroup* m_videoModeGroup    = nullptr;
    QStackedWidget* m_videoOptionsStack = nullptr;
    QComboBox*  m_screenCombo = nullptr;
    QLineEdit*  m_windowTitleEdit = nullptr;

    // Sources tab — audio
    QCheckBox*  m_micEnable = nullptr;
    QComboBox*  m_micCombo  = nullptr;
    QCheckBox*  m_desktopAudioEnable = nullptr;
    QComboBox*  m_desktopAudioCombo  = nullptr;

    // Encoder tab
    QSpinBox*   m_widthSpin    = nullptr;
    QSpinBox*   m_heightSpin   = nullptr;
    QSpinBox*   m_fpsSpin      = nullptr;
    QSpinBox*   m_videoBitrateSpin = nullptr;
    QSpinBox*   m_audioBitrateSpin = nullptr;
    QSpinBox*   m_keyframeSpin = nullptr;
    QComboBox*  m_encoderCombo = nullptr;
    QComboBox*  m_rateCombo    = nullptr;
    QComboBox*  m_x264PresetCombo = nullptr;

    // Appearance tab
    QComboBox*  m_themeCombo   = nullptr;
    QPushButton* m_accentButton = nullptr;
};

// Helpers to round-trip Settings through QSettings.
void saveSettings(const Settings& s);
Settings loadSettings();

} // namespace lumen
