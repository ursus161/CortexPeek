#pragma once
#include <stdexcept>
#include <string>
#include <cstring>
#include <cerrno>

// baza pentru toate exceptiile noastre derivata din std::exception 
// ideea e ca cumva toate exceptiile acestui program vor fi sub exceptia Cortex, a creierului programului   
class CortexException : public std::runtime_error {
public:
    explicit CortexException(const std::string& msg) : std::runtime_error(msg) {}
};

// erori de la apeluri ptrace (PEEKDATA, POKEDATA, GETREGS etc.)
class PtraceException : public CortexException {
public:
    PtraceException(const std::string& operation, int err)
        : CortexException(operation + " failed: " + std::strerror(err)) {}
};

// erori la fork/exec/waitpid
class ProcessException : public CortexException {
public:
    explicit ProcessException(const std::string& msg) : CortexException(msg) {}
};

// breakpoint deja exista, sau alta problema specifica
class BreakpointException : public CortexException {
public:
    explicit BreakpointException(const std::string& msg) : CortexException(msg) {}
};

// comanda invalida, argumente lipsa
class CommandException : public CortexException {
public:
    explicit CommandException(const std::string& msg) : CortexException(msg) {}
};

// capstone failure
class DisassemblyException : public CortexException {
public:
    explicit DisassemblyException(const std::string& msg) : CortexException(msg) {}
};