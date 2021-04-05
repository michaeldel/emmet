#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

struct tag {
    char * name;
    char * id;
    char * class;

    struct tag * child;
};

struct tag * new_tag() {
    /* TODO: check NULL */
    struct tag * result = (struct tag *) malloc(sizeof(struct tag));

    result->name = NULL;
    result->id = NULL;
    result->class = NULL;

    result->child = NULL;

    return result;
}

char * read_id() {
    const size_t start = counter;

    while (isalnum(peek())) advance();

    const size_t length = counter - start;
    char * id = (char *)calloc(length, sizeof(char));
    strncpy(id, &source[start], length);

    return id;
}

char * read_class() {
    const size_t start = counter;

    while (isalnum(peek())) advance();

    const size_t length = counter - start;
    char * class = (char *)calloc(length, sizeof(char));
    strncpy(class, &source[start], length);

    return class;
}

struct tag * read_tag() {
    struct tag * tag = (struct tag *)malloc(sizeof(struct tag));;
    assert(tag != NULL);

    const size_t start = counter;

    while (isalnum(peek())) advance();

    const size_t name_length = counter - start;
    tag->name = (char *)calloc(name_length, sizeof(char));
    strncpy(tag->name, &source[start], name_length);

    if (peek() == '#') {
        advance();
        tag->id = read_id();
    }
    if (peek() == '.') {
        advance();
        tag->class = read_class();
    }
    
    /* TODO: id, other classes, attrs, etc */
    while (peek() != '>' && peek() != '\0') advance();

    return tag;
}

void indent(unsigned int level) {
    for (unsigned int i = 0; i != level; i++)
        printf("  ");
}

void render(struct tag * tag, unsigned int level) {
    indent(level);

    putchar('<');
    fputs(tag->name, stdout);

    if (tag->id != NULL)
        printf(" id=\"%s\"", tag->id);
    if (tag->class != NULL)
        printf(" class=\"%s\"", tag->class);

    putchar('>');

    if (tag->child != NULL) {
        putchar('\n');
        render(tag->child, level + 1);
        indent(level);
    }

    fputs("</", stdout);
    fputs(tag->name, stdout);
    fputs(">\n", stdout);
}

int main(void) {
    /* TODO: handle other lengths */
    char * err = fgets(source, BUFSIZ, stdin);
    assert(err != NULL);

    struct tag * first = NULL;
    struct tag * previous = NULL;

    for (;;) {
        struct tag * tag = read_tag();
        if (first == NULL) first = tag;

        const char op = advance();
        assert(op == '>' || op == '\0');

        if (previous != NULL)
            previous->child = tag;
        previous = tag;

        if (op == '\0') break;
    }

    render(first, 0);

    return EXIT_SUCCESS;
}
