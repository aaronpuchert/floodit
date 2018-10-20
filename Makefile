# Settings
VARIANT ?= release
ifeq ($(VARIANT),release)
ADDITIONAL_FLAGS = -O2 -DNDEBUG
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
TESTS = test/floodtest.cpp
INCLUDE_DIR = include
HEADERS = $(INCLUDE_DIR)/floodit.hpp src/unionfind.hpp

MAIN_OBJS = $(patsubst %.cpp,$(BUILDDIR)/%.o,$(CPPS) $(MAIN))
TEST_OBJS = $(patsubst %.cpp,$(BUILDDIR)/%.o,$(CPPS) $(TESTS))

# Google Test shenanigans. Some distributions don't provide libgtest.so.
# So we have to compile it for ourselves first. Well that is fun.
ifdef GTEST_PREFIX
GTEST_DIR = $(GTEST_PREFIX)/src/gtest
GTEST_OBJ = $(BUILDDIR)/gtest-all.o $(BUILDDIR)/gtest_main.o
GTEST = $(GTEST_OBJ)
ifneq ($(GTEST_PREFIX),/usr)
CXXFLAGS += -I$(GTEST_PREFIX)/include
endif
$(GTEST_OBJ): $(BUILDDIR)/%.o: $(GTEST_DIR)/src/%.cc
	$(CXX) -c $(CXXFLAGS) -I$(GTEST_DIR) -o $@ $^
else
GTEST = -lgtest -lgtest_main
endif

all: $(SOLVER) $(GENERATOR)

# Main target
$(SOLVER): $(BUILDDIR)/ $(MAIN_OBJS)
	$(CXX) $(LFLAGS) -o $@ $(MAIN_OBJS)

# Test binary
$(TEST_TARGET): $(BUILDDIR)/ $(TEST_OBJS) $(GTEST_OBJ)
	$(CXX) $(LFLAGS) $(GTEST) -lpthread -o $@ $(TEST_OBJS)

# Object files
$(BUILDDIR)/%.o: %.cpp $(HEADERS)
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
