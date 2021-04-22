CC = gcc
CFLAGS = -Wall -Wextra -pedantic -g

VALGRIND = valgrind
VFLAGS = --quiet --error-exitcode=1 --leak-check=full --track-origins=yes

emmet: emmet.c
	$(CC) $(CFLAGS) -o $@ $^

runtests: test.sh emmet
	./test.sh

memcheck: emmet
	echo 'a>b>c' | $(VALGRIND) $(VFLAGS)./emmet

fullmemcheck: emmet
	EMMET='$(VALGRIND) $(VFLAGS) ./emmet' ./test.sh

clean:
	rm -f emmet

.PHONY: clean runtests
