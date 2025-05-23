CC := gcc
CFLAGS :=
INSTALL := install
PREFIX := /usr/local
bindir := $(PREFIX)/bin

lsb: main.c snode_list.c util.c parse_script.c update_script.c write_script.c parse_binary.c bpe_compression.c bpe_compression.h parse_binary.h parse_binary_psx.h parse_binary_reEng.c parse_binary_reEng.h parse_binary_psx.c psx_decode.c psx_decode.h snode_list.h util.h parse_script.h update_script.h script_node_types.h write_script.h
	$(CC) $(CFLAGS) -Wall main.c snode_list.c util.c parse_script.c parse_binary.c parse_binary_psx.c parse_binary_reEng.c psx_decode.c update_script.c write_script.c bpe_compression.c -o $@

.PHONY: all clean install

all: lsb

install: lsb
	$(INSTALL) -d $(bindir)
	$(INSTALL) lsb $(bindir)

clean:
	rm -f lsb
