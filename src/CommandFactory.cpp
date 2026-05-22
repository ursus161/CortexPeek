#include "CommandFactory.hpp"
#include "Exceptions.hpp"
#include <algorithm>

void CommandFactory::registerCommand(const std::string& name,
                                     const std::string& help,
                                     Creator creator)
{
    creators_[name] = { std::move(creator), help };
}

void CommandFactory::registerAlias(const std::string& alias, const std::string& target)
{
    if (creators_.find(target) == creators_.end())
        throw CommandException("registerAlias: unknown target command '" + target + "'");
    aliases_[alias] = target;
}

std::string CommandFactory::resolve(const std::string& name) const
{
    auto it = aliases_.find(name);
    return it != aliases_.end() ? it->second : name;
}

bool CommandFactory::has(const std::string& name) const
{
    return creators_.find(resolve(name)) != creators_.end();
}

std::unique_ptr<Command> CommandFactory::create(const std::string& name) const
{
    auto it = creators_.find(resolve(name));
    if (it == creators_.end())
        throw CommandException("unknown command: " + name);
    return it->second.creator();
}

std::vector<std::string> CommandFactory::availableCommands() const
{
    std::vector<std::string> names;
    names.reserve(creators_.size());
    for (const auto& [name, spec] : creators_)
        names.push_back(name);
    std::sort(names.begin(), names.end());
    return names;
}

std::string CommandFactory::helpFor(const std::string& name) const
{
    auto it = creators_.find(resolve(name));
    if (it == creators_.end())
        throw CommandException("unknown command: " + name);
    return it->second.help;
}
