#include "PreviewWidget.h"
#include "ThemeManager.h"

#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>

namespace lumen {

PreviewWidget::PreviewWidget(ThemeManager* theme, QWidget* parent)
    : QWidget(parent), m_theme(theme) {
    setMinimumHeight(280);

    // Drive a slow pulse so the preview feels alive even when the stream
    // is offline. We piggyback on theme accent changes for free repaint
    // when the RGB theme is active.
    auto* t = new QTimer(this);
    t->setInterval(50);
    connect(t, &QTimer::timeout, this, [this] {
        m_pulse = (m_pulse + 1) % 360;
        update();
    });
    t->start();

    if (m_theme) {
        connect(m_theme, &ThemeManager::accentChanged, this,
                [this] { update(); });
    }
}

void PreviewWidget::setSummary(const QString& s) {
    m_summary = s;
    update();
}

void PreviewWidget::setLive(bool live) {
    m_live = live;
    update();
}

void PreviewWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRect r = rect().adjusted(8, 8, -8, -8);
    QPainterPath path;
    path.addRoundedRect(r, 16, 16);

    // Background gradient — slight sheen around the accent.
    QColor accent = m_theme ? m_theme->accent() : QColor(255, 153, 0);
    QLinearGradient bg(r.topLeft(), r.bottomRight());
    bg.setColorAt(0.0, QColor(20, 20, 20));
    bg.setColorAt(1.0, QColor(8, 8, 8));
    p.fillPath(path, bg);

    // Accent border glow.
    QPen border(accent, 1.4);
    p.setPen(border);
    p.drawPath(path);

    // Soft radial accent bloom in the center.
    const QPointF center = r.center();
    QRadialGradient bloom(center, r.height() * 0.45);
    QColor bloomColor = accent;
    bloomColor.setAlpha(45);
    bloom.setColorAt(0.0, bloomColor);
    bloomColor.setAlpha(0);
    bloom.setColorAt(1.0, bloomColor);
    p.setPen(Qt::NoPen);
    p.setBrush(bloom);
    p.drawEllipse(center, r.height() * 0.45, r.height() * 0.45);

    // Pulse marker.
    const qreal phase = (m_pulse / 360.0) * 2.0 * 3.14159265;
    const qreal radius = 22.0 + 6.0 * std::sin(phase);
    QColor pulseColor = m_live ? QColor(224, 59, 59) : accent;
    p.setBrush(pulseColor);
    p.drawEllipse(center, radius, radius);

    // Inner play triangle / live square.
    QPainterPath glyph;
    if (m_live) {
        glyph.addRoundedRect(QRectF(center.x() - 8, center.y() - 8, 16, 16), 3, 3);
    } else {
        glyph.moveTo(center.x() - 6, center.y() - 9);
        glyph.lineTo(center.x() + 10, center.y());
        glyph.lineTo(center.x() - 6, center.y() + 9);
        glyph.closeSubpath();
    }
    p.fillPath(glyph, QColor(10, 10, 10));

    // Caption.
    p.setPen(QColor(180, 180, 180));
    QFont f = p.font();
    f.setPointSize(10);
    f.setWeight(QFont::Medium);
    p.setFont(f);
    const QString caption = m_live ? tr("В ЭФИРЕ")
                                   : tr("Превью отсутствует — настрой источник в настройках");
    QRect captionRect(r.left(), center.y() + 36, r.width(), 18);
    p.drawText(captionRect, Qt::AlignCenter, caption);

    // Summary.
    if (!m_summary.isEmpty()) {
        p.setPen(QColor(140, 140, 140));
        f.setPointSize(9);
        f.setWeight(QFont::Normal);
        p.setFont(f);
        QRect summaryRect(r.left(), captionRect.bottom() + 4, r.width(), 18);
        p.drawText(summaryRect, Qt::AlignCenter, m_summary);
    }
}

} // namespace lumen
