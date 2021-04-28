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

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define MAX_TAG_NAME_LEN 32
#define MAX_TAG_ID_LEN 32
#define MAX_TAG_CLASS 32

void die(const char * name) {
    fprintf(stderr, "%s: %s\n", name, strerror(errno));
    abort(); /* TODO: consider exit and cleanup */
}

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

char * expand_template(char * template, unsigned int value, unsigned int max) {
    /* TODO: properly refactor whole function */
    assert(max >= value);

    /* TODO: multiple templates in same string */
    char * pc = strstr(template, "$");
    if (pc == NULL) return strdup(template);

    char * pci = pc;
    while (*pci == '$') pci++;

    const unsigned int padding = pci - pc - 1;
    unsigned int min = 1;

    if (*pci == '@') {
        if (*(++pci) == '-') value = max - value + 1;
        sscanf(pci, "%ud", &min);
        while (isdigit(*pci)) pci++;
    }

    const size_t format_size = pci - pc;

    assert(pc > template);
    assert(format_size > padding);

    const size_t left_size = (size_t)(pc - template);
    const size_t right_size = strlen(template) - left_size - format_size;

    char format[BUFSIZ];
    sprintf(format, "%%0%dd", padding + 1);

    /* TODO: adjust to max value */
    char formatted[5];
    snprintf(formatted, sizeof(formatted), format, min - 1 + value);

    const size_t size = left_size + strlen(formatted) + right_size;
    char * expanded = (char *) calloc(size, sizeof(char));
    if (!expanded) die("calloc");

    strncat(expanded, template, left_size);
    strcat(expanded, formatted);
    strncat(expanded, pci + 1, right_size);

    return expanded;
}

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
    return isalnum(c) || c == '_' || c == '-' || c == '$' || c == '@';
}

char * readvalue() {
    const size_t start = counter;
    while (isvaluechar(peek())) advance();
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

struct attr * readattr(void) {
    switch (advance()) {
    case '#': return mkattr("id", readvalue());
    case '.': return mkattr("class", readvalue());
    case '[': {
        char * name = readname();
        consume('=');

        consume('"');
        char * value = readvalue();
        consume('"');

        consume(']');

        struct attr * result = mkattr(name, value);
        free(name);
        free(value);

        return result;
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
    attr->next = readattrs();

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
            char * name = expand_template(attr->name, i, max_counter);
            char * value = expand_template(attr->value, i, max_counter);

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
            char * text = expand_template(tag->text, i, max_counter);
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
            previous->parent->sibling = readtag(previous->parent->parent);
            previous = previous->parent->sibling;
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
