#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sysexits.h>

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

bool is_operator(char c) {
    return (
        c == '>' || c == '+' || c == '^' || c == '*'
    );
}

struct tag {
    char * name;
    char * id;
    char * class;

    struct tag * parent;
    struct tag * child;
    struct tag * sibling;
};

struct tag * new_tag() {
    struct tag * result = (struct tag *) malloc(sizeof(struct tag));

    if (result == NULL) {
        perror("could not allocate new tag");
        abort();
    }

    result->name = NULL;
    result->id = NULL;
    result->class = NULL;

    result->parent = NULL;
    result->child = NULL;
    result->sibling = NULL;

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
    struct tag * tag = new_tag();
    assert(tag != NULL);

    const size_t start = counter;

    while (isalnum(peek())) advance();

    const size_t name_length = counter - start;
    /* TODO: length == 0 */
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
    while (!is_operator(peek()) && peek() != '\0') advance();

    return tag;
}

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

    if (tag->sibling != NULL) {
        render(tag->sibling, level);
    }
}

void clean(struct tag * tag) {
    if (tag->sibling == NULL || tag->name != tag->sibling->name) {
        free(tag->name);
        free(tag->id);
        free(tag->class);
    }

    if (tag->sibling != NULL) clean(tag->sibling);
    if (tag->child != NULL) clean(tag->child);

    free(tag);
}

int main(void) {
    /* TODO: handle other lengths */
    char * err = fgets(source, BUFSIZ, stdin);
    assert(err != NULL);

    struct tag * first = read_tag();
    struct tag * previous = first;

    bool reading = true;

    while (reading)  {
        const char op = advance();
        switch(op) {
        case '>':
            previous->child = read_tag();
            previous->child->parent = previous;
            previous = previous->child;
            break;
        case '+':
            previous->sibling = read_tag();
            previous = previous->sibling;
            break;
        case '^':
            previous->parent->sibling = read_tag();
            previous = previous->parent->sibling;
            break;
        case '*':
            for (unsigned int n = read_uint(), i = 1; i != n; i++) {
                /* TODO: proper handling */
                assert(n != 0);

                struct tag * cloned = new_tag();
                memcpy(cloned, previous, sizeof(struct tag));

                previous->sibling = cloned;
                previous = previous->sibling;
            }
            break;            
        case '\n':
        case '\0':
            reading = false;
            break;
        default:
            fprintf(stderr, "ERROR: invalid operator: %c (%d)\n", op, op);
            exit(EX_DATAERR);
        }
    }

    render(first, 0);
    clean(first);

    return EXIT_SUCCESS;
}
