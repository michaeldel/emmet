CC = gcc
CFLAGS = -Wall -Wextra -pedantic

emmet: emmet.c
	$(CC) $(CFLAGS) -o $@ $^

runtests: test.sh emmet
	./test.sh

memcheck: emmet
	echo 'a>b>c' | valgrind --error-exitcode=1 --leak-check=full ./emmet

clean:
	rm -f emmet

.PHONY: clean runtests
