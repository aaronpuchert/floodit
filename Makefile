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
SOLVER = $(BUILDDIR)/floodit
GENERATOR = $(BUILDDIR)/floodit-generator

CPPS = src/floodit.cpp
MAIN = src/main.cpp
HEADERS = floodit.hpp unionfind.hpp
HPPS = $(patsubst %,src/%,$(HEADERS))

MAIN_OBJS = $(patsubst src/%.cpp,$(BUILDDIR)/%.o,$(CPPS) $(MAIN))

all: $(SOLVER) $(GENERATOR)

# Main target
$(SOLVER): $(BUILDDIR)/ $(MAIN_OBJS)
	$(CXX) $(LFLAGS) -o $@ $(MAIN_OBJS)

# Object files
$(BUILDDIR)/%.o: src/%.cpp $(HPPS)
	$(CXX) -c $(CFLAGS) -o $@ $<

# Generator
generator: $(GENERATOR)

$(GENERATOR): src/generator.cpp
	$(CXX) $(CFLAGS) $(LFLAGS) -o $@ src/generator.cpp

$(BUILDDIR)/:
	mkdir $(BUILDDIR)

clean:
	-rm $(BUILDDIR)/*.o $(SOLVER) $(GENERATOR)

.PHONY: all generator clean
