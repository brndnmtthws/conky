#ifndef MIXER_H_
#define MIXER_H_


#ifdef MIXER_IS_ALSA
int mixer_to_255(int, int);
#else
#define mixer_to_255(l,x) x
#endif

int mixer_init(const char *);
int mixer_get_avg(int);
int mixer_get_left(int);
int mixer_get_right(int);
int mixer_is_mute(int);

#endif /*MIXER_H_*/
