#include "PreviewWidget.h"

#include "ThemeManager.h"

#include <QFileInfo>
#include <QGuiApplication>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QScreen>
#include <QTimer>
#include <cmath>

namespace lumen {

namespace {

QPixmap renderColorPreview(const Source& s, QSize widgetSize) {
    QColor c(s.settings.value(SourceFields::COLOR_RGB,
                               QStringLiteral("#202020")).toString());
    if (!c.isValid()) c = QColor("#202020");
    QPixmap pm(widgetSize);
    pm.fill(c);
    return pm;
}

QPixmap renderTextPreview(const Source& s, QSize size) {
    QColor bg(s.settings.value(SourceFields::TEXT_BG_COLOR,
                                QStringLiteral("#000000")).toString());
    QColor fg(s.settings.value(SourceFields::TEXT_COLOR,
                                QStringLiteral("#ffffff")).toString());
    if (!bg.isValid()) bg = Qt::black;
    if (!fg.isValid()) fg = Qt::white;
    QPixmap pm(size);
    pm.fill(bg);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    QFont f = p.font();
    int ptSize = s.settings.value(SourceFields::TEXT_SIZE, 48).toInt();
    f.setPointSize(qMax(8, ptSize / 2));   // shrink for preview
    p.setFont(f);
    p.setPen(fg);
    QString body = s.settings.value(SourceFields::TEXT_BODY).toString();
    if (body.isEmpty()) body = QObject::tr("(пустой текст)");
    p.drawText(pm.rect(), Qt::AlignCenter | Qt::TextWordWrap, body);
    return pm;
}

QPixmap renderTestPattern(QSize size, int pulse) {
    QPixmap pm(size);
    pm.fill(Qt::black);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    static const QColor bars[] = {
        QColor("#ffffff"), QColor("#ffff00"), QColor("#00ffff"),
        QColor("#00ff00"), QColor("#ff00ff"), QColor("#ff0000"),
        QColor("#0000ff"), QColor("#202020"),
    };
    const int barCount = sizeof(bars) / sizeof(bars[0]);
    const qreal bw = size.width() / qreal(barCount);
    for (int i = 0; i < barCount; ++i) {
        p.fillRect(QRectF(i * bw, 0, bw, size.height()), bars[i]);
    }
    // Sweep marker so it's clearly an animated test pattern.
    qreal x = (pulse % 360) / 360.0 * size.width();
    p.setPen(QPen(QColor(255, 153, 0, 200), 4));
    p.drawLine(QPointF(x, 0), QPointF(x, size.height()));
    return pm;
}

} // namespace

PreviewWidget::PreviewWidget(ThemeManager* theme, QWidget* parent)
    : QWidget(parent), m_theme(theme) {
    setMinimumHeight(280);

    auto* tick = new QTimer(this);
    tick->setInterval(100); // 10 fps preview
    connect(tick, &QTimer::timeout, this, &PreviewWidget::tickFrame);
    tick->start();

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

void PreviewWidget::setActiveSource(const Source* source) {
    if (source) {
        m_source = *source;
        m_hasSource = true;
    } else {
        m_hasSource = false;
        m_frame = QPixmap();
        m_frameNote.clear();
    }
    grabFrameFromSource();
    update();
}

void PreviewWidget::tickFrame() {
    m_pulse = (m_pulse + 6) % 360;
    if (m_hasSource) grabFrameFromSource();
    update();
}

void PreviewWidget::grabFrameFromSource() {
    if (!m_hasSource) {
        m_frame = QPixmap();
        return;
    }
    const QSize target = size().boundedTo(QSize(1280, 720));
    if (target.width() < 16 || target.height() < 16) return;

    m_frameNote.clear();
    switch (m_source.type) {
        case SourceType::DisplayCapture: {
            const int idx = m_source.settings.value(
                SourceFields::SCREEN_INDEX, 0).toInt();
            const auto screens = QGuiApplication::screens();
            QScreen* sc = (idx >= 0 && idx < screens.size())
                              ? screens[idx]
                              : QGuiApplication::primaryScreen();
            if (!sc) {
                m_frameNote = tr("Монитор не найден");
                return;
            }
            QPixmap raw = sc->grabWindow(0);
            if (raw.isNull()) {
                m_frameNote = tr("Не удалось получить кадр (для X11/Wayland сборки нужен xcb).");
                return;
            }
            m_frame = raw.scaled(target, Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation);
            return;
        }
        case SourceType::WindowCapture: {
            m_frameNote = tr("Превью окна доступно только на Windows во время стрима.");
            return;
        }
        case SourceType::VideoCaptureDevice: {
            m_frameNote = tr("Превью камеры появится во время стрима.");
            return;
        }
        case SourceType::MediaFile: {
            const QString path = m_source.settings.value(
                SourceFields::FILE_PATH).toString();
            m_frameNote = path.isEmpty()
                ? tr("Файл не выбран.")
                : tr("Превью медиа-файла появится во время стрима:\n%1").arg(path);
            return;
        }
        case SourceType::Image: {
            const QString path = m_source.settings.value(
                SourceFields::FILE_PATH).toString();
            if (path.isEmpty() || !QFileInfo::exists(path)) {
                m_frameNote = tr("Файл изображения не выбран.");
                return;
            }
            QPixmap raw(path);
            if (raw.isNull()) {
                m_frameNote = tr("Не удалось загрузить изображение:\n%1").arg(path);
                return;
            }
            m_frame = raw.scaled(target, Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation);
            return;
        }
        case SourceType::ColorSource: {
            m_frame = renderColorPreview(m_source, target);
            return;
        }
        case SourceType::TextSource: {
            m_frame = renderTextPreview(m_source, target);
            return;
        }
        case SourceType::TestPattern: {
            m_frame = renderTestPattern(target, m_pulse);
            return;
        }
        case SourceType::Microphone:
        case SourceType::DesktopAudio: {
            m_frameNote = tr("Аудио-источник — без видео-превью.");
            return;
        }
    }
}

void PreviewWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRect r = rect().adjusted(8, 8, -8, -8);
    QPainterPath path;
    path.addRoundedRect(r, 16, 16);

    QColor accent = m_theme ? m_theme->accent() : QColor(255, 153, 0);
    QLinearGradient bg(r.topLeft(), r.bottomRight());
    bg.setColorAt(0.0, QColor(20, 20, 20));
    bg.setColorAt(1.0, QColor(8, 8, 8));
    p.fillPath(path, bg);

    p.save();
    p.setClipPath(path);
    if (m_hasSource && !m_frame.isNull()) {
        // Center the live preview frame on the card.
        const QSize fs = m_frame.size().scaled(r.size(), Qt::KeepAspectRatio);
        const QRect dst((r.width() - fs.width()) / 2 + r.left(),
                        (r.height() - fs.height()) / 2 + r.top(),
                        fs.width(), fs.height());
        p.drawPixmap(dst, m_frame);
    } else {
        paintIdle(p, r);
    }
    p.restore();

    QPen border(accent, 1.4);
    p.setPen(border);
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);

