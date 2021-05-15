#ifndef TAG_H
#define TAG_H

#include <stdbool.h>

struct tag {
    char * name;
    char * text;
    struct attr * attrs;
};

struct tag * mktag(void);

bool isselfclosing(const struct tag * tag);
bool isinline(const struct tag * tag);

#endif
