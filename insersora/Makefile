CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -pthread -Iinclude
LDFLAGS = -pthread -lrt
SRC     = $(wildcard src/*.c)
OBJ     = $(SRC:src/%.c=build/%.o)
BIN     = insersora

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

build/%.o: src/%.c include/maquina.h
	mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

run: $(BIN)
	./$(BIN)

clean:
	rm -rf build $(BIN)

.PHONY: all run clean
