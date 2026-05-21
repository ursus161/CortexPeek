#include "Breakpoint.hpp"
#include <sys/ptrace.h>
#include <cerrno>
#include <cstring>
#include "Exceptions.hpp"


Breakpoint::Breakpoint(pid_t pid, std::uintptr_t address)
    : pid_(pid), address_(address) {}

void Breakpoint::enable() {
    if (enabled_) return; // can't enable the enabled

    // PTRACE_PEEKDATA reads one word (8 bytes on x86-64) at a time
    errno = 0;
    long word = ptrace(PTRACE_PEEKDATA, pid_, reinterpret_cast<void*>(address_), nullptr);
    if (word == -1 && errno)
         throw PtraceException("PEEKDATA", errno);

    savedByte_ = static_cast<uint8_t>(word & 0xFF);

    // replace only the lowest byte with int3; leave the rest of the word intact
    long patched = (word & ~0xFFL) | 0xCC;
    if (ptrace(PTRACE_POKEDATA, pid_, reinterpret_cast<void*>(address_),
               reinterpret_cast<void*>(patched)) < 0)
        throw PtraceException("POKEDATA", errno);

    enabled_ = true;
}

void Breakpoint::disable() {
    if (!enabled_) return; // can't disable the disabled

    errno = 0;
    long word = ptrace(PTRACE_PEEKDATA, pid_, reinterpret_cast<void*>(address_), nullptr);
    if (word == -1 && errno)
         throw PtraceException("PEEKDATA", errno);

    // restore the original byte so the instruction is intact again
    long restored = (word & ~0xFFL) | savedByte_;
    if (ptrace(PTRACE_POKEDATA, pid_, reinterpret_cast<void*>(address_),
               reinterpret_cast<void*>(restored)) < 0)
        throw PtraceException("POKEDATA", errno);

    enabled_ = false;
}
