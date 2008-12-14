#ifndef _MAIL_H_
#define _MAIL_H_

extern char *current_mail_spool;

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

#endif /* _MAIL_H_ */
