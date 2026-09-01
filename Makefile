################################################################################
# Makefile - Build system for diserr utility.                                  #
#                                                                              #
# Targets:                                                                     #
#   all   - Build the diserr executable (default)                              #
#   clean - Remove all build artifacts                                         #
#                                                                              #
# Dependencies:                                                                #
#   IBM MQ libraries expected under $(MQINST)                                  #
################################################################################

# ----------------------------------------------------------
# environment configuration
# ----------------------------------------------------------
-include config.mk
MQINST ?= /opt/mqm

# ----------------------------------------------------------
# Compiler and flags
# ----------------------------------------------------------

CC = gcc

BUILD ?= debug
ifeq ($(BUILD),release)
  CFLAGS_BUILD = -O2
  MAKEFLAGS += -B
else
  CFLAGS_BUILD = -g -O0
endif

CFLAGS = -Wall -Wextra -std=c11 -I$(INC_DIR) -I$(MQINST)/inc $(CFLAGS_BUILD) -MMD -MP
LDFLAGS = -L$(MQINST)/lib64 -lmqm -Wl,-rpath,$(MQINST)/lib64

AWK   = awk
CHMOD = chmod
PERL  = perl
MQRC  = mqrc 

# Directories
SRC_DIR = src/
INC_DIR = inc/
OBJ_DIR = obj/
BIN_DIR = bin/
TMP_DIR = tmp/
ETC_DIR = etc/

# ----------------------------------------------------------
# Target executable
# ----------------------------------------------------------
TARGET = $(BIN_DIR)diserr

# ----------------------------------------------------------
# Source and object files
# ----------------------------------------------------------
SRC_FILES = main.c cmdline.c mqutil.c mqreason.c mqconvert.c osutil.c version.c

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
	$(CHMOD) a+rx $@

# ----------------------------------------------------------
# Compile source files to object files
# ----------------------------------------------------------
$(OBJ_DIR)%.o: $(SRC_DIR)%.c $(INC_DIR)mqreason.h | $(OBJ_DIR)
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

$(TMP_DIR)mqreason.h.0: $(TMP_DIR)mqreason.tmp
	@ echo generating $@
	@ $(PERL) -ne  '{                                  \
	   next unless /^\s*(\d+)\s+0x\w+\s+(l?rrcI_\S+)/; \
	   printf("#define %-40s %5d\n", $$2, $$1);        }' $< > $@

$(TMP_DIR)mqreason.c.0: $(TMP_DIR)mqreason.tmp
	@ echo generating $@
	@ $(PERL) -ne '{ \
	    next unless /^\s*(\d+)\s+0x\w+\s+((?:MQRC|l?rrcI_)\S+)/x; \
		next if $$2 =~ /MQRC_CONVERTED_MSG_TOO_BIG/;              \
		next if $$2 =~ /MQRC_PAGESET_FULL/;                       \
		next if $$2 =~ /MQRCCF_QUIESCE_VALUE_ERROR/;              \
		next if $$2 =~ /MQRCCF_IPADDR_ERROR/;                     \
        next if $$2 =~ /rrcI_NORMAL_CHANNEL_END_CONN/;            \
		next if $$2 =~ /rrcI_CHANNEL_START_CONN/;                 \
	    printf("     convert( %-40s) ;\n", $$2);         }' $< > $@

$(INC_DIR)mqreason.h: $(ETC_DIR)mqreason.sh $(TMP_DIR)mqreason.h.0 $(ETC_DIR)mqreason.eh 
	cat $^ > $@

$(SRC_DIR)mqreason.c: $(ETC_DIR)mqreason.sc $(TMP_DIR)mqreason.c.0 $(ETC_DIR)mqreason.ec 
	cat $^ > $@

# ----------------------------------------------------------
# create a version source file with the latest git commit timestamp and build type
# ----------------------------------------------------------
$(SRC_DIR)version.c: $(ETC_DIR)version.sc $(ETC_DIR)version.ec $(INC_DIR)version.h | $(SRC_DIR)
	@ cp $< $@
	@ echo "#define VER_CMT  \"$(shell git log -n 1 --format=%cI)\"" >> $@
	@ echo "#define VER_TYPE \"$(BUILD)\""                           >> $@
	@ echo "#define VER_DATE \"$(shell date -u +%Y-%m-%dT%H:%M:%SZ)\"" >> $@
	@ cat $(word 2,$^) >> $@

# ----------------------------------------------------------
# Auto-generated header dependencies (produced by -MMD -MP)
# ----------------------------------------------------------
-include $(DEPS)

# ----------------------------------------------------------
# Clean build artifacts
# ----------------------------------------------------------
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(TMP_DIR)
	rm -f $(SRC_DIR)mqreason.c $(SRC_DIR)version.c $(INC_DIR)mqreason.h

# ----------------------------------------------------------
# Phony targets
# ----------------------------------------------------------
.PHONY: all clean help

help:
	@echo "Usage: make [BUILD=debug|release] [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all            Build the diserr executable (default)"
	@echo "  clean          Remove all build artifacts"
	@echo "  help           Show this help message"
	@echo ""
	@echo "Options:"
	@echo "  BUILD=debug    Build with -g -O0 (default)"
	@echo "  BUILD=release  Build with -O2, no debug symbols"
	@echo ""
	@echo "Examples:"
	@echo "  make                   debug build"
	@echo "  make BUILD=release     release build"
	@echo "  make clean             remove obj/, bin/, tmp/"
