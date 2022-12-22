CFLAGS=-Wall -Wextra -Werror -pedantic
EXE=build/dragon
CC=clang++
SRC=src/dragon.cpp src/DragonConfig.cpp

compile:
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $(EXE) -std=gnu++17
