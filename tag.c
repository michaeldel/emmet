#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "tag.h"
#include "util.h"

/* TODO: complete lists */
const char * INLINES[] = {
    "a", "b", "em", "span"
};

const char * SELFCLOSINGS[] = {
    "input",
};

struct tag * mktag() {
    struct tag * tag = malloc(sizeof(struct tag));
    if (!tag) die("mktag: malloc tag");

    tag->name = NULL;
    tag->attrs = NULL;
    tag->text = NULL;

    return tag;
}

bool isselfclosing(const struct tag * tag) {
    assert(tag->name);

    for (size_t i = 0; i < sizeof SELFCLOSINGS / sizeof(const char *); i++)
        if (!strcmp(tag->name, SELFCLOSINGS[i]))
            return true;
    return false;
}

bool isinline(const struct tag * tag) {
    for (size_t i = 0; i < sizeof INLINES / sizeof(char *); i++)
        if (!strcmp(tag->name, INLINES[i])) return true;
    return false;
}
