#include "RegisterFile.hpp"
#include <sys/ptrace.h>
#include <stdexcept>
#include <cstring>
#include <cerrno>
#include <cstdio>

RegisterFile RegisterFile::get(pid_t pid) {
    RegisterFile result;
    // PTRACE_GETREGS fills the entire user_regs_struct in one call
    if (ptrace(PTRACE_GETREGS, pid, nullptr, &result.regs) < 0)
        throw std::runtime_error(std::string("PTRACE_GETREGS failed: ") + strerror(errno));
    return result;
}

void RegisterFile::set(pid_t pid) const {
    if (ptrace(PTRACE_SETREGS, pid, nullptr, const_cast<user_regs_struct*>(&regs)) < 0)
        throw std::runtime_error(std::string("PTRACE_SETREGS failed: ") + strerror(errno));
}
    
void RegisterFile::dump() const { //0x%016llx represents 16 chars on ULL format, it'll print in lowercase format
    std::printf("rip = 0x%016llx  rsp = 0x%016llx  rbp = 0x%016llx\n",
                regs.rip, regs.rsp, regs.rbp);
    std::printf("rax = 0x%016llx  rbx = 0x%016llx  rcx = 0x%016llx\n",
                regs.rax, regs.rbx, regs.rcx);
    std::printf("rdx = 0x%016llx  rsi = 0x%016llx  rdi = 0x%016llx\n",
                regs.rdx, regs.rsi, regs.rdi);
    std::printf("r8  = 0x%016llx  r9  = 0x%016llx  r10 = 0x%016llx\n",
                regs.r8,  regs.r9,  regs.r10);
    std::printf("r11 = 0x%016llx  r12 = 0x%016llx  r13 = 0x%016llx\n",
                regs.r11, regs.r12, regs.r13);
    std::printf("r14 = 0x%016llx  r15 = 0x%016llx\n",
                regs.r14, regs.r15);

    // eflags is 32-bit; bits 32-63 are reserved by Intel for backward compatibility
    // (x86 evolved from FLAGS -> EFLAGS -> RFLAGS) and are always zero, as it is noted on Intel's dev manual, they do not have a specific purpose, and maybe they never will

    std::printf("eflags = 0x%08x\n", static_cast<uint32_t>(regs.eflags));
}
