#include "AccentBadge.h"
#include "ThemeManager.h"

#include <QPainter>

namespace lumen {

AccentBadge::AccentBadge(ThemeManager* theme, QWidget* parent)
    : QWidget(parent), m_theme(theme) {
    setFixedSize(16, 16);
    if (m_theme) {
        connect(m_theme, &ThemeManager::accentChanged, this,
                [this] { update(); });
    }
}

void AccentBadge::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QColor c = m_theme ? m_theme->accent() : QColor(255, 153, 0);
    p.setPen(Qt::NoPen);
    p.setBrush(c);
    p.drawEllipse(rect().adjusted(2, 2, -2, -2));
    QColor halo = c;
    halo.setAlpha(80);
    p.setBrush(halo);
    p.drawEllipse(rect());
}

} // namespace lumen
