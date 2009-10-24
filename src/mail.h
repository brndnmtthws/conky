/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*- */

#ifndef _MAIL_H
#define _MAIL_H

#include "timed_thread.h"
#include <time.h>

/* forward declare to make gcc happy */
struct text_object;

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
	char *mbox;
	int mail_count;
	int new_mail_count;
	int seen_mail_count;
	int unseen_mail_count;
	int flagged_mail_count;
	int unflagged_mail_count;
	int forwarded_mail_count;
	int unforwarded_mail_count;
	int replied_mail_count;
	int unreplied_mail_count;
	int draft_mail_count;
	int trashed_mail_count;
	float interval;
	time_t last_mtime;
	double last_update;
};

void update_mail_count(struct local_mail_s *);

#define POP3_TYPE 1
#define IMAP_TYPE 2

void parse_local_mail_args(struct text_object *, const char *);

#define PRINT_MAILS_PROTO_GENERATOR(x) \
void print_##x##mails(struct text_object *, char *, int);

PRINT_MAILS_PROTO_GENERATOR()
PRINT_MAILS_PROTO_GENERATOR(new_)
PRINT_MAILS_PROTO_GENERATOR(seen_)
PRINT_MAILS_PROTO_GENERATOR(unseen_)
PRINT_MAILS_PROTO_GENERATOR(flagged_)
PRINT_MAILS_PROTO_GENERATOR(unflagged_)
PRINT_MAILS_PROTO_GENERATOR(forwarded_)
PRINT_MAILS_PROTO_GENERATOR(unforwarded_)
PRINT_MAILS_PROTO_GENERATOR(replied_)
PRINT_MAILS_PROTO_GENERATOR(unreplied_)
PRINT_MAILS_PROTO_GENERATOR(draft_)
PRINT_MAILS_PROTO_GENERATOR(trashed_)

/* FIXME: this is here for the config leftovers only */
struct mail_s *parse_mail_args(char, const char *);

void parse_imap_mail_args(struct text_object *, const char *);
void parse_pop3_mail_args(struct text_object *, const char *);
void free_mail_obj(struct text_object *);
void print_imap_unseen(struct text_object *, char *, int);
void print_imap_messages(struct text_object *, char *, int);
void print_pop3_unseen(struct text_object *, char *, int);
void print_pop3_used(struct text_object *, char *, int);

#endif /* _MAIL_H */
