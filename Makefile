CC=gcc
CFLAGS=-Wall -Werror -std=c11 -pedantic -ggdb
LIBS=-lSDL2 -lSDL2_ttf -lm

.PHONY: build run

build:
	$(CC) $(CFLAGS) -o white-noise main.c $(LIBS)

run: build
	./white-noise
