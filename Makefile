rubiks: rubiks.c
	gcc -o rubiks -O3 -Wall -Wextra -pthread -fshort-enums rubiks.c

.PHONY: debug

debug: rubiks.c
	gcc -o rubiks -O0 -Wall -Wextra -pthread -g rubiks.c

.PHONY: clean

clean:
	rm rubiks
