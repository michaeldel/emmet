#ifndef NODE_H
#define NODE_H

#include "tag.h"

enum nodetype { TAG, TEXT, GROUP };

struct node {
    enum nodetype type;
    union {
        struct node * group;
        struct tag * tag;
        char * text;
    } u;

    unsigned int counter;

    struct node * parent;
    struct node * child;
    struct node * sibling;
};

struct node * mknode(enum nodetype type);
struct node * lastsibling(struct node * node);

struct tag * youngesttagintree(struct node * node);

#endif
