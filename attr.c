#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "attr.h"
#include "util.h"

struct attr * mkattr(const char * name, const char * value) {
    struct attr * attr = malloc(sizeof(struct attr));
    if (!attr) die("mkattr: malloc attr");

    attr->name = strdup(name);
    if (!attr->name) die("mkattr: strdup name");

    attr->value = strdup(value);
    if (!attr->value) die("mkattr: strdup value");

    attr->next = NULL;

    return attr;     
}

struct attr * lastattr(struct attr * attr) {
    assert(attr);

    while (attr->next) attr = attr->next;
    return attr;
}

void mergeattrs(struct attr * attrs) {
    struct attr * parent = NULL;

    for (struct attr * attr = attrs; attr; parent = attr, attr = attr->next)
        for (struct attr * prev = attrs; prev != attr; prev = prev->next)
            if (!strcmp(prev->name, attr->name)) {
                /* 2 is for space and nul termination */
                const size_t newsize = strlen(prev->value) + strlen(attr->value) + 2;
                prev->value = realloc(prev->value, newsize);
                if (!prev->value) die("mergeattrs: realloc value");

                strcat(prev->value, " ");
                strcat(prev->value, attr->value);

                parent->next = attr->next;

                free(attr->name);
                free(attr->value);
                free(attr);

                if (!parent->next) return;

                attr = parent;
                break;
            }
}
