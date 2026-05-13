#pragma once
#include <sys/user.h>
#include <sys/types.h>
#include <cstdint>

struct RegisterFile {
    user_regs_struct regs{};

    // read all GPRs from the tracee in one ptrace call
    static RegisterFile get(pid_t pid);

    // write back modified registers to the tracee
    void set(pid_t pid) const;

    void dump() const;

    uint64_t rip() const { return regs.rip; }
    uint64_t rsp() const { return regs.rsp; }
    uint64_t rbp() const { return regs.rbp; }
    uint64_t rax() const { return regs.rax; }
};
