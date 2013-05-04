CC=gcc
FLAGS=-Wall -O2 -lSDL -lSDL_gfx


life2: life2.c
	$(CC) $(FLAGS) -o $@ $<


life3: life3.c
	$(CC) $(FLAGS) -o $@ $<


clean:
	rm -f life2 life3
