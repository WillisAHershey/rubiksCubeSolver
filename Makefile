rubiks: rubiks.c
	gcc -o rubiks -O3 -Wall -lpthread rubiks.c

.PHONY: clean

clean:
	rm rubiks
