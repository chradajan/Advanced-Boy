#pragma once

#include <QtWidgets/QtWidgets>
#include <array>
#include <cstdint>

namespace GUI
{
class RegularRegistersWidget : public QWidget
{
    Q_OBJECT

public:
    RegularRegistersWidget(QWidget* parent = nullptr);
    ~RegularRegistersWidget() = default;

    /// @brief Update the displayed register values.
    /// @param registers Array of regular registers (R0-R15, then CPSR).
    void SetRegisters(std::array<uint32_t, 17> const& registers);

private:
    void InitializeLayout();

    QPlainTextEdit* registerText_;
};

class BankedRegistersWidget : public QWidget
{
    Q_OBJECT

public:
    BankedRegistersWidget(QWidget* parent = nullptr);
    ~BankedRegistersWidget() = default;

    /// @brief Update the displayed register values.
    /// @param registers Array of banked registers.
    void SetRegisters(std::array<uint32_t, 20> const& registers);

private:
    void InitializeLayout();

    QPlainTextEdit* registerText_;
};

// class FlagsWidget : public QWidget
// {
//     Q_OBJECT

// public:
//     FlagsWidget(QWidget* parent = nullptr);
//     ~FlagsWidget() = default;

// private:
//     void InitializeLayout();
// };

class RegisterViewerWidget : public QWidget
{
    Q_OBJECT

public:
    RegisterViewerWidget(QWidget* parent = nullptr);
    ~RegisterViewerWidget() = default;

private:
    void InitializeLayout();

    RegularRegistersWidget* regularRegisters_;
};
}
