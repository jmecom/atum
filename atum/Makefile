CC=clang

LDFLAGS=
CFLAGS=-Wall -Werror
INCLUDE=

BUILD=build/macos

all: libatum

build/%.o: src/%.c
	$(CC) -c $(INCLUDE) -o $@ $< $(CFLAGS)

libatum: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f build/*

# PROG=cryptopals

# CC=gcc
# CFLAGS=-Wall -Werror -Wno-pragma-once-outside-header -g

# INCLUDE=-I/usr/local/opt/openssl/include
# LDFLAGS=-L/usr/local/opt/openssl/lib
# LDLIBS=-lssl -lcrypto

# BUILD_DIR=obj/
# SOURCE_DIR=src/

# SRC=$(wildcard $(SOURCE_DIR)*.c)
# OBJ=$(addprefix $(BUILD_DIR),$(notdir $(SRC:.c=.o)))

# all: $(PROG)

# obj/%.o: $(SOURCE_DIR)%.c
# 	$(CC) -c $(INCLUDE) -o $@ $< $(CFLAGS)

# $(PROG): $(OBJ)
# 	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LDFLAGS) $(LDLIBS)

# clean:
# 	rm -f $(BUILD_DIR)*
# 	rm -f ./$(PROG)

# .PHONY: all
# .PHONY: clean
