#pragma once
#include <string>
#include <vector>
#include <sys/types.h>

class Process {
public:
    // fork + exec the target binary; child calls PTRACE_TRACEME before execvp
    // in this instance i preffer execvp as a system func over execl because i won't always know the argument at compile time, by nature a processes are dynamic so i preffer passing them to the kernel this way

    explicit Process(const std::string& path, const std::vector<std::string>& args = {}); // explicit to prevent implicit casting made by the compiler 

    // attach to an already-running pid with PTRACE_ATTACH
    static Process attach(pid_t pid);

    ~Process();

    // no copy — only one owner of a traced process
    Process(const Process&)            = delete;
    Process& operator=(const Process&) = delete;
    Process(Process&&)                 = default;
    Process& operator=(Process&&)      = default;

    void continueExecution();
    void singleStep();

    // Blocks until the tracee stops; returns false if it exited
    bool waitForStop(int& status);

    void detach();

    pid_t pid()     const { return pid_; }
    bool  isAlive() const { return alive_; }
    const std::string& binaryPath() const { return path_; }

private:
    Process() = default;

    pid_t       pid_      = -1;
    bool        alive_    = false;
    bool        attached_ = false; // true = we PTRACE_ATTACHed, false = we forked
    std::string path_;
};
