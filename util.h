#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <stddef.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

void die(const char * name);
bool contains(const char container[], size_t size, char candidate);
void removebackslashes(char * string);

#endif
