# ====================================== DIRECTORIES ======================================

# .c files
SRCDIR = src
# .h files
IDIR   = include
# .o files
ODIR   = obj
# executable
BINDIR = bin

# ========================================= FLAGS =========================================

CC       = clang
CPPFLAGS = -I$(IDIR)
DEPFLAGS = -MMD -MP
CPPFLAGS += $(DEPFLAGS)
CFLAGS   = -Wall -Wextra -Wpedantic -Werror -Werror=return-type \
		   -Wno-padded -Wconversion -Wshadow -Wnull-dereference \
		   -Wformat-security -Wunused-function -Wunused-variable\
		   -std=c11 -O2 -g -fcolor-diagnostics

# ====================================== SOURCE FILES =====================================

TARGET = archivist
SRC := $(wildcard $(SRCDIR)/*.c)
OBJ = $(addprefix $(ODIR)/,$(notdir $(SRC:.c=.o)))
EXE = $(BINDIR)/$(TARGET)

# ====================================== BUILD RULES ======================================

# Creates directory for .o files
$(ODIR):
	@mkdir -p $@

# Creates directory for executable
$(BINDIR):
	@mkdir -p $@

# Converts .c files into .o files
$(ODIR)/%.o: $(SRCDIR)/%.c | $(ODIR)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Main Rule
$(EXE): $(OBJ) | $(BINDIR)
	@$(CC) $(CFLAGS) $^ -o $@

# ======================================== TARGETS ========================================

# Builds executable
all: $(EXE)

# Allows 'make archivist' command
archivist: all

# Cleans .o files, executable, and respective directories
clean:
	rm -rf $(ODIR) $(BINDIR)

# Cleans and rebuilds
rebuild: clean all

# Builds and Runs executable
run: $(EXE)
	./$(EXE)

# Creates executable for debug
debug:
	$(MAKE) CFLAGS="$(CFLAGS) -g -O0 -fsanitize=address,leak" all

# Creates optimized version of executable
release:
	$(MAKE) CFLAGS="-std=c11 -O3 -DNDEBUG" all

# Shows available commands
help:
	@echo "=== AVAILABLE COMMANDS ==="
	@echo "make               → compiles program"
	@echo "make clean         → erases .o files and executable"
	@echo "make debug         → compiles w/ debug (-g -O0)"
	@echo "make help          → shows this help"
	@echo "make rebuild       → cleans and recompiles"
	@echo "make release       → compiles w/ optimization (-O3)"
	@echo "make run           → compiles and executes"

# States false targets
.PHONY: all clean rebuild run debug release help

# Converts .o files into dependecy files (.d)
-include $(OBJ:.o=.d)