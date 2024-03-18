#pragma once

#include <QtWidgets/QtWidgets>

namespace GUI
{
class MemoryViewerWidget : public QWidget
{
    Q_OBJECT

public:
    MemoryViewerWidget(QWidget* parent = nullptr);
    ~MemoryViewerWidget() = default;

private:
    void InitializeLayout();

    QLineEdit* searchBox_;
    QComboBox* wordSizeDropdown_;
    QTableWidget* memoryTable_;
};
}
