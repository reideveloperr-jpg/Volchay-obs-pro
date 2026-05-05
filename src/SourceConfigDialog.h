#pragma once

#include "Source.h"

#include <QDialog>

class QComboBox;
class QLineEdit;
class QSpinBox;
class QCheckBox;
class QPushButton;
class QLabel;
class QPlainTextEdit;

namespace lumen {

// One modal dialog used for both "Add new source" and "Configure existing
// source". The dialog rebuilds its body whenever the type changes so we
// only need a single class to maintain.
class SourceConfigDialog : public QDialog {
    Q_OBJECT
public:
    enum class Mode { AddNew, EditExisting };

    SourceConfigDialog(const Source& initial, Mode mode, QWidget* parent = nullptr);

    Source result() const { return m_source; }

private slots:
    void onPickFile();
    void onPickColor();
    void onPickTextColor();
    void onPickTextBgColor();
    void onRefreshDevices();
    void accept() override;

private:
    void rebuildBody();      // rebuild type-specific controls
    void writeBackFromUi();
    void readToUi();

    Source         m_source;
    Mode           m_mode;

    QLineEdit*     m_nameEdit = nullptr;
    QComboBox*     m_typeCombo = nullptr;     // disabled when editing existing

    QWidget*       m_bodyHost = nullptr;      // container we re-fill
    QPushButton*   m_refreshBtn = nullptr;

    // Per-type control pointers (only one set is alive at a time).
    QComboBox*     m_screenCombo = nullptr;
    QLineEdit*     m_windowTitleEdit = nullptr;
    QComboBox*     m_videoDeviceCombo = nullptr;
    QComboBox*     m_audioDeviceCombo = nullptr;
    QLineEdit*     m_filePathEdit = nullptr;
    QPushButton*   m_filePickBtn = nullptr;
    QCheckBox*     m_loopCheck = nullptr;
    QPushButton*   m_colorBtn = nullptr;
    QSpinBox*      m_colorWidthSpin = nullptr;
    QSpinBox*      m_colorHeightSpin = nullptr;
    QPlainTextEdit* m_textBodyEdit = nullptr;
    QSpinBox*      m_textSizeSpin = nullptr;
    QPushButton*   m_textColorBtn = nullptr;
    QPushButton*   m_textBgBtn = nullptr;
    QSpinBox*      m_volumeSpin = nullptr;
};

} // namespace lumen
