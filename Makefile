CC = gcc
CFLAGS = -Wall -Wextra -pedantic

emmet: emmet.c
	$(CC) $(CFLAGS) -o $@ $^

runtests: test.sh emmet
	./test.sh

clean:
	rm -f emmet

.PHONY: clean runtests
