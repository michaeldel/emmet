#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void die(const char * name) {
    fprintf(stderr, "%s: %s\n", name, strerror(errno));
    abort(); /* TODO: consider exit and cleanup */
}

bool contains(const char container[], size_t size, char candidate) {
    for (size_t i = 0; i < size; i++)
        if (container[i] == candidate)
            return true;
    return false;
}

void removebackslashes(char * string) {
    assert(string);
    size_t offset = 0;

    for (char * pc = string; *pc != '\0'; pc++) {
        if (*pc == '\\') offset++;
        pc[0] = pc[offset];
    }
}
