CFLAGS=-Wall -Wextra -Werror -pedantic
EXE=build/dragon
CC=clang++
SRC=src/dragon.cpp

compile:
	$(CC) $(CFLAGS) $(SRC) -o $(EXE)
	./$(EXE)
