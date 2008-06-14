#ifndef BMPX_H_
#define BMPX_H_

void update_bmpx(void);
struct bmpx_s {
	char *title;
	char *artist;
	char *album;
	char *uri;
	int bitrate;
	int track;
};

#endif /*BMPX_H_*/
