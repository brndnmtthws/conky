/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*- */

#ifndef _MAIL_H
#define _MAIL_H

extern char *current_mail_spool;

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

void free_local_mails(struct text_object *obj);

void parse_global_imap_mail_args(const char *);
void parse_global_pop3_mail_args(const char *);

void parse_imap_mail_args(struct text_object *, const char *);
void parse_pop3_mail_args(struct text_object *, const char *);
void free_mail_obj(struct text_object *);
void print_imap_unseen(struct text_object *, char *, int);
void print_imap_messages(struct text_object *, char *, int);
void print_pop3_unseen(struct text_object *, char *, int);
void print_pop3_used(struct text_object *, char *, int);

#endif /* _MAIL_H */
