#pragma once
#include <string>
#include <unordered_map>
#include <cstddef>

class Config {
public:
    Config() = default;

    // loads key=value pairs from a file; throws ProcessException if file cannot be opened
    static Config loadFromFile(const std::string& path);

    // returns the value parsed as size_t, or defaultValue if the key is missing or unparseable
    std::size_t getSize(const std::string& key, std::size_t defaultValue) const;

    // read-only access to all entries, used by ConfigCommand for display
    const std::unordered_map<std::string, std::string>& values() const { return values_; }

private:
    std::unordered_map<std::string, std::string> values_;
};
