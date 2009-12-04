/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*- */

#ifndef MIXER_H_
#define MIXER_H_

void parse_mixer_arg(struct text_object *, const char *);
uint8_t mixer_percentage(struct text_object *obj);
uint8_t mixerl_percentage(struct text_object *obj);
uint8_t mixerr_percentage(struct text_object *obj);
int check_mixer_muted(struct text_object *);

void scan_mixer_bar(struct text_object *, const char *);
double mixer_barval(struct text_object *);
double mixerl_barval(struct text_object *);
double mixerr_barval(struct text_object *);

#endif /*MIXER_H_*/
