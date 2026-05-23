#include "Process.hpp"
#include "Breakpoint.hpp"
#include "RegisterFile.hpp"
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <csignal>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include "Exceptions.hpp"



Process::Process(const std::string& path, const std::vector<std::string>& args) {
    pid_ = fork();
    // in the future the debugger process should be kept in a container in case of malicious fork-bombs 
    if (pid_ < 0) //fork throws -1 as an error
        throw ProcessException("Fork failed");


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
        for (const auto& arg : args)
            argv.push_back(const_cast<char*>(arg.c_str()));

        
        argv.push_back(nullptr); // any exec(something) arg list ends in null

        execvp(path.c_str(), argv.data());
        _exit(1); // only reached if execvp fails
    }

    // parent: after execvp the kernel delivers a SIGTRAP to the child before
    // it runs a single instruction :  wait for that stop before returning
    int status;
    if (waitpid(pid_, &status, 0) < 0)      
         throw ProcessException("waitpid failed");

    if (!WIFSTOPPED(status))
       throw ProcessException("child exited before ptrace stop");
       
    alive_    = true;
    attached_ = false;
    path_     = path;
}

Process Process::attach(pid_t pid) {
    Process newProcess;
    newProcess.pid_ = pid;

    if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) < 0)
        throw PtraceException("PTRACE_ATTACH", errno);

    // PTRACE_ATTACH sends SIGSTOP to the target; wait until it actually stops
    int status;
    waitpid(pid, &status, 0);

    newProcess.alive_    = true;
    newProcess.attached_ = true;

    // read the binary path from /proc/<pid>/exe since we don't have it directly
    char linkBuffer[4096] = {};
    std::string procExe = "/proc/" + std::to_string(pid) + "/exe";
    ssize_t len = readlink(procExe.c_str(), linkBuffer, sizeof(linkBuffer) - 1);
    if (len > 0)
        newProcess.path_ = std::string(linkBuffer, len);

    return newProcess;
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
    steppingMode_ = false;
    if (ptrace(PTRACE_CONT, pid_, nullptr, nullptr) < 0)
        throw PtraceException("PTRACE_CONT", errno);
}

void Process::singleStep() {
    // executes exactly one instruction then re-delivers SIGTRAP
    steppingMode_ = true;
    if (ptrace(PTRACE_SINGLESTEP, pid_, nullptr, nullptr) < 0)
        throw PtraceException("PTRACE_SINGLESTEP", errno);
}

bool Process::waitForStop(int& status) {
    if (waitpid(pid_, &status, 0) < 0) {
        alive_ = false;
        return false;
    }

    if (WIFEXITED(status)) {
        notify({DebugEventType::ProcessExited, WEXITSTATUS(status)});
        alive_ = false;
        return false;
    }

    if (WIFSIGNALED(status)) {
        notify({DebugEventType::Signal, WTERMSIG(status)});
        alive_ = false;
        return false;
    }

    if (WIFSTOPPED(status)) {
        int sig = WSTOPSIG(status);
        if (sig == SIGTRAP)
            notify({steppingMode_ ? DebugEventType::SingleStep : DebugEventType::Breakpoint, 0});
        else
            notify({DebugEventType::Signal, sig});
    }

    steppingMode_ = false;
    return true;
}

void Process::resumeFromBreakpoint(Breakpoint& bp) {
    // RIP has already been backed up to the breakpoint address by the caller;
    // we just need to run the original instruction and put 0xCC back afterward

    // put the original byte back so the real instruction can execute
    bp.disable();

    // step over exactly that one instruction where we put the break;
    // going through ptrace directly avoids touching steppingMode_ or firing observer notifications
    if (ptrace(PTRACE_SINGLESTEP, pid_, nullptr, nullptr) < 0)
        throw PtraceException("PTRACE_SINGLESTEP", errno);

    int status;
    waitpid(pid_, &status, 0); // and when the singlestep is over..

    // restore the breakpoint so the next time we hit this address it fires again
    bp.enable();
}

void Process::detach() {
    if (ptrace(PTRACE_DETACH, pid_, nullptr, nullptr) < 0)
        throw PtraceException("PTRACE_DETACH", errno);
    alive_    = false;
    attached_ = false;
}
