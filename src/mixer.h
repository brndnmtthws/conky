#ifndef MIXER_H_
#define MIXER_H_

int mixer_init(const char *);
int mixer_get_avg(int);
int mixer_get_left(int);
int mixer_get_right(int);

#endif /*MIXER_H_*/
