################################################################################
# Makefile - Build system for diserr utility.                                  #
#                                                                              #
# Targets:                                                                     #
#   all   - Build the diserr executable (default)                              #
#   clean - Remove all build artifacts                                         #
#                                                                              #
# Dependencies:                                                                #
#   IBM MQ libraries expected under /opt/mqm                                   #
################################################################################

# ----------------------------------------------------------
# Compiler and flags
# ----------------------------------------------------------
CC = gcc
DEBUG = -g
CFLAGS = -Wall -Wextra -std=c11 -I$(INC_DIR) -I/opt/mqm/inc $(DEBUG) -MMD -MP
LDFLAGS = -L/opt/mqm/lib64 -lmqm -lmqmzf -Wl,-rpath,/opt/mqm/lib64 

PERL = perl
AWK  = awk
MQRC = mqrc 

# Directories
SRC_DIR = src/
INC_DIR = inc/
OBJ_DIR = obj/
BIN_DIR = bin/
TMP_DIR = tmp/

# ----------------------------------------------------------
# Target executable
# ----------------------------------------------------------
TARGET = $(BIN_DIR)diserr

# ----------------------------------------------------------
# Source and object files
# ----------------------------------------------------------
SRC_FILES = main.c cmdline.c mqutil.c

SOURCES = $(addprefix $(SRC_DIR), $(SRC_FILES))
OBJECTS = $(addprefix $(OBJ_DIR), $(SRC_FILES:.c=.o))
DEPS    = $(OBJECTS:.o=.d)

# ----------------------------------------------------------
# Default target
# ----------------------------------------------------------
all: $(TARGET)

# ----------------------------------------------------------
# Link object files to create executable
# ----------------------------------------------------------
$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# ----------------------------------------------------------
# Compile source files to object files
# ----------------------------------------------------------
$(OBJ_DIR)%.o: $(SRC_DIR)%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ----------------------------------------------------------
# Create directories if they don't exist
# ----------------------------------------------------------
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(TMP_DIR):
	mkdir -p $(TMP_DIR)

# ----------------------------------------------------------
# handling mq reason codes
# ----------------------------------------------------------
$(TMP_DIR)mqreason.tmp: | $(TMP_DIR)
	@ echo generating $@
	@ $(MQRC) -R -a               \
	|   $(AWK) '                  \
	    # mqrc wraps long symbol  \
	    # names at 80 cols; join  \
	    # continuation lines back \
	    # into one record.        \
	    /^ +[0-9]/ {              \
	      if (cur) print cur;     \
	      cur = $$0;              \
	      next;                   \
	    }                         \
	    {                         \
	      sub (/^ +/, "");        \
	      cur = cur $$0;          \
	    }                         \
	    END {                     \
	      if (cur) print cur;     \
	    } '                       \
	| grep -v "^$$" > $@

$(INC_DIR)mqreason.h: $(TMP_DIR)mqreason.tmp
	@ echo generating $@
	@ $(PERL) -n \
	  -e 'next unless /^\s*(\d+)\s+0x\w+\s+(l?rrcI_\S+)/;' \
	  -e 'printf("#define %-40s %5d\n", $$2, $$1);'         \
	  $< > $@

$(SRC_DIR)mqreason.c: $(TMP_DIR)mqreason.tmp
	@ echo generating $@
	@ $(PERL) -n \
	  -e '/^\s*\d+\s+0x\w+\s+(\S+)/ or next;'                   \
	  -e 'my $$sym = $$1;'                                         \
	  -e 'next unless $$sym =~ /^MQRC/ || $$sym =~ /^l?rrcI_/;'   \
	  -e 'next if $$sym =~ /MQRC_CONVERTED_MSG_TOO_BIG/;'          \
	  -e 'next if $$sym =~ /MQRC_PAGESET_FULL/;'                   \
	  -e 'next if $$sym =~ /MQRCCF_QUIESCE_VALUE_ERROR/;'          \
	  -e 'next if $$sym =~ /rrcI_NORMAL_CHANNEL_END_CONN/;'        \
	  -e 'next if $$sym =~ /rrcI_CHANNEL_START_CONN/;'             \
	  -e 'printf("    convert(%-40s) ;\n", $$sym);'               \
	  $< > $@


# ----------------------------------------------------------
# Auto-generated header dependencies (produced by -MMD -MP)
# ----------------------------------------------------------
-include $(DEPS)

# ----------------------------------------------------------
# Clean build artifacts
# ----------------------------------------------------------
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(TMP_DIR)

# ----------------------------------------------------------
# Phony targets
# ----------------------------------------------------------
.PHONY: all clean