    p.setPen(QColor(180, 180, 180));
    QFont f = p.font();
    f.setPointSize(10);
    f.setWeight(QFont::Medium);
    p.setFont(f);

    QString caption;
    if (m_live) {
        caption = tr("В ЭФИРЕ");
    } else if (m_hasSource && !m_frameNote.isEmpty()) {
        caption = m_frameNote;
    } else if (m_hasSource) {
        caption = tr("Превью: %1").arg(m_source.name);
    } else {
        caption = tr("Добавь источник на панели «Источники»");
    }
    QRect captionRect(r.left() + 12, r.bottom() - 44, r.width() - 24, 18);
    p.setPen(QColor(0, 0, 0, 160));
    p.drawText(captionRect.translated(1, 1), Qt::AlignLeft | Qt::AlignVCenter, caption);
    p.setPen(QColor(220, 220, 220));
    p.drawText(captionRect, Qt::AlignLeft | Qt::AlignVCenter, caption);

    if (!m_summary.isEmpty()) {
        p.setPen(QColor(160, 160, 160));
        f.setPointSize(9);
        f.setWeight(QFont::Normal);
        p.setFont(f);
        QRect sumRect(r.left() + 12, captionRect.bottom() + 2,
                       r.width() - 24, 18);
        p.drawText(sumRect, Qt::AlignLeft | Qt::AlignVCenter, m_summary);
    }
}

void PreviewWidget::paintIdle(QPainter& p, const QRect& r) {
    QColor accent = m_theme ? m_theme->accent() : QColor(255, 153, 0);
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

    const qreal phase = (m_pulse / 360.0) * 2.0 * M_PI;
    const qreal radius = 22.0 + 6.0 * std::sin(phase);
    QColor pulseColor = m_live ? QColor(224, 59, 59) : accent;
    p.setBrush(pulseColor);
    p.drawEllipse(center, radius, radius);

    QPainterPath glyph;
    if (m_live) {
        glyph.addRoundedRect(QRectF(center.x() - 8, center.y() - 8, 16, 16),
                             3, 3);
    } else {
        glyph.moveTo(center.x() - 6, center.y() - 9);
        glyph.lineTo(center.x() + 10, center.y());
        glyph.lineTo(center.x() - 6, center.y() + 9);
        glyph.closeSubpath();
    }
    p.fillPath(glyph, QColor(10, 10, 10));
}

void PreviewWidget::paintFrame(QPainter&, const QRect&) {
    // Drawing happens inline in paintEvent so we can apply clipping.
}

} // namespace lumen
