CXX      := g++
CXXFLAGS := -std=c++23 -g -Iinclude -Wall -Wextra -Wpedantic
LDFLAGS  := -lcapstone

SRCS := $(wildcard src/*.cpp)
OBJS := $(SRCS:src/%.cpp=build/%.o)
TARGET := cortexpeek

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) 
#linkex cu libcapstone pt disassembler

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build $(TARGET) $(TEST_TARGET)




#testing w catch2

TEST_SRCS   := $(wildcard tests/test_*.cpp)
TEST_OBJS   := $(TEST_SRCS:tests/%.cpp=build/tests/%.o)
TEST_PROD   := $(filter-out build/main.o, $(OBJS))
TEST_TARGET := test_runner

.PHONY: test

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJS) $(TEST_PROD)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

build/tests/%.o: tests/%.cpp | build/tests
	$(CXX) $(CXXFLAGS) -Itests -MMD -MP -c $< -o $@

build/tests:
	mkdir -p build/tests

-include $(OBJS:.o=.d)
-include $(TEST_OBJS:.o=.d)
