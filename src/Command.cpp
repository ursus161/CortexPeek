#include "Command.hpp"
#include "Process.hpp"
#include "RegisterFile.hpp"
#include "Disassembler.hpp"
#include "MemoryView.hpp"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>

void ContinueCommand::execute(DebuggerContext& ctx, const std::vector<std::string>&) {
    ctx.process.continueExecution();
}

void StepCommand::execute(DebuggerContext& ctx, const std::vector<std::string>&) {
    ctx.process.singleStep();
}

void BreakCommand::execute(DebuggerContext& ctx, const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "usage: break <address>\n";
        return;
    }
    std::uintptr_t addr = std::stoull(args[0], nullptr, 16);

    if (ctx.breakpoints.count(addr)) {
        std::cerr << "breakpoint already set at 0x" << std::hex << addr << '\n';
        return;
    }

    auto breakpoint = std::make_unique<Breakpoint>(ctx.process.pid(), addr);
    breakpoint->enable();
    std::printf("breakpoint set at 0x%lx\n", addr);
    ctx.breakpoints.emplace(addr, std::move(breakpoint));
}

void RegistersCommand::execute(DebuggerContext& ctx, const std::vector<std::string>&) {
    RegisterFile::get(ctx.process.pid()).dump();
}

void DisassembleCommand::execute(DebuggerContext& ctx, const std::vector<std::string>& args) {
    auto     registers = RegisterFile::get(ctx.process.pid());
    uint64_t addr      = registers.rip();
    size_t count = 100;

    if (!args.empty() && args[0] != ".")
        addr = std::stoull(args[0], nullptr, 16);

    if (args.size() >= 2)

        count = std::stoull(args[1]);

    // 15 bytes is the max length of a single x86-64 instruction
    // it's calculated in the manner that the instruction that have the most occurances get the smallest bytelengths, for example push ebp is really small compared to other things
    
    const size_t         bufferSize = 15 * count;
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemoryView<uint64_t> memory(ctx.process.pid());
    for (size_t offset = 0; offset < bufferSize; offset += sizeof(uint64_t)) {
        uint64_t word = memory.read(addr + offset);
        std::memcpy(buffer.data() + offset, &word, sizeof(word));
    }

    Disassembler disassembler;
    for (const auto& instr : disassembler.disassemble(buffer.data(), bufferSize, addr, count))
        std::printf("0x%016lx  %-8s %s\n",
                    instr.address, instr.mnemonic.c_str(), instr.operands.c_str());
}

void HelpCommand::execute(DebuggerContext&, const std::vector<std::string>&) {
    for (const auto& [name, cmd] : cmds_)
        std::printf("  %-12s  %s\n", cmd->name().c_str(), cmd->help().c_str());
}
