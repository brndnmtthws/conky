#ifndef _TAIL_H
#define _TAIL_H

#include "text_object.h"

#define MAX_TAIL_LINES 100

int init_tail_object(struct text_object *, const char *);
int print_tail_object(struct text_object *, char *, size_t);

#endif /* _TAIL_H */
