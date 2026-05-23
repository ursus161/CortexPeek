#pragma once
#include <string>
#include <vector>
#include <sys/types.h>
#include "Observer.hpp"

// full definition lives in Breakpoint.hpp; Process.cpp includes it directly
class Breakpoint;

class Process : public DebugEventSource {
public:
    // fork + exec the target binary; child calls PTRACE_TRACEME before execvp
    // in this instance i preffer execvp as a system func over execl because i won't always know the argument at compile time, by nature a processes are dynamic so i preffer passing them to the kernel this way

    explicit Process(const std::string& path, const std::vector<std::string>& args = {}); // explicit to prevent implicit casting made by the compiler 

    // attach to an already-running pid with PTRACE_ATTACH
    static Process attach(pid_t pid);

    ~Process();

    // no copy only one owner of a traced process
    Process(const Process&)            = delete;
    Process& operator=(const Process&) = delete;
    Process(Process&&)                 = default;
    Process& operator=(Process&&)      = default;

    void continueExecution();
    void singleStep();

    // steps over a fired breakpoint: backs up RIP, disables the bp, internal
    // single-step + waitpid (no observer notifications), then re-enables the bp
    void resumeFromBreakpoint(Breakpoint& bp);

    // Blocks until the tracee stops; returns false if it exited
    bool waitForStop(int& status);

    void detach();

    pid_t pid()     const { return pid_; }
    bool  isAlive() const { return alive_; }
    const std::string& binaryPath() const { return path_; }

private:
    Process() = default;

    pid_t       pid_          = -1;
    bool        alive_        = false;
    bool        attached_     = false; // true = we PTRACE_ATTACHed, false = we forked
    bool        steppingMode_ = false; // true after singleStep(), reset in waitForStop()
    std::string path_;
};
