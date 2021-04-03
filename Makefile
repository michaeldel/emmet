CC = gcc
CFLAGS = -Wall -Wextra -pedantic

emmet: emmet.c
	$(CC) $(CFLAGS) -o $@ $^
