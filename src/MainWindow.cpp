#include "MainWindow.h"

#include "AccentBadge.h"
#include "PresetManager.h"
#include "PreviewWidget.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QToolTip>
#include <QVBoxLayout>

namespace lumen {

namespace {

QPushButton* makeNavButton(const QString& label, const QString& iconRes) {
    auto* b = new QPushButton(label);
    b->setIcon(QIcon(iconRes));
    b->setIconSize({18, 18});
    b->setProperty("role", "nav");
    b->setCheckable(true);
    b->setCursor(Qt::PointingHandCursor);
    b->setMinimumHeight(44);
    return b;
}

QFrame* makeCard() {
    auto* card = new QFrame;
    card->setProperty("role", "card");
    return card;
}

void restyle(QWidget* w) {
    if (!w) return;
    w->style()->unpolish(w);
    w->style()->polish(w);
    w->update();
}

} // namespace

MainWindow::MainWindow(ThemeManager* theme, QWidget* parent)
    : QMainWindow(parent), m_theme(theme), m_engine(new StreamEngine(this)) {
    setWindowTitle(QStringLiteral("Lumen Stream"));
    setWindowIcon(QIcon(QStringLiteral(":/resources/icons/logo.svg")));
    resize(1100, 700);

    m_settings = loadSettings();
    if (auto* p = PresetManager::findById(m_settings.presetId)) {
        // Honor the preset's resolution/fps/bitrate when the user hasn't
        // hand-edited away from a preset since last save.
        // We trust the persisted config; reapply happens via Settings UI.
        Q_UNUSED(p);
    }

    if (m_theme) {
        m_theme->setAccent(m_settings.accent);
        m_theme->applyTheme(m_settings.theme);
    }

    auto* central = new QWidget;
    auto* root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Sidebar
    auto* sidebar = new QWidget;
    sidebar->setObjectName(QStringLiteral("sideBar"));
    sidebar->setFixedWidth(220);
    auto* sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(0, 24, 0, 24);
    sideLayout->setSpacing(0);

    auto* brand = new QWidget;
    auto* brandLayout = new QHBoxLayout(brand);
    brandLayout->setContentsMargins(20, 0, 20, 24);
    brandLayout->setSpacing(10);
    m_badge = new AccentBadge(m_theme);
    auto* brandTextWrap = new QWidget;
    auto* brandText = new QVBoxLayout(brandTextWrap);
    brandText->setContentsMargins(0, 0, 0, 0);
    brandText->setSpacing(0);
    auto* brandTitle = new QLabel(QStringLiteral("LUMEN"));
    brandTitle->setObjectName(QStringLiteral("brandTitle"));
    auto* brandSub = new QLabel(QStringLiteral("STREAM"));
    brandSub->setObjectName(QStringLiteral("brandSub"));
    brandText->addWidget(brandTitle);
    brandText->addWidget(brandSub);
    brandLayout->addWidget(m_badge);
    brandLayout->addWidget(brandTextWrap, 1);

    m_navStream = makeNavButton(tr("Стрим"), QStringLiteral(":/resources/icons/stream.svg"));
    m_navSettings = makeNavButton(tr("Настройки"), QStringLiteral(":/resources/icons/settings.svg"));
    m_navAbout = makeNavButton(tr("О программе"), QStringLiteral(":/resources/icons/info.svg"));

    sideLayout->addWidget(brand);
    sideLayout->addWidget(m_navStream);
    sideLayout->addWidget(m_navSettings);
    sideLayout->addWidget(m_navAbout);
    sideLayout->addStretch(1);

    auto* version = new QLabel(QStringLiteral("v0.1.0"));
    version->setProperty("role", "sectionHeader");
    version->setContentsMargins(20, 0, 20, 0);
    sideLayout->addWidget(version);

    // Content area
    auto* content = new QWidget;
    content->setObjectName(QStringLiteral("contentArea"));
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(28, 28, 28, 28);
    contentLayout->setSpacing(20);

    m_pages = new QStackedWidget;
    contentLayout->addWidget(m_pages);

    buildStreamPage();
    buildAboutPage();

    // Wire navigation
    connect(m_navStream, &QPushButton::clicked, this, [this] { selectNav(0); });
    connect(m_navAbout,  &QPushButton::clicked, this, [this] { selectNav(1); });
    connect(m_navSettings, &QPushButton::clicked, this,
            &MainWindow::onSettingsClicked);
    selectNav(0);

    root->addWidget(sidebar);
    root->addWidget(content, 1);
    setCentralWidget(central);

    // Engine wiring
    connect(m_engine, &StreamEngine::started, this, &MainWindow::onStreamStarted);
    connect(m_engine, &StreamEngine::stopped, this, &MainWindow::onStreamStopped);
    connect(m_engine, &StreamEngine::logLine, this, &MainWindow::onStreamLog);
    connect(m_engine, &StreamEngine::errorOccurred, this, &MainWindow::onStreamError);

    refreshStreamSummary();
}

void MainWindow::buildStreamPage() {
    auto* page = new QWidget;
    auto* lay = new QVBoxLayout(page);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(20);

    // Header card with status + go-live action.
    auto* header = makeCard();
    auto* headerLay = new QHBoxLayout(header);
    headerLay->setContentsMargins(20, 16, 20, 16);
    headerLay->setSpacing(16);

    auto* leftCol = new QVBoxLayout;
    leftCol->setSpacing(2);
    auto* hintLabel = new QLabel(tr("ТЕКУЩИЙ ПРЕСЕТ"));
    hintLabel->setProperty("role", "sectionHeader");
    m_presetLabel = new QLabel(QStringLiteral("Твич 1080p60"));
    QFont f = m_presetLabel->font();
    f.setPointSize(15);
    f.setWeight(QFont::DemiBold);
    m_presetLabel->setFont(f);
    leftCol->addWidget(hintLabel);
    leftCol->addWidget(m_presetLabel);

    auto* midCol = new QVBoxLayout;
    midCol->setSpacing(2);
    auto* bitrateHint = new QLabel(tr("БИТРЕЙТ"));
    bitrateHint->setProperty("role", "sectionHeader");
    m_bitrateLabel = new QLabel(QStringLiteral("6000 kbps"));
    QFont bf = m_bitrateLabel->font();
    bf.setPointSize(15);
    bf.setWeight(QFont::DemiBold);
    m_bitrateLabel->setFont(bf);
    midCol->addWidget(bitrateHint);
    midCol->addWidget(m_bitrateLabel);

    m_statusPill = new QLabel(tr("OFFLINE"));
    m_statusPill->setObjectName(QStringLiteral("statusPill"));
    m_statusPill->setAlignment(Qt::AlignCenter);

    m_goLiveButton = new QPushButton(tr("Начать стрим"));
    m_goLiveButton->setProperty("role", "primary");
    m_goLiveButton->setIcon(QIcon(QStringLiteral(":/resources/icons/play.svg")));
    m_goLiveButton->setIconSize({16, 16});
    m_goLiveButton->setCursor(Qt::PointingHandCursor);
    connect(m_goLiveButton, &QPushButton::clicked,
            this, &MainWindow::onGoLiveClicked);

    headerLay->addLayout(leftCol, 2);
    headerLay->addLayout(midCol, 1);
    headerLay->addWidget(m_statusPill, 0);
    headerLay->addWidget(m_goLiveButton, 0);

    // Preview card
    auto* previewCard = makeCard();
    auto* previewLay = new QVBoxLayout(previewCard);
    previewLay->setContentsMargins(0, 0, 0, 0);
    m_preview = new PreviewWidget(m_theme);
    previewLay->addWidget(m_preview);

    // Log card
    auto* logCard = makeCard();
    auto* logLay = new QVBoxLayout(logCard);
    logLay->setContentsMargins(20, 16, 20, 16);
    auto* logHeader = new QLabel(tr("ЛОГ ENCODER'А"));
    logHeader->setProperty("role", "sectionHeader");
    m_logView = new QPlainTextEdit;
    m_logView->setObjectName(QStringLiteral("logView"));
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(2000);
    logLay->addWidget(logHeader);
    logLay->addWidget(m_logView);

    lay->addWidget(header);
    lay->addWidget(previewCard, 1);
    lay->addWidget(logCard, 1);

    m_pages->addWidget(page);
}

void MainWindow::buildAboutPage() {
    auto* page = new QWidget;
    auto* lay = new QVBoxLayout(page);
    lay->setContentsMargins(0, 0, 0, 0);

    auto* card = makeCard();
    auto* cardLay = new QVBoxLayout(card);
    cardLay->setContentsMargins(28, 28, 28, 28);
    cardLay->setSpacing(12);

    auto* title = new QLabel(QStringLiteral("Lumen Stream"));
    QFont tf = title->font();
    tf.setPointSize(20);
    tf.setWeight(QFont::Bold);
    title->setFont(tf);

    auto* sub = new QLabel(tr("Современный клиент стриминга на Twitch — C++/Qt6"));
    sub->setProperty("role", "sectionHeader");
    sub->setWordWrap(true);

    auto* body = new QLabel(tr(
        "<p>Минималистичный стример с пресетами по официальной документации Twitch.<br>"
        "Темы: Светлая, Blackout (OLED), RGB (анимация акцента).</p>"
        "<p>Поток уходит в Twitch RTMP через локальный <b>ffmpeg</b> — "
        "его нужно установить и иметь в PATH.</p>"
        "<p>Twitch Broadcasting Guidelines: "
        "<a href=\"https://help.twitch.tv/s/article/broadcasting-guidelines\">help.twitch.tv</a></p>"));
    body->setOpenExternalLinks(true);
    body->setWordWrap(true);

    cardLay->addWidget(title);
    cardLay->addWidget(sub);
    cardLay->addWidget(body);
    cardLay->addStretch(1);

    lay->addWidget(card);
    m_pages->addWidget(page);
}

void MainWindow::selectNav(int index) {
    m_navStream->setProperty("selected", index == 0);
    m_navAbout->setProperty("selected", index == 1);
    m_navSettings->setProperty("selected", false);
    restyle(m_navStream);
    restyle(m_navAbout);
    restyle(m_navSettings);
    m_pages->setCurrentIndex(index);
}

void MainWindow::onSettingsClicked() {
    SettingsDialog dlg(m_settings, m_theme, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_settings = dlg.result();
        if (m_theme) {
            m_theme->applyTheme(m_settings.theme);
            if (m_settings.theme != Theme::Rgb) {
                m_theme->setAccent(m_settings.accent);
            }
        }
        saveSettings(m_settings);
        refreshStreamSummary();
    } else if (m_theme && m_theme->currentTheme() != Theme::Rgb) {
        // The dialog live-previews accent picks; revert to the saved one
        // when the user cancels so the rest of the app stays in sync.
        m_theme->setAccent(m_settings.accent);
    }
}

void MainWindow::refreshStreamSummary() {
    QString presetName = tr("Custom");
    if (auto* p = PresetManager::findById(m_settings.presetId)) {
        presetName = p->displayName;
    }
    m_presetLabel->setText(presetName);
    m_bitrateLabel->setText(
        QString("%1 kbps").arg(m_settings.config.videoBitrateKbps));
    if (m_preview) {
        m_preview->setSummary(QString("%1×%2 @ %3 fps · %4 kbps")
            .arg(m_settings.config.widthPx)
            .arg(m_settings.config.heightPx)
            .arg(m_settings.config.fps)
            .arg(m_settings.config.videoBitrateKbps));
    }
}

void MainWindow::onGoLiveClicked() {
    if (m_engine->isRunning()) {
        m_engine->stop();
        return;
    }
    m_engine->start(m_settings.config, m_settings.target, m_settings.source);
}

void MainWindow::onStreamStarted() {
    m_statusPill->setText(tr("LIVE"));
    m_statusPill->setProperty("live", true);
    restyle(m_statusPill);
    m_goLiveButton->setText(tr("Остановить"));
    m_goLiveButton->setIcon(QIcon(QStringLiteral(":/resources/icons/stop.svg")));
    m_goLiveButton->setProperty("live", true);
    restyle(m_goLiveButton);
    if (m_preview) m_preview->setLive(true);
}

void MainWindow::onStreamStopped(int exitCode, QProcess::ExitStatus status) {
    Q_UNUSED(exitCode);
    Q_UNUSED(status);
    m_statusPill->setText(tr("OFFLINE"));
    m_statusPill->setProperty("live", false);
    restyle(m_statusPill);
    m_goLiveButton->setText(tr("Начать стрим"));
    m_goLiveButton->setIcon(QIcon(QStringLiteral(":/resources/icons/play.svg")));
    m_goLiveButton->setProperty("live", false);
    restyle(m_goLiveButton);
    if (m_preview) m_preview->setLive(false);
}

void MainWindow::onStreamLog(const QString& line) {
    if (m_logView) m_logView->appendPlainText(line);
}

void MainWindow::onStreamError(const QString& message) {
    if (m_logView) {
        m_logView->appendPlainText(QStringLiteral("[ERROR] ") + message);
    }
    statusBar()->showMessage(message, 5000);
}

} // namespace lumen
