# Shiny Profiler
# GNU Makefile for Linux
#
# By default libShiny.a will be compiled with g++ C++ compiler. If you prefer
# or need a plain C version use the 'cclib' target. The C compiled lib should
# be compatible to C AND C++ code conforming to ISO C99 and ISO C++98 (or
# later).
#
# To build Shiny as a static C++ library use:
# $ make
# or
# $ make shiny-cxx
#
# To build it as a static C library (ISO C99) use:
# $ make shiny-c
#
# For building either library including debug symbols use:
# $ make debug-cxx
# or
# $ make debug-c
#
# If you experience incompatibilities please feel free to tailor this makefile
# to your needs and report problems here:
# - sourceforge: http://sourceforge.net/projects/shinyprofiler/
# - Google Code: http://code.google.com/p/shinyprofiler/


SRCS		= $(wildcard src/*.c)
OBJS		= $(SRCS:.c=.o)

LIB_NAME	= libShiny.a
LIB_PATH	= lib
LIB_PATH_DEBUG	= lib/debug


##################
# Compiler options
CFLAGS		= -std=c99
CXXFLAGS	= -std=c++98
CCFLAGS_RELEASE	= -O3 -fPIC -Wall -Wpedantic
CCFLAGS_DEBUG	= -g3 -O0 -Wall -Wpedantic
INCLUDE		= -Iinclude

# C options
shiny-c debug-c: CCOMP = $(CC)
shiny-c: CCFLAGS = $(CCFLAGS_RELEASE) $(CFLAGS)
debug-c: CCFLAGS = $(CCFLAGS_DEBUG) $(CFLAGS)

# C++ options
shiny-cxx debug-cxx: CCOMP = $(CXX)
shiny-cxx: CCFLAGS = $(CCFLAGS_RELEASE) $(CXXFLAGS)
debug-cxx: CCFLAGS = $(CCFLAGS_DEBUG) $(CXXFLAGS)


##################
# Target definitions

# Default target builds static C++ lib
all: shiny-cxx

# Copy release build to lib/
shiny-c shiny-cxx: clean $(LIB_NAME)
	mkdir -p $(LIB_PATH)
	mv $(LIB_NAME) $(LIB_PATH)

# Copy debug build to lib/debug/
debug-c debug-cxx: clean $(LIB_NAME)
	mkdir -p $(LIB_PATH_DEBUG)
	mv $(LIB_NAME) $(LIB_PATH_DEBUG)

# Build static lib in either C or C++
$(LIB_NAME): $(OBJS)
	$(AR) rcs $@ $^

# Create object files from source code
.c.o:
	$(CCOMP) $(CCFLAGS) $(INCLUDE) -c $< -o $@


# Remove temporary files and compilation results
clean:
	$(RM) $(OBJS) $(LIB_PATH)/$(LIB_NAME) $(LIB_PATH_DEBUG)/$(LIB_NAME) 


help:
	@echo "Available targets for Shiny"
	@echo " all: Build static library libShiny.a using C++ compiler"
	@echo " shiny-cxx: Build static library libShiny.a using C++ compiler"
	@echo " shiny-c: Build static library libShiny.a using C compiler"
	@echo ""
	@echo " debug-cxx: Build static library libShiny.a with debug symbols using C++ compiler"
	@echo " debug-c: Build static library libShiny.a with debug symbols using C compiler"
	@echo ""
	@echo " clean: Clean up directories"

.PHONY: all shiny-c shiny-cxx debug-c debug-cxx help clean
