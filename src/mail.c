/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=c
 *
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2010 Brenden Matthews, Philip Kovacs, et. al.
 *	(see AUTHORS)
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "config.h"
#include "conky.h"
#include "common.h"
#include "logging.h"
#include "text_object.h"
#include "timed_thread.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/param.h>

#include <dirent.h>
#include <errno.h>
#include <termios.h>

/* MAX() is defined by a header included from conky.h
 * maybe once this is not true anymore, so have an alternative
 * waiting to drop in.
 *
 * #define MAX(a, b)  ((a > b) ? a : b)
 */

#define POP3_TYPE 1
#define IMAP_TYPE 2

#define MAXSIZE 1024

struct mail_s {			// for imap and pop3
	unsigned long unseen;
	unsigned long messages;
	unsigned long used;
	unsigned long quota;
	unsigned long port;
	unsigned int retries;
	float interval;
	double last_update;
	char host[MAXSIZE];
	char user[MAXSIZE];
	char pass[MAXSIZE];
	char command[MAXSIZE];
	char folder[MAXSIZE];
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

char *current_mail_spool;

static struct mail_s *global_mail;
static int global_mail_use = 0;

static void update_mail_count(struct local_mail_s *mail)
{
	struct stat st;

	if (mail == NULL) {
		return;
	}

	/* TODO: use that fine file modification notify on Linux 2.4 */

	/* don't check mail so often (9.5s is minimum interval) */
	if (current_update_time - mail->last_update < 9.5) {
		return;
	} else {
		mail->last_update = current_update_time;
	}

	if (stat(mail->mbox, &st)) {
		static int rep = 0;

		if (!rep) {
			NORM_ERR("can't stat %s: %s", mail->mbox, strerror(errno));
			rep = 1;
		}
		return;
	}
#if HAVE_DIRENT_H
	/* maildir format */
	if (S_ISDIR(st.st_mode)) {
		DIR *dir;
		char *dirname;
		struct dirent *dirent;
		char *mailflags;

		mail->mail_count = mail->new_mail_count = 0;
		mail->seen_mail_count = mail->unseen_mail_count = 0;
		mail->flagged_mail_count = mail->unflagged_mail_count = 0;
		mail->forwarded_mail_count = mail->unforwarded_mail_count = 0;
		mail->replied_mail_count = mail->unreplied_mail_count = 0;
		mail->draft_mail_count = mail->trashed_mail_count = 0;
		dirname = (char *) malloc(sizeof(char) * (strlen(mail->mbox) + 5));
		if (!dirname) {
			NORM_ERR("malloc");
			return;
		}
		strcpy(dirname, mail->mbox);
		strcat(dirname, "/");
		/* checking the cur subdirectory */
		strcat(dirname, "cur");

		dir = opendir(dirname);
		if (!dir) {
			NORM_ERR("cannot open directory");
			free(dirname);
			return;
		}
		dirent = readdir(dir);
		while (dirent) {
			/* . and .. are skipped */
			if (dirent->d_name[0] != '.') {
				mail->mail_count++;
				mailflags = (char *) malloc(sizeof(char) * strlen(strrchr(dirent->d_name, ',')));
				if (!mailflags) {
					NORM_ERR("malloc");
					free(dirname);
					return;
				}
				strcpy(mailflags, strrchr(dirent->d_name, ','));
				if (!strchr(mailflags, 'T')) { /* The message is not in the trash */
					if (strchr(mailflags, 'S')) { /*The message has been seen */
						mail->seen_mail_count++;
					} else {
						mail->unseen_mail_count++;
					}
					if (strchr(mailflags, 'F')) { /*The message was flagged */
						mail->flagged_mail_count++;
					} else {
						mail->unflagged_mail_count++;
					}
					if (strchr(mailflags, 'P')) { /*The message was forwarded */
						mail->forwarded_mail_count++;
					} else {
						mail->unforwarded_mail_count++;
					}
					if (strchr(mailflags, 'R')) { /*The message was replied */
						mail->replied_mail_count++;
					} else {
						mail->unreplied_mail_count++;
					}
					if (strchr(mailflags, 'D')) { /*The message is a draft */
						mail->draft_mail_count++;
					}
				} else {
					mail->trashed_mail_count++;
				}
				free(mailflags);
			}
			dirent = readdir(dir);
		}
		closedir(dir);

		dirname[strlen(dirname) - 3] = '\0';
		strcat(dirname, "new");

		dir = opendir(dirname);
		if (!dir) {
			NORM_ERR("cannot open directory");
			free(dirname);
			return;
		}
		dirent = readdir(dir);
		while (dirent) {
			/* . and .. are skipped */
			if (dirent->d_name[0] != '.') {
				mail->new_mail_count++;
				mail->mail_count++;
				mail->unseen_mail_count++;  /* new messages cannot have been seen */
			}
			dirent = readdir(dir);
		}
		closedir(dir);

		free(dirname);
		return;
	}
#endif
	/* mbox format */
	if (st.st_mtime != mail->last_mtime) {
		/* yippee, modification time has changed, let's read mail count! */
		static int rep;
		FILE *fp;
		int reading_status = 0;

		/* could lock here but I don't think it's really worth it because
		 * this isn't going to write mail spool */

		mail->new_mail_count = mail->mail_count = 0;

		/* these flags are not supported for mbox */
		mail->seen_mail_count = mail->unseen_mail_count = -1;
		mail->flagged_mail_count = mail->unflagged_mail_count = -1;
		mail->forwarded_mail_count = mail->unforwarded_mail_count = -1;
		mail->replied_mail_count = mail->unreplied_mail_count = -1;
		mail->draft_mail_count = mail->trashed_mail_count = -1;

		fp = open_file(mail->mbox, &rep);
		if (!fp) {
			return;
		}

		/* NOTE: adds mail as new if there isn't Status-field at all */

		while (!feof(fp)) {
			char buf[128];
			int was_new = 0;

			if (fgets(buf, 128, fp) == NULL) {
				break;
			}

			if (strncmp(buf, "From ", 5) == 0) {
				/* ignore MAILER-DAEMON */
				if (strncmp(buf + 5, "MAILER-DAEMON ", 14) != 0) {
					mail->mail_count++;
					was_new = 0;

					if (reading_status == 1) {
						mail->new_mail_count++;
					} else {
						reading_status = 1;
					}
				}
			} else {
				if (reading_status == 1
						&& strncmp(buf, "X-Mozilla-Status:", 17) == 0) {
					int xms = strtol(buf + 17, NULL, 16);
					/* check that mail isn't marked for deletion */
					if (xms & 0x0008) {
						mail->trashed_mail_count++;
						reading_status = 0;
						/* Don't check whether the trashed email is unread */
						continue;
					}
					/* check that mail isn't already read */
					if (!(xms & 0x0001)) {
						mail->new_mail_count++;
						was_new = 1;
					}

					/* check for an additional X-Status header */
					reading_status = 2;
					continue;
				}
				if (reading_status == 1 && strncmp(buf, "Status:", 7) == 0) {
					/* check that mail isn't already read */
					if (strchr(buf + 7, 'R') == NULL) {
						mail->new_mail_count++;
						was_new = 1;
					}

					reading_status = 2;
					continue;
				}
				if (reading_status >= 1 && strncmp(buf, "X-Status:", 9) == 0) {
					/* check that mail isn't marked for deletion */
					if (strchr(buf + 9, 'D') != NULL) {
						mail->trashed_mail_count++;
						/* If the mail was previously detected as new,
						   subtract it from the new mail count */
						if (was_new)
							mail->new_mail_count--;
					}

					reading_status = 0;
					continue;
				}
			}

			/* skip until \n */
			while (strchr(buf, '\n') == NULL && !feof(fp)) {
				fgets(buf, 128, fp);
			}
		}

		fclose(fp);

		if (reading_status) {
			mail->new_mail_count++;
		}

		mail->last_mtime = st.st_mtime;
	}
}

void parse_local_mail_args(struct text_object *obj, const char *arg)
{
	float n1;
	char mbox[256], dst[256];
	struct local_mail_s *locmail;

	if (!arg) {
		n1 = 9.5;
		/* Kapil: Changed from MAIL_FILE to
		   current_mail_spool since the latter
		   is a copy of the former if undefined
		   but the latter should take precedence
		   if defined */
		strncpy(mbox, current_mail_spool, sizeof(mbox));
	} else {
		if (sscanf(arg, "%s %f", mbox, &n1) != 2) {
			n1 = 9.5;
			strncpy(mbox, arg, sizeof(mbox));
		}
	}

	variable_substitute(mbox, dst, sizeof(dst));

	locmail = malloc(sizeof(struct local_mail_s));
	memset(locmail, 0, sizeof(struct local_mail_s));
	locmail->mbox = strndup(dst, text_buffer_size);
	locmail->interval = n1;
	obj->data.opaque = locmail;
}

#define PRINT_MAILS_GENERATOR(x) \
void print_##x##mails(struct text_object *obj, char *p, int p_max_size) \
{ \
	struct local_mail_s *locmail = obj->data.opaque; \
	if (!locmail) \
		return; \
	update_mail_count(locmail); \
	snprintf(p, p_max_size, "%d", locmail->x##mail_count); \
}

PRINT_MAILS_GENERATOR()
PRINT_MAILS_GENERATOR(new_)
PRINT_MAILS_GENERATOR(seen_)
PRINT_MAILS_GENERATOR(unseen_)
PRINT_MAILS_GENERATOR(flagged_)
PRINT_MAILS_GENERATOR(unflagged_)
PRINT_MAILS_GENERATOR(forwarded_)
PRINT_MAILS_GENERATOR(unforwarded_)
PRINT_MAILS_GENERATOR(replied_)
PRINT_MAILS_GENERATOR(unreplied_)
PRINT_MAILS_GENERATOR(draft_)
PRINT_MAILS_GENERATOR(trashed_)

void free_local_mails(struct text_object *obj)
{
	struct local_mail_s *locmail = obj->data.opaque;

	if (!locmail)
		return;

	if (locmail->mbox)
		free(locmail->mbox);
	free(obj->data.opaque);
	obj->data.opaque = 0;
}

#define MAXDATASIZE 1000

struct mail_s *parse_mail_args(char type, const char *arg)
{
	struct mail_s *mail;
	char *tmp;

	mail = malloc(sizeof(struct mail_s));
	memset(mail, 0, sizeof(struct mail_s));

#define lenstr "%1023s"
	if (sscanf(arg, lenstr " " lenstr " " lenstr, mail->host, mail->user, mail->pass)
			!= 3) {
		if (type == POP3_TYPE) {
			NORM_ERR("Scanning POP3 args failed");
		} else if (type == IMAP_TYPE) {
			NORM_ERR("Scanning IMAP args failed");
		}
		return 0;
	}
	// see if password needs prompting
	if (mail->pass[0] == '*' && mail->pass[1] == '\0') {
		int fp = fileno(stdin);
		struct termios term;

		tcgetattr(fp, &term);
		term.c_lflag &= ~ECHO;
		tcsetattr(fp, TCSANOW, &term);
		printf("Enter mailbox password (%s@%s): ", mail->user, mail->host);
		scanf(lenstr, mail->pass);
#undef lenstr
		printf("\n");
		term.c_lflag |= ECHO;
		tcsetattr(fp, TCSANOW, &term);
	}
	// now we check for optional args
	tmp = strstr(arg, "-r ");
	if (tmp) {
		tmp += 3;
		sscanf(tmp, "%u", &mail->retries);
	} else {
		mail->retries = 5;	// 5 retries after failure
	}
	tmp = strstr(arg, "-i ");
	if (tmp) {
		tmp += 3;
		sscanf(tmp, "%f", &mail->interval);
	} else {
		mail->interval = 300;	// 5 minutes
	}
	tmp = strstr(arg, "-p ");
	if (tmp) {
		tmp += 3;
		sscanf(tmp, "%lu", &mail->port);
	} else {
		if (type == POP3_TYPE) {
			mail->port = 110;	// default pop3 port
		} else if (type == IMAP_TYPE) {
			mail->port = 143;	// default imap port
		}
	}
	if (type == IMAP_TYPE) {
		tmp = strstr(arg, "-f ");
		if (tmp) {
			int len = MAXSIZE - 1;
			tmp += 3;
			if (tmp[0] == '\'') {
				len = strstr(tmp + 1, "'") - tmp;
				if (len > MAXSIZE) {
					len = MAXSIZE;
				}
			}
			strncpy(mail->folder, tmp + 1, len - 1);
		} else {
			strncpy(mail->folder, "INBOX", MAXSIZE - 1);	// default imap inbox
		}
	}
	tmp = strstr(arg, "-e ");
	if (tmp) {
		int len = MAXSIZE - 1;
		tmp += 3;

		if (tmp[0] == '\'') {
			len = strstr(tmp + 1, "'") - tmp;
			if (len > MAXSIZE) {
				len = MAXSIZE;
			}
		}
		strncpy(mail->command, tmp + 1, len - 1);
	} else {
		mail->command[0] = '\0';
	}
	DBGP("mail args parsed: folder: '%s' command: '%s' user: '%s' host: '%s'\n",
			mail->folder, mail->command, mail->user, mail->host);
	mail->p_timed_thread = NULL;
	return mail;
}

void parse_imap_mail_args(struct text_object *obj, const char *arg)
{
	static int rep = 0;

	if (!arg) {
		if (!global_mail && !rep) {
			// something is wrong, warn once then stop
			NORM_ERR("There's a problem with your mail settings.  "
					"Check that the global mail settings are properly defined"
					" (line %li).", obj->line);
			rep = 1;
			return;
		}
		obj->data.opaque = global_mail;
		global_mail_use++;
		return;
	}
	// proccss
	obj->data.opaque = parse_mail_args(IMAP_TYPE, arg);
}

void parse_pop3_mail_args(struct text_object *obj, const char *arg)
{
	static int rep = 0;

	if (!arg) {
		if (!global_mail && !rep) {
			// something is wrong, warn once then stop
			NORM_ERR("There's a problem with your mail settings.  "
					"Check that the global mail settings are properly defined"
					" (line %li).", obj->line);
			rep = 1;
			return;
		}
		obj->data.opaque = global_mail;
		global_mail_use++;
		return;
	}
	// proccss
	obj->data.opaque = parse_mail_args(POP3_TYPE, arg);
}

void parse_global_imap_mail_args(const char *value)
{
	global_mail = parse_mail_args(IMAP_TYPE, value);
}

void parse_global_pop3_mail_args(const char *value)
{
	global_mail = parse_mail_args(POP3_TYPE, value);
}

void free_mail_obj(struct text_object *obj)
{
	if (!obj->data.opaque)
		return;

	if (obj->data.opaque == global_mail) {
		if (--global_mail_use == 0) {
			free(global_mail);
			global_mail = 0;
		}
	} else {
		free(obj->data.opaque);
		obj->data.opaque = 0;
	}
}

int imap_command(int sockfd, const char *command, char *response, const char *verify)
{
	struct timeval fetchtimeout;
	fd_set fdset;
	int res, numbytes = 0;
	if (send(sockfd, command, strlen(command), 0) == -1) {
		perror("send");
		return -1;
	}
	fetchtimeout.tv_sec = 60;	// 60 second timeout i guess
	fetchtimeout.tv_usec = 0;
	FD_ZERO(&fdset);
	FD_SET(sockfd, &fdset);
	res = select(sockfd + 1, &fdset, NULL, NULL, &fetchtimeout);
	if (res > 0) {
		if ((numbytes = recv(sockfd, response, MAXDATASIZE - 1, 0)) == -1) {
			perror("recv");
			return -1;
		}
	}
	DBGP2("imap_command()  command: %s", command);
	DBGP2("imap_command() received: %s", response);
	response[numbytes] = '\0';
	if (strstr(response, verify) == NULL) {
		return -1;
	}
	return 0;
}

int imap_check_status(char *recvbuf, struct mail_s *mail)
{
	char *reply;
	reply = strstr(recvbuf, " (MESSAGES ");
	if (!reply || strlen(reply) < 2) {
		return -1;
	}
	reply += 2;
	*strchr(reply, ')') = '\0';
	if (reply == NULL) {
		NORM_ERR("Error parsing IMAP response: %s", recvbuf);
		return -1;
	} else {
		timed_thread_lock(mail->p_timed_thread);
		sscanf(reply, "MESSAGES %lu UNSEEN %lu", &mail->messages,
				&mail->unseen);
		timed_thread_unlock(mail->p_timed_thread);
	}
	return 0;
}

void imap_unseen_command(struct mail_s *mail, unsigned long old_unseen, unsigned long old_messages)
{
	if (strlen(mail->command) > 1 && (mail->unseen > old_unseen
				|| (mail->messages > old_messages && mail->unseen > 0))) {
		// new mail goodie
		if (system(mail->command) == -1) {
			perror("system()");
		}
	}
}

static void ensure_mail_thread(struct mail_s *mail,
		void *thread(void *), const char *text)
{
	if (mail->p_timed_thread)
		return;

	mail->p_timed_thread = timed_thread_create(thread,
				mail, mail->interval * 1000000);
	if (!mail->p_timed_thread) {
		NORM_ERR("Error creating %s timed thread", text);
	}
	timed_thread_register(mail->p_timed_thread,
			&mail->p_timed_thread);
	if (timed_thread_run(mail->p_timed_thread)) {
		NORM_ERR("Error running %s timed thread", text);
	}
}

static void *imap_thread(void *arg)
{
	int sockfd, numbytes;
	char recvbuf[MAXDATASIZE];
	char sendbuf[MAXDATASIZE];
	unsigned int fail = 0;
	unsigned long old_unseen = ULONG_MAX;
	unsigned long old_messages = ULONG_MAX;
	struct stat stat_buf;
	struct mail_s *mail = (struct mail_s *)arg;
	int has_idle = 0;
	int threadfd = timed_thread_readfd(mail->p_timed_thread);
	char resolved_host = 0;
	struct addrinfo hints;
	struct addrinfo *ai = 0, *rp;
	char portbuf[8];

	while (fail < mail->retries) {
		struct timeval fetchtimeout;
		int res;
		fd_set fdset;

		if (fail > 0) {
			NORM_ERR("Trying IMAP connection again for %s@%s (try %u/%u)",
					mail->user, mail->host, fail + 1, mail->retries);
			resolved_host = 0; /* force us to resolve the hostname again */
			sleep(fail); /* sleep more for the more failures we have */
		}
		if (!resolved_host) {
			memset(&hints, 0, sizeof(struct addrinfo));
			hints.ai_family = AF_UNSPEC;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_flags = 0;
			hints.ai_protocol = 0;
			snprintf(portbuf, 8, "%lu", mail->port);

			res = getaddrinfo(mail->host, portbuf, &hints, &ai);
			if (res != 0) {
				NORM_ERR("IMAP getaddrinfo: %s", gai_strerror(res));
				fail++;
				break;
			}
			resolved_host = 1;
		}
		do {
			for (rp = ai; rp != NULL; rp = rp->ai_next) {
				sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
				if (sockfd == -1) {
					continue;
				}
				if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) {
					break;
				}
				close(sockfd);
			}
			freeaddrinfo(ai);
			ai = 0;
			if (rp == NULL) {
				perror("connect");
				fail++;
				break;
			}

			fetchtimeout.tv_sec = 60;	// 60 second timeout i guess
			fetchtimeout.tv_usec = 0;
			FD_ZERO(&fdset);
			FD_SET(sockfd, &fdset);
			res = select(sockfd + 1, &fdset, NULL, NULL, &fetchtimeout);
			if (res > 0) {
				if ((numbytes = recv(sockfd, recvbuf, MAXDATASIZE - 1, 0)) == -1) {
					perror("recv");
					fail++;
					break;
				}
			} else {
				NORM_ERR("IMAP connection failed: timeout");
				fail++;
				break;
			}
			recvbuf[numbytes] = '\0';
			DBGP2("imap_thread() received: %s", recvbuf);
			if (strstr(recvbuf, "* OK") != recvbuf) {
				NORM_ERR("IMAP connection failed, probably not an IMAP server");
				fail++;
				break;
			}
			strncpy(sendbuf, "abc CAPABILITY\r\n", MAXDATASIZE);
			if (imap_command(sockfd, sendbuf, recvbuf, "abc OK")) {
				fail++;
				break;
			}
			if (strstr(recvbuf, " IDLE ") != NULL) {
				has_idle = 1;
			}

			strncpy(sendbuf, "a1 login ", MAXDATASIZE);
			strncat(sendbuf, mail->user, MAXDATASIZE - strlen(sendbuf) - 1);
			strncat(sendbuf, " ", MAXDATASIZE - strlen(sendbuf) - 1);
			strncat(sendbuf, mail->pass, MAXDATASIZE - strlen(sendbuf) - 1);
			strncat(sendbuf, "\r\n", MAXDATASIZE - strlen(sendbuf) - 1);
			if (imap_command(sockfd, sendbuf, recvbuf, "a1 OK")) {
				fail++;
				break;
			}

			strncpy(sendbuf, "a2 STATUS \"", MAXDATASIZE);
			strncat(sendbuf, mail->folder, MAXDATASIZE - strlen(sendbuf) - 1);
			strncat(sendbuf, "\" (MESSAGES UNSEEN)\r\n",
					MAXDATASIZE - strlen(sendbuf) - 1);
			if (imap_command(sockfd, sendbuf, recvbuf, "a2 OK")) {
				fail++;
				break;
			}

			if (imap_check_status(recvbuf, mail)) {
				fail++;
				break;
			}
			imap_unseen_command(mail, old_unseen, old_messages);
			fail = 0;
			old_unseen = mail->unseen;
			old_messages = mail->messages;

			if (has_idle) {
				strncpy(sendbuf, "a4 SELECT \"", MAXDATASIZE);
				strncat(sendbuf, mail->folder, MAXDATASIZE - strlen(sendbuf) - 1);
				strncat(sendbuf, "\"\r\n", MAXDATASIZE - strlen(sendbuf) - 1);
				if (imap_command(sockfd, sendbuf, recvbuf, "a4 OK")) {
					fail++;
					break;
				}

				strncpy(sendbuf, "a5 IDLE\r\n", MAXDATASIZE);
				if (imap_command(sockfd, sendbuf, recvbuf, "+ idling")) {
					fail++;
					break;
				}
				recvbuf[0] = '\0';

				while (1) {
					/*
					 * RFC 2177 says we have to re-idle every 29 minutes.
					 * We'll do it every 10 minutes to be safe.
					 */
					fetchtimeout.tv_sec = 600;
					fetchtimeout.tv_usec = 0;
					DBGP("idling...");
					FD_ZERO(&fdset);
					FD_SET(sockfd, &fdset);
					FD_SET(threadfd, &fdset);
					res = select(MAX(sockfd + 1, threadfd + 1), &fdset, NULL,
							NULL, &fetchtimeout);
					DBGP("done idling");
					if (timed_thread_test(mail->p_timed_thread, 1) || (res == -1 && errno == EINTR) || FD_ISSET(threadfd, &fdset)) {
						if ((fstat(sockfd, &stat_buf) == 0) && S_ISSOCK(stat_buf.st_mode)) {
							/* if a valid socket, close it */
							close(sockfd);
						}
						timed_thread_exit(mail->p_timed_thread);
					} else if (res > 0) {
						if ((numbytes = recv(sockfd, recvbuf, MAXDATASIZE - 1, 0)) == -1) {
							perror("recv idling");
							fail++;
							break;
						}
					}
					recvbuf[numbytes] = '\0';
					DBGP2("imap_thread() received: %s", recvbuf);
					if (strlen(recvbuf) > 2) {
						unsigned long messages, recent = 0;
						char *buf = recvbuf;
						char force_check = 0;
						buf = strstr(buf, "EXISTS");
						while (buf && strlen(buf) > 1 && strstr(buf + 1, "EXISTS")) {
							buf = strstr(buf + 1, "EXISTS");
						}
						if (buf) {
							// back up until we reach '*'
							while (buf >= recvbuf && buf[0] != '*') {
								buf--;
							}
							if (sscanf(buf, "* %lu EXISTS\r\n", &messages) == 1) {
								timed_thread_lock(mail->p_timed_thread);
								if (mail->messages != messages) {
									force_check = 1;
								}
								timed_thread_unlock(mail->p_timed_thread);
							}
						}
						buf = recvbuf;
						buf = strstr(buf, "RECENT");
						while (buf && strlen(buf) > 1 && strstr(buf + 1, "RECENT")) {
							buf = strstr(buf + 1, "RECENT");
						}
						if (buf) {
							// back up until we reach '*'
							while (buf >= recvbuf && buf[0] != '*') {
								buf--;
							}
							if (sscanf(buf, "* %lu RECENT\r\n", &recent) != 1) {
								recent = 0;
							}
						}
						/*
						 * check if we got a FETCH from server, recent was
						 * something other than 0, or we had a timeout
						 */
						buf = recvbuf;
						if (recent > 0 || (buf && strstr(buf, " FETCH ")) || fetchtimeout.tv_sec == 0 || force_check) {
							// re-check messages and unseen
							if (imap_command(sockfd, "DONE\r\n", recvbuf, "a5 OK")) {
								fail++;
								break;
							}
							strncpy(sendbuf, "a2 STATUS \"", MAXDATASIZE);
							strncat(sendbuf, mail->folder, MAXDATASIZE - strlen(sendbuf) - 1);
							strncat(sendbuf, "\" (MESSAGES UNSEEN)\r\n",
									MAXDATASIZE - strlen(sendbuf) - 1);
							if (imap_command(sockfd, sendbuf, recvbuf, "a2 OK")) {
								fail++;
								break;
							}
							if (imap_check_status(recvbuf, mail)) {
								fail++;
								break;
							}
							strncpy(sendbuf, "a5 IDLE\r\n", MAXDATASIZE);
							if (imap_command(sockfd, sendbuf, recvbuf, "+ idling")) {
								fail++;
								break;
							}
						}
						/*
						 * check if we got a BYE from server
						 */
						buf = recvbuf;
						if (buf && strstr(buf, "* BYE")) {
							// need to re-connect
							break;
						}
					} else {
						fail++;
						break;
					}
					imap_unseen_command(mail, old_unseen, old_messages);
					fail = 0;
					old_unseen = mail->unseen;
					old_messages = mail->messages;
				}
				if (fail) break;
			} else {
				strncpy(sendbuf, "a3 logout\r\n", MAXDATASIZE);
				if (send(sockfd, sendbuf, strlen(sendbuf), 0) == -1) {
					perror("send a3");
					fail++;
					break;
				}
				fetchtimeout.tv_sec = 60;	// 60 second timeout i guess
				fetchtimeout.tv_usec = 0;
				FD_ZERO(&fdset);
				FD_SET(sockfd, &fdset);
				res = select(sockfd + 1, &fdset, NULL, NULL, &fetchtimeout);
				if (res > 0) {
					if ((numbytes = recv(sockfd, recvbuf, MAXDATASIZE - 1, 0)) == -1) {
						perror("recv a3");
						fail++;
						break;
					}
				}
				recvbuf[numbytes] = '\0';
				DBGP2("imap_thread() received: %s", recvbuf);
				if (strstr(recvbuf, "a3 OK") == NULL) {
					NORM_ERR("IMAP logout failed: %s", recvbuf);
					fail++;
					break;
				}
			}
		} while (0);
		if ((fstat(sockfd, &stat_buf) == 0) && S_ISSOCK(stat_buf.st_mode)) {
			/* if a valid socket, close it */
			close(sockfd);
		}
		if (timed_thread_test(mail->p_timed_thread, 0)) {
			timed_thread_exit(mail->p_timed_thread);
		}
	}
	mail->unseen = 0;
	mail->messages = 0;
	return 0;
}

void print_imap_unseen(struct text_object *obj, char *p, int p_max_size)
{
	struct mail_s *mail = obj->data.opaque;

	if (!mail)
		return;

	ensure_mail_thread(mail, imap_thread, "imap");

	if (mail && mail->p_timed_thread) {
		timed_thread_lock(mail->p_timed_thread);
		snprintf(p, p_max_size, "%lu", mail->unseen);
		timed_thread_unlock(mail->p_timed_thread);
	}
}

void print_imap_messages(struct text_object *obj, char *p, int p_max_size)
{
	struct mail_s *mail = obj->data.opaque;

	if (!mail)
		return;

	ensure_mail_thread(mail, imap_thread, "imap");

	if (mail && mail->p_timed_thread) {
		timed_thread_lock(mail->p_timed_thread);
		snprintf(p, p_max_size, "%lu", mail->messages);
		timed_thread_unlock(mail->p_timed_thread);
	}
}

int pop3_command(int sockfd, const char *command, char *response, const char *verify)
{
	struct timeval fetchtimeout;
	fd_set fdset;
	int res, numbytes = 0;
	if (send(sockfd, command, strlen(command), 0) == -1) {
		perror("send");
		return -1;
	}
	fetchtimeout.tv_sec = 60;	// 60 second timeout i guess
	fetchtimeout.tv_usec = 0;
	FD_ZERO(&fdset);
	FD_SET(sockfd, &fdset);
	res = select(sockfd + 1, &fdset, NULL, NULL, &fetchtimeout);
	if (res > 0) {
		if ((numbytes = recv(sockfd, response, MAXDATASIZE - 1, 0)) == -1) {
			perror("recv");
			return -1;
		}
	}
	DBGP2("pop3_command() received: %s", response);
	response[numbytes] = '\0';
	if (strstr(response, verify) == NULL) {
		return -1;
	}
	return 0;
}

static void *pop3_thread(void *arg)
{
	int sockfd, numbytes;
	char recvbuf[MAXDATASIZE];
	char sendbuf[MAXDATASIZE];
	char *reply;
	unsigned int fail = 0;
	unsigned long old_unseen = ULONG_MAX;
	struct stat stat_buf;
	struct mail_s *mail = (struct mail_s *)arg;
	char resolved_host = 0;
	struct addrinfo hints;
	struct addrinfo *ai = 0, *rp;
	char portbuf[8];

	while (fail < mail->retries) {
		struct timeval fetchtimeout;
		int res;
		fd_set fdset;

		if (fail > 0) {
			NORM_ERR("Trying POP3 connection again for %s@%s (try %u/%u)",
					mail->user, mail->host, fail + 1, mail->retries);
			resolved_host = 0; /* force us to resolve the hostname again */
			sleep(fail); /* sleep more for the more failures we have */
		}
		if (!resolved_host) {
			memset(&hints, 0, sizeof(struct addrinfo));
			hints.ai_family = AF_UNSPEC;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_flags = 0;
			hints.ai_protocol = 0;
			snprintf(portbuf, 8, "%lu", mail->port);

			res = getaddrinfo(mail->host, portbuf, &hints, &ai);
			if (res != 0) {
				NORM_ERR("POP3 getaddrinfo: %s", gai_strerror(res));
				fail++;
				break;
			}
			resolved_host = 1;
		}
		do {
			for (rp = ai; rp != NULL; rp = rp->ai_next) {
				sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
				if (sockfd == -1) {
					continue;
				}
				if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) {
					break;
				}
				close(sockfd);
			}
			freeaddrinfo(ai);
			ai = 0;
			if (rp == NULL) {
				perror("connect");
				fail++;
				break;
			}

			fetchtimeout.tv_sec = 60;	// 60 second timeout i guess
			fetchtimeout.tv_usec = 0;
			FD_ZERO(&fdset);
			FD_SET(sockfd, &fdset);
			res = select(sockfd + 1, &fdset, NULL, NULL, &fetchtimeout);
			if (res > 0) {
				if ((numbytes = recv(sockfd, recvbuf, MAXDATASIZE - 1, 0)) == -1) {
					perror("recv");
					fail++;
					break;
				}
			} else {
				NORM_ERR("POP3 connection failed: timeout\n");
				fail++;
				break;
			}
			DBGP2("pop3_thread received: %s", recvbuf);
			recvbuf[numbytes] = '\0';
			if (strstr(recvbuf, "+OK ") != recvbuf) {
				NORM_ERR("POP3 connection failed, probably not a POP3 server");
				fail++;
				break;
			}
			strncpy(sendbuf, "USER ", MAXDATASIZE);
			strncat(sendbuf, mail->user, MAXDATASIZE - strlen(sendbuf) - 1);
			strncat(sendbuf, "\r\n", MAXDATASIZE - strlen(sendbuf) - 1);
			if (pop3_command(sockfd, sendbuf, recvbuf, "+OK ")) {
				fail++;
				break;
			}

			strncpy(sendbuf, "PASS ", MAXDATASIZE);
			strncat(sendbuf, mail->pass, MAXDATASIZE - strlen(sendbuf) - 1);
			strncat(sendbuf, "\r\n", MAXDATASIZE - strlen(sendbuf) - 1);
			if (pop3_command(sockfd, sendbuf, recvbuf, "+OK ")) {
				NORM_ERR("POP3 server login failed: %s", recvbuf);
				fail++;
				break;
			}

			strncpy(sendbuf, "STAT\r\n", MAXDATASIZE);
			if (pop3_command(sockfd, sendbuf, recvbuf, "+OK ")) {
				perror("send STAT");
				fail++;
				break;
			}

			// now we get the data
			reply = recvbuf + 4;
			if (reply == NULL) {
				NORM_ERR("Error parsing POP3 response: %s", recvbuf);
				fail++;
				break;
			} else {
				timed_thread_lock(mail->p_timed_thread);
				sscanf(reply, "%lu %lu", &mail->unseen, &mail->used);
				timed_thread_unlock(mail->p_timed_thread);
			}

			strncpy(sendbuf, "QUIT\r\n", MAXDATASIZE);
			if (pop3_command(sockfd, sendbuf, recvbuf, "+OK")) {
				NORM_ERR("POP3 logout failed: %s", recvbuf);
				fail++;
				break;
			}

			if (strlen(mail->command) > 1 && mail->unseen > old_unseen) {
				// new mail goodie
				if (system(mail->command) == -1) {
					perror("system()");
				}
			}
			fail = 0;
			old_unseen = mail->unseen;
		} while (0);
		if ((fstat(sockfd, &stat_buf) == 0) && S_ISSOCK(stat_buf.st_mode)) {
			/* if a valid socket, close it */
			close(sockfd);
		}
		if (timed_thread_test(mail->p_timed_thread, 0)) {
			timed_thread_exit(mail->p_timed_thread);
		}
	}
	mail->unseen = 0;
	mail->used = 0;
	return 0;
}

void print_pop3_unseen(struct text_object *obj, char *p, int p_max_size)
{
	struct mail_s *mail = obj->data.opaque;

	if (!mail)
		return;

	ensure_mail_thread(mail, pop3_thread, "pop3");

	if (mail && mail->p_timed_thread) {
		timed_thread_lock(mail->p_timed_thread);
		snprintf(p, p_max_size, "%lu", mail->unseen);
		timed_thread_unlock(mail->p_timed_thread);
	}
}

void print_pop3_used(struct text_object *obj, char *p, int p_max_size)
{
	struct mail_s *mail = obj->data.opaque;

	if (!mail)
		return;

	ensure_mail_thread(mail, pop3_thread, "pop3");

	if (mail && mail->p_timed_thread) {
		timed_thread_lock(mail->p_timed_thread);
		snprintf(p, p_max_size, "%.1f",
				mail->used / 1024.0 / 1024.0);
		timed_thread_unlock(mail->p_timed_thread);
	}
}
