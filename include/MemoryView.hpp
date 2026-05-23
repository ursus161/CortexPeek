#pragma once
#include <sys/ptrace.h>
#include <sys/types.h>
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <algorithm>
#include <type_traits>
#include <concepts>
#include "Exceptions.hpp"

// concept: T must have no non-trivial constructor/destructor/copy
// required because read/write copy raw bytes via memcpy over T's binary representation
template<typename T>
concept TriviallyCopyable = std::is_trivially_copyable_v<T>;

// template so callers can read typed values directly: MemoryView<uint64_t>, MemoryView<uint32_t> etc.
template<TriviallyCopyable T>
class MemoryView {
public:
    explicit MemoryView(pid_t pid) : pid_(pid) {}

    T    read(std::uintptr_t address) const;
    void write(std::uintptr_t address, T value);

private:
    pid_t pid_;
};

template<TriviallyCopyable T>
T MemoryView<T>::read(std::uintptr_t address) const {
    T value{};
    auto*  destination = reinterpret_cast<uint8_t*>(&value);
    size_t remaining   = sizeof(T);
    size_t offset      = 0;

    while (remaining > 0) {
        // PTRACE_PEEKDATA always reads one full word (8 bytes); we copy only what we need
        errno = 0;
        long word = ptrace(PTRACE_PEEKDATA, pid_,
                           reinterpret_cast<void*>(address + offset), nullptr);
        if (word == -1 && errno)
            throw PtraceException("PEEKDATA", errno);

        size_t bytesToCopy = std::min(remaining, sizeof(long));
        std::memcpy(destination + offset, &word, bytesToCopy);
        offset    += bytesToCopy;
        remaining -= bytesToCopy;
    }
    return value;
}

template<TriviallyCopyable T>
void MemoryView<T>::write(std::uintptr_t address, T value) {
    const auto* source    = reinterpret_cast<const uint8_t*>(&value);
    size_t      remaining = sizeof(T);
    size_t      offset    = 0;

    while (remaining > 0) {
        long   word        = 0;
        size_t bytesToCopy = std::min(remaining, sizeof(long));

        // partial word: read first so adjacent bytes aren't clobbered
        if (bytesToCopy < sizeof(long)) {
            errno = 0;
            word = ptrace(PTRACE_PEEKDATA, pid_,
                          reinterpret_cast<void*>(address + offset), nullptr);
            if (word == -1 && errno)
               throw PtraceException("PEEKDATA", errno);
        }

        std::memcpy(&word, source + offset, bytesToCopy);

        if (ptrace(PTRACE_POKEDATA, pid_,
                   reinterpret_cast<void*>(address + offset),
                   reinterpret_cast<void*>(word)) < 0)
            throw PtraceException("PEEKDATA", errno);

        offset    += bytesToCopy;
        remaining -= bytesToCopy;
    }
}
