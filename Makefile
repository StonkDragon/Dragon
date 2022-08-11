CFLAGS=-Wall -Wextra -Werror -pedantic
EXE=build/dragon
CC=clang++
SRC=src/dragon.cpp

compile:
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $(EXE) -std=gnu++17
	./$(EXE) build
