#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sysexits.h>

#include "attr.h"
#include "config.h"
#include "tag.h"
#include "template.h"
#include "util.h"

#define MAX_TAG_NAME_LEN 32
#define MAX_TAG_ID_LEN 32
#define MAX_TAG_CLASS 32

size_t counter = 0;
char source[BUFSIZ];

char advance() {
    return source[counter++];
}

char peek() {
    return counter == BUFSIZ ? '\0' : source[counter];
}

void consume(char c) {
    for (;;) {
        const char current = advance();

        if (current == c) return;
        if (current == '\0') die("syntax error"); /* TODO: better message */
        /* TODO: handle edge cases */
    }
}

enum mode {
    HTML,
    SGML,
};

enum mode mode = HTML;

struct node * parse();

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

unsigned int readuint() {
    const size_t start = counter;
    while (isdigit(peek())) advance();
    const long result = strtol(&source[start], NULL, 10);

    /* TODO: proper overflow handling */
    assert(result >= 0 && result <= UINT_MAX);

    return (unsigned int) result;
}

void indent(unsigned int level) {
    for (unsigned int i = 0; i != level; i++)
        printf("  ");
}

bool isnamechar(char c) {
    return isalnum(c) || c == '_' || c == '-';
}

char * readname() {
    /* returns NULL if name empty */
    const size_t start = counter;
    while (isnamechar(peek())) advance();

    if (counter == start) return NULL;
    return strndup(&source[start], counter - start);
}

bool isvaluechar(char c) {
    /* TODO: match official emmet value chars */
    return isalnum(c) || c == '_' || c == '-' || c == '$' || c == '@' || c == '\\';
}

bool isquotedvaluechar(char c) {
    /* TODO: match official emmet value chars */
    return isprint(c) && c != '"' && c != '\n';
}

char * readvalue() {
    const size_t start = counter;
    while (isvaluechar(peek())) advance();
    return strndup(&source[start], counter - start);
}

char * readquotedvalue() {
    const size_t start = counter;
    while (isquotedvaluechar(peek())) advance();
    return strndup(&source[start], counter - start);
}
struct attr * readbracketedattr(void) {
    while (peek() == ' ') advance();
    if (peek() == ']') return NULL;

    char * name = readname();

    /* TODO: handle syntax errors */
    assert(peek() == '=');
    advance(); /* equal sign */

    char * value;

    if (peek() == '"') {
        advance();
        value = readquotedvalue();

        /* TODO: handle syntax errors */
        assert(peek() == '"');
        advance();
    } else {
        value = readvalue();
    }

    struct attr * result = mkattr(name, value);

    free(name);
    free(value);

    return result;
}

struct attr * readattr(void) {
    switch (advance()) {
    case '#': {
        char * value = readvalue();
        struct attr * attr = mkattr("id", value);

        free(value);
        return attr;
    }
    case '.': {
        char * value = readvalue();
        struct attr * attr = mkattr("class", value);

        free(value);
        return attr;
    }
    case '[': {
        struct attr dummy = { .next = NULL };
        struct attr * current = &dummy;

        while ((current->next = readbracketedattr())) {
            current = current->next;
        }
        consume(']');
        return dummy.next;
    }
    default: return NULL;
    }
}

struct attr * readattrs() {
    const char prefixes[] = {'#', '.', '['};

    if (!contains(prefixes, sizeof prefixes / sizeof(char), peek()))
        return NULL;

    struct attr * attr = readattr();

    if (attr) lastattr(attr)->next = readattrs();

    return attr;
}

char * readtext() {
    if (peek() != '{') return strdup("");

    advance();
    const size_t start = counter;
    consume('}');

    return strndup(&source[start], counter - start - 1);
}

const char * defaultname(const struct tag * parent) {
    if (!parent) return "div";
    if (!strcmp(parent->name, "ul")) return "li";
    if (!strcmp(parent->name, "ol")) return "li";
    if (!strcmp(parent->name, "table")) return "tr";
    if (!strcmp(parent->name, "tr")) return "td";

    if (isinline(parent)) return "span";
    return "div";
}

void insertdefaultattrs(struct tag * tag) {
    for (size_t i = 0; i < sizeof DEFAULTATTRS / sizeof(const char *[2]); i++)
        if (!strcmp(tag->name, DEFAULTATTRS[i][0])) {
            if (!tag->attrs)
                tag->attrs = mkattr(DEFAULTATTRS[i][1], "");
            else {
                for (const struct attr * attr = tag->attrs; attr; attr = attr->next)
                    if (!strcmp(attr->name, DEFAULTATTRS[i][1]))
                        return;

                struct attr * defaultattr = mkattr(DEFAULTATTRS[i][1], "");
                defaultattr->next = tag->attrs;
                tag->attrs = defaultattr;

                return;
            }
        }
}

void expandabbreviations(struct tag * tag) {
    for (size_t i = 0; i < sizeof ABBREVIATIONS / sizeof(const char *[2]); i++)
        if (!strcmp(tag->name, ABBREVIATIONS[i][0])) {
            free(tag->name);
            tag->name = strdup(ABBREVIATIONS[i][1]);
            break;
        }
}

struct tag * readtag(struct tag * parent) {
    struct tag * tag = mktag();
    tag->name = readname();

    tag->attrs = readattrs(); /* left attrs */
    tag->text = readtext();

    if (tag->attrs) {
        struct attr * last = NULL;
        for (struct attr * attr = tag->attrs; attr; attr = attr->next)
            last = attr;

        last->next = readattrs(); /* right attrs */
    } else {
        tag->attrs = readattrs();
    }

