CC ?= gcc
CFLAGS ?= -std=c17 -O2 -Wall -Wextra -Wno-misleading-indentation
LDLIBS =
ifeq ($(OS),Windows_NT)
LDLIBS = -lws2_32
endif

all: ambulance

ambulance: src/server.c src/engine.h
	$(CC) $(CFLAGS) src/server.c -o ambulance $(LDLIBS)

test: ambulance
	./ambulance test

run: ambulance
	./ambulance serve 8080

clean:
	rm -f ambulance ambulance.exe
