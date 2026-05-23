#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstdint>
#include "Breakpoint.hpp"
#include "Observers.hpp"
#include "Process.hpp"

// forward declaration to break the circular include:
// CommandFactory.hpp already includes Command.hpp, so we can't include it back here
class CommandFactory;

// full definition lives in Config.hpp; Command.cpp includes it directly
class Config;
    
// context passed to every command so they can access shared debugger state
struct DebuggerContext {
    Process& process;
    std::unordered_map<std::uintptr_t, std::unique_ptr<Breakpoint>>& breakpoints;
    std::unordered_map<std::string, std::uintptr_t>&                 symbols;
    HistoryObserver&                                                  eventHistory;
    History<std::string>&                                             cmdHistory;
    const Config&                                                     config;
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
private:
    static std::optional<size_t> functionSize(
        std::uintptr_t addr,
        const std::unordered_map<std::string, std::uintptr_t>& symbols);
};

class HelpCommand : public Command {
public:
    explicit HelpCommand(const CommandFactory& factory) : factory_(factory) {}
    void        execute(DebuggerContext& ctx, const std::vector<std::string>& args) override;
    std::string name() const override { return "help"; }
    std::string help() const override { return "show this message"; }

private:
    const CommandFactory& factory_;
};

class CommandsCommand : public Command {
public:
    explicit CommandsCommand(const CommandFactory& factory) : factory_(factory) {}
    void        execute(DebuggerContext& ctx, const std::vector<std::string>& args) override;
    std::string name() const override { return "commands"; }
    std::string help() const override { return "list all registered commands and aliases"; }

private:
    const CommandFactory& factory_;
};

class EventsCommand : public Command {
public:
    void        execute(DebuggerContext& ctx, const std::vector<std::string>& args) override;
    std::string name() const override { return "events"; }
    std::string help() const override { return "show event history: events [count]"; }
};

class HistoryCommand : public Command {
public:
    void        execute(DebuggerContext& ctx, const std::vector<std::string>& args) override;
    std::string name() const override { return "history"; }
    std::string help() const override { return "show command history: history [count]"; }
};

class ConfigCommand : public Command {
public:
    void        execute(DebuggerContext& ctx, const std::vector<std::string>& args) override;
    std::string name() const override { return "config"; }
    std::string help() const override { return "show loaded configuration"; }
};
