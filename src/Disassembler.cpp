#include "Disassembler.hpp"
#include <stdexcept>

Disassembler::Disassembler() {
    if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle_) != CS_ERR_OK)
        throw std::runtime_error("failed to initialize capstone");
}

Disassembler::~Disassembler() {
    cs_close(&handle_);
}

std::vector<Instruction> Disassembler::disassemble(const uint8_t* data, size_t size,
                                                    uint64_t address, size_t count) const {
    cs_insn* insns = nullptr;
    size_t   instructionCount = cs_disasm(handle_, data, size, address, count, &insns);

    std::vector<Instruction> result;
    result.reserve(instructionCount);

    for (size_t index = 0; index < instructionCount; ++index) {
        Instruction instr;
        instr.address  = insns[index].address;
        instr.mnemonic = insns[index].mnemonic;
        instr.operands = insns[index].op_str;
        instr.bytes.assign(insns[index].bytes, insns[index].bytes + insns[index].size);
        result.push_back(std::move(instr));
    }

    cs_free(insns, instructionCount);
    return result;
}
