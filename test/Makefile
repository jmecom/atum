CC=clang

CFLAGS=-Wall -Werror

BUILD_DIR=build
SOURCE_DIR=.

TARGET=build/target
PAYLOAD=build/payload

all: $(TARGET) $(PAYLOAD)

$(TARGET): $(TARGET).o
	@$(CC) $(TARGET).o -o $(TARGET)

$(TARGET).o: target.c
	$(CC) -c target.c -o $(TARGET).o

$(PAYLOAD): $(PAYLOAD).o
	@$(CC) -dynamiclib $(PAYLOAD).o -o $(PAYLOAD).dylib

$(PAYLOAD).o: payload.c
	$(CC) -c payload.c -o $(PAYLOAD).o

clean:
	rm -rf $(BUILD_DIR)/*

.PHONY: clean
