#include "Config.hpp"
#include "Exceptions.hpp"
#include <fstream>

static std::string trim(const std::string& s) {
    const auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    return s.substr(start, s.find_last_not_of(" \t\r\n") - start + 1);
}

Config Config::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        throw ProcessException("cannot open config file: " + path);

    Config config;
    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        const auto sep = line.find('=');
        if (sep == std::string::npos) continue;

        const std::string key   = trim(line.substr(0, sep));
        const std::string value = trim(line.substr(sep + 1));
        if (!key.empty())
            config.values_[key] = value;
    }
    return config;
}

std::size_t Config::getSize(const std::string& key, std::size_t defaultValue) const {
    auto it = values_.find(key);
    if (it == values_.end()) return defaultValue;
    try {
        return static_cast<std::size_t>(std::stoull(it->second));
    } catch (const std::exception&) {
        return defaultValue;
    }
}
