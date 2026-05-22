#include "Process.hpp"
#include "Command.hpp"
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
#include <unordered_map>
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
    DebuggerContext ctx{ proc, breakpoints, symbols, *histObs };

    std::unordered_map<std::string, std::unique_ptr<Command>> commands;

    
    commands["continue"]  = std::make_unique<ContinueCommand>();
    commands["step"]      = std::make_unique<StepCommand>();
    commands["break"]     = std::make_unique<BreakCommand>();
    commands["registers"] = std::make_unique<RegistersCommand>();
    commands["disasm"]    = std::make_unique<DisassembleCommand>();
    commands["events"]    = std::make_unique<EventsCommand>();
    commands["help"]      = std::make_unique<HelpCommand>(commands);

    History<std::string> history;

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

        auto found = commands.find(cmd);
        if (found == commands.end()) {
            std::cerr << "unknown command: " << cmd << " (type 'help')\n";
            continue;
        }

        try {
            std::vector<std::string> cmdArgs(tokens.begin() + 1, tokens.end());
            found->second->execute(ctx, cmdArgs);
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
        if (cmd == "continue" || cmd == "step") {
            int status = 0;
            if (!proc.waitForStop(status)) {
                // process exited or was killed — event already printed by LogObserver
                break;
            }
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
