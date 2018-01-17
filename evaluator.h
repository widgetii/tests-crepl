#ifndef _EVALUATOR_H_
#define _EVALUATOR_H_

int evaluate_expression(const char* expr, char* outbuf, size_t buflen);
size_t ev_get_last_error(char* buf, size_t buflen);

#endif

