rubiks: rubiks.c
	gcc -o rubiks -O3 -Wall -Wextra -lpthread -fshort-enums rubiks.c

.PHONY: debug

debug: rubiks.c
	gcc -o rubiks -O0 -Wall -Wextra -lpthread -g rubiks.c

.PHONY: clean

clean:
	rm rubiks
