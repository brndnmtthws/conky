/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*- */

#ifndef MIXER_H_
#define MIXER_H_

void parse_mixer_arg(struct text_object *, const char *);
void print_mixer(struct text_object *, char *, int);
void print_mixerl(struct text_object *, char *, int);
void print_mixerr(struct text_object *, char *, int);
int check_mixer_muted(struct text_object *);

#ifdef X11
void scan_mixer_bar(struct text_object *, const char *);
void print_mixer_bar(struct text_object *, char *, int);
void print_mixerl_bar(struct text_object *, char *, int);
void print_mixerr_bar(struct text_object *, char *, int);
#endif /* X11 */

#endif /*MIXER_H_*/
