#include "Command.hpp"
#include "CommandFactory.hpp"
#include "Process.hpp"
#include "RegisterFile.hpp"
#include "Disassembler.hpp"
#include "MemoryView.hpp"
#include "Exceptions.hpp"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>

// returns the byte distance from addr to the nearest symbol address above it,
// or nullopt if addr is the last symbol
std::optional<size_t>
DisassembleCommand::functionSize(std::uintptr_t addr,
                                 const std::unordered_map<std::string, std::uintptr_t>& symbols)
{
    std::optional<std::uintptr_t> next;
    for (const auto& [name, symAddr] : symbols)
        if (symAddr > addr && (!next || symAddr < *next))
            next = symAddr;
    if (next)
        return *next - addr;
    return std::nullopt;
}

// if RIP-1 matches an active breakpoint, backs RIP up to the breakpoint address
// and returns a raw pointer to it; returns nullptr if we are not at a breakpoint
static Breakpoint* stepBackFromBreakpoint(DebuggerContext& ctx) {
    auto regs = RegisterFile::get(ctx.process.pid());
    auto it   = ctx.breakpoints.find(regs.rip() - 1);
    if (it == ctx.breakpoints.end() || !it->second->isEnabled())
        return nullptr;
    // set it back 1 byte so it won't hover over the 0xCC byte anymore
    regs.setRip(regs.rip() - 1);
    regs.set(ctx.process.pid());
    return it->second.get();
}

void ContinueCommand::execute(DebuggerContext& ctx, const std::vector<std::string>&) {
    // if the INT3 already fired, step over the original instruction first
    if (auto* bp = stepBackFromBreakpoint(ctx))
        ctx.process.resumeFromBreakpoint(*bp);

    ctx.process.continueExecution();
}

void StepCommand::execute(DebuggerContext& ctx, const std::vector<std::string>&) {
    // disable the breakpoint so the step lands on the real instruction;
    // main.cpp re-enables any disabled breakpoints after waitForStop returns
    if (auto* bp = stepBackFromBreakpoint(ctx))
        bp->disable();

    ctx.process.singleStep();
}

void BreakCommand::execute(DebuggerContext& ctx, const std::vector<std::string>& args) {
    if (args.empty())
        throw CommandException("usage: break <address>");

    std::uintptr_t addr = 0;
    auto it = ctx.symbols.find(args[0]);
    if (it != ctx.symbols.end())
        addr = it->second;
    else {
        try {
            addr = std::stoull(args[0], nullptr, 16);
        } catch (const std::exception&) {
            throw CommandException("break: unknown symbol or invalid address '" + args[0] + "'");
        }
    }

    if (ctx.breakpoints.count(addr)) {
        std::ostringstream oss;
        oss << "breakpoint already set at 0x" << std::hex << addr;
        throw BreakpointException(oss.str());
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
    // 0 = no explicit count, read until unmapped memory
    size_t count     = 0;
    size_t maxCount  = SIZE_MAX;

    std::optional<size_t> symSize;
    if (!args.empty() && args[0] != ".") {
        auto it = ctx.symbols.find(args[0]);
        if (it != ctx.symbols.end()) {
            addr    = it->second;
            symSize = functionSize(addr, ctx.symbols);
        } else {
            try {
                addr = std::stoull(args[0], nullptr, 16);
            } catch (const std::exception&) {
                throw CommandException("disasm: unknown symbol or invalid address '" + args[0] + "'");
            }
        }
    }
    if (args.size() >= 2) {
        count    = std::stoull(args[1]);
        maxCount = count;
    }

    // 15 bytes is the max length of a single x86-64 instruction
    // use symbol boundary when known and no explicit count; cap at 64KB otherwise
    const size_t bufferSize = (symSize && count == 0) ? *symSize
                            : (count > 0 ? 15 * count : 64 * 1024);
    std::vector<uint8_t> buffer(bufferSize, 0);
    MemoryView<uint64_t> memory(ctx.process.pid());

    size_t bytesRead = 0;
    for (size_t offset = 0; offset < bufferSize; offset += sizeof(uint64_t)) {
        try {
            uint64_t word = memory.read(addr + offset);
            // clamp the copy to avoid writing past the buffer end on the last chunk
            size_t toCopy = std::min(sizeof(uint64_t), bufferSize - offset);
            std::memcpy(buffer.data() + offset, &word, toCopy);
            bytesRead += toCopy;
        } catch (const PtraceException&) {
            break; // hit unmapped memory, stop here and disassemble what we have
        }
    }

    Disassembler disassembler;
    for (const auto& instr : disassembler.disassemble(buffer.data(), bytesRead, addr, maxCount)) {
        std::printf("0x%016lx  %-8s %s\n",
                    instr.address, instr.mnemonic.c_str(), instr.operands.c_str());
        if (instr.mnemonic == "ret") break; // stop at function boundary, don't decode padding
    }
}

void HelpCommand::execute(DebuggerContext&, const std::vector<std::string>&) {
    for (const auto& name : factory_.availableCommands())
        std::printf("  %-12s  %s\n", name.c_str(), factory_.helpFor(name).c_str());
}

void CommandsCommand::execute(DebuggerContext&, const std::vector<std::string>&) {
    std::printf("commands:\n");
    for (const auto& name : factory_.availableCommands())
        std::printf("  %s\n", name.c_str());

    std::printf("aliases:\n");
    for (const auto& [alias, target] : factory_.aliases())
        std::printf("  %-4s -> %s\n", alias.c_str(), target.c_str());
}

void EventsCommand::execute(DebuggerContext& ctx, const std::vector<std::string>& args) {
    size_t count = 10;
    if (!args.empty()) {
        try {
            count = std::stoull(args[0]);
        } catch (const std::exception&) {
            throw CommandException("events: invalid count '" + args[0] + "'");
        }
    }

    const auto& entries = ctx.eventHistory.history().entries();
    size_t start = entries.size() > count ? entries.size() - count : 0;
    for (size_t i = start; i < entries.size(); ++i) {
        const auto& ev = entries[i];
        switch (ev.type) {
            case DebugEventType::Breakpoint:
                std::printf("  breakpoint hit\n");
                break;
            case DebugEventType::SingleStep:
                std::printf("  single step\n");
                break;
            case DebugEventType::ProcessExited:
                std::printf("  process exited with code %d\n", ev.data);
                break;
            case DebugEventType::Signal:
                std::printf("  signal %d (%s)\n", ev.data, strsignal(ev.data));
                break;
        }
    }
}

void HistoryCommand::execute(DebuggerContext& ctx, const std::vector<std::string>& args) {
    size_t count = 10;
    if (!args.empty()) {
        try {
            count = std::stoull(args[0]);
        } catch (const std::exception&) {
            throw CommandException("history: invalid count '" + args[0] + "'");
        }
    }

    const auto& entries = ctx.cmdHistory.entries();
    size_t start = entries.size() > count ? entries.size() - count : 0;
    for (size_t i = start; i < entries.size(); ++i)
        std::printf("  %zu  %s\n", i + 1, entries[i].c_str());
}