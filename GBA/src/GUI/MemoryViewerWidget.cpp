#include <GUI/MemoryViewerWidget.hpp>
#include <QtWidgets/QtWidgets>
#include <string>

namespace GUI
{
MemoryViewerWidget::MemoryViewerWidget(QWidget* parent) :
    QWidget(parent)
{
    InitializeLayout();
}

void MemoryViewerWidget::InitializeLayout()
{
    auto layout = new QVBoxLayout(this);

    auto label = new QLabel("Memory Viewer", this);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);

    searchBox_ = new QLineEdit(this);
    searchBox_->setPlaceholderText("Search...");
    layout->addWidget(searchBox_);

    memoryTable_ = new QTableWidget(1, 2, this);
    memoryTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    memoryTable_->setHorizontalHeaderLabels({"Address", "Word"});
    memoryTable_->verticalHeader()->setVisible(false);
    memoryTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(memoryTable_);

    setLayout(layout);
}
}
