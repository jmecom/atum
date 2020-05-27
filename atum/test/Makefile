CC=clang

CFLAGS=-Werror
INCLUDE=

BUILD_DIR=../build/test/

TARGET=$(BUILD_DIR)/target
PAYLOAD=$(BUILD_DIR)/payload

all: setup $(TARGET) $(PAYLOAD)

setup:
	mkdir -p $(BUILD_DIR)

$(TARGET): target.o
	@$(CC) $(CFLAGS) target.o -o target

$(PAYLOAD): payload.o
	@$(CC) -dynamiclib payload.o -o payload.dylib

$(TARGET).o:
	$(CC) -c target.c

$(PAYLOAD).o:
	$(CC) -c payload.c

clean:
	rm -f $(BUILD_DIR)*

.PHONY: clean
