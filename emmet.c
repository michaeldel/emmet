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
        c == '>' || c == '+' || c == '^' || c == '*' || c == ')'
    );
}

char * expand_template(const char * template, unsigned int value, unsigned int max) {
    /* TODO: properly refactor whole function */
    assert(max >= value);

    /* TODO: multiple templates in same string */
    char * pc = strstr(template, "$");
    if (pc == NULL) return template;

    char * pci = pc;
    while (*pci == '$') pci++;

    const unsigned int padding = pci - pc - 1;

    if (*pci == '@') {
        if (*(++pci) == '-') value = max - value + 1;
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
    snprintf(formatted, sizeof(formatted), format, value);

    const size_t size = left_size + strlen(formatted) + right_size;
    char * expanded = (char *)calloc(size, sizeof(char));
    assert(expanded != NULL); /* TODO: proper check */

    strncat(expanded, template, left_size);
    strcat(expanded, formatted);
    strncat(expanded, pci + 1, right_size);

    return expanded;
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

bool is_name_char(char c) {
    return isalnum(c) || c == '$' || c == '@' || c == '-';
}

char * read_id() {
    const size_t start = counter;

    while (is_name_char(peek())) advance();

    const size_t length = counter - start;
    char * id = (char *)calloc(length + 1, sizeof(char));
    strncpy(id, &source[start], length);

    return id;
}

char * read_class() {
    const size_t start = counter;

    while (is_name_char(peek())) advance();

    const size_t length = counter - start;
    char * class = (char *)calloc(length, sizeof(char));
    strncpy(class, &source[start], length);

    return class;
}

struct tag * parse();

struct tag * read_tag() {
    if (peek() == '(') {
        advance();
        return parse();
    }

    struct tag * tag = new_tag();
    assert(tag != NULL);

    const size_t start = counter;

    while (isalnum(peek())) advance();

    const size_t name_length = counter - start;
    /* TODO: length == 0 */
    tag->name = (char *)calloc(name_length + 1, sizeof(char));
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

struct tag * parse(void) {
    struct tag * root = read_tag();
    struct tag * previous = root;

    for (;;)  {
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
        case '*': {
            /* TODO: max value */
            const unsigned int n = read_uint();
            const struct tag * first = previous;

            for (unsigned int i = 1; i < n; i++) {
                struct tag * cloned = new_tag();
                memcpy(cloned, previous, sizeof(struct tag));

                previous->sibling = cloned;
                previous = previous->sibling;
            }

            struct tag * current = first;

            /* TODO: same for id, text, attr, etc */
            for (unsigned int i = 1; i <= n; i++) {
                if (current->class != NULL) {
                    current->class = expand_template(current->class, i, n);
                }

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
