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
};

struct tag * new_tag() {
    /* TODO: check NULL */
    struct tag * result = (struct tag *) malloc(sizeof(struct tag));
    result->name = NULL;
    result->id = NULL;
    result->class = NULL;

    return result;
}

static const struct tag * tagstack[BUFSIZ];
static size_t num_tags = 0;

void push_tag(const struct tag * tag) {
    /* TODO: check size */
    tagstack[num_tags++] = tag;
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

int main(void) {
    bool reading = true;

    while (reading) {
        struct tag * tag = new_tag();
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

        push_tag(tag);
    }

    for (size_t i = 0; i != num_tags; i++) {
        for (size_t j = 0; j != i; j++)
            printf("  ");
        putchar('<');

        const struct tag * tag = tagstack[i];
        printf("%s", tag->name);

        if (tag->id != NULL)
            printf(" id=\"%s\"", tag->id);
        if (tag->class != NULL)
            printf(" class=\"%s\"", tag->class);

        puts(">");
    }
    for (size_t i = 0; i != num_tags; i++) {
        for (size_t j = 0; j != num_tags - i - 1; j++)
            printf("  ");
        printf("</%s>\n", tagstack[num_tags - i - 1]->name);
    }

    return EXIT_SUCCESS;
}
