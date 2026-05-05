#pragma once

#include "Source.h"

#include <QPixmap>
#include <QString>
#include <QWidget>

namespace lumen {

class ThemeManager;

// Live preview canvas. When the user selects a video source, we render
// its current frame at ~10fps so the GUI feels OBS-like even though we
// don't ship a full GPU compositor. Audio sources show a placeholder.
//
// Falls back to the original glassmorphic "no source" pulse animation
// when the source list is empty or no video source is enabled.
class PreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit PreviewWidget(ThemeManager* theme, QWidget* parent = nullptr);

    void setSummary(const QString& s);
    void setLive(bool live);
    // Owned by MainWindow; we hold a const-ref pointer just to render it.
    void setActiveSource(const Source* source);

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void tickFrame();

private:
    void grabFrameFromSource();
    void paintIdle(QPainter& p, const QRect& r);
    void paintFrame(QPainter& p, const QRect& r);

    ThemeManager* m_theme;
    QString       m_summary;
    bool          m_live = false;
    int           m_pulse = 0;

    Source        m_source;
    bool          m_hasSource = false;
    QPixmap       m_frame;        // last grabbed frame, scaled to widget
    QString       m_frameNote;    // shown when source can't be previewed locally
};

} // namespace lumen
