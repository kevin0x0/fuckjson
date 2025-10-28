.POSIX:

.PHONY: dirs clean install

OBJ_DIR = obj
BIN_DIR = bin

INSTALL_PREFIX = ~/.local/bin/
CC = gcc
DEBUG = -DNDEBUG
OPTIMIZE = -O3
CFLAGS = $(DEBUG) $(OPTIMIZE) -Wall -Wextra --std=c23 -D_POSIX_C_SOURCE=2

BINARIES = $(BIN_DIR)/fj

$(BINARIES):

# Rules defined here
include rules.mk
# OBJECTS defined here
include objects.mk
# TARGET_OBJECTS defined here
include target_objects.mk

$(BIN_DIR)/fj: $(OBJECTS) $(OBJ_DIR)/src-main.o
	cd $(CURDIR); $(CC) -o $@ $^ $(CFLAGS) $(LINK_FLAGS)

dirs:
	cd $(CURDIR); mkdir -p $(OBJ_DIR) $(BIN_DIR)

clean:
	cd $(CURDIR); rm -f $(OBJECTS) $(TARGET_OBJECTS) $(BINARIES)

install:
	cd $(CURDIR); install -s $(BINARIES) $(INSTALL_PREFIX)
