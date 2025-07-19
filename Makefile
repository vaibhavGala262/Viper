

# Define the C compiler to use
CC = gcc

# Define the name of the executable
TARGET = server

# Define source directories and build directories
SRCDIR = src
OBJDIR = obj
BINDIR = bin

# List all source files in the SRCDIR
# This finds all .c files in the src/ directory
SRCS = $(wildcard $(SRCDIR)/*.c)

# Generate a list of object files (.o) from the source files
# Each .c file in src/ will have a corresponding .o file in obj/
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

# Define any compiler flags
# -Wall: Enables all common warning messages
# -Wextra: Enables extra warning messages
# -g: Includes debugging information (useful if you want to use a debugger like GDB)
# -O0: No optimization (good for debugging, change to -O2 or -O3 for release)
# -I$(SRCDIR): Tells the compiler to look for header files in the src/ directory
CFLAGS = -Wall -Wextra -g -O0 -I$(SRCDIR)

# Define any linker flags
# -lpthread: Link with the POSIX threads library (needed for multi-threading)
LDFLAGS = -lpthread

# --- Rules ---

# Default target: 'all'
# This rule is executed when you just type 'make' in the terminal.
# It ensures the build directories exist and then builds the target executable.
all: $(BINDIR) $(OBJDIR) $(BINDIR)/$(TARGET)

# Rule to create the executable (e.g., bin/server)
# It depends on all object files.
$(BINDIR)/$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Rule to compile each .c file into a .o file
# This is a pattern rule: it applies to any .o file that depends on a .c file.
# $<: Automatic variable for the prerequisite (e.g., src/main.c)
# $@: Automatic variable for the target (e.g., obj/main.o)
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to create the object directory if it doesn't exist
$(OBJDIR):
	@mkdir -p $(OBJDIR)

# Rule to create the binary directory if it doesn't exist
$(BINDIR):
	@mkdir -p $(BINDIR)

# Rule to clean up all build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BINDIR) $(OBJDIR)
	@find . -name "*~" -exec rm {} \; # Remove editor backup files

# Phony targets:
# .PHONY declares targets that are not actual files.
# This prevents 'make all' or 'make clean' from being skipped if a file named 'all' or 'clean' exists.
.PHONY: all clean


run: all
	@echo "Running $(TARGET)..."
	./$(BINDIR)/$(TARGET)

