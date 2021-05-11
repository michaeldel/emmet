#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void die(const char * name) {
    fprintf(stderr, "%s: %s\n", name, strerror(errno));
    abort(); /* TODO: consider exit and cleanup */
}
