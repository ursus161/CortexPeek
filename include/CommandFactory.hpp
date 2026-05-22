#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>
#include "Command.hpp"

class CommandFactory {
public:

    // i ve been typing the whole name and i want it to come in handy as:  std::function<std::unique_ptr<Command>()>
    using Creator = std::function<std::unique_ptr<Command>()>;

    struct CommandSpec {
        Creator     creator;
        std::string help;
    };

    void registerCommand(const std::string& name,
                         const std::string& help,
                         Creator creator);
    // throws CommandException if target is not a registered command
    void registerAlias(const std::string& alias, const std::string& target);

    // throws CommandException if name (or its alias) is not registered
    std::unique_ptr<Command> create(const std::string& name) const;

    bool has(const std::string& name) const;

    // sorted list of registered command names, excluding aliases
    std::vector<std::string> availableCommands() const;

    // throws CommandException if name is not registered
    std::string helpFor(const std::string& name) const;

    // resolves one level of alias; returns name unchanged if not an alias
    std::string resolve(const std::string& name) const;

    const std::unordered_map<std::string, std::string>& aliases() const { return aliases_; }

private:
    std::unordered_map<std::string, CommandSpec>  creators_;
    std::unordered_map<std::string, std::string>  aliases_;
};
