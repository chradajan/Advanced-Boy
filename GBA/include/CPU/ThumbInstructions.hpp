#pragma once

#include <cstdint>
#include <CPU/CPUTypes.hpp>

namespace CPU { class ARM7TDMI; }

namespace CPU::THUMB
{
/// @brief Decode a 16 bit value into a THUMB instruction.
/// @param undecodedInstruction 16 bit value to decode.
/// @param buffer Pointer to buffer to place instruction object.
/// @return Pointer to newly constructed instruction object.
CPU::Instruction* DecodeInstruction(uint16_t undecodedInstruction, void* buffer);

class SoftwareInterrupt : public virtual Instruction
{
public:
    /// @brief SoftwareInterrupt instruction constructor.
    /// @param instruction 16-bit THUMB instruction.
    /// @pre IsInstanceOf must be true.
    SoftwareInterrupt(uint16_t instruction) { instruction_.halfword = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 16-bit THUMB instruction.
    /// @return Whether this instruction is a Software Interrupt instruction.
    static constexpr bool IsInstanceOf(uint16_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    void SetMnemonic(std::string& mnemonic);

    static constexpr uint16_t FORMAT =      0b1101'1111'0000'0000;
    static constexpr uint16_t FORMAT_MASK = 0b1111'1111'0000'0000;

    union
    {
        struct
        {
            uint16_t Value8 : 8;
            uint16_t : 8;
        };

        uint16_t halfword;
    } instruction_;
};

class UnconditionalBranch : public virtual Instruction
{
public:
    /// @brief UnconditionalBranch instruction constructor.
    /// @param instruction 16-bit THUMB instruction.
    /// @pre IsInstanceOf must be true.
    UnconditionalBranch(uint16_t instruction) { instruction_.halfword = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 16-bit THUMB instruction.
    /// @return Whether this instruction is an Unconditional Branch instruction.
    static constexpr bool IsInstanceOf(uint16_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    /// @param newPC New PC to branch to.
    void SetMnemonic(std::string& mnemonic, uint32_t newPC);

    static constexpr uint16_t FORMAT =      0b1110'0000'0000'0000;
    static constexpr uint16_t FORMAT_MASK = 0b1111'1000'0000'0000;

    union
    {
        struct
        {
            uint16_t Offset11 : 11;
            uint16_t : 5;
        };

        uint16_t halfword;
    } instruction_;
};

class ConditionalBranch : public virtual Instruction
{
public:
    /// @brief ConditionalBranch instruction constructor.
    /// @param instruction 16-bit THUMB instruction.
    /// @pre IsInstanceOf must be true.
    ConditionalBranch(uint16_t instruction) { instruction_.halfword = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 16-bit THUMB instruction.
    /// @return Whether this instruction is a Conditional Branch instruction.
    static constexpr bool IsInstanceOf(uint16_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    /// @param newPC New PC to branch to.
    void SetMnemonic(std::string& mnemonic, uint32_t newPC);

    static constexpr uint16_t FORMAT =      0b1101'0000'0000'0000;
    static constexpr uint16_t FORMAT_MASK = 0b1111'0000'0000'0000;

    union
    {
        struct
        {
            uint16_t SOffset8 : 8;
            uint16_t Cond : 4;
            uint16_t : 4;
        };

        uint16_t halfword;
    } instruction_;
};

class MultipleLoadStore : public virtual Instruction
{
public:
    /// @brief MultipleLoadStore instruction constructor.
    /// @param instruction 16-bit THUMB instruction.
    /// @pre IsInstanceOf must be true.
    MultipleLoadStore(uint16_t instruction) { instruction_.halfword = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 16-bit THUMB instruction.
    /// @return Whether this instruction is a Multiple Load/Store instruction.
    static constexpr bool IsInstanceOf(uint16_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    void SetMnemonic(std::string& mnemonic);

    static constexpr uint16_t FORMAT =      0b1100'0000'0000'0000;
    static constexpr uint16_t FORMAT_MASK = 0b1111'0000'0000'0000;

    union
    {
        struct
        {
            uint16_t Rlist : 8;
            uint16_t Rb : 3;
            uint16_t L : 1;
            uint16_t : 4;
        };

        uint16_t halfword;
    } instruction_;
};

class LongBranchWithLink : public virtual Instruction
{
public:
    /// @brief LongBranchWithLink instruction constructor.
    /// @param instruction 16-bit THUMB instruction.
    /// @pre IsInstanceOf must be true.
    LongBranchWithLink(uint16_t instruction) { instruction_.halfword = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 16-bit THUMB instruction.
    /// @return Whether this instruction is a Long Branch with Link instruction.
    static constexpr bool IsInstanceOf(uint16_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    /// @param newPC New PC value to branch to when H=1.
    void SetMnemonic(std::string& mnemonic, uint32_t newPC);

    static constexpr uint16_t FORMAT =      0b1111'0000'0000'0000;
    static constexpr uint16_t FORMAT_MASK = 0b1111'0000'0000'0000;

    union
    {
        struct
        {
            uint16_t Offset : 11;
            uint16_t H : 1;
            uint16_t : 4;
        };

        uint16_t halfword;
    } instruction_;
};

class AddOffsetToStackPointer : public virtual Instruction
{
public:
    /// @brief AddOffsetToStackPointer instruction constructor.
    /// @param instruction 16-bit THUMB instruction.
    /// @pre IsInstanceOf must be true.
    AddOffsetToStackPointer(uint16_t instruction) { instruction_.halfword = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 16-bit THUMB instruction.
    /// @return Whether this instruction is an Add Offset to Stack Pointer instruction.
    static constexpr bool IsInstanceOf(uint16_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    /// @param offset Offset to add/subtract to SP.
    void SetMnemonic(std::string& mnemonic, uint16_t offset);

    static constexpr uint16_t FORMAT =      0b1011'0000'0000'0000;
    static constexpr uint16_t FORMAT_MASK = 0b1111'1111'0000'0000;

    union
    {
        struct
        {
            uint16_t SWord7 : 7;
            uint16_t S : 1;
            uint16_t : 8;
        };

        uint16_t halfword;
    } instruction_;
};

class PushPopRegisters : public virtual Instruction
{
public:
    /// @brief PushPopRegisters instruction constructor.
    /// @param instruction 16-bit THUMB instruction.
    /// @pre IsInstanceOf must be true.
    PushPopRegisters(uint16_t instruction) { instruction_.halfword = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 16-bit THUMB instruction.
    /// @return Whether this instruction is a Push/Pop Registers instruction.
    static constexpr bool IsInstanceOf(uint16_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    void SetMnemonic(std::string& mnemonic);

    static constexpr uint16_t FORMAT =      0b1011'0100'0000'0000;
    static constexpr uint16_t FORMAT_MASK = 0b1111'0110'0000'0000;

    union
    {
        struct
        {
            uint16_t Rlist : 8;
            uint16_t R : 1;
            uint16_t : 2;
            uint16_t L : 1;
            uint16_t : 4;
        };

        uint16_t halfword;
    } instruction_;
};

class LoadStoreHalfword : public virtual Instruction
{
public:
    /// @brief LoadStoreHalfword instruction constructor.
    /// @param instruction 16-bit THUMB instruction.
    /// @pre IsInstanceOf must be true.
    LoadStoreHalfword(uint16_t instruction) { instruction_.halfword = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 16-bit THUMB instruction.
    /// @return Whether this instruction is a Load/Store Halfword instruction.
    static constexpr bool IsInstanceOf(uint16_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    void SetMnemonic(std::string& mnemonic);

    static constexpr uint16_t FORMAT =      0b1000'0000'0000'0000;
    static constexpr uint16_t FORMAT_MASK = 0b1111'0000'0000'0000;

    union
    {
        struct
        {
            uint16_t Rd : 3;
            uint16_t Rb : 3;
            uint16_t  Offset5 : 5;
            uint16_t L : 1;
            uint16_t : 4;
        };

        uint16_t halfword;
    } instruction_;
};

class SPRelativeLoadStore : public virtual Instruction
{
public:
    /// @brief SPRelativeLoadStore instruction constructor.
    /// @param instruction 16-bit THUMB instruction.
    /// @pre IsInstanceOf must be true.
    SPRelativeLoadStore(uint16_t instruction) { instruction_.halfword = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 16-bit THUMB instruction.
    /// @return Whether this instruction is an SP Relative Load/Store instruction.
    static constexpr bool IsInstanceOf(uint16_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    void SetMnemonic(std::string& mnemonic);

    static constexpr uint16_t FORMAT =      0b1001'0000'0000'0000;
    static constexpr uint16_t FORMAT_MASK = 0b1111'0000'0000'0000;

    union
    {
        struct
        {
            uint16_t Word8 : 8;
            uint16_t Rd : 3;
            uint16_t L : 1;
            uint16_t : 4;
        };

        uint16_t halfword;
    } instruction_;
};

class LoadAddress : public virtual Instruction
{
public:
    /// @brief LoadAddress instruction constructor.
    /// @param instruction 16-bit THUMB instruction.
    /// @pre IsInstanceOf must be true.
    LoadAddress(uint16_t instruction) { instruction_.halfword = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 16-bit THUMB instruction.
    /// @return Whether this instruction is a Load Address instruction.
    static constexpr bool IsInstanceOf(uint16_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    /// @param destIndex Register index to store address.
    /// @param offset Offset added to PC/SP.
    void SetMnemonic(std::string& mnemonic, uint8_t destIndex, uint16_t offset);

    static constexpr uint16_t FORMAT =      0b1010'0000'0000'0000;
    static constexpr uint16_t FORMAT_MASK = 0b1111'0000'0000'0000;

    union
    {
        struct
        {
            uint16_t Word8 : 8;
            uint16_t Rd : 3;
            uint16_t SP : 1;
            uint16_t : 4;
        };

        uint16_t halfword;
    } instruction_;
};

class LoadStoreWithImmediateOffset : public virtual Instruction
{
public:
    /// @brief LoadStoreWithImmediateOffset instruction constructor.
    /// @param instruction 16-bit THUMB instruction.
    /// @pre IsInstanceOf must be true.
    LoadStoreWithImmediateOffset(uint16_t instruction) { instruction_.halfword = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 16-bit THUMB instruction.
    /// @return Whether this instruction is a Load/Store with Immediate Offset instruction.
    static constexpr bool IsInstanceOf(uint16_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    void SetMnemonic(std::string& mnemonic);

    static constexpr uint16_t FORMAT =      0b0110'0000'0000'0000;
    static constexpr uint16_t FORMAT_MASK = 0b1110'0000'0000'0000;

    union
    {
        struct
        {
            uint16_t Rd : 3;
            uint16_t Rb : 3;
            uint16_t Offset5 : 5;
            uint16_t L : 1;
            uint16_t B : 1;
            uint16_t : 3;
        };

        uint16_t halfword;
    } instruction_;
};

class LoadStoreWithRegisterOffset : public virtual Instruction
{
public:
    /// @brief LoadStoreWithRegisterOffset instruction constructor.
    /// @param instruction 16-bit THUMB instruction.
    /// @pre IsInstanceOf must be true.
    LoadStoreWithRegisterOffset(uint16_t instruction) { instruction_.halfword = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 16-bit THUMB instruction.
    /// @return Whether this instruction is a Load/Store with Register Offset instruction.
    static constexpr bool IsInstanceOf(uint16_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    void SetMnemonic(std::string& mnemonic);

    static constexpr uint16_t FORMAT =      0b0101'0000'0000'0000;
    static constexpr uint16_t FORMAT_MASK = 0b1111'0010'0000'0000;

    union
    {
        struct
        {
            uint16_t Rd : 3;
            uint16_t Rb : 3;
            uint16_t Ro : 3;
            uint16_t : 1;
            uint16_t B : 1;
            uint16_t L : 1;
            uint16_t : 4;
        };

        uint16_t halfword;
    } instruction_;
};

class LoadStoreSignExtendedByteHalfword : public virtual Instruction
{
public:
    /// @brief LoadStoreSignExtendedByteHalfword instruction constructor.
    /// @param instruction 16-bit THUMB instruction.
    /// @pre IsInstanceOf must be true.
    LoadStoreSignExtendedByteHalfword(uint16_t instruction) { instruction_.halfword = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 16-bit THUMB instruction.
    /// @return Whether this instruction is a Load/Store Sign-Extended Byte/Halfword instruction.
    static constexpr bool IsInstanceOf(uint16_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    void SetMnemonic(std::string& mnemonic);

    static constexpr uint16_t FORMAT =      0b0101'0010'0000'0000;
    static constexpr uint16_t FORMAT_MASK = 0b1111'0010'0000'0000;

    union
    {
        struct
        {
            uint16_t Rd : 3;
            uint16_t Rb : 3;
            uint16_t Ro : 3;
            uint16_t : 1;
            uint16_t S : 1;
            uint16_t H : 1;
            uint16_t : 4;
        };

        uint16_t halfword;
    } instruction_;
};

class PCRelativeLoad : public virtual Instruction
{
public:
    /// @brief PCRelativeLoad instruction constructor.
    /// @param instruction 16-bit THUMB instruction.
    /// @pre IsInstanceOf must be true.
    PCRelativeLoad(uint16_t instruction) { instruction_.halfword = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 16-bit THUMB instruction.
    /// @return Whether this instruction is a PC Relative Load instruction.
    static constexpr bool IsInstanceOf(uint16_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    void SetMnemonic(std::string& mnemonic);

    static constexpr uint16_t FORMAT =      0b0100'1000'0000'0000;
    static constexpr uint16_t FORMAT_MASK = 0b1111'1000'0000'0000;

    union
    {
        struct
        {
            uint16_t Word8 : 8;
            uint16_t Rd : 3;
            uint16_t : 5;
        };

        uint16_t halfword;
    } instruction_;
};

class HiRegisterOperationsBranchExchange : public virtual Instruction
{
public:
    /// @brief HiRegisterOperationsBranchExchange instruction constructor.
    /// @param instruction 16-bit THUMB instruction.
    /// @pre IsInstanceOf must be true.
    HiRegisterOperationsBranchExchange(uint16_t instruction) { instruction_.halfword = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 16-bit THUMB instruction.
    /// @return Whether this instruction is an HI Register Operations/Branch Exchange instruction.
    static constexpr bool IsInstanceOf(uint16_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    /// @param destIndex Register index to save result to.
    /// @param srcIndex Register index source of operand.
    void SetMnemonic(std::string& mnemonic, uint8_t destIndex, uint8_t srcIndex);

    static constexpr uint16_t FORMAT =      0b0100'0100'0000'0000;
    static constexpr uint16_t FORMAT_MASK = 0b1111'1100'0000'0000;

    union
    {
        struct
        {
            uint16_t RdHd : 3;
            uint16_t RsHs : 3;
            uint16_t H2 : 1;
            uint16_t H1 : 1;
            uint16_t Op : 2;
            uint16_t : 6;
        };

        uint16_t halfword;
    } instruction_;
};

class ALUOperations : public virtual Instruction
{
public:
    /// @brief ALUOperations instruction constructor.
    /// @param instruction 16-bit THUMB instruction.
    /// @pre IsInstanceOf must be true.
    ALUOperations(uint16_t instruction) { instruction_.halfword = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 16-bit THUMB instruction.
    /// @return Whether this instruction is an ALU Operations instruction.
    static constexpr bool IsInstanceOf(uint16_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    void SetMnemonic(std::string& mnemonic);

    static constexpr uint16_t FORMAT =      0b0100'0000'0000'0000;
    static constexpr uint16_t FORMAT_MASK = 0b1111'1100'0000'0000;

    union
    {
        struct
        {
            uint16_t Rd : 3;
            uint16_t Rs : 3;
            uint16_t Op : 4;
            uint16_t : 6;
        };

        uint16_t halfword;
    } instruction_;
};

class MoveCompareAddSubtractImmediate : public virtual Instruction
{
public:
    /// @brief MoveCompareAddSubtractImmediate instruction constructor.
    /// @param instruction 16-bit THUMB instruction.
    /// @pre IsInstanceOf must be true.
    MoveCompareAddSubtractImmediate(uint16_t instruction) { instruction_.halfword = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 16-bit THUMB instruction.
    /// @return Whether this instruction is a Move/Compare/Add/Subtract Immediate instruction.
    static constexpr bool IsInstanceOf(uint16_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    void SetMnemonic(std::string& mnemonic);

    static constexpr uint16_t FORMAT =      0b0010'0000'0000'0000;
    static constexpr uint16_t FORMAT_MASK = 0b1110'0000'0000'0000;

    union
    {
        struct
        {
            uint16_t Offset8 : 8;
            uint16_t Rd : 3;
            uint16_t Op : 2;
            uint16_t : 3;
        };

        uint16_t halfword;
    } instruction_;
};

class AddSubtract : public virtual Instruction
{
public:
    /// @brief AddSubtract instruction constructor.
    /// @param instruction 16-bit THUMB instruction.
    /// @pre IsInstanceOf must be true.
    AddSubtract(uint16_t instruction) { instruction_.halfword = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 16-bit THUMB instruction.
    /// @return Whether this instruction is an Add/Subtract instruction.
    static constexpr bool IsInstanceOf(uint16_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    void SetMnemonic(std::string& mnemonic);

    static constexpr uint16_t FORMAT =      0b0001'1000'0000'0000;
    static constexpr uint16_t FORMAT_MASK = 0b1111'1000'0000'0000;

    union
    {
        struct
        {
            uint16_t Rd : 3;
            uint16_t Rs : 3;
            uint16_t RnOffset3 : 3;
            uint16_t Op : 1;
            uint16_t I : 1;
            uint16_t : 5;
        };

        uint16_t halfword;
    } instruction_;
};

class MoveShiftedRegister : public virtual Instruction
{
public:
    /// @brief MoveShiftedRegister instruction constructor.
    /// @param instruction 16-bit THUMB instruction.
    /// @pre IsInstanceOf must be true.
    MoveShiftedRegister(uint16_t instruction) { instruction_.halfword = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 16-bit THUMB instruction.
    /// @return Whether this instruction is a Move Shifted Register instruction.
    static constexpr bool IsInstanceOf(uint16_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    void SetMnemonic(std::string& mnemonic);

    static constexpr uint16_t FORMAT =      0b0000'0000'0000'0000;
    static constexpr uint16_t FORMAT_MASK = 0b1110'0000'0000'0000;

    union
    {
        struct
        {
            uint16_t Rd : 3;
            uint16_t Rs : 3;
            uint16_t Offset5 : 5;
            uint16_t Op : 2;
            uint16_t : 3;
        };

        uint16_t halfword;
    } instruction_;
};
}
