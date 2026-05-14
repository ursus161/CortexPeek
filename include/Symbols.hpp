#pragma once
#include <string>
#include <unordered_map>
#include <cstdint>
#include <cstdio>

// parse the symbol table of a binary using nm and return a name -> address map.
// only text symbols (functions) are included.
inline std::unordered_map<std::string, std::uintptr_t> parseSymbols(const std::string& binaryPath) {
    std::unordered_map<std::string, std::uintptr_t> symbols;

    std::string command = "nm -n --defined-only " + binaryPath + " 2>/dev/null";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe)
        return symbols;

    char line[512];
    while (fgets(line, sizeof(line), pipe)) {
        std::uintptr_t address = 0;
        char type = 0;
        char name[256] = {};

        // nm output format: <address> <type> <name>
        if (sscanf(line, "%lx %c %255s", &address, &type, name) != 3)
            continue;

        // 'T' and 't' are text (code) symbols
        if (type != 'T' && type != 't')
            continue;

        symbols[std::string(name)] = address;
    }

    pclose(pipe);
    return symbols;
}
