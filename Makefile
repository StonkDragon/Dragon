CFLAGS=-Wall -Wextra -Werror -pedantic
EXE=build/dragon
CC=g++
SRC=src/dragon.cpp src/DragonConfig.cpp src/commands/build.cpp src/commands/clean.cpp src/commands/init.cpp src/commands/presets.cpp src/commands/run.cpp

compile:
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $(EXE) -std=gnu++17
