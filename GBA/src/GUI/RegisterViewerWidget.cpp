#include <GUI/RegisterViewerWidget.hpp>
#include <QtWidgets/QtWidgets>
#include <array>
#include <cstdint>
#include <format>
#include <iomanip>
#include <sstream>
#include <string>

namespace GUI
{
RegisterViewerWidget::RegisterViewerWidget(QWidget* parent) :
    QWidget(parent)
{
    InitializeLayout();
}

void RegisterViewerWidget::InitializeLayout()
{
    auto layout = new QHBoxLayout(this);

    regularRegisters_ = new RegularRegistersWidget(this);
    layout->addWidget(regularRegisters_);

    setLayout(layout);
}

RegularRegistersWidget::RegularRegistersWidget(QWidget* parent) :
    QWidget(parent)
{
    InitializeLayout();
    SetRegisters({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
}

void RegularRegistersWidget::SetRegisters(std::array<uint32_t, 17> const& registers)
{
    std::stringstream registerString;

    for (size_t i = 0; i < 10; ++i)
    {
        registerString << "R" << i << "      0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << (unsigned int)registers[i] << "\n";
    }

    for (size_t i = 10; i < 16; ++i)
    {
        registerString << std::dec << "R" << i << "    0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << (unsigned int)registers[i] << "\n";
    }

    registerString << "CPSR  0x" << std::hex << std::setw(8) << (unsigned int)registers[16];
    registerText_->document()->setPlainText(registerString.str().c_str());
}

void RegularRegistersWidget::InitializeLayout()
{
    auto layout = new QVBoxLayout(this);

    auto label = new QLabel("Registers", this);
    layout->addWidget(label);

    registerText_ = new QPlainTextEdit(this);
    registerText_->setReadOnly(true);
    layout->addWidget(registerText_);

    setLayout(layout);
}

BankedRegistersWidget::BankedRegistersWidget(QWidget* parent) :
    QWidget(parent)
{
    InitializeLayout();
    SetRegisters({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
}

void BankedRegistersWidget::SetRegisters(std::array<uint32_t, 20> const& registers)
{
    std::stringstream registerString;

    for (size_t i = 8; i < 15; ++i)
    {
        registerString << "R" << i << "      0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << (unsigned int)registers[i] << "_fiq\n";
    }

    for (size_t i = 10; i < 20; ++i)
    {
        registerString << std::dec << "R" << i << "    0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << (unsigned int)registers[i] << "\n";
    }

    registerText_->document()->setPlainText(registerString.str().c_str());
}

void BankedRegistersWidget::InitializeLayout()
{
    auto layout = new QVBoxLayout(this);

    auto label = new QLabel("Registers", this);
    layout->addWidget(label);

    registerText_ = new QPlainTextEdit(this);
    registerText_->setReadOnly(true);
    layout->addWidget(registerText_);

    setLayout(layout);
}
}
