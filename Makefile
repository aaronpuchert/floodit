# Settings
VARIANT ?= release
ifeq ($(VARIANT),release)
ifeq ($(CXX),g++)
LTO_FLAGS = -flto=auto
else ifeq ($(CXX),clang++)
LTO_FLAGS = -flto=thin
endif
ADDITIONAL_FLAGS = -O2 -DNDEBUG $(LTO_FLAGS)
else ifeq ($(VARIANT),debug)
ADDITIONAL_FLAGS = -ggdb3
else
ADDITIONAL_FLAGS = $(error Unknown variant, set VARIANT={debug|release})
endif
CFLAGS = -Wall -Wextra -std=c++11 $(ADDITIONAL_FLAGS)
LFLAGS = -Wall

# Files
BUILDDIR = $(VARIANT)
SOLVER = $(BUILDDIR)/floodit
GENERATOR = $(BUILDDIR)/floodit-generator
TEST_TARGET = $(BUILDDIR)/floodit-test

SRC_DIR = src
CPPS = src/floodit.cpp
MAIN = src/main.cpp
TEST_DIR = test
TESTS = test/floodtest.cpp test/trietest.cpp
INCLUDE_DIR = include
HEADERS = $(INCLUDE_DIR)/floodit.hpp $(INCLUDE_DIR)/trie.hpp src/unionfind.hpp

MAIN_OBJS = $(patsubst %.cpp,$(BUILDDIR)/%.o,$(CPPS) $(MAIN))
TEST_OBJS = $(patsubst %.cpp,$(BUILDDIR)/%.o,$(CPPS) $(TESTS))

all: $(SOLVER) $(GENERATOR)

# Google Test shenanigans. Some distributions don't provide libgtest.so.
# So we have to compile it for ourselves first. Well that is fun.
ifdef GTEST_PREFIX
GTEST_DIR = $(GTEST_PREFIX)/src/gtest
GTEST_OBJ = $(BUILDDIR)/gtest-all.o $(BUILDDIR)/gtest_main.o
GTEST = $(GTEST_OBJ)
ifneq ($(GTEST_PREFIX),/usr)
CXXFLAGS += -I$(GTEST_PREFIX)/include
endif
$(GTEST_OBJ): $(BUILDDIR)/%.o: $(GTEST_DIR)/src/%.cc | $(BUILDDIR)/
	$(CXX) -c $(CXXFLAGS) -I$(GTEST_DIR) -o $@ $<
else
GTEST = -lgtest -lgtest_main
endif

# Main target
$(SOLVER): $(MAIN_OBJS)
	$(CXX) $(CFLAGS) $(LFLAGS) -pthread -o $@ $(MAIN_OBJS)

# Test binary
$(TEST_TARGET): $(TEST_OBJS) $(GTEST_OBJ)
	$(CXX) $(CFLAGS) $(LFLAGS) $(GTEST) -pthread -o $@ $(TEST_OBJS)

# Object files
$(BUILDDIR)/%.o: %.cpp $(HEADERS) | $(BUILDDIR)/
	$(CXX) -c $(CFLAGS) -I $(INCLUDE_DIR) -o $@ $<

# Generator
generator: $(GENERATOR)

$(GENERATOR): src/generator.cpp
	$(CXX) $(CFLAGS) $(LFLAGS) -o $@ src/generator.cpp

$(BUILDDIR)/:
	mkdir $(BUILDDIR)
	mkdir $(BUILDDIR)/$(SRC_DIR)
	mkdir $(BUILDDIR)/$(TEST_DIR)

# Tests
test: $(SOLVER) $(TEST_TARGET)
	./$(TEST_TARGET)
	./test/verify $(SOLVER)

clean:
	-rm $(BUILDDIR)/$(SRC_DIR)/*.o $(BUILDDIR)/$(TEST_DIR)/*.o
	-rm $(SOLVER) $(GENERATOR) $(TEST_TARGET)

.PHONY: all generator test clean
