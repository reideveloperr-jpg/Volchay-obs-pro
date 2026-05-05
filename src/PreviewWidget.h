#pragma once

#include <QString>
#include <QWidget>

namespace lumen {

class ThemeManager;

// Placeholder canvas where a video preview would live. Renders a soft
// glassmorphic card with the current preset summary and a centered
// pulse marker so the UI never looks empty before streaming begins.
class PreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit PreviewWidget(ThemeManager* theme, QWidget* parent = nullptr);

    void setSummary(const QString& s);
    void setLive(bool live);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    ThemeManager* m_theme;
    QString       m_summary;
    bool          m_live = false;
    int           m_pulse = 0;
};

} // namespace lumen
