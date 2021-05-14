#ifndef ATTR_H
#define ATTR_H

struct attr {
    char * name;
    char * value;

    struct attr * next;
};

struct attr * mkattr(const char * name, const char *  value);
struct attr * lastattr(struct attr * attr);
void mergeattrs(struct attr * attrs);

#endif
