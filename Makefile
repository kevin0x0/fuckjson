.POSIX:

.PHONY: dirs clean install

OBJ_DIR = obj
BIN_DIR = bin

INSTALL_PREFIX = ~/.local/bin/
CC = gcc
DEBUG = -DNDEBUG
OPTIMIZE = -O3
CFLAGS = $(DEBUG) $(OPTIMIZE) -Wall -Wextra --std=c23 -D_POSIX_C_SOURCE=2

BINARIES = $(CURDIR)/$(BIN_DIR)/fj

$(BIN_DIR)/fj:

# Rules defined here
include rules.mk
# OBJECTS defined here
include objects.mk

$(BIN_DIR)/fj: $(OBJECTS) $(OBJ_DIR)/src-main.o
	$(CC) -o $(CURDIR)/$@ $(CFLAGS) $(LINK_FLAGS) $(OBJECT_FILES) $(CURDIR)/$(OBJ_DIR)/src-main.o

dirs:
	mkdir -p $(CURDIR)/$(OBJ_DIR) $(CURDIR)/$(BIN_DIR)

clean:
	rm -f $(OBJECT_FILES) $(EXCLUSIVE_OBJECT_FILES) $(BINARIES)

install:
	install -s $(BINARIES) $(INSTALL_PREFIX)
