#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "template.h"

bool succeeded;
unsigned int line;

#define check(condition) if(!(condition)) { succeeded = false; line = __LINE__; return; }

void test_expandtemplate_empty() {
    check(expandtemplate("", 0, 0));
    check(expandtemplate("", 0, 1));
    check(expandtemplate("", 1, 1));
}

void test_expandtemplate_no_placeholder() {
    check(!strcmp(expandtemplate("foo", 1, 3), "foo"));
    check(!strcmp(expandtemplate("bar", 2, 3), "bar"));
    check(!strcmp(expandtemplate("baz", 3, 3), "baz"));
}

void test_expandtemplate_zero() {
    check(!strcmp(expandtemplate("$", 0, 0), "$"));
    check(!strcmp(expandtemplate("foo $", 0, 0), "foo $"));
}

void test_expandtemplate_one() {
    check(!strcmp(expandtemplate("$", 1, 1), "1"));
    check(!strcmp(expandtemplate("foo $", 1, 1), "foo 1"));
    check(!strcmp(expandtemplate("foo $ bar", 1, 1), "foo 1 bar"));
}

void test_expandtemplate_padding() {
    check(!strcmp(expandtemplate("$$", 1, 1), "01"));
    check(!strcmp(expandtemplate("$$", 99, 99), "99"));
    check(!strcmp(expandtemplate("$$", 100, 100), "100"));
}

void test_expandtemplate_min() {
    check(!strcmp(expandtemplate("$@1", 1, 1), "1"));
    check(!strcmp(expandtemplate("$@2", 1, 1), "2"));
    check(!strcmp(expandtemplate("$@2", 2, 2), "3"));
}

void test_expandtemplate_reverse() {
    check(!strcmp(expandtemplate("$@-", 1, 1), "1"));
    check(!strcmp(expandtemplate("$@-", 1, 2), "2"));

    check(!strcmp(expandtemplate("$@-10", 1, 5), "14"));
    check(!strcmp(expandtemplate("$@-10", 2, 5), "13"));
}

void (*const TESTS[])(void) = { DEFTESTS };
const char * const TESTNAMES[] = { DEFTESTNAMES };

int main(void) {
    bool ret = EXIT_SUCCESS;

    for (size_t i = 0; i < sizeof TESTS / sizeof TESTS[0]; i++) {
        void (*const test)(void) = TESTS[i];
        const char * const testname = TESTNAMES[i];
        succeeded = true;

        test();

        if (!succeeded) {
            printf("FAIL %s (check at line %u)\n", testname, line);
            ret = EXIT_FAILURE;
        } else {
            printf("ok %s\n", testname);
        }
    }

    return ret;
}

