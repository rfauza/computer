SHELL := /bin/sh

CXX := g++
CXXFLAGS := -O2 -Wall -g -fsanitize=address -fno-omit-frame-pointer
DEPFLAGS := -MMD -MP -MF $(dir $@)$(basename $(notdir $<)).d

# Source directory (project `src` folder)
TOP := ../src

# Output: executable in project root
BINDIR := ..

# Find all .cpp files in TOP excluding any path that contains /test/
SRCS := $(shell find $(TOP) -type f -name '*.cpp' -not -path '*/test/*' 2>/dev/null || true)

# Map source files to object files placed under build/ preserving subdirs
# (produce paths like components/AND_Gate.o so pattern rules match)
OBJS := $(patsubst $(TOP)/%.cpp,%.o,$(SRCS))

# Dependency files
DEPS := $(OBJS:.o=.d)

# Tell Make where to search for source files in subdirectories
VPATH := $(TOP) $(sort $(dir $(SRCS)))

.PHONY: all clean fullclean dirs

all: dirs $(BINDIR)/main

dirs:
	@mkdir -p $(sort $(dir $(OBJS)))

# Link all object files into a single executable in project root
$(BINDIR)/main: $(OBJS)
	@echo Linking $@
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@

# Compile rule: search for .cpp via VPATH, generate dependency files
%.o: %.cpp
	@echo Compiling $< -> $@
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

# Include dependency files
-include $(DEPS)

clean:
	@echo Removing stale object files...
	@for obj in $(OBJS); do \
		src=$(TOP)/$$(echo $$obj | sed 's/\.o$$/\.cpp/'); \
		if [ -f "$$src" ] && [ -f "$$obj" ] && [ "$$src" -nt "$$obj" ]; then \
			rm -f "$$obj" "$$(echo $$obj | sed 's/\.o$$/\.d/')"; \
			echo "  Removed $$obj"; \
		fi; \
	done
	@if [ -f $(BINDIR)/main ]; then rm -f $(BINDIR)/main && echo "  Removed executable"; fi
	@echo Done.

fullclean:
	@echo Removing all build artifacts...
	@rm -f $(OBJS) $(DEPS) $(BINDIR)/main
	@echo Done.

# Don't try to track suffix rules
.SUFFIXES: