#include "SettingsDialog.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

namespace lumen {

namespace {

constexpr auto K_PRESET    = "stream/presetId";
constexpr auto K_RTMP      = "stream/rtmpUrl";
constexpr auto K_KEY       = "stream/streamKey";
constexpr auto K_REMEMBER  = "stream/rememberStreamKey";
constexpr auto K_SOURCE_LIST = "sources/list";  // JSON array

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

    q.setValue(K_SOURCE_LIST, serializeSources(s.sources));

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

    s.sources = deserializeSources(q.value(K_SOURCE_LIST).toString());

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
    resize(560, 520);

    auto* tabs = new QTabWidget(this);
    auto* streamTab = new QWidget;
    auto* encoderTab = new QWidget;
    auto* appearanceTab = new QWidget;
    buildStreamTab(streamTab);
    buildEncoderTab(encoderTab);
    buildAppearanceTab(appearanceTab);
    tabs->addTab(streamTab, tr("Стрим"));
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

void SettingsDialog::loadFromSettings() {
    int idx = m_presetCombo->findData(m_settings.presetId);
    m_presetCombo->setCurrentIndex(idx >= 0 ? idx : (m_presetCombo->count() - 1));

    m_rtmpEdit->setText(m_settings.target.rtmpUrl);
    m_streamKeyEdit->setText(m_settings.target.streamKey);
    m_rememberKey->setChecked(m_settings.rememberStreamKey);

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
    // No auto-apply.
}

void SettingsDialog::onApplyPreset() {
    const QString id = m_presetCombo->currentData().toString();
    if (id.isEmpty()) return;
    if (auto* p = PresetManager::findById(id)) {
        applyConfigToInputs(p->config);
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

void SettingsDialog::writeBackFromUi() {
    m_settings.presetId = m_presetCombo->currentData().toString();
    m_settings.target.rtmpUrl = m_rtmpEdit->text().trimmed();
    if (m_settings.target.rtmpUrl.isEmpty()) {
        m_settings.target.rtmpUrl = QStringLiteral("rtmp://live.twitch.tv/app");
    }
    m_settings.target.streamKey = m_streamKeyEdit->text().trimmed();
    m_settings.rememberStreamKey = m_rememberKey->isChecked();

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
