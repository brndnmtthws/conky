/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*- */

#ifndef MPD_H_
#define MPD_H_

//#include "conky.h"

struct mpd_s {
	char *title;
	char *artist;
	char *album;
	const char *status;
	const char *random;
	const char *repeat;
	char *track;
	char *name;
	char *file;
	int is_playing;
	int volume;
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
void update_mpd(void);

#endif /*MPD_H_*/
