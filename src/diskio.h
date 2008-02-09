#ifndef DISKIO_H_
#define DISKIO_H_

struct diskio_stat {
	char *dev;
	unsigned int current, current_read, current_write, last, last_read,
		last_write;
};

#define MAX_DISKIO_STATS 64

struct diskio_stat *diskio_stats;

struct diskio_stat *prepare_diskio_stat(const char *s);
void clear_diskio_stats();

#endif /* DISKIO_H_ */
