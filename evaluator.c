#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <setjmp.h>

// constant declarations
#define TAB '\t'
#define CR '\n'
#define FLOAT_DELIM '.'
#define ASSIGNMENT '='


typedef union {
    int i;
    double d;
} var_u;

typedef struct {
    var_u value;
    enum { NONE, INT, DOUBLE } type;
} var_t;

static var_t ev_expression();
static var_t ev_extract_val_from_variable(char var);

// assuming that we can have 26 variables of int or double type (or NONE while starting) from a to z
static var_t g_variables[26];

// lookahead character
static char look;
// evaluation string pointer
static const char* evptr;
// pointer to error message
static const char* errmsg;
// index of error in expression
static size_t errptr;

// structure from unwinding recursions call
static jmp_buf jmpenv;

// whitespace and TABs detection
static bool ev_iswhite(char c) {
    return c == ' ' || c == TAB;
}

// get next character from input string
static void ev_getchar() {
    look = *evptr++;
    errptr++;
    assert(look != 0 && "parser logic error");
}

// skip over leading white space
static void ev_skipwhite() {
    if (ev_iswhite(look)) {
        ev_getchar();
    }
}

// init evaluation process
static void ev_init(const char* expr) {
    evptr = expr;
    errmsg = NULL;
    errptr = 0;
    ev_getchar();
    ev_skipwhite();
}

// recognize an addop (+ or - operation)
static bool ev_isaddop(char c)
{
    return c == '+' || c == '-';
}

static void ev_expected(const char* str) {
    errmsg = str;
    longjmp(jmpenv, 1);
}

// get a number
static var_t ev_getnum() {
    var_t value;

    value.type = NONE;
    
    if (!isdigit(look)) {
        ev_expected("numeric value expected");
    }

    char buf[BUFSIZ];
    char *sptr = buf;
    bool float_flag = false;
    while (isdigit(look) || look == FLOAT_DELIM) {
        if (look == FLOAT_DELIM) {
            if (float_flag) {
                // already have . in number
                ev_expected("bad floating point value");
            } else
                float_flag = true;
        }
        *sptr = look;         
        sptr++;
        ev_getchar();
    }
    *sptr = 0;

    if (float_flag) {
        value.type = DOUBLE;
        value.value.d = strtod(buf, NULL);
    } else {
        value.type = INT;
        value.value.i = strtol(buf, NULL, 10);
    }

    return value;
}

// match a specific input character
static void ev_match(char x) {
    if (look == x) {
        ev_getchar();
        ev_skipwhite();
    } else {
        ev_expected("No match operation");
    }
}

// get a sum of two values (probably of different types)
static var_t ev_operation_add(var_t lhs, var_t rhs) {
    var_t value;
    if (lhs.type == DOUBLE || rhs.type == DOUBLE) {
        value.type = DOUBLE;
        if (lhs.type == DOUBLE && rhs.type == DOUBLE) {
            value.value.d = lhs.value.d + rhs.value.d;
        } else if (lhs.type == DOUBLE) {
            value.value.d = lhs.value.d + rhs.value.i;
        } else // rhs.type == DOUBLE {
            value.value.d = lhs.value.i + rhs.value.d;
    } else {
        // both INT
        value.type = INT;
        value.value.i = lhs.value.i + rhs.value.i;
    } 

    return value;
}

static var_t ev_operation_sub(var_t lhs, var_t rhs) {
    var_t value;
    if (lhs.type == DOUBLE || rhs.type == DOUBLE) {
        value.type = DOUBLE;
        if (lhs.type == DOUBLE && rhs.type == DOUBLE) {
            value.value.d = lhs.value.d - rhs.value.d;
        } else if (lhs.type == DOUBLE) {
            value.value.d = lhs.value.d - rhs.value.i;
        } else // rhs.type == DOUBLE {
            value.value.d = lhs.value.i - rhs.value.d;
    } else {
        // both INT
        value.type = INT;
        value.value.i = lhs.value.i - rhs.value.i;
    } 

    return value;
}

// parse and evaluate a math factor
// <factor> ::= <number> | (<expression>) | <variable>
static var_t ev_factor() {
    var_t value;

    if (look == '(') {
        ev_match('(');
        value = ev_expression();
        ev_match(')');
    } else if (isalpha(look)) {
        // if <variable> (or function in future versions
        value = ev_extract_val_from_variable(look);
    } else // <number>
    value = ev_getnum();

    return value;
}

// parse and evaluate a math expression
static void ev_term() {
    ev_factor();

}

// <factor> ::= <number> | (<expression>) | <variable>
// <term> ::= <factor> [ * factor] [ / factor]

// save some value to global variable
static void ev_assign_val_to_variable(char var, var_t value) {
    char v = toupper(var); 
    assert(var >= 'A' && var <= 'Z' && "variable range not supported");

    int offset = v - 'A';
    g_variables[offset] = value; 
}

// extract value from global variable
static var_t ev_extract_val_from_variable(char var) {
    char v = toupper(var);
    assert(var >= 'A' && var <= 'Z' && "variable range not supported");

    int offset = v - 'A';
    return g_variables[offset]; 
}

/// process one expression
// expression -> [variable = ] <expression> 
static var_t ev_evaluate_one() {
    char current_variable = 0;
    var_t result;

    if (isalpha(look)) {
        current_variable = look;
        ev_match(look);
        ev_skipwhite();
        if (look == ASSIGNMENT) {
            // l-value
            ev_match(look);
            ev_skipwhite();
            result = ev_expression();
            ev_assign_val_to_variable(current_variable, result);
            return result;
        } else {
            // get value of current_value and 
            // evaluate futher
            result = ev_extract_val_from_variable(current_variable);


            // TODO: next calculations
            //
            return result;
        }
    } else return ev_expression();
}

// expression -> [variable]
static var_t ev_expression() {
    var_t value;
    value.type = NONE;

/*    if (isalpha(look)) {
       variable = look;
       ev_skipwhite();
       if (look != ASSIGNMENT) {
       }
       ev_skipwhite();
    }*/

    if (ev_isaddop(look)) {
        value.type = INT;
        value.value.i = 0;
    } else {
        value = ev_getnum();
    }

    while (ev_isaddop(look)) {
        switch (look) {
            case '+':
                ev_match('+');
                value = ev_operation_add(value, ev_getnum());
                break;
            case '-':
                ev_match('-');
                value = ev_operation_sub(value, ev_getnum());
                break;
        }
    }

//    if (variable && value.type != NONE) {
        // assign new value
//    }

    return value;
}

// get last error and string number
size_t ev_get_last_error(char* buf, size_t buflen) {
    if (errmsg != NULL) {
        strncpy(buf, errmsg, buflen);
        return errptr;
    }
    return 0;
}

// convert internal variable value to human-readable string
static void var_t_to_string(var_t value, char* buf, size_t bufsize) {
    const char* fmt;

    *buf = 0;

    if (value.type == NONE) {
        strncpy(buf, "None", bufsize);
    } else {
        if (value.type == INT) {
            fmt = "%d";
            snprintf(buf, bufsize, fmt, value.value.i);
        } else {
            fmt = "%f";
            snprintf(buf, bufsize, fmt, value.value.d);
        }
    }
}

// process expression evaluation from user
int evaluate_expression(const char* expr, char* output, size_t bufsize) {
    ev_init(expr);

    int res = setjmp(jmpenv);
    if (res) 
        return res;
    var_t_to_string(ev_expression(), output, bufsize);
    return 0;
}


