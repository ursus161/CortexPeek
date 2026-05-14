#pragma once
#include <capstone/capstone.h>
#include <cstdint>
#include <string>
#include <vector>

struct Instruction {
    uint64_t             address;
    std::string          mnemonic;
    std::string          operands;
    std::vector<uint8_t> bytes;
};

class Disassembler {
public:
    Disassembler();
    ~Disassembler();

    // no copy — capstone handle is not duplicable
    Disassembler(const Disassembler&)            = delete;
    Disassembler& operator=(const Disassembler&) = delete;

    // disassemble up to count instructions from data starting at address
    std::vector<Instruction> disassemble(const uint8_t* data, size_t size,
                                         uint64_t address, size_t count = 10) const;

private:
    csh handle_;
};
