#ifndef _HEAD_H
#define _HEAD_H

#include "text_object.h"

int init_head_object(struct text_object *, const char *);
int print_head_object(struct text_object *, char *, size_t);

#endif /* _HEAD_H */
