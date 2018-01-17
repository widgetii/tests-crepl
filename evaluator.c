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

static var_t ev_expression(var_t start_value);
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
        fprintf(stderr, "c = %c\n", x);
        ev_expected("No match operation");
    }
}

// get a sum of two values (probably of different types)
static var_t ev_operation_add(var_t lhs, var_t rhs) {
    if (lhs.type == NONE || rhs.type == NONE) {
        ev_expected("Cannot evaluate expression");
    }

    var_t result;
    if (lhs.type == DOUBLE || rhs.type == DOUBLE) {
        result.type = DOUBLE;
        if (lhs.type == DOUBLE && rhs.type == DOUBLE) {
            result.value.d = lhs.value.d + rhs.value.d;
        } else if (lhs.type == DOUBLE) {
            result.value.d = lhs.value.d + rhs.value.i;
        } else // rhs.type == DOUBLE {
            result.value.d = lhs.value.i + rhs.value.d;
    } else {
        // both INT
        result.type = INT;
        result.value.i = lhs.value.i + rhs.value.i;
    } 

    return result;
}

static var_t ev_operation_sub(var_t lhs, var_t rhs) {
    if (lhs.type == NONE || rhs.type == NONE) {
        ev_expected("Cannot evaluate expression");
    }

    var_t result;
    if (lhs.type == DOUBLE || rhs.type == DOUBLE) {
        result.type = DOUBLE;
        if (lhs.type == DOUBLE && rhs.type == DOUBLE) {
            result.value.d = lhs.value.d - rhs.value.d;
        } else if (lhs.type == DOUBLE) {
            result.value.d = lhs.value.d - rhs.value.i;
        } else // rhs.type == DOUBLE {
            result.value.d = lhs.value.i - rhs.value.d;
    } else {
        // both INT
        result.type = INT;
        result.value.i = lhs.value.i - rhs.value.i;
    } 

    return result;
}

static var_t ev_operation_mul(var_t lhs, var_t rhs) {
    if (lhs.type == NONE || rhs.type == NONE) {
        ev_expected("Cannot evaluate expression");
    }

    var_t result;
    if (lhs.type == DOUBLE || rhs.type == DOUBLE) {
        result.type = DOUBLE;
        if (lhs.type == DOUBLE && rhs.type == DOUBLE) {
            result.value.d = lhs.value.d * rhs.value.d;
        } else if (lhs.type == DOUBLE) {
            result.value.d = lhs.value.d * rhs.value.i;
        } else // rhs.type == DOUBLE {
            result.value.d = lhs.value.i * rhs.value.d;
    } else {
        // both INT
        result.type = INT;
        result.value.i = lhs.value.i * rhs.value.i;
    } 

    return result;
}

static var_t ev_operation_div(var_t lhs, var_t rhs) {
    if (lhs.type == NONE || rhs.type == NONE) {
        ev_expected("Cannot evaluate expression");
    }

    if ((rhs.type == DOUBLE && rhs.value.d == 0) || (rhs.type == INT && rhs.value.i == 0)) {
        ev_expected("Division per zero detected");
    }

    var_t result;
    if (lhs.type == DOUBLE || rhs.type == DOUBLE) {
        result.type = DOUBLE;
        if (lhs.type == DOUBLE && rhs.type == DOUBLE) {
            result.value.d = lhs.value.d / rhs.value.d;
        } else if (lhs.type == DOUBLE) {
            result.value.d = lhs.value.d / rhs.value.i;
        } else // rhs.type == DOUBLE {
            result.value.d = lhs.value.i / rhs.value.d;
    } else {
        // both INT
        // BUT! it's really strange but we need to get in value
        // even on two int's division, so checking if we have remainder
        // and if does, doing DOUBLE result
        if (lhs.value.i % rhs.value.i) {
            result.type = DOUBLE;
            result.value.d = (double)lhs.value.i / rhs.value.i;
        } else {
            result.type = INT;
            result.value.i = lhs.value.i / rhs.value.i;
        }
    } 

    return result;
}


// parse and evaluate a math factor
// <factor> ::= <number> | (<expression>) | <variable>
static var_t ev_factor() {
    var_t result;
    result.type = NONE;

    if (look == '(') {
        ev_match('(');
        result = ev_expression(result);
        ev_match(')');
    } else if (isalpha(look)) {
        // if <variable> (or function in future versions
        result = ev_extract_val_from_variable(look);
        ev_getchar();
    } else // <number>
    result = ev_getnum();

    return result;
}

static var_t ev_multiply(var_t lhs) {
    ev_match('*');

    return ev_operation_mul(lhs, ev_factor());
}

static var_t ev_divide(var_t lhs) {
    ev_match('/');

    return ev_operation_div(lhs, ev_factor());
}

// parse and evaluate a math expression
// <term> ::= <factor> [ * factor] [ / factor]
static var_t ev_term() {
    var_t result;

    result = ev_factor();
    while (look == '*' || look == '/') {
        switch (look) {
            case '*':
                result = ev_multiply(result);
                break;
            case '/':
                result = ev_divide(result);
                break;
        }
    }

    return result;
}

// helper function to get actual var index in global vars array
static int var_offset(char var) {
    char v = toupper(var); 
    assert(v >= 'A' && v <= 'Z' && "variable range not supported");
    return v - 'A';
} 

// save some value to global variable
static void ev_assign_val_to_variable(char var, var_t value) {
//    fprintf(stderr, "%c = %d\n", var, value.value.i); 
    g_variables[var_offset(var)] = value; 
}

// extract value from global variable
static var_t ev_extract_val_from_variable(char var) {
/*    g_variables[0].type = INT;
    g_variables[0].value.i = 100;

    g_variables[1].type = INT;
    g_variables[1].value.i = 200;
*/


//    fprintf(stderr, "got from %c\n", var);
    return g_variables[var_offset(var)]; 
}

/// process one expression
// expression -> [variable = ] <expression> 
static var_t ev_evaluate_one() {
    char current_variable = 0;
    var_t result;
    result.type = NONE;

    if (isalpha(look)) {
        current_variable = look;
        ev_match(look);
        ev_skipwhite();
        if (look == ASSIGNMENT) {
            // l-value
            ev_match(look);
            result = ev_expression(result);
            ev_assign_val_to_variable(current_variable, result);
            return result;
        } else {
            // get value of current_value and 
            // evaluate further
            result = ev_extract_val_from_variable(current_variable);
            
            return ev_expression(result);
        }
    } else return ev_expression(result);
}

// expression -> [+][-] term [- term] [+ term]
static var_t ev_expression(var_t start_value) {
    var_t result = start_value;

    if (result.type == NONE) {
        if (ev_isaddop(look)) {
            result.type = INT;
            result.value.i = 0;
        } else {
            result = ev_term();
        }
    }

    while (ev_isaddop(look)) {
        switch (look) {
            case '+':
                ev_match('+');
                result = ev_operation_add(result, ev_term());
                break;
            case '-':
                ev_match('-');
                result = ev_operation_sub(result, ev_term());
                break;
        }
    }

    return result;
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
    var_t_to_string(ev_evaluate_one(), output, bufsize);
    return 0;
}


