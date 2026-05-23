#include "Process.hpp"
#include "Command.hpp"
#include "CommandFactory.hpp"
#include "History.hpp"
#include "RegisterFile.hpp"
#include "Symbols.hpp"
#include "Utils.hpp"
#include "Exceptions.hpp"
#include "Observers.hpp"
#include <iostream>
#include <string>
#include <cstring>
#include <sys/wait.h>
#include <vector>
#include <memory>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: cortexpeek <program> [args...]\n";
        return 1;
    }

    std::vector<std::string> args(argv + 2, argv + argc);
    Process proc(argv[1], args);

    auto logger  = std::make_shared<LogObserver>();
    auto histObs = std::make_shared<HistoryObserver>();
    proc.subscribe(logger);
    proc.subscribe(histObs);

    std::unordered_map<std::uintptr_t, std::unique_ptr<Breakpoint>> breakpoints;
    auto symbols = parseSymbols(proc.binaryPath());
    History<std::string> history;
    DebuggerContext ctx{ proc, breakpoints, symbols, *histObs, history };

    CommandFactory factory;

    factory.registerCommand("continue",  "continue execution",
        []() { return std::make_unique<ContinueCommand>(); });

    factory.registerCommand("step",      "single-step one instruction",
        []() { return std::make_unique<StepCommand>(); });
        
    factory.registerCommand("break",     "set breakpoint: break <addr>",
        []() { return std::make_unique<BreakCommand>(); });


    factory.registerCommand("registers", "dump all registers",
        []() { return std::make_unique<RegistersCommand>(); });

        
    factory.registerCommand("disasm",    "disassemble: disasm [addr] [count]",
        []() { return std::make_unique<DisassembleCommand>(); });
    factory.registerCommand("events",    "show event history: events [count]",
        []() { return std::make_unique<EventsCommand>(); });

    factory.registerCommand("history",   "show command history: history [count]",
        []() { return std::make_unique<HistoryCommand>(); });
    // HelpCommand and CommandsCommand capture factory by reference safe because
    // factory outlives the entire REPL loop
    factory.registerCommand("help",      "show this message",
        [&factory]() { return std::make_unique<HelpCommand>(factory); });
    factory.registerCommand("commands",  "list all registered commands and aliases",
        [&factory]() { return std::make_unique<CommandsCommand>(factory); });

    factory.registerAlias("c", "continue");
    factory.registerAlias("s", "step");
    factory.registerAlias("br", "break");
    factory.registerAlias("regs", "registers");
    factory.registerAlias("dis", "disasm");
    factory.registerAlias("e", "events");
    factory.registerAlias("h", "help");

    std::string line;
    while (proc.isAlive()) {
        std::printf("(cortexpeek) ");
        std::fflush(stdout);

        if (!std::getline(std::cin, line))
            break;

        // repeat last command on empty input, like gdb
        if (line.empty()) {
            if (auto last = history.last())
                line = *last;
            else
                continue;
        }

        history.push(line);

        auto tokens = tokenize(line);
        if (tokens.empty()) continue;

        const std::string& cmd = tokens[0];
        if (cmd == "quit" || cmd == "q") break;

        if (!factory.has(cmd)) {
            std::cerr << "unknown command: " << cmd << " (type 'help')\n";
            continue;
        }

        try {
            std::vector<std::string> cmdArgs(tokens.begin() + 1, tokens.end());
            factory.create(cmd)->execute(ctx, cmdArgs);
        } catch (const CommandException& e) {
            std::cerr << "command error: " << e.what() << '\n';
        } catch (const PtraceException& e) {
            std::cerr << "ptrace error: " << e.what() << '\n';
        } catch (const CortexException& e) {
            std::cerr << "error: " << e.what() << '\n';
        } catch (const std::exception& e) {
            std::cerr << "unexpected error: " << e.what() << '\n';
        }

        // after continue/step wait for the next stop; LogObserver prints the event
        // resolve alias first so "c" and "s" trigger the same post-step logic
        const std::string resolved = factory.resolve(cmd);
        if (resolved == "continue" || resolved == "step") {
            int status = 0;
            if (!proc.waitForStop(status)) {
                // process exited or was killed, event already printed by LogObserver
                break;
            }

            // StepCommand disables a breakpoint before stepping so the INT3 byte
            // doesn't re-trigger; put it back now that we have stopped safely
            for (auto& [addr, bp] : breakpoints)
                if (!bp->isEnabled())
                    bp->enable();

            // fatal signals mean the process is in an unrecoverable crash,
            // re-delivering them via PTRACE_CONT just loops forever
            if (WIFSTOPPED(status)) {
                int signal = WSTOPSIG(status);
                if (signal == SIGSEGV || signal == SIGBUS  ||
                    signal == SIGFPE  || signal == SIGILL  || signal == SIGABRT)
                    break;
            }
        }
    }

    return 0;
}
