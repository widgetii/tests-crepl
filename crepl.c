#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "evaluator.h"

#define VERSION "0.1"

// constant declarations
#define OUTS stdout
#define INS stdin

static void repl_loop() {
    char inbuf[BUFSIZ];
    char outbuf[BUFSIZ];

    fputs("REPL simple environment, "VERSION" "__DATE__" "__TIME__"\n", OUTS);
    for (;;) {
        fputs("> ", OUTS);
        if (fgets(inbuf, sizeof(inbuf), INS) == NULL) {
            fputs("Breaked, closing...\n", OUTS);
            break;
        }
        if (inbuf[0] != '\n') {
            if (evaluate_expression(inbuf, outbuf, sizeof(outbuf))) {
                size_t n = ev_get_last_error(outbuf, sizeof(outbuf));
                memset(inbuf, ' ', n+1);
                inbuf[n+1] = '^';
                inbuf[n+2] = 0;
                fprintf(OUTS, "%s\nError: %s\n", inbuf, outbuf);
            } else 
                fprintf(OUTS, "%s\n", outbuf); 
        }
    }
}

int main() {
    repl_loop();
}

