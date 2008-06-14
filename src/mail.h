#ifndef MAIL_H_
#define MAIL_H_

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

#endif /*MAIL_H_*/
