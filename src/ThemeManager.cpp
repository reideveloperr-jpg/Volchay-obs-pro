#include "ThemeManager.h"

#include <QApplication>
#include <QFile>
#include <QTextStream>

namespace lumen {

QString themeDisplayName(Theme t) {
    switch (t) {
        case Theme::Light:    return QObject::tr("Светлая");
        case Theme::Blackout: return QObject::tr("Blackout");
        case Theme::Rgb:      return QObject::tr("RGB");
    }
    return QString();
}

ThemeManager::ThemeManager(QApplication* app, QObject* parent)
    : QObject(parent), m_app(app) {
    m_rgbTimer.setInterval(33); // ~30 Hz hue cycling — enough to look smooth
    connect(&m_rgbTimer, &QTimer::timeout, this, &ThemeManager::onRgbTick);
}

void ThemeManager::applyTheme(Theme t) {
    m_theme = t;
    if (t == Theme::Rgb) {
        m_rgbTimer.start();
    } else {
        m_rgbTimer.stop();
        // Restore the user's static accent — m_accent gets clobbered by
        // every onRgbTick(), so we have to recover it from m_userAccent.
        m_accent = m_userAccent.isValid() ? m_userAccent : QColor(255, 153, 0);
    }
    rebuildStylesheet();
    emit themeChanged(t);
}

void ThemeManager::setAccent(const QColor& color) {
    if (!color.isValid()) return;
    m_userAccent = color;
    // Under RGB the displayed color is owned by the animation timer, so
    // we don't touch m_accent here — but we still persist the user's pick
    // so a later switch back to Light/Blackout restores it.
    if (m_theme != Theme::Rgb) {
        m_accent = color;
        rebuildStylesheet();
    }
    emit accentChanged(color);
}

void ThemeManager::onRgbTick() {
    m_rgbHue += 1.5; // degrees per tick
    if (m_rgbHue >= 360.0) m_rgbHue -= 360.0;
    // Only mutate the display accent; m_userAccent stays untouched.
    m_accent = QColor::fromHsvF(m_rgbHue / 360.0, 0.85, 1.0);
    rebuildStylesheet();
    emit accentChanged(m_accent);
}

QString ThemeManager::loadQssTemplate(Theme t) const {
    const char* path = ":/resources/themes/blackout.qss";
    switch (t) {
        case Theme::Light:    path = ":/resources/themes/light.qss"; break;
        case Theme::Blackout: path = ":/resources/themes/blackout.qss"; break;
        case Theme::Rgb:      path = ":/resources/themes/rgb.qss"; break;
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
    return QString::fromUtf8(f.readAll());
}

void ThemeManager::rebuildStylesheet() {
    QString qss = loadQssTemplate(m_theme);
    if (qss.isEmpty()) return;

    auto rgbStr = [](const QColor& c) {
        return QString("rgb(%1, %2, %3)").arg(c.red()).arg(c.green()).arg(c.blue());
    };
    auto rgbaStr = [](const QColor& c, qreal a) {
        return QString("rgba(%1, %2, %3, %4)")
            .arg(c.red()).arg(c.green()).arg(c.blue())
            .arg(int(a * 255));
    };

    QColor accentDim = m_accent.darker(140);
    QColor accentSoft = m_accent.lighter(120);

    qss.replace(QStringLiteral("{{ACCENT}}"),       rgbStr(m_accent));
    qss.replace(QStringLiteral("{{ACCENT_DIM}}"),   rgbStr(accentDim));
    qss.replace(QStringLiteral("{{ACCENT_SOFT}}"),  rgbStr(accentSoft));
    qss.replace(QStringLiteral("{{ACCENT_GHOST}}"), rgbaStr(m_accent, 0.18));

    if (m_app) m_app->setStyleSheet(qss);
}

} // namespace lumen
