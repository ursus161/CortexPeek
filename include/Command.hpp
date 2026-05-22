#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstdint>
#include "Breakpoint.hpp"
#include "Observers.hpp"

class Process;

// context passed to every command so they can access shared debugger state
struct DebuggerContext {
    Process& process;
    std::unordered_map<std::uintptr_t, std::unique_ptr<Breakpoint>>& breakpoints;
    std::unordered_map<std::string, std::uintptr_t>&                 symbols;
    HistoryObserver&                                                  eventHistory;
};

class Command {
public:
    virtual ~Command() = default;
    virtual void       execute(DebuggerContext& ctx, const std::vector<std::string>& args) = 0;
    virtual std::string name() const = 0;
    virtual std::string help() const = 0;
};

class ContinueCommand : public Command {
public:
    void  execute(DebuggerContext& ctx, const std::vector<std::string>& args) override;
    std::string name() const override { return "continue"; }
    std::string help() const override { return "continue execution"; }
};

class StepCommand : public Command {
public:
    void  execute(DebuggerContext& ctx, const std::vector<std::string>& args) override;
    std::string name() const override { return "step"; }
    std::string help() const override { return "single-step one instruction"; }
};

class BreakCommand : public Command {
public:
    void  execute(DebuggerContext& ctx, const std::vector<std::string>& args) override;
    std::string name() const override { return "break"; }
    std::string help() const override { return "set breakpoint: break <addr>"; }
};

class RegistersCommand : public Command {
public:
    void  execute(DebuggerContext& ctx, const std::vector<std::string>& args) override;
    std::string name() const override { return "registers"; }
    std::string help() const override { return "dump all registers"; }
};

class DisassembleCommand : public Command {
public:
    void  execute(DebuggerContext& ctx, const std::vector<std::string>& args) override;
    std::string name() const override { return "disasm"; }
    std::string help() const override { return "disassemble: disasm [addr] [count]"; }
};

class HelpCommand : public Command {
public:
 explicit HelpCommand(const std::unordered_map<std::string,
                                                  std::unique_ptr<Command>>& cmds)
        : cmds_(cmds) {}
    void       execute(DebuggerContext& ctx, const std::vector<std::string>& args) override;
    std::string name() const override { return "help"; }
    std::string help() const override { return "show this message"; }

private:
    const std::unordered_map<std::string, std::unique_ptr<Command>>& cmds_;
};

class EventsCommand : public Command {
public:
    void        execute(DebuggerContext& ctx, const std::vector<std::string>& args) override;
    std::string name() const override { return "events"; }
    std::string help() const override { return "show event history: events [count]"; }
};
