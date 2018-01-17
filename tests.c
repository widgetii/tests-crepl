#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "evaluator.h"

#define SHOW_RESULTS 1

bool test_ev(const char* expr, const char* result) {
    char buf[BUFSIZ];
    char exprbuf[BUFSIZ];

    strncpy(exprbuf, expr, sizeof(exprbuf));
    strcat(exprbuf, "\n");
    if (evaluate_expression(exprbuf, buf, sizeof(buf))) {
        // error occured while parsing
        // check syntax error value
        return *result == '?';
    }

#ifdef SHOW_RESULTS
    fprintf(stderr, "Eval = \"%s\", result = \"%s\", expected = \"%s\"\n", expr, buf, result);
#endif

    return strcmp(buf, result) == 0;
}

int main() {
    assert(test_ev("1", "1"));
    assert(test_ev("3.14", "3.140000"));
    assert(test_ev("3.14.14", "?"));
    assert(test_ev("+1000", "1000"));
    assert(test_ev("-555", "-555"));
    assert(test_ev("1+1", "2"));
    assert(test_ev("1+2+3+4+5+6+7+8+10", "46"));
    assert(test_ev("1+3.14", "4.140000"));
    assert(test_ev("5.534+234.22", "239.754000"));
    assert(test_ev("555-1000", "-445"));
    assert(test_ev("3*8", "24"));
    assert(test_ev("100*0", "0"));
    assert(test_ev("(1+2)*5", "15"));
    assert(test_ev("1+2*5", "11"));
    assert(test_ev("a=(2*2)", "4"));
    assert(test_ev("b  =3 ", "3"));
    assert(test_ev("4/7", "0.571429"));
    assert(test_ev("1/2", "0.500000"));
    assert(test_ev("0/2", "0"));
    assert(test_ev("1/0", "?"));
    assert(test_ev("(a+b)", "7"));
    assert(test_ev("c=a+b", "7"));
    assert(test_ev("g", "?"));
    assert(test_ev("+4+d", "?"));
    assert(test_ev("8823yhjdjkjw822", "?"));
    assert(test_ev("(1 + 2)", "3"));
    assert(test_ev("a / (a + b)", "0.571429"));
    assert(test_ev("n + 0", "?"));

    puts("All tests are ok!\n");
}

