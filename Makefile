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

CPPS = src/floodit.cpp
MAIN = src/main.cpp
HEADERS = floodit.hpp pq.hpp unionfind.hpp
HPPS = $(patsubst %,src/%,$(HEADERS))

MAIN_OBJS = $(patsubst src/%.cpp,$(BUILDDIR)/%.o,$(CPPS) $(MAIN))

# Main target
$(TARGET): $(BUILDDIR)/ $(MAIN_OBJS)
	$(CXX) $(LFLAGS) -o $@ $(MAIN_OBJS)

# Object files
$(BUILDDIR)/%.o: src/%.cpp $(HPPS)
	$(CXX) -c $(CFLAGS) -o $@ $<

$(BUILDDIR)/:
	mkdir $(BUILDDIR)

clean:
	-rm $(BUILDDIR)/*.o $(TARGET)

.PHONY: clean
