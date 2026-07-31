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
ETC_DIR = etc/

# ----------------------------------------------------------
# Target executable
# ----------------------------------------------------------
TARGET = $(BIN_DIR)diserr

# ----------------------------------------------------------
# Source and object files
# ----------------------------------------------------------
SRC_FILES = main.c cmdline.c mqutil.c mqreason.c mqconvert.c

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
	    printf("     convert(%-40s) ;\n", $$2);          }' $< > $@

$(INC_DIR)mqreason.h: $(ETC_DIR)mqreason.sh $(TMP_DIR)mqreason.h.0 $(ETC_DIR)mqreason.eh 
	cat $^ > $@

$(SRC_DIR)mqreason.c: $(ETC_DIR)mqreason.sc $(TMP_DIR)mqreason.c.0 $(ETC_DIR)mqreason.ec 
	cat $^ > $@

# ----------------------------------------------------------
# Auto-generated header dependencies (produced by -MMD -MP)
# ----------------------------------------------------------
-include $(DEPS)

# ----------------------------------------------------------
# Clean build artifacts
# ----------------------------------------------------------
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(TMP_DIR)
	rm -f $(SRC_DIR)mqreason.c $(INC_DIR)mqreason.h

# ----------------------------------------------------------
# Phony targets
# ----------------------------------------------------------
.PHONY: all clean
