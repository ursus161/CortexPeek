#pragma once
#include <sys/types.h>
#include <cstdint>

class Breakpoint {
public:
    Breakpoint(pid_t pid, std::uintptr_t address);
    ~Breakpoint() = default;

    // no copy — two breakpoints at the same address would corrupt each other's savedByte_
    Breakpoint(const Breakpoint&)            = delete;
    Breakpoint& operator=(const Breakpoint&) = delete;
    Breakpoint(Breakpoint&&)                 = default;
    Breakpoint& operator=(Breakpoint&&)      = default;

    void enable();
    void disable();

    bool              isEnabled() const { return enabled_; }
    std::uintptr_t    address()   const { return address_; }

private:
    pid_t          pid_;
    std::uintptr_t address_;
    bool           enabled_   = false;
    uint8_t        savedByte_ = 0; // original byte overwritten by 0xCC
};
