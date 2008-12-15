#ifndef _MAIL_H
#define _MAIL_H

#include "timed_thread.h"

extern char *current_mail_spool;

struct mail_s {			// for imap and pop3
	unsigned long unseen;
	unsigned long messages;
	unsigned long used;
	unsigned long quota;
	unsigned long port;
	unsigned int retries;
	float interval;
	double last_update;
	char host[128];
	char user[128];
	char pass[128];
	char command[1024];
	char folder[128];
	timed_thread *p_timed_thread;
	char secure;
};

struct local_mail_s {
	char *box;
	int mail_count;
	int new_mail_count;
	float interval;
	time_t last_mtime;
	double last_update;
};

void update_mail_count(struct local_mail_s *);

#define POP3_TYPE 1
#define IMAP_TYPE 2

struct mail_s *parse_mail_args(char type, const char *arg);
void *imap_thread(void *arg);
void *pop3_thread(void *arg);

#endif /* _MAIL_H */
