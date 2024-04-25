#pragma once

#include <ARM7TDMI/CpuTypes.hpp>

#include <array>
#include <cstdint>
#include <functional>
#include <utility>

#include <ARM7TDMI/ArmInstructions.hpp>
#include <ARM7TDMI/Registers.hpp>
#include <ARM7TDMI/ThumbInstructions.hpp>
#include <System/MemoryMap.hpp>

namespace CPU
{
class ARM7TDMI
{
public:
    /// @brief Initialize the CPU to ARM state in System mode with all other registers set to 0.
    ARM7TDMI(std::function<std::pair<uint32_t, int>(uint32_t, AccessSize)> ReadMemory,
             std::function<int(uint32_t, uint32_t, AccessSize)> WriteMemory,
             bool biosLoaded);

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
    /// @brief Determine whether the command should execute based on its condition.
    /// @param condition 4-bit condition.
    /// @return True if command should be executed, false otherwise
    bool ArmConditionMet(uint8_t condition);

    /// @brief Callback function for an IRQ event.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void IRQ(int);

    /// @brief Handle a CPU halt. Toggles between halted/normal when called.
    /// @param  extraCycles Number of cycles that passed since this event was supposed to execute.
    void HALT(int) { halted_ = !halted_; };

    // F/D/E cycle state
    class InstructionPipeline
    {
        public:
            /// @brief Initialize the instruction pipeline by clearing it.
            InstructionPipeline() { Clear(); }

            /// @brief Fill pipeline with invalid instructions and clear counts.
            void Clear();
        
            /// @brief Add an instruction to the pipeline to be executed.
            /// @param nextInstruction Instruction code to decode and execute.
            /// @param pc Address of instruction being added to pipeline.
            void Push(uint32_t nextInstruction, uint32_t pc);

            /// @brief Get the oldest instruction in the pipeline and remove it.
            /// @return Raw instruction code and address of instruction.
            std::pair<uint32_t, uint32_t> Pop();

            /// @brief Peak at the oldest instruction in the pipeline without removing it.
            /// @return Raw instruction code and address of instruction.
            std::pair<uint32_t, uint32_t> Front() const { return fetchedInstructions_[front_]; }

            /// @brief Check if the pipeline is empty.
            /// @return True if no instructions are waiting in pipeline to be executed.
            bool Empty() const { return count_ == 0; }

            /// @brief Check if the pipeline is full and ready to start executing from.
            /// @return True if 3 instructions are waiting in pipeline to be executed.
            bool Full() const { return count_ == fetchedInstructions_.size(); }

        private:
            size_t front_;
            size_t insertionPoint_;
            size_t count_;
            std::array<std::pair<uint32_t, uint32_t>, 3> fetchedInstructions_;
    };

    InstructionPipeline pipeline_;
    bool flushPipeline_;

    // R/W Functions
    std::function<std::pair<uint32_t, int>(uint32_t, AccessSize)> ReadMemory;
    std::function<int(uint32_t, uint32_t, AccessSize)> WriteMemory;

    // Registers
    Registers registers_;

    // Current state
    bool halted_;

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
