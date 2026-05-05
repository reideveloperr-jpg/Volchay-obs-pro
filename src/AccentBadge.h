#pragma once

#include <QWidget>

namespace lumen {

class ThemeManager;

// Tiny circular dot that always shows the live accent color. Used in the
// sidebar header to signal which theme/accent is active and to give the
// RGB theme a visible animated focal point.
class AccentBadge : public QWidget {
    Q_OBJECT
public:
    explicit AccentBadge(ThemeManager* theme, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize sizeHint() const override { return {16, 16}; }

private:
    ThemeManager* m_theme;
};

} // namespace lumen
