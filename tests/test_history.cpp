#include "catch.hpp"
#include "History.hpp"
#include "Observer.hpp"
#include <string>

TEST_CASE("History<int> basic operations", "[history]") {
    History<int> h(3);

    SECTION("starts empty") {
        REQUIRE(h.size() == 0);
        REQUIRE(h.empty());
        REQUIRE(!h.last().has_value());
    }

    SECTION("push adds items") {
        h.push(10);
        h.push(20);
        REQUIRE(h.size() == 2);
        REQUIRE_FALSE(h.empty());
        REQUIRE(h.last() == 20);
    }

    SECTION("respects max size, drops oldest") {
        h.push(1);
        h.push(2);
        h.push(3);
        h.push(4);
        REQUIRE(h.size() == 3);
        REQUIRE(h.last() == 4);
        REQUIRE(h.entries().front() == 2);
    }

    SECTION("entries iterable in order") {
        h.push(100);
        h.push(200);
        h.push(300);
        auto it = h.entries().begin();
        REQUIRE(*it == 100); ++it;
        REQUIRE(*it == 200); ++it;
        REQUIRE(*it == 300);
    }
}

TEST_CASE("History<std::string> works generically", "[history][template]") {
    History<std::string> h(2);
    h.push("first");
    h.push("second");
    REQUIRE(h.last() == "second");
    REQUIRE(h.size() == 2);
    h.push("third");
    REQUIRE(h.last() == "third");
    REQUIRE(h.entries().front() == "second");
}

TEST_CASE("History<DebugEvent> works with custom struct", "[history][template]") {
    History<DebugEvent> h(5);
    h.push({DebugEventType::Breakpoint, 0});
    h.push({DebugEventType::Signal, 11});
    REQUIRE(h.size() == 2);
    REQUIRE(h.last()->type == DebugEventType::Signal);
    REQUIRE(h.last()->data == 11);
}
