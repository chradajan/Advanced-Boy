#pragma once

#include <array>
#include <cstdint>
#include <utility>
#include <ARM7TDMI/ArmInstructions.hpp>
#include <ARM7TDMI/CpuTypes.hpp>
#include <ARM7TDMI/Registers.hpp>
#include <ARM7TDMI/ThumbInstructions.hpp>
#include <Utilities/CircularBuffer.hpp>
#include <Utilities/MemoryUtilities.hpp>

class GameBoyAdvance;

namespace CPU
{
class ARM7TDMI
{
public:
    /// @brief Initialize the CPU to ARM state in Supervisor mode.
    /// @param gbaPtr Pointer to GBA instance used for reading/writing memory.
    /// @param biosLoaded Whether BIOS is loaded into memory.
    ARM7TDMI(GameBoyAdvance* gbaPtr, bool biosLoaded);

    ARM7TDMI() = delete;
    ARM7TDMI(ARM7TDMI const&) = delete;
    ARM7TDMI(ARM7TDMI&&) = delete;
    ARM7TDMI& operator=(ARM7TDMI const&) = delete;
    ARM7TDMI& operator=(ARM7TDMI&&) = delete;
    ~ARM7TDMI() = default;

    /// @brief Execute a CPU instruction.
    /// @return Number of cycles CPU spent executing instruction.
    int Tick();

    /// @brief Get the current value of the PC register.
    /// @return Current PC value;
    uint32_t GetPC() const { return registers_.GetPC(); }

private:
    /// @brief Read from memory managed by GBA.
    /// @param addr Address to read.
    /// @param alignment BYTE, HALFWORD, or WORD.
    /// @return Value from specified address and number of cycles taken to read.
    std::pair<uint32_t, int> ReadMemory(uint32_t addr, AccessSize alignment);

    /// @brief Write to memory managed by GBA.
    /// @param addr Address to write.
    /// @param value Value to write to specified address.
    /// @param alignment BYTE, HALFWORD, or WORD.
    /// @return Number of cycles taken to write.
    int WriteMemory(uint32_t addr, uint32_t value, AccessSize alignment);

    /// @brief Determine whether the command should execute based on its condition.
    /// @param condition 4-bit condition.
    /// @return True if command should be executed, false otherwise
    bool ArmConditionMet(uint8_t condition);

    /// @brief Callback function for an IRQ event.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void IRQ(int);

    // F/D/E cycle state
    CircularBuffer<std::pair<uint32_t, uint32_t>, 3> pipeline_;
    bool flushPipeline_;

    // GBA
    GameBoyAdvance* gbaPtr_;

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
    friend Instruction* THUMB::DecodeInstruction(uint16_t, ARM7TDMI&);

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
    friend Instruction* ARM::DecodeInstruction(uint32_t, ARM7TDMI&);

    THUMB::SoftwareInterrupt thumbSoftwareInterrupt;
    THUMB::UnconditionalBranch thumbUnconditionalBranch;
    THUMB::ConditionalBranch thumbConditionalBranch;
    THUMB::MultipleLoadStore thumbMultipleLoadStore;
    THUMB::LongBranchWithLink thumbLongBranchWithLink;
    THUMB::AddOffsetToStackPointer thumbAddOffsetToStackPointer;
    THUMB::PushPopRegisters thumbPushPopRegisters;
    THUMB::LoadStoreHalfword thumbLoadStoreHalfword;
    THUMB::SPRelativeLoadStore thumbSPRelativeLoadStore;
    THUMB::LoadAddress thumbLoadAddress;
    THUMB::LoadStoreWithImmediateOffset thumbLoadStoreWithImmediateOffset;
    THUMB::LoadStoreWithRegisterOffset thumbLoadStoreWithRegisterOffset;
    THUMB::LoadStoreSignExtendedByteHalfword thumbLoadStoreSignExtendedByteHalfword;
    THUMB::PCRelativeLoad thumbPCRelativeLoad;
    THUMB::HiRegisterOperationsBranchExchange thumbHiRegisterOperationsBranchExchange;
    THUMB::ALUOperations thumbALUOperations;
    THUMB::MoveCompareAddSubtractImmediate thumbMoveCompareAddSubtractImmediate;
    THUMB::AddSubtract thumbAddSubtract;
    THUMB::MoveShiftedRegister thumbMoveShiftedRegister;

    ARM::BranchAndExchange armBranchAndExchange;
    ARM::BlockDataTransfer armBlockDataTransfer;
    ARM::Branch armBranch;
    ARM::SoftwareInterrupt armSoftwareInterrupt;
    ARM::Undefined armUndefined;
    ARM::SingleDataTransfer armSingleDataTransfer;
    ARM::SingleDataSwap armSingleDataSwap;
    ARM::Multiply armMultiply;
    ARM::MultiplyLong armMultiplyLong;
    ARM::HalfwordDataTransferRegisterOffset armHalfwordDataTransferRegisterOffset;
    ARM::HalfwordDataTransferImmediateOffset armHalfwordDataTransferImmediateOffset;
    ARM::PSRTransferMRS armPSRTransferMRS;
    ARM::PSRTransferMSR armPSRTransferMSR;
    ARM::DataProcessing armDataProcessing;
};

}  // namespace CPU
