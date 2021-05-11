#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h> /* TODO: use strtol instead of scanf */
#include <stdlib.h>
#include <string.h>

#include "util.h"

#define MAX_PLACEHOLDERS 64
#define MAX_COUNTER_LENGTH 8

struct placeholder {
    size_t padding;
    size_t length;
    size_t position;

    unsigned int min;
    bool reverse;
};

char * expandtemplate(char * template, unsigned int value, unsigned int max) {
    assert(max >= value);

    if (value == 0) return strdup(template);

    struct placeholder placeholders[MAX_PLACEHOLDERS];
    size_t placeholders_count = 0;

    char * pc = template;

    /* TODO: cache placeholders for a given template */
    for (size_t i = 0; i < MAX_PLACEHOLDERS; i++) {
        pc = strstr(pc, "$");
        if (!pc) break;
        if (pc > template && pc[-1] == '\\') continue; /* ignore escaped */

        const size_t position = (size_t) (pc - template);
        const char * start = pc++;

        while (*pc == '$') pc++;
        size_t padding = pc - start;

        unsigned int min = 1;
        bool reverse = false;

        if (*pc == '@') {
            pc++;

            if (*pc == '-') {
                reverse = true;
                pc++;
            }

            /* TODO: use strol, handle lack of counter */
            sscanf(pc, "%ud", &min);
            while (isdigit(*pc)) pc++;
        }

        placeholders[i] = (struct placeholder) {
            .padding = padding,
            .length = pc - start,
            .position = position,
            .min = min,
            .reverse = reverse
        };
        placeholders_count++;
    }

    if (placeholders_count == 0) return strdup(template);

    char * expanded = (char *) calloc(
        strlen(template) + MAX_COUNTER_LENGTH * placeholders_count,
        sizeof(char)
    );
    if (!expanded) die("expandtemplate: calloc");
    assert(strlen(expanded) == 0);

    pc = template;
    for (size_t i = 0; i < placeholders_count; i++) {
        const struct placeholder ph = placeholders[i];
        strncat(expanded, pc, ph.position);

        /* TODO: optimize */
        char format[MAX_COUNTER_LENGTH + 4] = "";
        sprintf(format, "%%0%zud", ph.padding);

        /* TODO: optimize */
        char * destination = &expanded[strlen(expanded)];
        const int v = ph.reverse ?
            max + ph.min - value :
            value + ph.min - 1;
        sprintf(destination, format, v);

        pc = &template[ph.position + ph.length];
        // printf("exp |%s|\n", expanded);
        // printf("PC %zu :: |%s|\n", i, pc);
        // printf("phl %zu\n", ph.length);
    }
    if (*pc) strcat(expanded, pc);

    return expanded;
}
