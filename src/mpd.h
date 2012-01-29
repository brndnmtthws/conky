/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*- */

#ifndef MPD_H_
#define MPD_H_

struct mpd_s {
	char *title;
	char *artist;
	char *album;
	char *date;
	const char *status;
	const char *random;
	const char *repeat;
	char *track;
	char *name;
	char *file;
	int is_playing;
	int vol;
	float progress;
	int bitrate;
	int length;
	int elapsed;
};

/* functions for setting the configuration values */
void mpd_set_host(const char *);
void mpd_set_password(const char *, int);
void mpd_clear_password(void);
int mpd_set_port(const char *);

/* text object functions */
void init_mpd(void);
struct mpd_s *mpd_get_info(void);
void free_mpd(void);
int update_mpd(void);

void print_mpd_elapsed(struct text_object *, char *, int);
void print_mpd_length(struct text_object *, char *, int);
void print_mpd_percent(struct text_object *, char *, int);
void print_mpd_bar(struct text_object *, char *, int);
void print_mpd_smart(struct text_object *, char *, int);
void print_mpd_title(struct text_object *, char *, int);
void print_mpd_artist(struct text_object *, char *, int);
void print_mpd_album(struct text_object *, char *, int);
void print_mpd_date(struct text_object *, char *, int);
void print_mpd_random(struct text_object *, char *, int);
void print_mpd_repeat(struct text_object *, char *, int);
void print_mpd_track(struct text_object *, char *, int);
void print_mpd_name(struct text_object *, char *, int);
void print_mpd_file(struct text_object *, char *, int);
void print_mpd_vol(struct text_object *, char *, int);
void print_mpd_bitrate(struct text_object *, char *, int);
void print_mpd_status(struct text_object *, char *, int);

#endif /*MPD_H_*/
