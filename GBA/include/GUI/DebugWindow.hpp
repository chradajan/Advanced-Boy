#pragma once

#include <QtWidgets/QtWidgets>

namespace GUI
{
class MemoryViewerWidget;
class RegisterViewerWidget;

class DebugWindow : public QWidget
{
    Q_OBJECT

public:
    DebugWindow(QWidget* parent = nullptr);
    ~DebugWindow() = default;

private:
    void InitializeLayout();

    QPlainTextEdit* instructionBox_;
    MemoryViewerWidget* memoryViewer_;
    RegisterViewerWidget* registerViewer_;
};
}
