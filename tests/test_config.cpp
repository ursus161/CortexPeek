#include "catch.hpp"
#include "Config.hpp"
#include "Exceptions.hpp"
#include <fstream>
#include <cstdio>

static std::string writeTempConfig(const std::string& content) {
    const std::string path = "/tmp/cortexpeek_test_config.txt";
    std::ofstream out(path);
    out << content;
    out.close();
    return path;
}

TEST_CASE("Config parses valid file", "[config]") {
    const auto path = writeTempConfig(
        "# comment\n"
        "key1=100\n"
        "key2=hello\n"
        "\n"
        "  salut           =  67  \n"
    );

    auto config = Config::loadFromFile(path);

    SECTION("integer keys parse correctly") {
        REQUIRE(config.getSize("key1", 0) == 100);
        REQUIRE(config.getSize("salut", 0) == 67);
    }

    SECTION("missing key returns default") {
        REQUIRE(config.getSize("missing", 999) == 999);
    }

    SECTION("non-numeric value falls back to default") {
        REQUIRE(config.getSize("key2", 7) == 7);
    }

    SECTION("comments and blank lines are ignored") {
        const auto& values = config.values();
        REQUIRE(values.find("# comment") == values.end());
    }

    std::remove(path.c_str());
}

TEST_CASE("Config throws on missing file", "[config]") {
    REQUIRE_THROWS_AS(
        Config::loadFromFile("/nonexistent/path/to/config.txt"),
        ProcessException
    );
}

TEST_CASE("Config handles empty file gracefully", "[config]") {
    const auto path = writeTempConfig("");
    auto config = Config::loadFromFile(path);
    REQUIRE(config.values().empty());
    REQUIRE(config.getSize("anything", 67) == 67);
    std::remove(path.c_str());
}