    if (!tag->name && (tag->attrs || !tag->text)) tag->name = strdup(defaultname(parent));
    if (tag->name && mode == HTML) {
        insertdefaultattrs(tag);
        expandabbreviations(tag);
    }

    mergeattrs(tag->attrs);

    return tag;
}

struct tag * youngesttagintree(struct node * node) {
    if (!node) return NULL;
    if (node->type == TAG) return node->u.tag;

    assert(node->type == GROUP);
    return youngesttagintree(node->parent);
}

struct node * readnode(struct node * parent) {
    if (peek() == '(') {
        advance();

        struct node * node = mknode(GROUP);
        node->u.group = parse();
        return node;
    }

    if (peek() == '{') {
        struct node * node = mknode(TEXT);
        node->u.text = readtext();
        return node;
    }

    struct node * node = mknode(TAG);
    node->parent = parent;
    node->u.tag = readtag(youngesttagintree(parent));

    return node;
}

void renderstarttag(const struct tag * tag, unsigned int counter, unsigned int maxcounter) {
    putchar('<');
    fputs(tag->name, stdout);

    for (struct attr * attr = tag->attrs; attr; attr = attr->next) {
        char * name = expandtemplate(attr->name, counter, maxcounter);
        char * value = expandtemplate(attr->value, counter, maxcounter);

        removebackslashes(value);

        printf(" %s=\"%s\"", name, value);

        free(name);
        free(value);
    }

    if (isselfclosing(tag)) {
        puts("/>");
        return;
    }

    putchar('>');

    if (tag->text) {
        char * text = expandtemplate(tag->text, counter, maxcounter);
        printf("%s", text);
        free(text);
    }
}

void renderendtag(const struct tag * tag) {
    if (isselfclosing(tag)) return;
    printf("</%s>", tag->name);
    if (!isinline(tag) || mode != HTML) putchar('\n');
}

void render(const struct node * node, unsigned int level, unsigned int initcounter) {
    const unsigned int maxcounter = MAX(initcounter, node->counter);
    if (node->counter) initcounter = 1;

    for (unsigned int i = initcounter; i <= maxcounter; i++) {
        switch (node->type) {
        case GROUP:
            render(node->u.group, level, i);
            break;
        case TEXT:
            fputs(node->u.text, stdout);
            break;
        case TAG:
            indent(level);
            renderstarttag(node->u.tag, i, maxcounter);

            if (node->child) {
                /* TODO: what about groups ? nested groups ? */
                if (node->child->type == TEXT) {
                    render(node->child, 0, i);
                } else if (
                    node->child->type == TAG &&
                    isinline(node->child->u.tag) &&
                    mode == HTML
                ) {
                    render(node->child, 0, i);
                } else {
                    putchar('\n');
                    render(node->child, level + 1, i);
                    indent(level);
                }
            }

            renderendtag(node->u.tag);
            break;
        }
    }

    if (node->sibling) render(node->sibling, level, initcounter);
}

void clean(struct node * node) {
    switch (node->type) {
    case TEXT:
        free(node->u.text);
        break;
    case TAG:
        free(node->u.tag->name);
        free(node->u.tag->text);

        struct attr * attr = node->u.tag->attrs;

        while (attr) {
            free(attr->name);
            free(attr->value);

            struct attr * next = attr->next;
            free(attr);
            attr = next;
        }

        free(node->u.tag);

        break;
    case GROUP:
        clean(node->u.group);
        break;
    }

    if (node->child) clean(node->child);
    if (node->sibling) clean(node->sibling);

    free(node);
}

struct node * lastsibling(struct node * node) {
    assert(node);

    struct node * result = node;
    while (result->sibling) result = result->sibling;

    return result;
}

struct node * parse(void) {
    struct node * root = readnode(NULL);
    struct node * previous = root;

    for (;;)  {
        const char op = advance();
        switch(op) {
        case '>':
            previous->child = readnode(previous);
            previous = previous->child;
            break;
        case '+':
            previous->sibling = readnode(previous->parent);
            previous = previous->sibling;
            break;
        case '^':
            {
                struct node * ancestor = previous->parent;
                while (peek() == '^') {
                    /* ignore excess climbs */
                    ancestor = ancestor ? ancestor->parent : ancestor;
                    advance();
                }
                if (!ancestor) ancestor = lastsibling(root);

                /* TODO: what if ancestor already has siblings ? */
                assert(!ancestor->sibling);

                ancestor->sibling = readnode(ancestor->parent);
                previous = ancestor->sibling;
            }
            break;
        case '*': 
            previous->counter = readuint();
            break;
        case ')':
            return root;
        case '\n':
        case '\0':
            return root;
        default:
            fprintf(stderr, "ERROR: invalid operator: %c (%d)\n", op, op);
            exit(EX_DATAERR);
        }
    }
}

int main(int argc, char * argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "m:")) != -1)
        switch (opt) {
        case 'm':
            if (!strcmp(optarg, "html")) mode = HTML;
            else if (!strcmp(optarg, "sgml")) mode = SGML;
            else {
                fprintf(stderr, "Invalid mode, available modes are: html, sgml");
                exit(EX_USAGE);
            }
            break;
        default:
            break;
        }

    /* TODO: handle other lengths */
    char * err = fgets(source, BUFSIZ, stdin);
    assert(err != NULL);

    struct node * tree = parse();    
    render(tree, 0, 1);
    clean(tree);

    return EXIT_SUCCESS;
}
