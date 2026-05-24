#include "catch.hpp"
#include "CommandFactory.hpp"
#include "Command.hpp"
#include "Exceptions.hpp"

TEST_CASE("CommandFactory registers and creates commands", "[factory]") {
    CommandFactory factory;
    factory.registerCommand("step", "single step",
        []() { return std::make_unique<StepCommand>(); });

    REQUIRE(factory.has("step"));
    REQUIRE(factory.helpFor("step") == "single step");

    auto cmd = factory.create("step");
    REQUIRE(cmd != nullptr);
    REQUIRE(cmd->name() == "step");
}

TEST_CASE("CommandFactory handles aliases", "[factory]") {
    CommandFactory factory;
    factory.registerCommand("continue", "continue execution",
        []() { return std::make_unique<ContinueCommand>(); });
    factory.registerAlias("c", "continue");

    REQUIRE(factory.has("c"));
    REQUIRE(factory.resolve("c") == "continue");

    auto cmd = factory.create("c");
    REQUIRE(cmd != nullptr);
    REQUIRE(cmd->name() == "continue");
}

TEST_CASE("CommandFactory rejects invalid operations", "[factory]") {
    CommandFactory factory;

    REQUIRE_THROWS_AS(factory.create("nonexistent"),   CommandException);
    REQUIRE_THROWS_AS(factory.helpFor("nope"),         CommandException);
    REQUIRE_THROWS_AS(factory.registerAlias("c", "unknown_cmd"), CommandException);
}

TEST_CASE("CommandFactory lists commands sorted", "[factory]") {
    CommandFactory factory;
    factory.registerCommand("zebra", "z", []() { return std::make_unique<StepCommand>(); });
    factory.registerCommand("alpha", "a", []() { return std::make_unique<StepCommand>(); });
    factory.registerCommand("beta",  "b", []() { return std::make_unique<StepCommand>(); });

    auto list = factory.availableCommands();
    REQUIRE(list.size() == 3);
    REQUIRE(list[0] == "alpha");
    REQUIRE(list[1] == "beta");
    REQUIRE(list[2] == "zebra");
}
