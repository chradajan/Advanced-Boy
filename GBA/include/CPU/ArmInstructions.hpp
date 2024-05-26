#pragma once

#include <cstdint>
#include <string>
#include <CPU/CPUTypes.hpp>

namespace CPU { class ARM7TDMI; }

namespace CPU::ARM
{
/// @brief Decode a 32 bit value into an ARM instruction.
/// @param undecodedInstruction 32 bit value to decode.
/// @param buffer Pointer to buffer to place instruction object.
/// @return Pointer to newly constructed instruction object.
CPU::Instruction* DecodeInstruction(uint32_t undecodedInstruction, void* buffer);

class BranchAndExchange : public virtual Instruction
{
public:
    /// @brief BranchAndExchange instruction constructor.
    /// @param instruction 32-bit ARM instruction.
    /// @pre IsInstanceOf must be true.
    BranchAndExchange(uint32_t instruction) { instruction_.word = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 32-bit ARM instruction.
    /// @return Whether this instruction is a Branch and Exchange instruction.
    static constexpr bool IsInstanceOf(uint32_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    void SetMnemonic(std::string& mnemonic);

    static constexpr uint32_t FORMAT =      0b0000'0001'0010'1111'1111'1111'0001'0000;
    static constexpr uint32_t FORMAT_MASK = 0b0000'1111'1111'1111'1111'1111'1111'0000;

    union
    {
        struct
        {
            uint32_t Rn : 4;
            uint32_t : 24;
            uint32_t Cond : 4;
        };

        uint32_t word;
    } instruction_;
};

class BlockDataTransfer : public virtual Instruction
{
public:
    /// @brief BlockDataTransfer instruction constructor.
    /// @param instruction 32-bit ARM instruction.
    /// @pre IsInstanceOf must be true.
    BlockDataTransfer(uint32_t instruction) { instruction_.word = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 32-bit ARM instruction.
    /// @return Whether this instruction is a Block Data Transfer instruction.
    static constexpr bool IsInstanceOf(uint32_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    void SetMnemonic(std::string& mnemonic);

    static constexpr uint32_t FORMAT =      0b0000'1000'0000'0000'0000'0000'0000'0000;
    static constexpr uint32_t FORMAT_MASK = 0b0000'1110'0000'0000'0000'0000'0000'0000;

    union
    {
        struct
        {
            uint32_t RegisterList : 16;
            uint32_t Rn : 4;
            uint32_t L : 1;  // 0 = store, 1 = load
            uint32_t W : 1;  // 0 = no write-back, 1 = write address into base
            uint32_t S : 1;  // 0 = do not load PSR or force user mode, 1 = load PSR or force user mode
            uint32_t U : 1;  // 0 = down - subtract offset from base, 1 = up - add offset to base
            uint32_t P : 1;  // 0 = post - add offset after transfer, 1 = pre - add offset before transfer
            uint32_t : 3;
            uint32_t Cond : 4;
        };

        uint32_t word;
    } instruction_;
};

class Branch : public virtual Instruction
{
public:
    /// @brief Branch instruction constructor.
    /// @param instruction 32-bit ARM instruction.
    /// @pre IsInstanceOf must be true.
    Branch(uint32_t instruction) { instruction_.word = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 32-bit ARM instruction.
    /// @return Whether this instruction is a Branch instruction.
    static constexpr bool IsInstanceOf(uint32_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    /// @param newPc PC to branch to.
    void SetMnemonic(std::string& mnemonic, uint32_t newPC);

    static constexpr uint32_t FORMAT =      0b0000'1010'0000'0000'0000'0000'0000'0000;
    static constexpr uint32_t FORMAT_MASK = 0b0000'1110'0000'0000'0000'0000'0000'0000;

    union
    {
        struct
        {
            uint32_t Offset : 24;
            uint32_t L : 1;
            uint32_t : 3;
            uint32_t Cond : 4;
        };

        uint32_t word;
    } instruction_;
};

class SoftwareInterrupt : public virtual Instruction
{
public:
    /// @brief SoftwareInterrupt instruction constructor.
    /// @param instruction 32-bit ARM instruction.
    /// @pre IsInstanceOf must be true.
    SoftwareInterrupt(uint32_t instruction) { instruction_.word = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 32-bit ARM instruction.
    /// @return Whether this instruction is a Software Interrupt instruction.
    static constexpr bool IsInstanceOf(uint32_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    void SetMnemonic(std::string& mnemonic);

    static constexpr uint32_t FORMAT =      0b0000'1111'0000'0000'0000'0000'0000'0000;
    static constexpr uint32_t FORMAT_MASK = 0b0000'1111'0000'0000'0000'0000'0000'0000;

    union
    {
        struct
        {
            uint32_t CommentField : 24;
            uint32_t : 4;
            uint32_t Cond : 4;
        };

        uint32_t word;
    } instruction_;
};

class Undefined : public virtual Instruction
{
public:
    /// @brief Undefined instruction constructor.
    /// @param instruction 32-bit ARM instruction.
    /// @pre IsInstanceOf must be true.
    Undefined(uint32_t instruction) { instruction_.word = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 32-bit ARM instruction.
    /// @return Whether this instruction is a Undefined instruction.
    static constexpr bool IsInstanceOf(uint32_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    void SetMnemonic(std::string& mnemonic);

    static constexpr uint32_t FORMAT =      0b0000'0110'0000'0000'0000'0000'0001'0000;
    static constexpr uint32_t FORMAT_MASK = 0b0000'1110'0000'0000'0000'0000'0001'0000;

    union
    {
        struct
        {
            uint32_t : 28;
            uint32_t Cond : 4;
        };

        uint32_t word;
    } instruction_;
};

class SingleDataTransfer : public virtual Instruction
{
public:
    /// @brief SingleDataTransfer instruction constructor.
    /// @param instruction 32-bit ARM instruction.
    /// @pre IsInstanceOf must be true.
    SingleDataTransfer(uint32_t instruction) { instruction_.word = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 32-bit ARM instruction.
    /// @return Whether this instruction is a Single Data Transfer instruction.
    static constexpr bool IsInstanceOf(uint32_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    /// @param offset Offset to add/subtract to base address.
    void SetMnemonic(std::string& mnemonic, uint32_t offset);

    static constexpr uint32_t FORMAT =      0b0000'0100'0000'0000'0000'0000'0000'0000;
    static constexpr uint32_t FORMAT_MASK = 0b0000'1100'0000'0000'0000'0000'0000'0000;

    union
    {
        struct
        {
            uint32_t Offset : 12;
            uint32_t Rd : 4;
            uint32_t Rn : 4;
            uint32_t L : 1;
            uint32_t W : 1;
            uint32_t B : 1;
            uint32_t U : 1;
            uint32_t P : 1;
            uint32_t I : 1;
            uint32_t : 2;
            uint32_t Cond : 4;
        } flags;

        struct
        {
            uint32_t Rm : 4;
            uint32_t : 1;
            uint32_t ShiftType : 2;
            uint32_t ShiftAmount : 5;
            uint32_t : 20;
        } registerOffset;

        uint32_t word;
    } instruction_;
};

class SingleDataSwap : public virtual Instruction
{
public:
    /// @brief SingleDataSwap instruction constructor.
    /// @param instruction 32-bit ARM instruction.
    /// @pre IsInstanceOf must be true.
    SingleDataSwap(uint32_t instruction) { instruction_.word = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 32-bit ARM instruction.
    /// @return Whether this instruction is a Single Data Swap instruction.
    static constexpr bool IsInstanceOf(uint32_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    void SetMnemonic(std::string& mnemonic);

    static constexpr uint32_t FORMAT =      0b0000'0001'0000'0000'0000'0000'1001'0000;
    static constexpr uint32_t FORMAT_MASK = 0b0000'1111'1000'0000'0000'1111'1111'0000;

    union
    {
        struct
        {
            uint32_t Rm : 4;
            uint32_t : 8;
            uint32_t Rd : 4;
            uint32_t Rn : 4;
            uint32_t : 2;
            uint32_t B : 1;
            uint32_t : 5;
            uint32_t Cond : 4;
        };

        uint32_t word;
    } instruction_;
};

class Multiply : public virtual Instruction
{
public:
    /// @brief Multiply instruction constructor.
    /// @param instruction 32-bit ARM instruction.
    /// @pre IsInstanceOf must be true.
    Multiply(uint32_t instruction) { instruction_.word = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 32-bit ARM instruction.
    /// @return Whether this instruction is a Multiply instruction.
    static constexpr bool IsInstanceOf(uint32_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    void SetMnemonic(std::string& mnemonic);

    static constexpr uint32_t FORMAT =      0b0000'0000'0000'0000'0000'0000'1001'0000;
    static constexpr uint32_t FORMAT_MASK = 0b0000'1111'1000'0000'0000'0000'1111'0000;

    union
    {
        struct
        {
            uint32_t Rm : 4;
            uint32_t : 4;
            uint32_t Rs : 4;
            uint32_t Rn : 4;
            uint32_t Rd : 4;
            uint32_t S : 1;
            uint32_t A : 1;
            uint32_t : 6;
            uint32_t Cond : 4;
        };

        uint32_t word;
    } instruction_;
};

class MultiplyLong : public virtual Instruction
{
public:
    /// @brief MultiplyLong instruction constructor.
    /// @param instruction 32-bit ARM instruction.
    /// @pre IsInstanceOf must be true.
    MultiplyLong(uint32_t instruction) { instruction_.word = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 32-bit ARM instruction.
    /// @return Whether this instruction is a Multiply Long instruction.
    static constexpr bool IsInstanceOf(uint32_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    void SetMnemonic(std::string& mnemonic);

    static constexpr uint32_t FORMAT =      0b0000'0000'1000'0000'0000'0000'1001'0000;
    static constexpr uint32_t FORMAT_MASK = 0b0000'1111'1000'0000'0000'0000'1111'0000;

    union
    {
        struct
        {
            uint32_t Rm : 4;
            uint32_t : 4;
            uint32_t Rs : 4;
            uint32_t RdLo : 4;
            uint32_t RdHi : 4;
            uint32_t S : 1;
            uint32_t A : 1;
            uint32_t U : 1;
            uint32_t : 5;
            uint32_t Cond : 4;
        };

        uint32_t word;
    } instruction_;
};

class HalfwordDataTransferRegisterOffset : public virtual Instruction
{
public:
    /// @brief HalfwordDataTransferRegisterOffset instruction constructor.
    /// @param instruction 32-bit ARM instruction.
    /// @pre IsInstanceOf must be true.
    HalfwordDataTransferRegisterOffset(uint32_t instruction) { instruction_.word = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 32-bit ARM instruction.
    /// @return Whether this instruction is a Halfword Data Transfer Register Offset instruction.
    static constexpr bool IsInstanceOf(uint32_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    /// @param offset Offset to add to transfer address.
    void SetMnemonic(std::string& mnemonic, uint32_t offset);

    static constexpr uint32_t FORMAT =      0b0000'0000'0000'0000'0000'0000'1001'0000;
    static constexpr uint32_t FORMAT_MASK = 0b0000'1110'0100'0000'0000'1111'1001'0000;

    union
    {
        struct
        {
            uint32_t Rm : 4;
            uint32_t : 1;
            uint32_t H : 1;
            uint32_t S : 1;
            uint32_t : 5;
            uint32_t Rd : 4;
            uint32_t Rn : 4;
            uint32_t L : 1;
            uint32_t W : 1;
            uint32_t : 1;
            uint32_t U : 1;
            uint32_t P : 1;
            uint32_t : 3;
            uint32_t Cond : 4;
        };

        uint32_t word;
    } instruction_;
};

class HalfwordDataTransferImmediateOffset : public virtual Instruction
{
public:
    /// @brief HalfwordDataTransferImmediateOffset instruction constructor.
    /// @param instruction 32-bit ARM instruction.
    /// @pre IsInstanceOf must be true.
    HalfwordDataTransferImmediateOffset(uint32_t instruction) { instruction_.word = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 32-bit ARM instruction.
    /// @return Whether this instruction is a Halfword Data Transfer Immediate Offset instruction.
    static constexpr bool IsInstanceOf(uint32_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    /// @param offset Offset to add to transfer address.
    void SetMnemonic(std::string& mnemonic, uint8_t offset);

    static constexpr uint32_t FORMAT =      0b0000'0000'0100'0000'0000'0000'1001'0000;
    static constexpr uint32_t FORMAT_MASK = 0b0000'1110'0100'0000'0000'0000'1001'0000;

    union
    {
        struct
        {
            uint32_t Offset : 4;
            uint32_t : 1;
            uint32_t H : 1;
            uint32_t S : 1;
            uint32_t : 1;
            uint32_t Offset1 : 4;
            uint32_t Rd : 4;
            uint32_t Rn : 4;
            uint32_t L : 1;
            uint32_t W : 1;
            uint32_t : 1;
            uint32_t U : 1;
            uint32_t P : 1;
            uint32_t : 3;
            uint32_t Cond : 4;
        };

        uint32_t word;
    } instruction_;
};

class PSRTransferMRS : public virtual Instruction
{
public:
    /// @brief PSRTransferMRS instruction constructor.
    /// @param instruction 32-bit ARM instruction.
    /// @pre IsInstanceOf must be true.
    PSRTransferMRS(uint32_t instruction) { instruction_.word = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 32-bit ARM instruction.
    /// @return Whether this instruction is a PSR Transfer (MRS) instruction.
    static constexpr bool IsInstanceOf(uint32_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    void SetMnemonic(std::string& mnemonic);

    static constexpr uint32_t FORMAT =      0b0000'0001'0000'1111'0000'0000'0000'0000;
    static constexpr uint32_t FORMAT_MASK = 0b0000'1111'1011'1111'0000'0000'0000'0000;

    union
    {
        struct
        {
            uint32_t : 12;
            uint32_t Rd : 4;
            uint32_t : 6;
            uint32_t Ps : 1;
            uint32_t : 5;
            uint32_t Cond : 4;
        };

        uint32_t word;
    } instruction_;
};

class PSRTransferMSR : public virtual Instruction
{
public:
    /// @brief PSRTransferMSR instruction constructor.
    /// @param instruction 32-bit ARM instruction.
    /// @pre IsInstanceOf must be true.
    PSRTransferMSR(uint32_t instruction) { instruction_.word = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 32-bit ARM instruction.
    /// @return Whether this instruction is a PSR Transfer (MSR) instruction.
    static constexpr bool IsInstanceOf(uint32_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    void SetMnemonic(std::string& mnemonic);

    static constexpr uint32_t FORMAT =      0b0000'0001'0010'0000'1111'0000'0000'0000;
    static constexpr uint32_t FORMAT_MASK = 0b0000'1101'1011'0000'1111'0000'0000'0000;

    union
    {
        struct
        {
            uint32_t : 16;
            uint32_t Control : 1;
            uint32_t Extension : 1;
            uint32_t Status : 1;
            uint32_t Flags : 1;
            uint32_t : 2;
            uint32_t Pd : 1;
            uint32_t : 2;
            uint32_t I : 1;
            uint32_t : 2;
            uint32_t Cond : 4;
        } commonFlags;

        struct
        {
            uint8_t Rm : 4;
            uint32_t : 28;
        } regFlags;

        struct
        {
            uint32_t Imm : 8;
            uint32_t Rotate : 4;
            uint32_t : 20;
        } immFlag;

        uint32_t word;
    } instruction_;
};

class DataProcessing : public virtual Instruction
{
public:
    /// @brief DataProcessing instruction constructor.
    /// @param instruction 32-bit ARM instruction.
    /// @pre IsInstanceOf must be true.
    DataProcessing(uint32_t instruction) { instruction_.word = instruction; }

    /// @brief Check if instruction matches layout of this command type.
    /// @param instruction 32-bit ARM instruction.
    /// @return Whether this instruction is a Data Processing instruction.
    static constexpr bool IsInstanceOf(uint32_t instruction) { return (instruction & FORMAT_MASK) == FORMAT; }

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    void Execute(ARM7TDMI& cpu) override;

private:
    /// @brief Generate a mnemonic string for this instruction.
    /// @param mnemonic String to assign mnemonic to.
    /// @param operand2 Second operand of data processing operation.
    void SetMnemonic(std::string& mnemonic, uint32_t operand2);

    static constexpr uint32_t FORMAT =      0b0000'0000'0000'0000'0000'0000'0000'0000;
    static constexpr uint32_t FORMAT_MASK = 0b0000'1100'0000'0000'0000'0000'0000'0000;

    union
    {
        struct
        {
            uint32_t Operand2 : 12;
            uint32_t Rd : 4;
            uint32_t Rn : 4;
            uint32_t S : 1;
            uint32_t OpCode : 4;
            uint32_t I : 1;
            uint32_t : 2;
            uint32_t Cond : 4;
        } flags;

        struct
        {
            uint32_t Rm : 4;
            uint32_t : 1;
            uint32_t ShiftType : 2;
            uint32_t : 1;
            uint32_t Rs : 4;
            uint32_t : 20;
        } shiftRegByReg;

        struct
        {
            uint32_t Rm : 4;
            uint32_t : 1;
            uint32_t ShiftType : 2;
            uint32_t ShiftAmount : 5;
            uint32_t : 20;
        } shiftRegByImm;

        struct
        {
            uint32_t Imm : 8;
            uint32_t RotateAmount : 4;
            uint32_t : 20;
        } rotatedImmediate;

        uint32_t word;
    } instruction_;
};
}