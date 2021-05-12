#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sysexits.h>

#include "config.h"
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

struct attr {
    char * name;
    char * value;

    struct attr * next;
};

struct attr * mkattr(const char * name, const char * value) {
    struct attr * attr = (struct attr *) malloc(sizeof(struct attr));
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

struct tag {
    char * name;
    struct attr * attrs;
    char * text;

    unsigned int counter;

    struct tag * parent;
    struct tag * child;
    struct tag * sibling;
};

struct tag * new_tag() {
    struct tag * result = (struct tag *) malloc(sizeof(struct tag));
    if (!result) die("malloc");

    result->name = NULL;
    result->attrs = NULL;
    result->text = NULL;

    result->counter = 0;

    result->parent = NULL;
    result->child = NULL;
    result->sibling = NULL;

    return result;
}

bool is_selfclosing(const struct tag * tag) {
    assert(tag->name != NULL);

    for (size_t i = 0; i < sizeof SELFCLOSINGS / sizeof(const char *); i++)
        if (!strcmp(tag->name, SELFCLOSINGS[i]))
            return true;
    return false;
}

struct tag * parse();

unsigned int readuint() {
    const size_t start = counter;
    while (isdigit(peek())) advance();
    const long result = strtol(&source[start], NULL, 10);

    /* TODO: proper overflow handling */
    assert(result >= 0 && result <= UINT_MAX);

    return (unsigned int)result;
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

void mergeattrs(struct attr * attrs) {
    struct attr * parent = NULL;

    for (struct attr * attr = attrs; attr; parent = attr, attr = attr->next)
        for (struct attr * prev = attrs; prev != attr; prev = prev->next)
            if (!strcmp(prev->name, attr->name)) {
                /* TODO: prevent using too many mallocs */
                /* TODO: prevent memory leaks */
                prev->value = strcat(prev->value, " ");
                prev->value = strcat(prev->value, attr->value);

                parent->next = attr->next;

                free(attr->name);
                free(attr->value);
                free(attr);

                if (!parent->next) return;

                attr = parent;
                break;
            }
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
    case '#': return mkattr("id", readvalue());
    case '.': return mkattr("class", readvalue());
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

bool contains(const char container[], size_t size, char candidate) {
    for (size_t i = 0; i < size; i++)
        if (container[i] == candidate)
            return true;
    return false;
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

bool isinline(const char * name) {
    for (size_t i = 0; i < sizeof INLINES / sizeof(char *); i++)
        if (!strcmp(name, INLINES[i])) return true;
    return false;
}

const char * defaultname(const struct tag * parent) {
    if (!parent) return "div";
    if (!strcmp(parent->name, "ul")) return "li";
    if (!strcmp(parent->name, "ol")) return "li";
    if (!strcmp(parent->name, "table")) return "tr";
    if (!strcmp(parent->name, "tr")) return "td";

    if (isinline(parent->name)) return "span";
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
    if (peek() == '(') {
        advance();
        return parse();
    }

    struct tag * tag = new_tag();
    tag->parent = parent;

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

    if (!tag->name && !tag->attrs && tag->text && tag->parent) {
        tag->parent->text = tag->text;
        free(tag);
        return parse(); /* TODO: proper return tag */
    }

    if (!tag->name && (tag->attrs || !tag->text)) tag->name = strdup(defaultname(parent));
    if (tag->name && mode == HTML) {
        insertdefaultattrs(tag);
        expandabbreviations(tag);
    }

    mergeattrs(tag->attrs);

    return tag;
}

void removebackslashes(char * string) {
    assert(string);
    size_t offset = 0;

    for (char * pc = string; *pc != '\0'; pc++) {
        if (*pc == '\\') offset++;
        pc[0] = pc[offset];
    }
}

void render(struct tag * tag, unsigned int level, unsigned int initcounter) {
    if (!tag->name) {
        assert(tag->text);
        fputs(tag->text, stdout);
        if (tag->sibling != NULL) render(tag->sibling, level, initcounter);
        return;
    }

    const unsigned int max_counter = MAX(initcounter, tag->counter);

    for (unsigned int i = initcounter; i <= max_counter; i++) {
        indent(level);

        putchar('<');
        fputs(tag->name, stdout);

        for (struct attr * attr = tag->attrs; attr != NULL; attr = attr->next) {
            char * name = expandtemplate(attr->name, i, max_counter);
            char * value = expandtemplate(attr->value, i, max_counter);

            removebackslashes(value);

            printf(" %s=\"%s\"", name, value);

            free(name);
            free(value);
        }

        if (is_selfclosing(tag)) {
            puts("/>");
            return;
        }

        putchar('>');

        if (tag->text != NULL) {
            char * text = expandtemplate(tag->text, i, max_counter);
            printf("%s", text);
            free(text);
        }

        if (tag->child != NULL) {
            if (!tag->child->name && mode == HTML) {
                render(tag->child, 0, i);
            } else if (isinline(tag->child->name) && mode == HTML) {
                render(tag->child, 0, i);
            } else {
                putchar('\n');
                render(tag->child, level + 1, i);
                indent(level);
            }
        }

        printf("</%s>", tag->name);

        if (!isinline(tag->name) || mode != HTML) putchar('\n');
    }

    if (tag->sibling != NULL) render(tag->sibling, level, initcounter);
}

void clean(struct tag * tag) {
    /* TODO: re-enable cleanup */
    return;

    if (tag->sibling == NULL || tag->name != tag->sibling->name) {
        free(tag->name);

        struct attr * previous = NULL;

        for (struct attr * attr = tag->attrs; attr != NULL; attr = attr->next) {
            free(attr->name);
            free(attr->value);

            free(previous);
            previous = attr;
        }
        free(previous);
    }

    free(tag->text);

    if (tag->sibling != NULL) clean(tag->sibling);
    if (tag->child != NULL) clean(tag->child);

    free(tag);
}

struct tag * lastsibling(struct tag * tag) {
    assert(tag);

    struct tag * result = tag;
    while (result->sibling) result = result->sibling;

    return result;
}

struct tag * parse(void) {
    struct tag * root = readtag(NULL);
    struct tag * previous = root;

    for (;;)  {
        const char op = advance();
        switch(op) {
        case '>':
            previous->child = readtag(previous);
            previous = previous->child;
            break;
        case '+':
            previous->sibling = readtag(previous->parent);
            previous = previous->sibling;
            break;
        case '^':
            {
                struct tag * ancestor = previous->parent;
                while (peek() == '^') {
                    /* TODO: check existing */
                    ancestor = ancestor->parent;
                    advance();
                }
                if (!ancestor) ancestor = lastsibling(root);

                /* TODO: what if ancestor already has siblings ? */
                assert(!ancestor->sibling);

                ancestor->sibling = readtag(ancestor->parent);
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
            /* TODO: handle properly in groups */
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

    struct tag * tree = parse();    
    render(tree, 0, 1);
    clean(tree);

    return EXIT_SUCCESS;
}
