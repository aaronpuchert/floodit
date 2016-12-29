# Settings
VARIANT ?= release
ifeq ($(VARIANT),release)
    ADDITIONAL_FLAGS = -O2 -DNDEBUG
else
    ADDITIONAL_FLAGS = -ggdb3
endif
CFLAGS = -Wall -Wextra -std=c++11 $(ADDITIONAL_FLAGS)
LFLAGS = -Wall

# Files
BUILDDIR = $(VARIANT)
TARGET = $(BUILDDIR)/floodit
#TEST_TARGET = $(BUILDDIR)/test

CPPS = src/floodit.cpp
MAIN = src/main.cpp
#TEST = src/test.cpp
HEADERS = floodit.hpp pq.hpp unionfind.hpp
HPPS = $(patsubst %,src/%,$(HEADERS))

MAIN_OBJS = $(patsubst src/%.cpp,$(BUILDDIR)/%.o,$(CPPS) $(MAIN))
TEST_OBJS = $(patsubst src/%.cpp,$(BUILDDIR)/%.o,$(CPPS) $(TEST))

# Main target
$(TARGET): $(BUILDDIR)/ $(MAIN_OBJS)
	$(CXX) $(LFLAGS) -o $@ $(MAIN_OBJS)

# Test binary
#$(TEST_TARGET): $(BUILDDIR)/ $(TEST_OBJS)
#	$(CXX) $(LFLAGS) -lboost_unit_test_framework -o $@ $(TEST_OBJS)

# Object files
$(BUILDDIR)/%.o: src/%.cpp $(HPPS)
	$(CXX) -c $(CFLAGS) -o $@ $<

$(BUILDDIR)/:
	mkdir $(BUILDDIR)

# Tests
#test: $(TEST_TARGET)
#	./$(TEST_TARGET)

clean:
	-rm $(BUILDDIR)/*.o $(TARGET) $(TEST_TARGET)

.PHONY: test clean
