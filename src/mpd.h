#ifndef MPD_H_
#define MPD_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "libmpdclient.h"
#include "timed_thread.h"

struct mpd_s {
	char *title;
	char *artist;
	char *album;
	char *status;
	char *random;
	char *repeat;
	char *track;
	char *name;
	char *file;
	int volume;
	unsigned int port;
	char host[128];
	char password[128];
	float progress;
	int bitrate;
	int length;
	int elapsed;
	mpd_Connection *conn;
	timed_thread *timed_thread;
};

#include "conky.h"

extern void init_mpd_stats(struct mpd_s *mpd);
void clear_mpd_stats(struct mpd_s *mpd);
void *update_mpd(void *) __attribute__((noreturn));
void free_mpd_vars(struct mpd_s *mpd);

#endif /*MPD_H_*/
