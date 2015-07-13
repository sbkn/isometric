CFLAGS=-Wall -Wextra -ggdb -O0 -std=c++0x `sdl-config --cflags`
CC=g++

.PHONY: default

default: main

main: main.cpp Astar.cpp
	$(CC) $(CFLAGS) -o main Astar.cpp main.cpp -lSDL2 -lSDL2_image
	
clean:
	rm -f *.o main
