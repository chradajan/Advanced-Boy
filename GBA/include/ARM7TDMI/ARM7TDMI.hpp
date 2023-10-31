#pragma once

#include <ARM7TDMI/CpuTypes.hpp>
#include <ARM7TDMI/Registers.hpp>
#include <cstdint>
#include <functional>
#include <memory>

namespace CPU::THUMB
{
    class SoftwareInterrupt;
    class UnconditionalBranch;
    class ConditionalBranch;
    class MultipleLoadStore;
    class LongBranchWithLink;
    class AddOffsetToStackPointer;
    class PushPopRegisters;
    class LoadStoreHalfword;
    class SPRelativeLoadStore;
    class LoadAddress;
    class LoadStoreWithImmediateOffset;
    class LoadStoreWithRegisterOffset;
    class LoadStoreSignExtendedByteHalfword;
    class PCRelativeLoad;
    class HiRegisterOperationsBranchExchange;
    class ALUOperations;
    class MoveCompareAddSubtractImmediate;
    class AddSubtract;
    class MoveShiftedRegister;
}

namespace CPU::ARM
{
    class BranchAndExchange;
    class BlockDataTransfer;
    class Branch;
    class SoftwareInterrupt;
    class Undefined;
    class SingleDataTransfer;
    class SingleDataSwap;
    class Multiply;
    class MultiplyLong;
    class HalfwordDataTransferRegisterOffset;
    class HalfwordDataTransferImmediateOffset;
    class PSRTransferMRS;
    class PSRTransferMSR;
    class DataProcessing;
}

namespace CPU
{
class ARM7TDMI
{
public:
    /// @brief Initialize the CPU to ARM state in System mode with all other registers set to 0.
    ARM7TDMI(std::function<uint32_t(uint32_t, uint8_t)> ReadMemory,
             std::function<void(uint32_t, uint32_t, uint8_t)> WriteMemory);

    ARM7TDMI() = delete;
    ARM7TDMI(ARM7TDMI const&) = delete;
    ARM7TDMI(ARM7TDMI&&) = delete;
    ARM7TDMI& operator=(ARM7TDMI const&) = delete;
    ARM7TDMI& operator=(ARM7TDMI&&) = delete;
    ~ARM7TDMI() = default;

    /// @brief Advance the CPU by one clock cycle. For now, always run at 1CPI.
    void Clock();

private:
    /// @brief Determine whether the command should execute based on its condition.
    /// @param condition 4-bit condition.
    /// @return True if command should be executed, false otherwise
    bool ArmConditionMet(uint8_t condition);

    // F/D/E cycle state
    uint32_t rawFetchedInstruction_;
    std::unique_ptr<Instruction> decodedInstruction_;

    // R/W Functions
    std::function<uint32_t(uint32_t, uint8_t)> ReadMemory;
    std::function<void(uint32_t, uint32_t, uint8_t)> WriteMemory;

    // Registers
    Registers registers_;

    // Lots of friends. Kind of annoying, but it's the only way to easily couple the CPU with individual instruction classes.
    friend class THUMB::SoftwareInterrupt;
    friend class THUMB::UnconditionalBranch;
    friend class THUMB::ConditionalBranch;
    friend class THUMB::MultipleLoadStore;
    friend class THUMB::LongBranchWithLink;
    friend class THUMB::AddOffsetToStackPointer;
    friend class THUMB::PushPopRegisters;
    friend class THUMB::LoadStoreHalfword;
    friend class THUMB::SPRelativeLoadStore;
    friend class THUMB::LoadAddress;
    friend class THUMB::LoadStoreWithImmediateOffset;
    friend class THUMB::LoadStoreWithRegisterOffset;
    friend class THUMB::LoadStoreSignExtendedByteHalfword;
    friend class THUMB::PCRelativeLoad;
    friend class THUMB::HiRegisterOperationsBranchExchange;
    friend class THUMB::ALUOperations;
    friend class THUMB::MoveCompareAddSubtractImmediate;
    friend class THUMB::AddSubtract;
    friend class THUMB::MoveShiftedRegister;

    friend class ARM::BranchAndExchange;
    friend class ARM::BlockDataTransfer;
    friend class ARM::Branch;
    friend class ARM::SoftwareInterrupt;
    friend class ARM::Undefined;
    friend class ARM::SingleDataTransfer;
    friend class ARM::SingleDataSwap;
    friend class ARM::Multiply;
    friend class ARM::MultiplyLong;
    friend class ARM::HalfwordDataTransferRegisterOffset;
    friend class ARM::HalfwordDataTransferImmediateOffset;
    friend class ARM::PSRTransferMRS;
    friend class ARM::PSRTransferMSR;
    friend class ARM::DataProcessing;
};

}  // namespace CPU
