#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_TAG_NAME_LEN 32
#define MAX_TAG_ID_LEN 32
#define MAX_TAG_CLASS 32

size_t bufindex = 0;
char buffer[BUFSIZ];

char next_buffer_char() {
    if (buffer[bufindex] == '\0' || buffer[bufindex] == '\n') {
        if (fgets(buffer, BUFSIZ, stdin) == NULL)
            return '\0';
        bufindex = 0;
    }

    return buffer[bufindex++];
}

void seek_buffer_back() {
    bufindex--;
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

char * read_word() {
    size_t n = 16;
    char * result = (char *) calloc(n, sizeof(char));

    char c;

    size_t i = 0;
    assert(i < n);

    while ((c = next_buffer_char())) {
        if (!isalnum(c)) {
            seek_buffer_back();
            break;
        }

        result[i++] = c;

        if (i == n) {
            n <<= 2;
            assert(n > i);

            /* TODO: check NULL */
            result = (char *) reallocarray(result, n, sizeof(char));
            assert(result != NULL);
        }
    }

    return result;
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
    bool reading = true;

    struct tag * first = NULL;
    struct tag * previous = NULL;

    while (reading) {
        struct tag * tag = new_tag();
        if (first == NULL) first = tag;
        tag->name = read_word();

        char op;

        while ((op = next_buffer_char()) != '>' && reading)
        switch (op) {
        case '#':
            tag->id = read_word();
            break;
        case '.':
            tag->class = read_word();
            break;
        case '\0':
            reading = false;
            break;
        default:
            exit(EXIT_FAILURE);
        }

        if (previous != NULL)
            previous->child = tag;
        previous = tag;
    }

    render(first, 0);

    return EXIT_SUCCESS;
}
