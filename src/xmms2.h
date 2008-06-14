#ifndef XMMS2_H_
#define XMMS2_H_

struct xmms2_s {
	char *artist;
	char *album;
	char *title;
	char *genre;
	char *comment;
	char *url;
	char *date;
	char* playlist;
	int tracknr;
	int bitrate;
	unsigned int id;
	int duration;
	int elapsed;
	int timesplayed;
	float size;

	float progress;
	char *status;
};

void update_xmms2(void);

#endif /*XMMS2_H_*/
