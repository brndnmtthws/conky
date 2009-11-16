/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*- */

#ifndef MPD_H_
#define MPD_H_

/* functions for setting the configuration values */
void mpd_set_host(const char *);
void mpd_set_password(const char *, int);
void mpd_clear_password(void);
int mpd_set_port(const char *);

/* text object functions */
void init_mpd(void);
void free_mpd(void);
void update_mpd(void);

void print_mpd_elapsed(struct text_object *, char *, int);
void print_mpd_length(struct text_object *, char *, int);
void print_mpd_percent(struct text_object *, char *, int);
void print_mpd_bar(struct text_object *, char *, int);
void print_mpd_smart(struct text_object *, char *, int);
void print_mpd_title(struct text_object *, char *, int);
void print_mpd_artist(struct text_object *, char *, int);
void print_mpd_album(struct text_object *, char *, int);
void print_mpd_random(struct text_object *, char *, int);
void print_mpd_repeat(struct text_object *, char *, int);
void print_mpd_track(struct text_object *, char *, int);
void print_mpd_name(struct text_object *, char *, int);
void print_mpd_file(struct text_object *, char *, int);
void print_mpd_vol(struct text_object *, char *, int);
void print_mpd_bitrate(struct text_object *, char *, int);
void print_mpd_status(struct text_object *, char *, int);
int check_mpd_playing(struct text_object *);

#endif /*MPD_H_*/
