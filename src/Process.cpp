#include "Process.hpp"
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <cerrno>

Process::Process(const std::string& path, const std::vector<std::string>& args) {
    pid_ = fork();
    // in the future the debugger process should be kept in a container in case of malicious fork-bombs 
    if (pid_ < 0) //fork throws -1 as an error
        throw std::runtime_error("fork failed");


    if (pid_ == 0) { // the pid is 0 only when i'm regarding a cloned process
        
        // child: tell the kernel "let my parent trace me" before exec
        if (ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) < 0)

            // _exit() because after fork() the child inherits copies of the
            // parent's buffers calling exit() would flush them twice (once in
            // the child, once in the parent), corrupting output _exit() skips all
            // cleanup and goes straight to the exit_group syscall  
            _exit(1);

        // build argv for execvp: path + args + nullptr sentinel
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(path.c_str()));
        for (const auto& a : args)

            argv.push_back(const_cast<char*>(a.c_str()));

        
        argv.push_back(nullptr); // any exec(something) arg list ends in null

        execvp(path.c_str(), argv.data());
        _exit(1); // only reached if execvp fails
    }

    // parent: after execvp the kernel delivers a SIGTRAP to the child before
    // it runs a single instruction :  wait for that stop before returning
    int status;
    waitpid(pid_, &status, 0);
    alive_    = true;
    attached_ = false;
}

Process Process::attach(pid_t pid) {
    Process p;
    p.pid_ = pid;

    if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) < 0)
        throw std::runtime_error(std::string("PTRACE_ATTACH failed: ") + strerror(errno));

    // PTRACE_ATTACH sends SIGSTOP to the target; wait until it actually stops
    int status;
    waitpid(pid, &status, 0);

    p.alive_    = true;
    p.attached_ = true;
    return p;
}

Process::~Process() {
    if (!alive_) return;

    if (attached_)  
        ptrace(PTRACE_DETACH, pid_, nullptr, nullptr);
    else {
        kill(pid_, SIGKILL);
        waitpid(pid_, nullptr, 0); // reap the child so it doesn't stay zombie
    }
}

void Process::continueExecution() {
    if (ptrace(PTRACE_CONT, pid_, nullptr, nullptr) < 0)
        throw std::runtime_error(std::string("PTRACE_CONT failed: ") + strerror(errno));
}

void Process::singleStep() {
    // executes exactly one instruction then re-delivers SIGTRAP
    if (ptrace(PTRACE_SINGLESTEP, pid_, nullptr, nullptr) < 0)
        throw std::runtime_error(std::string("PTRACE_SINGLESTEP failed: ") + strerror(errno));
}

bool Process::waitForStop(int& status) {
    if (waitpid(pid_, &status, 0) < 0) {
        alive_ = false;
        return false;
    }

    if (WIFEXITED(status) || WIFSIGNALED(status)) {
        alive_ = false;
        return false;
    }
    return true;
}

void Process::detach() {
    if (ptrace(PTRACE_DETACH, pid_, nullptr, nullptr) < 0)
        throw std::runtime_error(std::string("PTRACE_DETACH failed: ") + strerror(errno));
    alive_    = false;
    attached_ = false;
}
