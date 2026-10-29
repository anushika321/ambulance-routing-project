CC=cc
CFLAGS=-std=c11 -Wall -Wextra -pedantic -O2
all: ambulance_engine
ambulance_engine: main.c engine.c engine.h
	$(CC) $(CFLAGS) main.c engine.c -o ambulance_engine
clean:
	rm -f ambulance_engine
