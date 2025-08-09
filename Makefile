ROOT_DIR = $(shell pwd)
SRC_DIR = $(ROOT_DIR)/src
OBJ_DIR = $(ROOT_DIR)/obj
BIN_DIR = $(ROOT_DIR)/bin
BINARIES = $(BIN_DIR)/fj

INSTALL_TARGET = $(HOME)/.local/bin/
# OBJECTS defined here
include objects.mk

CC = gcc
DEBUG = -DNDEBUG
OPTIMIZE = -O3
CFLAGS = $(DEBUG) $(OPTIMIZE) -Wall -Wextra --std=c23


$(BIN_DIR)/fj: $(OBJECTS) | create_dir
	$(CC) -o $@ $^ $(CFLAGS) $(LINK_FLAGS)

include deps.mk

.PHONY: create_dir clean install

create_dir:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

clean:
	$(RM) $(OBJECTS) $(BINARIES)

install:
	install -s $(BINARIES) $(INSTALL_TARGET)
