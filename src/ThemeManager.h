#pragma once

#include <QColor>
#include <QObject>
#include <QString>
#include <QTimer>

class QApplication;

namespace lumen {

// Three first-class visual modes. Light + Blackout share static QSS
// templates; RGB animates the accent hue continuously.
enum class Theme {
    Light,
    Blackout,
    Rgb,
};

// Resolves a theme keyword to a translated display label.
QString themeDisplayName(Theme t);

// Owns the global application stylesheet plus the optional RGB animation
// timer. The QSS files in resources/themes/*.qss contain {{ACCENT}},
// {{ACCENT_DIM}}, etc. placeholders that this class substitutes at apply
// time so we can change the accent at runtime without reloading files.
class ThemeManager : public QObject {
    Q_OBJECT
public:
    explicit ThemeManager(QApplication* app, QObject* parent = nullptr);

    Theme currentTheme() const { return m_theme; }
    QColor accent() const { return m_accent; }

    void applyTheme(Theme t);
    // Override the accent color (only meaningful for Light/Blackout —
    // RGB ignores the override and rotates).
    void setAccent(const QColor& color);

signals:
    void themeChanged(Theme t);
    void accentChanged(const QColor& c);

private slots:
    void onRgbTick();

private:
    QString loadQssTemplate(Theme t) const;
    void rebuildStylesheet();

    QApplication* m_app;
    Theme         m_theme = Theme::Blackout;
    // m_accent is the *displayed* color, which is mutated every RGB tick.
    // m_userAccent is the user's static pick, preserved across RGB cycles.
    QColor        m_accent     = QColor(255, 153, 0); // amber-orange
    QColor        m_userAccent = QColor(255, 153, 0);
    QTimer        m_rgbTimer;
    qreal         m_rgbHue = 30.0; // start near amber
};

} // namespace lumen
