rubiks: rubiks.c
	gcc -o rubiks -O3 -Wall -lpthread -fshort-enums rubiks.c

.PHONY: debug

debug: rubiks.c
	gcc -o rubiks -O0 -Wall -lpthread -g rubiks.c

.PHONY: clean

clean:
	rm rubiks
