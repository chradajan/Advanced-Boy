#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <CPU/ArmInstructions.hpp>
#include <CPU/CPUTypes.hpp>
#include <CPU/Registers.hpp>
#include <CPU/ThumbInstructions.hpp>
#include <Utilities/CircularBuffer.hpp>
#include <Utilities/Functor.hpp>
#include <Utilities/MemoryUtilities.hpp>

class GameBoyAdvance;

namespace CPU
{
class ARM7TDMI
{
public:
    /// @brief Initialize the ARM CPU.
    /// @param readCallback Callback function to read memory.
    /// @param writeCallback Callback function to write memory.
    ARM7TDMI(MemReadCallback readCallback, MemWriteCallback writeCallback);

    ARM7TDMI() = delete;
    ARM7TDMI(ARM7TDMI const&) = delete;
    ARM7TDMI(ARM7TDMI&&) = delete;
    ARM7TDMI& operator=(ARM7TDMI const&) = delete;
    ARM7TDMI& operator=(ARM7TDMI&&) = delete;

    /// @brief Reset the CPU to its power-up state.
    void Reset();

    /// @brief Advance the CPU by one instruction. GLobal scheduler will be advanced as well.
    void Step();

    /// @brief Get the current value of the PC register.
    /// @return Value of PC.
    uint32_t GetPC() const { return registers_.GetPC(); }

private:
    /// @brief Determine whether a command should execute based on its condition code.
    /// @param condition 4-bit ARM condition code.
    /// @return True if condition is satisfied and the instruction should be executed.
    bool ArmConditionSatisfied(uint8_t condition);

    /// @brief Callback function for an IRQ event.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void IRQ(int);

    // Memory R/W callbacks
    MemReadCallback ReadMemory;
    MemWriteCallback WriteMemory;

    // Instruction pipeline
    CircularBuffer<std::pair<uint32_t, uint32_t>, 3> pipeline_;
    alignas(32) char instructionBuffer_[32];
    bool flushPipeline_;

    // ARM registers
    Registers registers_;

    // Logging
    std::string mnemonic_;
    std::string regString_;

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
};
}
