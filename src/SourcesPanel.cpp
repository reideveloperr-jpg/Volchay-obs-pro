#include "SourcesPanel.h"

#include "SourceConfigDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

namespace lumen {

namespace {

QString glyphFor(SourceType t) {
    switch (t) {
        case SourceType::DisplayCapture:     return QStringLiteral("\U0001F5A5");  // 🖥
        case SourceType::WindowCapture:      return QStringLiteral("\U0001F5C2");  // 🗂
        case SourceType::VideoCaptureDevice: return QStringLiteral("\U0001F4F7");  // 📷
        case SourceType::MediaFile:          return QStringLiteral("\U0001F39E");  // 🎞
        case SourceType::Image:              return QStringLiteral("\U0001F5BC");  // 🖼
        case SourceType::ColorSource:        return QStringLiteral("\u25A0");      // ■
        case SourceType::TextSource:         return QStringLiteral("\U0001F1F9");  // 🇹
        case SourceType::TestPattern:        return QStringLiteral("\u25A6");      // ▦
        case SourceType::Microphone:         return QStringLiteral("\U0001F3A4");  // 🎤
        case SourceType::DesktopAudio:       return QStringLiteral("\U0001F50A");  // 🔊
    }
    return QString();
}

} // namespace

SourcesPanel::SourcesPanel(ThemeManager* theme, QWidget* parent)
    : QWidget(parent), m_theme(theme) {
    Q_UNUSED(m_theme);
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(8);

    auto* header = new QHBoxLayout;
    auto* title = new QLabel(tr("ИСТОЧНИКИ"));
    title->setProperty("role", "sectionHeader");

    m_addBtn = new QPushButton(tr("+ Добавить"));
    m_addBtn->setProperty("role", "primary");
    m_addBtn->setCursor(Qt::PointingHandCursor);
    connect(m_addBtn, &QPushButton::clicked,
            this, &SourcesPanel::onAddRequested);

    header->addWidget(title);
    header->addStretch(1);
    header->addWidget(m_addBtn);

    m_list = new QListWidget;
    m_list->setObjectName(QStringLiteral("sourcesList"));
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setAlternatingRowColors(true);
    m_list->setUniformItemSizes(true);
    connect(m_list, &QListWidget::itemChanged,
            this, &SourcesPanel::onItemEnableToggled);
    connect(m_list, &QListWidget::itemSelectionChanged,
            this, [this] { emit selectionChanged(); });
    connect(m_list, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem*) { onConfigureSelected(); });

    auto* footer = new QHBoxLayout;
    m_configBtn = new QToolButton; m_configBtn->setText(tr("Настроить"));
    m_removeBtn = new QToolButton; m_removeBtn->setText(tr("Удалить"));
    m_upBtn     = new QToolButton; m_upBtn->setText(QStringLiteral("\u25B2"));
    m_downBtn   = new QToolButton; m_downBtn->setText(QStringLiteral("\u25BC"));
    for (auto* b : { m_configBtn, m_removeBtn, m_upBtn, m_downBtn }) {
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(28);
    }
    connect(m_configBtn, &QToolButton::clicked,
            this, &SourcesPanel::onConfigureSelected);
    connect(m_removeBtn, &QToolButton::clicked,
            this, &SourcesPanel::onRemoveSelected);
    connect(m_upBtn, &QToolButton::clicked,
            this, &SourcesPanel::onMoveUp);
    connect(m_downBtn, &QToolButton::clicked,
            this, &SourcesPanel::onMoveDown);
    footer->addWidget(m_configBtn);
    footer->addWidget(m_removeBtn);
    footer->addStretch(1);
    footer->addWidget(m_upBtn);
    footer->addWidget(m_downBtn);

    root->addLayout(header);
    root->addWidget(m_list, 1);
    root->addLayout(footer);
}

void SourcesPanel::setSources(const SourceList& sources) {
    m_sources = sources;
    rebuildList();
}

int SourcesPanel::selectedIndex() const {
    return m_list ? m_list->currentRow() : -1;
}

QListWidgetItem* SourcesPanel::makeItem(const Source& s) const {
    auto* it = new QListWidgetItem;
    it->setText(QString("%1  %2").arg(glyphFor(s.type), s.name));
    it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
    it->setCheckState(s.enabled ? Qt::Checked : Qt::Unchecked);
    it->setToolTip(sourceTypeDisplayName(s.type));
    return it;
}

void SourcesPanel::rebuildList() {
    QSignalBlocker block(m_list);
    m_list->clear();
    for (const auto& s : m_sources) m_list->addItem(makeItem(s));
}

void SourcesPanel::onItemEnableToggled(QListWidgetItem* item) {
    const int row = m_list->row(item);
    if (row < 0 || row >= m_sources.size()) return;
    const bool enabled = item->checkState() == Qt::Checked;
    if (m_sources[row].enabled == enabled) return;
    m_sources[row].enabled = enabled;
    emit sourcesChanged();
}

void SourcesPanel::onAddRequested() {
    QMenu menu(this);
    const SourceType allTypes[] = {
        SourceType::DisplayCapture, SourceType::WindowCapture,
        SourceType::VideoCaptureDevice, SourceType::MediaFile,
        SourceType::Image, SourceType::ColorSource, SourceType::TextSource,
        SourceType::TestPattern, SourceType::Microphone, SourceType::DesktopAudio,
    };
    for (auto t : allTypes) {
        const QString label = QString("%1  %2").arg(glyphFor(t),
                                                    sourceTypeDisplayName(t));
        QAction* a = menu.addAction(label);
        a->setData(int(t));
    }
    QAction* picked = menu.exec(m_addBtn->mapToGlobal(
        QPoint(0, m_addBtn->height())));
    if (!picked) return;
    Source s;
    s.type = static_cast<SourceType>(picked->data().toInt());
    s.name = sourceTypeDisplayName(s.type);
    SourceConfigDialog dlg(s, SourceConfigDialog::Mode::AddNew, this);
    if (dlg.exec() != QDialog::Accepted) return;
    m_sources.append(dlg.result());
    rebuildList();
    m_list->setCurrentRow(m_sources.size() - 1);
    emit sourcesChanged();
}

void SourcesPanel::onConfigureSelected() {
    const int row = selectedIndex();
    if (row < 0) return;
    SourceConfigDialog dlg(m_sources[row],
                           SourceConfigDialog::Mode::EditExisting, this);
    if (dlg.exec() != QDialog::Accepted) return;
    m_sources[row] = dlg.result();
    rebuildList();
    m_list->setCurrentRow(row);
    emit sourcesChanged();
}

void SourcesPanel::onRemoveSelected() {
    const int row = selectedIndex();
    if (row < 0) return;
    m_sources.removeAt(row);
    rebuildList();
    if (!m_sources.isEmpty()) {
        m_list->setCurrentRow(qMin(row, m_sources.size() - 1));
    }
    emit sourcesChanged();
}

void SourcesPanel::onMoveUp() {
    const int row = selectedIndex();
    if (row <= 0) return;
    m_sources.swapItemsAt(row, row - 1);
    rebuildList();
    m_list->setCurrentRow(row - 1);
    emit sourcesChanged();
}

void SourcesPanel::onMoveDown() {
    const int row = selectedIndex();
    if (row < 0 || row >= m_sources.size() - 1) return;
    m_sources.swapItemsAt(row, row + 1);
    rebuildList();
    m_list->setCurrentRow(row + 1);
    emit sourcesChanged();
}

} // namespace lumen
