#include <assert.h>
#include <stdlib.h>

#include "node.h"
#include "util.h"

struct node * mknode(enum nodetype type) {
    struct node * result = malloc(sizeof(struct node));
    if (!result) die("malloc node");

    result->type = type;
    result->u.group = NULL;

    result->counter = 0;

    result->parent = NULL;
    result->child = NULL;
    result->sibling = NULL;
    
    return result;
}

struct node * lastsibling(struct node * node) {
    assert(node);

    struct node * result = node;
    while (result->sibling) result = result->sibling;

    return result;
}

struct tag * youngesttagintree(struct node * node) {
    if (!node) return NULL;
    if (node->type == TAG) return node->u.tag;

    assert(node->type == GROUP);
    return youngesttagintree(node->parent);
}
