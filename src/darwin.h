#ifndef DARWIN_H
#define DARWIN_H

#include <strings.h>    // strncasecmp
#include <stdio.h>      // asprintf

int get_entropy_avail(unsigned int *);
int get_entropy_poolsize(unsigned int *);

#endif /*DARWIN_H*/
