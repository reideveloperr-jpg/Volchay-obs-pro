#include "SettingsDialog.h"

#include "Devices.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTabWidget>
#include <QVBoxLayout>

namespace lumen {

namespace {

// QSettings keys live in one place so a typo in one accessor doesn't
// silently desynchronize from another.
constexpr auto K_PRESET    = "stream/presetId";
constexpr auto K_RTMP      = "stream/rtmpUrl";
constexpr auto K_KEY       = "stream/streamKey";
constexpr auto K_REMEMBER  = "stream/rememberStreamKey";

constexpr auto K_VIDEO_MODE     = "sources/videoMode";
constexpr auto K_SCREEN_INDEX   = "sources/screenIndex";
constexpr auto K_WINDOW_TITLE   = "sources/windowTitle";
constexpr auto K_MIC_ENABLED    = "sources/micEnabled";
constexpr auto K_MIC_DEVICE     = "sources/micDevice";
constexpr auto K_DESKAUD_ENABLED = "sources/desktopAudioEnabled";
constexpr auto K_DESKAUD_DEVICE  = "sources/desktopAudioDevice";

constexpr auto K_W         = "encoder/widthPx";
constexpr auto K_H         = "encoder/heightPx";
constexpr auto K_FPS       = "encoder/fps";
constexpr auto K_VBR       = "encoder/videoBitrateKbps";
constexpr auto K_ABR       = "encoder/audioBitrateKbps";
constexpr auto K_AR        = "encoder/audioSampleRateHz";
constexpr auto K_KEYINT    = "encoder/keyframeIntervalSec";
constexpr auto K_ENC       = "encoder/encoder";
constexpr auto K_RC        = "encoder/rateControl";
constexpr auto K_X264P     = "encoder/x264Preset";
constexpr auto K_PROFILE   = "encoder/profile";

constexpr auto K_THEME     = "ui/theme";
constexpr auto K_ACCENT    = "ui/accent";

} // namespace

void saveSettings(const Settings& s) {
    QSettings q;
    q.setValue(K_PRESET,   s.presetId);
    q.setValue(K_RTMP,     s.target.rtmpUrl);
    if (s.rememberStreamKey) q.setValue(K_KEY, s.target.streamKey);
    else q.remove(K_KEY);
    q.setValue(K_REMEMBER, s.rememberStreamKey);

    q.setValue(K_VIDEO_MODE,     int(s.sources.videoMode));
    q.setValue(K_SCREEN_INDEX,   s.sources.screenIndex);
    q.setValue(K_WINDOW_TITLE,   s.sources.windowTitle);
    q.setValue(K_MIC_ENABLED,    s.sources.micEnabled);
    q.setValue(K_MIC_DEVICE,     s.sources.micDeviceId);
    q.setValue(K_DESKAUD_ENABLED, s.sources.desktopAudioEnabled);
    q.setValue(K_DESKAUD_DEVICE,  s.sources.desktopAudioDeviceId);

    q.setValue(K_W,        s.config.widthPx);
    q.setValue(K_H,        s.config.heightPx);
    q.setValue(K_FPS,      s.config.fps);
    q.setValue(K_VBR,      s.config.videoBitrateKbps);
    q.setValue(K_ABR,      s.config.audioBitrateKbps);
    q.setValue(K_AR,       s.config.audioSampleRateHz);
    q.setValue(K_KEYINT,   s.config.keyframeIntervalSec);
    q.setValue(K_ENC,      int(s.config.encoder));
    q.setValue(K_RC,       int(s.config.rateControl));
    q.setValue(K_X264P,    s.config.x264Preset);
    q.setValue(K_PROFILE,  s.config.profile);

    q.setValue(K_THEME,    int(s.theme));
    q.setValue(K_ACCENT,   s.accent.name(QColor::HexArgb));
}

Settings loadSettings() {
    Settings s;
    QSettings q;
    s.presetId = q.value(K_PRESET, s.presetId).toString();
    s.target.rtmpUrl = q.value(K_RTMP, s.target.rtmpUrl).toString();
    s.target.streamKey = q.value(K_KEY).toString();
    s.rememberStreamKey = q.value(K_REMEMBER, true).toBool();

    s.sources.videoMode    = static_cast<VideoSourceMode>(
        q.value(K_VIDEO_MODE, int(s.sources.videoMode)).toInt());
    s.sources.screenIndex  = q.value(K_SCREEN_INDEX, s.sources.screenIndex).toInt();
    s.sources.windowTitle  = q.value(K_WINDOW_TITLE, s.sources.windowTitle).toString();
    s.sources.micEnabled   = q.value(K_MIC_ENABLED, s.sources.micEnabled).toBool();
    s.sources.micDeviceId  = q.value(K_MIC_DEVICE, s.sources.micDeviceId).toString();
    s.sources.desktopAudioEnabled = q.value(K_DESKAUD_ENABLED, s.sources.desktopAudioEnabled).toBool();
    s.sources.desktopAudioDeviceId = q.value(K_DESKAUD_DEVICE, s.sources.desktopAudioDeviceId).toString();

    s.config.widthPx       = q.value(K_W,      s.config.widthPx).toInt();
    s.config.heightPx      = q.value(K_H,      s.config.heightPx).toInt();
    s.config.fps           = q.value(K_FPS,    s.config.fps).toInt();
    s.config.videoBitrateKbps = q.value(K_VBR, s.config.videoBitrateKbps).toInt();
    s.config.audioBitrateKbps = q.value(K_ABR, s.config.audioBitrateKbps).toInt();
    s.config.audioSampleRateHz = q.value(K_AR, s.config.audioSampleRateHz).toInt();
    s.config.keyframeIntervalSec = q.value(K_KEYINT, s.config.keyframeIntervalSec).toInt();
    s.config.encoder       = static_cast<Encoder>(q.value(K_ENC, int(s.config.encoder)).toInt());
    s.config.rateControl   = static_cast<RateControl>(q.value(K_RC,  int(s.config.rateControl)).toInt());
    s.config.x264Preset    = q.value(K_X264P,  s.config.x264Preset).toString();
    s.config.profile       = q.value(K_PROFILE, s.config.profile).toString();

    s.theme  = static_cast<Theme>(q.value(K_THEME, int(s.theme)).toInt());
    s.accent = QColor(q.value(K_ACCENT, s.accent.name(QColor::HexArgb)).toString());
    if (!s.accent.isValid()) s.accent = QColor(255, 153, 0);
    return s;
}

SettingsDialog::SettingsDialog(const Settings& initial,
                               ThemeManager* theme,
                               QWidget* parent)
    : QDialog(parent), m_settings(initial), m_theme(theme) {
    setWindowTitle(tr("Настройки — Lumen Stream"));
    setModal(true);
    resize(640, 600);

    auto* tabs = new QTabWidget(this);
    auto* streamTab = new QWidget;
    auto* sourcesTab = new QWidget;
    auto* encoderTab = new QWidget;
    auto* appearanceTab = new QWidget;
    buildStreamTab(streamTab);
    buildSourcesTab(sourcesTab);
    buildEncoderTab(encoderTab);
    buildAppearanceTab(appearanceTab);
    tabs->addTab(streamTab, tr("Стрим"));
    tabs->addTab(sourcesTab, tr("Источники"));
    tabs->addTab(encoderTab, tr("Кодировщик"));
    tabs->addTab(appearanceTab, tr("Внешний вид"));

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setProperty("role", "primary");
    buttons->button(QDialogButtonBox::Cancel)->setProperty("role", "secondary");
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* root = new QVBoxLayout(this);
    root->addWidget(tabs);
    root->addWidget(buttons);

    loadFromSettings();
}

void SettingsDialog::buildStreamTab(QWidget* tab) {
    auto* form = new QFormLayout(tab);

    m_presetCombo = new QComboBox;
    for (const auto& p : PresetManager::builtinPresets()) {
        m_presetCombo->addItem(p.displayName, p.id);
    }
    m_presetCombo->addItem(tr("Custom (ручные настройки)"), QString());
    connect(m_presetCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onPresetChanged);

    auto* applyPreset = new QPushButton(tr("Применить пресет"));
    applyPreset->setProperty("role", "secondary");
    connect(applyPreset, &QPushButton::clicked,
            this, &SettingsDialog::onApplyPreset);

    auto* presetRow = new QHBoxLayout;
    presetRow->addWidget(m_presetCombo, 1);
    presetRow->addWidget(applyPreset);

    m_rtmpEdit = new QLineEdit;
    m_rtmpEdit->setPlaceholderText(QStringLiteral("rtmp://live.twitch.tv/app"));

    m_streamKeyEdit = new QLineEdit;
    m_streamKeyEdit->setEchoMode(QLineEdit::Password);
    m_streamKeyEdit->setPlaceholderText(tr("Ключ из dashboard.twitch.tv → Settings → Stream"));

    m_rememberKey = new QCheckBox(tr("Запомнить ключ (хранится в QSettings)"));

    form->addRow(tr("Пресет"), presetRow);
    form->addRow(tr("RTMP-сервер"), m_rtmpEdit);
    form->addRow(tr("Stream Key"), m_streamKeyEdit);
    form->addRow(QString(), m_rememberKey);
}

void SettingsDialog::buildSourcesTab(QWidget* tab) {
    auto* root = new QVBoxLayout(tab);
    root->setSpacing(14);

    // ---- Video group ----
    auto* videoGroup = new QGroupBox(tr("Видео"));
    auto* videoLay = new QVBoxLayout(videoGroup);

    m_videoScreenRadio = new QRadioButton(tr("Захват экрана (монитор)"));
    m_videoWindowRadio = new QRadioButton(tr("Захват окна по заголовку (только Windows)"));
    m_videoTestRadio   = new QRadioButton(tr("Тестовая картинка (testsrc2 + sine)"));
    m_videoModeGroup = new QButtonGroup(this);
    m_videoModeGroup->addButton(m_videoScreenRadio, int(VideoSourceMode::Screen));
    m_videoModeGroup->addButton(m_videoWindowRadio, int(VideoSourceMode::Window));
    m_videoModeGroup->addButton(m_videoTestRadio,   int(VideoSourceMode::TestPattern));

    m_videoOptionsStack = new QStackedWidget;

    auto* screenPage = new QWidget;
    {
        auto* lay = new QFormLayout(screenPage);
        lay->setContentsMargins(0, 4, 0, 0);
        m_screenCombo = new QComboBox;
        lay->addRow(tr("Монитор"), m_screenCombo);
    }
    auto* windowPage = new QWidget;
    {
        auto* lay = new QFormLayout(windowPage);
        lay->setContentsMargins(0, 4, 0, 0);
        m_windowTitleEdit = new QLineEdit;
        m_windowTitleEdit->setPlaceholderText(
            tr("Точный заголовок окна (как в taskbar)"));
        lay->addRow(tr("Заголовок окна"), m_windowTitleEdit);
    }
    auto* testPage = new QWidget;
    {
        auto* lay = new QVBoxLayout(testPage);
        lay->setContentsMargins(0, 4, 0, 0);
        auto* hint = new QLabel(tr(
            "ffmpeg сгенерирует движущийся testsrc2 + 440 Hz sine. "
            "Полезно чтобы убедиться что RTMP проходит без живого источника."));
        hint->setWordWrap(true);
        hint->setProperty("role", "sectionHeader");
        lay->addWidget(hint);
    }

    m_videoOptionsStack->addWidget(screenPage);  // index matches VideoSourceMode
    m_videoOptionsStack->addWidget(windowPage);
    m_videoOptionsStack->addWidget(testPage);

    videoLay->addWidget(m_videoScreenRadio);
    videoLay->addWidget(m_videoWindowRadio);
    videoLay->addWidget(m_videoTestRadio);
    videoLay->addWidget(m_videoOptionsStack);

#if !defined(Q_OS_WIN)
    // Window-title capture relies on gdigrab title=, which only exists on
    // Windows. Disable the radio elsewhere with an explanatory tooltip.
    m_videoWindowRadio->setEnabled(false);
    m_videoWindowRadio->setToolTip(
        tr("Захват по заголовку окна доступен только в Windows-сборке."));
#endif

    connect(m_videoScreenRadio, &QRadioButton::toggled, this, &SettingsDialog::onVideoModeChanged);
    connect(m_videoWindowRadio, &QRadioButton::toggled, this, &SettingsDialog::onVideoModeChanged);
    connect(m_videoTestRadio,   &QRadioButton::toggled, this, &SettingsDialog::onVideoModeChanged);

    // ---- Audio: microphone ----
    auto* micGroup = new QGroupBox(tr("Микрофон"));
    auto* micLay = new QVBoxLayout(micGroup);
    m_micEnable = new QCheckBox(tr("Включить захват микрофона"));
    m_micCombo  = new QComboBox;
    m_micCombo->setEnabled(false);
    connect(m_micEnable, &QCheckBox::toggled, m_micCombo, &QWidget::setEnabled);
    micLay->addWidget(m_micEnable);
    micLay->addWidget(m_micCombo);

    // ---- Audio: desktop ----
    auto* deskGroup = new QGroupBox(tr("Звук рабочего стола"));
    auto* deskLay = new QVBoxLayout(deskGroup);
    m_desktopAudioEnable = new QCheckBox(tr("Включить захват звука рабочего стола"));
    m_desktopAudioCombo  = new QComboBox;
    m_desktopAudioCombo->setEnabled(false);
    connect(m_desktopAudioEnable, &QCheckBox::toggled,
            m_desktopAudioCombo, &QWidget::setEnabled);
    deskLay->addWidget(m_desktopAudioEnable);
    deskLay->addWidget(m_desktopAudioCombo);

    auto* refresh = new QPushButton(tr("Обновить список устройств"));
    refresh->setProperty("role", "secondary");
    connect(refresh, &QPushButton::clicked,
            this, &SettingsDialog::onRefreshDevices);

    auto* hintRow = new QLabel(tr(
        "Если выбраны и микрофон, и звук рабочего стола — они микшируются "
        "в одну дорожку через ffmpeg amix."));
    hintRow->setWordWrap(true);
    hintRow->setProperty("role", "sectionHeader");

    root->addWidget(videoGroup);
    root->addWidget(micGroup);
    root->addWidget(deskGroup);
    root->addWidget(hintRow);
    root->addWidget(refresh, 0, Qt::AlignLeft);
    root->addStretch(1);
}

void SettingsDialog::buildEncoderTab(QWidget* tab) {
    auto* form = new QFormLayout(tab);

    m_widthSpin = new QSpinBox;  m_widthSpin->setRange(320, 3840);  m_widthSpin->setSingleStep(2);
    m_heightSpin = new QSpinBox; m_heightSpin->setRange(240, 2160); m_heightSpin->setSingleStep(2);
    m_fpsSpin = new QSpinBox;    m_fpsSpin->setRange(15, 240);
    m_videoBitrateSpin = new QSpinBox; m_videoBitrateSpin->setRange(500, 50000);
    m_videoBitrateSpin->setSuffix(QStringLiteral(" kbps"));
    m_audioBitrateSpin = new QSpinBox; m_audioBitrateSpin->setRange(64, 320);
    m_audioBitrateSpin->setSuffix(QStringLiteral(" kbps"));
    m_keyframeSpin = new QSpinBox; m_keyframeSpin->setRange(1, 6);
    m_keyframeSpin->setSuffix(QStringLiteral(" сек"));

    m_encoderCombo = new QComboBox;
    m_encoderCombo->addItem(QStringLiteral("x264 (CPU)"), int(Encoder::X264));
    m_encoderCombo->addItem(QStringLiteral("NVENC H.264 (NVIDIA)"), int(Encoder::NVENC_H264));
    m_encoderCombo->addItem(QStringLiteral("QuickSync H.264 (Intel)"), int(Encoder::QSV_H264));
    m_encoderCombo->addItem(QStringLiteral("AMF H.264 (AMD)"), int(Encoder::AMF_H264));

    m_rateCombo = new QComboBox;
    m_rateCombo->addItem(tr("CBR (рекомендуется Twitch)"), int(RateControl::CBR));
    m_rateCombo->addItem(QStringLiteral("VBR"), int(RateControl::VBR));
    m_rateCombo->addItem(QStringLiteral("CQP"), int(RateControl::CQP));

    m_x264PresetCombo = new QComboBox;
    m_x264PresetCombo->addItems({
        "ultrafast", "superfast", "veryfast",
        "faster", "fast", "medium", "slow"
    });

    form->addRow(tr("Ширина"), m_widthSpin);
    form->addRow(tr("Высота"), m_heightSpin);
    form->addRow(tr("Кадры в секунду"), m_fpsSpin);
    form->addRow(tr("Видео битрейт"), m_videoBitrateSpin);
    form->addRow(tr("Аудио битрейт"), m_audioBitrateSpin);
    form->addRow(tr("Keyframe interval"), m_keyframeSpin);
    form->addRow(tr("Кодировщик"), m_encoderCombo);
    form->addRow(tr("Rate control"), m_rateCombo);
    form->addRow(tr("Пресет x264"), m_x264PresetCombo);
}

void SettingsDialog::buildAppearanceTab(QWidget* tab) {
    auto* form = new QFormLayout(tab);

    m_themeCombo = new QComboBox;
    m_themeCombo->addItem(themeDisplayName(Theme::Light), int(Theme::Light));
    m_themeCombo->addItem(themeDisplayName(Theme::Blackout), int(Theme::Blackout));
    m_themeCombo->addItem(themeDisplayName(Theme::Rgb), int(Theme::Rgb));

    m_accentButton = new QPushButton(tr("Выбрать акцент…"));
    m_accentButton->setProperty("role", "secondary");
    connect(m_accentButton, &QPushButton::clicked,
            this, &SettingsDialog::onPickAccent);

    auto* hint = new QLabel(tr(
        "RGB-тема анимирует акцент автоматически и игнорирует выбранный цвет."));
    hint->setWordWrap(true);
    hint->setProperty("role", "sectionHeader");

    form->addRow(tr("Тема"), m_themeCombo);
    form->addRow(tr("Акцент"), m_accentButton);
    form->addRow(QString(), hint);
}

void SettingsDialog::populateDeviceCombos() {
    // Screens.
    m_screenCombo->clear();
    const auto screens = enumerateScreens();
    for (const auto& s : screens) {
        m_screenCombo->addItem(s.label, s.index);
    }
    if (m_screenCombo->count() == 0) {
        m_screenCombo->addItem(tr("(дисплеи не обнаружены)"), 0);
    }
    int sIdx = m_screenCombo->findData(m_settings.sources.screenIndex);
    m_screenCombo->setCurrentIndex(sIdx >= 0 ? sIdx : 0);

    // Mics — preserve user's saved selection if it's still in the list.
    m_micCombo->clear();
    const auto mics = enumerateMicrophones();
    for (const auto& d : mics) m_micCombo->addItem(d.label, d.id);
    if (m_micCombo->count() == 0) {
        m_micCombo->addItem(tr("(микрофоны не найдены)"), QString());
    }
    int mIdx = m_micCombo->findData(m_settings.sources.micDeviceId);
    if (mIdx < 0 && !m_settings.sources.micDeviceId.isEmpty()) {
        // Saved device disappeared; surface it anyway as "(missing)" so
        // the user can see *why* their stream stopped working.
        m_micCombo->addItem(
            tr("%1 (не найден сейчас)").arg(m_settings.sources.micDeviceId),
            m_settings.sources.micDeviceId);
        mIdx = m_micCombo->count() - 1;
    }
    if (mIdx >= 0) m_micCombo->setCurrentIndex(mIdx);

    // Desktop audio — same treatment as mics.
    m_desktopAudioCombo->clear();
    const auto desks = enumerateDesktopAudio();
    for (const auto& d : desks) m_desktopAudioCombo->addItem(d.label, d.id);
    if (m_desktopAudioCombo->count() == 0) {
        m_desktopAudioCombo->addItem(tr("(устройств не найдено)"), QString());
    }
    int dIdx = m_desktopAudioCombo->findData(m_settings.sources.desktopAudioDeviceId);
    if (dIdx < 0 && !m_settings.sources.desktopAudioDeviceId.isEmpty()) {
        m_desktopAudioCombo->addItem(
            tr("%1 (не найдено сейчас)").arg(m_settings.sources.desktopAudioDeviceId),
            m_settings.sources.desktopAudioDeviceId);
        dIdx = m_desktopAudioCombo->count() - 1;
    }
    if (dIdx >= 0) m_desktopAudioCombo->setCurrentIndex(dIdx);
}

void SettingsDialog::loadFromSettings() {
    int idx = m_presetCombo->findData(m_settings.presetId);
    m_presetCombo->setCurrentIndex(idx >= 0 ? idx : (m_presetCombo->count() - 1));

    m_rtmpEdit->setText(m_settings.target.rtmpUrl);
    m_streamKeyEdit->setText(m_settings.target.streamKey);
    m_rememberKey->setChecked(m_settings.rememberStreamKey);

    populateDeviceCombos();

    switch (m_settings.sources.videoMode) {
        case VideoSourceMode::Screen:      m_videoScreenRadio->setChecked(true); break;
        case VideoSourceMode::Window:      m_videoWindowRadio->setChecked(true); break;
        case VideoSourceMode::TestPattern: m_videoTestRadio->setChecked(true);   break;
    }
    m_windowTitleEdit->setText(m_settings.sources.windowTitle);
    onVideoModeChanged();

    m_micEnable->setChecked(m_settings.sources.micEnabled);
    m_micCombo->setEnabled(m_settings.sources.micEnabled);
    m_desktopAudioEnable->setChecked(m_settings.sources.desktopAudioEnabled);
    m_desktopAudioCombo->setEnabled(m_settings.sources.desktopAudioEnabled);

    applyConfigToInputs(m_settings.config);

    m_themeCombo->setCurrentIndex(m_themeCombo->findData(int(m_settings.theme)));

    QString swatch = m_settings.accent.name();
    m_accentButton->setText(tr("Акцент: %1").arg(swatch));
}

void SettingsDialog::applyConfigToInputs(const StreamConfig& cfg) {
    m_widthSpin->setValue(cfg.widthPx);
    m_heightSpin->setValue(cfg.heightPx);
    m_fpsSpin->setValue(cfg.fps);
    m_videoBitrateSpin->setValue(cfg.videoBitrateKbps);
    m_audioBitrateSpin->setValue(cfg.audioBitrateKbps);
    m_keyframeSpin->setValue(cfg.keyframeIntervalSec);
    m_encoderCombo->setCurrentIndex(m_encoderCombo->findData(int(cfg.encoder)));
    m_rateCombo->setCurrentIndex(m_rateCombo->findData(int(cfg.rateControl)));
    int xi = m_x264PresetCombo->findText(cfg.x264Preset);
    if (xi >= 0) m_x264PresetCombo->setCurrentIndex(xi);
}

void SettingsDialog::onPresetChanged(int) {
    // Don't auto-apply on change; the user has to press "Apply" so we
    // never silently overwrite their custom encoder tweaks just because
    // they were inspecting the preset list.
}

void SettingsDialog::onApplyPreset() {
    const QString id = m_presetCombo->currentData().toString();
    if (id.isEmpty()) return; // "Custom"
    if (auto* p = PresetManager::findById(id)) {
        applyConfigToInputs(p->config);
        // audioSampleRateHz and profile have no UI controls; carry them
        // through directly so writeBackFromUi() preserves the preset's
        // intended values instead of stale data from m_settings.
        m_settings.config.audioSampleRateHz = p->config.audioSampleRateHz;
        m_settings.config.profile = p->config.profile;
    }
}

void SettingsDialog::onPickAccent() {
    QColor c = QColorDialog::getColor(m_settings.accent, this,
        tr("Выбрать акцентный цвет"));
    if (!c.isValid()) return;
    m_settings.accent = c;
    m_accentButton->setText(tr("Акцент: %1").arg(c.name()));
    if (m_theme && m_theme->currentTheme() != Theme::Rgb) {
        m_theme->setAccent(c);
    }
}

void SettingsDialog::onRefreshDevices() {
    // Snapshot current selections back into m_settings so populate() can
    // reapply them; user expects "Refresh" to keep their pick if it's
    // still present, not silently reset to the first device.
    if (m_micCombo->currentIndex() >= 0)
        m_settings.sources.micDeviceId = m_micCombo->currentData().toString();
    if (m_desktopAudioCombo->currentIndex() >= 0)
        m_settings.sources.desktopAudioDeviceId =
            m_desktopAudioCombo->currentData().toString();
    if (m_screenCombo->currentIndex() >= 0)
        m_settings.sources.screenIndex = m_screenCombo->currentData().toInt();
    populateDeviceCombos();
}

void SettingsDialog::onVideoModeChanged() {
    int id = m_videoModeGroup->checkedId();
    if (id < 0) return;
    m_videoOptionsStack->setCurrentIndex(id);
}

void SettingsDialog::writeBackFromUi() {
    m_settings.presetId = m_presetCombo->currentData().toString();
    m_settings.target.rtmpUrl = m_rtmpEdit->text().trimmed();
    if (m_settings.target.rtmpUrl.isEmpty()) {
        m_settings.target.rtmpUrl = QStringLiteral("rtmp://live.twitch.tv/app");
    }
    m_settings.target.streamKey = m_streamKeyEdit->text().trimmed();
    m_settings.rememberStreamKey = m_rememberKey->isChecked();

    int vmodeId = m_videoModeGroup->checkedId();
    if (vmodeId >= 0) {
        m_settings.sources.videoMode = static_cast<VideoSourceMode>(vmodeId);
    }
    m_settings.sources.screenIndex = m_screenCombo->currentData().toInt();
    m_settings.sources.windowTitle = m_windowTitleEdit->text().trimmed();
    m_settings.sources.micEnabled = m_micEnable->isChecked();
    m_settings.sources.micDeviceId = m_micCombo->currentData().toString();
    m_settings.sources.micDeviceLabel = m_micCombo->currentText();
    m_settings.sources.desktopAudioEnabled = m_desktopAudioEnable->isChecked();
    m_settings.sources.desktopAudioDeviceId = m_desktopAudioCombo->currentData().toString();
    m_settings.sources.desktopAudioDeviceLabel = m_desktopAudioCombo->currentText();

    m_settings.config.widthPx = m_widthSpin->value();
    m_settings.config.heightPx = m_heightSpin->value();
    m_settings.config.fps = m_fpsSpin->value();
    m_settings.config.videoBitrateKbps = m_videoBitrateSpin->value();
    m_settings.config.audioBitrateKbps = m_audioBitrateSpin->value();
    m_settings.config.keyframeIntervalSec = m_keyframeSpin->value();
    m_settings.config.encoder = static_cast<Encoder>(m_encoderCombo->currentData().toInt());
    m_settings.config.rateControl = static_cast<RateControl>(m_rateCombo->currentData().toInt());
    m_settings.config.x264Preset = m_x264PresetCombo->currentText();

    m_settings.theme = static_cast<Theme>(m_themeCombo->currentData().toInt());
}

void SettingsDialog::accept() {
    writeBackFromUi();
    QDialog::accept();
}

} // namespace lumen
