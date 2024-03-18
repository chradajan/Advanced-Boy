#include <GUI/DebugWindow.hpp>
#include <GUI/MemoryViewerWidget.hpp>
#include <GUI/RegisterViewerWidget.hpp>
#include <QtWidgets/QtWidgets>

namespace GUI
{
DebugWindow::DebugWindow(QWidget* parent) :
    QWidget(parent)
{
    setWindowTitle("Debugger");
    InitializeLayout();
}

void DebugWindow::InitializeLayout()
{
    auto layout = new QHBoxLayout(this);

    instructionBox_ = new QPlainTextEdit(this);
    instructionBox_->setReadOnly(true);
    layout->addWidget(instructionBox_, 2);

    memoryViewer_ = new MemoryViewerWidget(this);
    layout->addWidget(memoryViewer_, 2);

    registerViewer_ = new RegisterViewerWidget(this);
    layout->addWidget(registerViewer_, 1);

    setLayout(layout);
}
}
