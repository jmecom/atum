CC=clang

LDFLAGS=-dynamiclib
CFLAGS=-Wall -Werror
INCLUDE=-Iinc

BUILD_DIR=build
SOURCE_DIR=src

SRC=$(wildcard $(SOURCE_DIR)/*.c)
OBJ=$(addprefix $(BUILD_DIR)/,$(notdir $(SRC:.c=.o)))

LIBATUM=$(BUILD_DIR)/libatum.dylib
LIBATUM_DEBUG=$(BUILD_DIR)/libatum-debug.dylib

all: $(LIBATUM)

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c
	$(CC) -c $(INCLUDE) -o $@ $< $(CFLAGS)

$(LIBATUM): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LDFLAGS)

$(LIBATUM_DEBUG): $(OBJ)
	CFLAGS+=-g
	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)/*

.PHONY: clean
