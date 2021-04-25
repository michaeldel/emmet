#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sysexits.h>

#include "constants.h"

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
    unsigned int counter_max;

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
    result->counter_max = 0;

    result->parent = NULL;
    result->child = NULL;
    result->sibling = NULL;

    return result;
}

bool is_selfclosing(const struct tag * tag) {
    assert(tag->name != NULL);

    for (size_t i = 0; i < sizeof selfclosing / sizeof(const char *); i++)
        if (!strcmp(tag->name, selfclosing[i]))
            return true;
    return false;
}

struct tag * parse();

unsigned int read_uint() {
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

void render(struct tag * tag, unsigned int level) {
    indent(level);

    putchar('<');
    fputs(tag->name, stdout);

    for (struct attr * attr = tag->attrs; attr != NULL; attr = attr->next) {
        char * name = expand_template(attr->name, tag->counter, tag->counter_max);
        char * value = expand_template(attr->value, tag->counter, tag->counter_max);

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
        char * text = expand_template(tag->text, tag->counter, tag->counter_max);
        printf("%s", text);
        free(text);
    }

    if (tag->child != NULL) {
        putchar('\n');
        render(tag->child, level + 1);
        indent(level);
    }

    fputs("</", stdout);
    fputs(tag->name, stdout);
    fputs(">\n", stdout);

    if (tag->sibling != NULL) {
        render(tag->sibling, level);
    }
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

bool isnamechar(char c) {
    return isalnum(c) || c == '_' || c == '-';
}

char * readname() {
    /* TODO: enforce length != 0 */
    const size_t start = counter;
    while (isnamechar(peek())) advance();
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

struct tag * readtag(void) {
    if (peek() == '(') {
        advance();
        return parse();
    }

    struct tag * tag = new_tag();

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

    mergeattrs(tag->attrs);

    return tag;
}

struct tag * parse(void) {
    struct tag * root = readtag();
    struct tag * previous = root;

    for (;;)  {
        const char op = advance();
        switch(op) {
        case '>':
            previous->child = readtag();
            previous->child->parent = previous;
            previous = previous->child;
            break;
        case '+':
            previous->sibling = readtag();
            previous = previous->sibling;
            break;
        case '^':
            previous->parent->sibling = readtag();
            previous = previous->parent->sibling;
            break;
        case '*': {
            const unsigned int n = read_uint();
            struct tag * first = previous;

            for (unsigned int i = 1; i < n; i++) {
                struct tag * cloned = new_tag();
                memcpy(cloned, previous, sizeof(struct tag));

                previous->sibling = cloned;
                previous = previous->sibling;
            }

            struct tag * current = first;

            for (unsigned int i = 1; i <= n; i++) {
                current->counter = i;
                current->counter_max = n;
                current = current->sibling;
            }
            } break;
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

int main(void) {
    /* TODO: handle other lengths */
    char * err = fgets(source, BUFSIZ, stdin);
    assert(err != NULL);

    struct tag * tree = parse();    
    render(tree, 0);
    clean(tree);

    return EXIT_SUCCESS;
}
