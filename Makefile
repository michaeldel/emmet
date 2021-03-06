CC = gcc
CFLAGS = -Wall -Wextra -pedantic -g

VALGRIND = valgrind
VFLAGS = --quiet --error-exitcode=1 --leak-check=full --track-origins=yes

SRC = attr.c node.c tag.c template.c util.c
HEADERS = attr.h node.h tag.h config.h template.h util.h
COMMONDEPS = $(SRC) $(HEADERS)

emmet: emmet.c $(COMMONDEPS)
	$(CC) $(CFLAGS) -o $@ $^

DEFTESTS = $(shell \
	ctags -f - --c-kinds=fp test/unit.c | grep '^test_' | cut -f 1 | tr '\n' ',' \
)
DEFTESTNAMES = $(shell \
	ctags -f - --c-kinds=fp test/unit.c | grep '^test_' | cut -f 1 | \
	sed -E 's/(.*)/"\1"/' | tr '\n' ',' \
)

test/unit: test/unit.c $(COMMONDEPS)
	$(CC) $(CFLAGS) \
		-I . \
		-D DEFTESTS='$(DEFTESTS)' \
		-D DEFTESTNAMES='$(DEFTESTNAMES)' \
		-o $@ $^

runtests: test/unit test/functional.sh emmet
	./test/unit
	./test/functional.sh

memcheck: emmet
	echo 'a>b>c' | $(VALGRIND) $(VFLAGS) ./emmet

fullmemcheck: emmet
	EMMET='$(VALGRIND) $(VFLAGS) ./emmet' ./test/functional.sh

ci: runtests memcheck fullmemcheck

clean:
	rm -f emmet
	rm -f test/unit

.PHONY: ci clean fullmemcheck memcheck runtests
