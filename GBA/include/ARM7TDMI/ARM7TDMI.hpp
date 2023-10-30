#pragma once

#include <ARM7TDMI/CpuTypes.hpp>
#include <ARM7TDMI/Registers.hpp>

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
    ARM7TDMI();

private:
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
