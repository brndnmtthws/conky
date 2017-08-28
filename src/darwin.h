#ifndef DARWIN_H
#define DARWIN_H

#include <sys/param.h>
#include <sys/mount.h>
#include <strings.h>
#include <stdio.h>

int get_entropy_avail(unsigned int *);
int get_entropy_poolsize(unsigned int *);

#endif /*DARWIN_H*/
