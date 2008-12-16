/* Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2008 Brenden Matthews, Philip Kovacs, et. al.
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
#include "mail.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <dirent.h>
#include <errno.h>
#include <termios.h>

/* MAX() is defined by a header included from conky.h
 * maybe once this is not true anymore, so have an alternative
 * waiting to drop in.
 *
 * #define MAX(a, b)  ((a > b) ? a : b)
 */

char *current_mail_spool;

void update_mail_count(struct local_mail_s *mail)
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

	if (stat(mail->box, &st)) {
		static int rep;

		if (!rep) {
			ERR("can't stat %s: %s", mail->box, strerror(errno));
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

		mail->mail_count = mail->new_mail_count = 0;
		dirname = (char *) malloc(sizeof(char) * (strlen(mail->box) + 5));
		if (!dirname) {
			ERR("malloc");
			return;
		}
		strcpy(dirname, mail->box);
		strcat(dirname, "/");
		/* checking the cur subdirectory */
		strcat(dirname, "cur");

		dir = opendir(dirname);
		if (!dir) {
			ERR("cannot open directory");
			free(dirname);
			return;
		}
		dirent = readdir(dir);
		while (dirent) {
			/* . and .. are skipped */
			if (dirent->d_name[0] != '.') {
				mail->mail_count++;
			}
			dirent = readdir(dir);
		}
		closedir(dir);

		dirname[strlen(dirname) - 3] = '\0';
		strcat(dirname, "new");

		dir = opendir(dirname);
		if (!dir) {
			ERR("cannot open directory");
			free(dirname);
			return;
		}
		dirent = readdir(dir);
		while (dirent) {
			/* . and .. are skipped */
			if (dirent->d_name[0] != '.') {
				mail->new_mail_count++;
				mail->mail_count++;
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

		fp = open_file(mail->box, &rep);
		if (!fp) {
			return;
		}

		/* NOTE: adds mail as new if there isn't Status-field at all */

		while (!feof(fp)) {
			char buf[128];

			if (fgets(buf, 128, fp) == NULL) {
				break;
			}

			if (strncmp(buf, "From ", 5) == 0) {
				/* ignore MAILER-DAEMON */
				if (strncmp(buf + 5, "MAILER-DAEMON ", 14) != 0) {
					mail->mail_count++;

					if (reading_status) {
						mail->new_mail_count++;
					} else {
						reading_status = 1;
					}
				}
			} else {
				if (reading_status
						&& strncmp(buf, "X-Mozilla-Status:", 17) == 0) {
					/* check that mail isn't already read */
					if (strchr(buf + 21, '0')) {
						mail->new_mail_count++;
					}

					reading_status = 0;
					continue;
				}
				if (reading_status && strncmp(buf, "Status:", 7) == 0) {
					/* check that mail isn't already read */
					if (strchr(buf + 7, 'R') == NULL) {
						mail->new_mail_count++;
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

#define MAXDATASIZE 1000

struct mail_s *parse_mail_args(char type, const char *arg)
{
	struct mail_s *mail;
	char *tmp;

	mail = malloc(sizeof(struct mail_s));
	memset(mail, 0, sizeof(struct mail_s));

	if (sscanf(arg, "%128s %128s %128s", mail->host, mail->user, mail->pass)
			!= 3) {
		if (type == POP3_TYPE) {
			ERR("Scanning IMAP args failed");
		} else if (type == IMAP_TYPE) {
			ERR("Scanning POP3 args failed");
		}
	}
	// see if password needs prompting
	if (mail->pass[0] == '*' && mail->pass[1] == '\0') {
		int fp = fileno(stdin);
		struct termios term;

		tcgetattr(fp, &term);
		term.c_lflag &= ~ECHO;
		tcsetattr(fp, TCSANOW, &term);
		printf("Enter mailbox password (%s@%s): ", mail->user, mail->host);
		scanf("%128s", mail->pass);
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
			tmp += 3;
			sscanf(tmp, "%s", mail->folder);
		} else {
			strncpy(mail->folder, "INBOX", 128);	// default imap inbox
		}
	}
	tmp = strstr(arg, "-e ");
	if (tmp) {
		int len = 1024;
		tmp += 3;

		if (tmp[0] == '\'') {
			len = strstr(tmp + 1, "'") - tmp - 1;
			if (len > 1024) {
				len = 1024;
			}
		}
		strncpy(mail->command, tmp + 1, len);
	} else {
		mail->command[0] = '\0';
	}
	mail->p_timed_thread = NULL;
	return mail;
}

int imap_command(int sockfd, const char *command, char *response, const char *verify)
{
	struct timeval timeout;
	fd_set fdset;
	int res, numbytes = 0;
	if (send(sockfd, command, strlen(command), 0) == -1) {
		perror("send");
		return -1;
	}
	timeout.tv_sec = 60;	// 60 second timeout i guess
	timeout.tv_usec = 0;
	FD_ZERO(&fdset);
	FD_SET(sockfd, &fdset);
	res = select(sockfd + 1, &fdset, NULL, NULL, &timeout);
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
		ERR("Error parsing IMAP response: %s", recvbuf);
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

void *imap_thread(void *arg)
{
	int sockfd, numbytes;
	char recvbuf[MAXDATASIZE];
	char sendbuf[MAXDATASIZE];
	unsigned int fail = 0;
	unsigned long old_unseen = ULONG_MAX;
	unsigned long old_messages = ULONG_MAX;
	struct stat stat_buf;
	struct hostent he, *he_res = 0;
	int he_errno;
	char hostbuff[2048];
	struct sockaddr_in their_addr;	// connector's address information
	struct mail_s *mail = (struct mail_s *)arg;
	int has_idle = 0;
	int threadfd = timed_thread_readfd(mail->p_timed_thread);

#ifdef HAVE_GETHOSTBYNAME_R
	if (gethostbyname_r(mail->host, &he, hostbuff, sizeof(hostbuff), &he_res, &he_errno)) {	// get the host info
		ERR("IMAP gethostbyname_r: %s", hstrerror(h_errno));
		exit(1);
	}
#else /* HAVE_GETHOSTBYNAME_R */
	if ((he_res = gethostbyname(mail->host)) == NULL) {	// get the host info
		herror("gethostbyname");
		exit(1);
	}
#endif /* HAVE_GETHOSTBYNAME_R */
	while (fail < mail->retries) {
		struct timeval timeout;
		int res;
		fd_set fdset;

		if (fail > 0) {
			ERR("Trying IMAP connection again for %s@%s (try %u/%u)",
					mail->user, mail->host, fail + 1, mail->retries);
		}
		do {
			if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
				perror("socket");
				fail++;
				break;
			}

			// host byte order
			their_addr.sin_family = AF_INET;
			// short, network byte order
			their_addr.sin_port = htons(mail->port);
			their_addr.sin_addr = *((struct in_addr *) he_res->h_addr);
			// zero the rest of the struct
			memset(&(their_addr.sin_zero), '\0', 8);

			if (connect(sockfd, (struct sockaddr *) &their_addr,
						sizeof(struct sockaddr)) == -1) {
				perror("connect");
				fail++;
				break;
			}

			timeout.tv_sec = 60;	// 60 second timeout i guess
			timeout.tv_usec = 0;
			FD_ZERO(&fdset);
			FD_SET(sockfd, &fdset);
			res = select(sockfd + 1, &fdset, NULL, NULL, &timeout);
			if (res > 0) {
				if ((numbytes = recv(sockfd, recvbuf, MAXDATASIZE - 1, 0)) == -1) {
					perror("recv");
					fail++;
					break;
				}
			} else {
				ERR("IMAP connection failed: timeout");
				fail++;
				break;
			}
			recvbuf[numbytes] = '\0';
			DBGP2("imap_thread() received: %s", recvbuf);
			if (strstr(recvbuf, "* OK") != recvbuf) {
				ERR("IMAP connection failed, probably not an IMAP server");
				fail++;
				break;
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
			if (strstr(recvbuf, " IDLE ") != NULL) {
				has_idle = 1;
			}

			strncpy(sendbuf, "a2 STATUS ", MAXDATASIZE);
			strncat(sendbuf, mail->folder, MAXDATASIZE - strlen(sendbuf) - 1);
			strncat(sendbuf, " (MESSAGES UNSEEN)\r\n",
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
				strncpy(sendbuf, "a4 SELECT ", MAXDATASIZE);
				strncat(sendbuf, mail->folder, MAXDATASIZE - strlen(sendbuf) - 1);
				strncat(sendbuf, "\r\n", MAXDATASIZE - strlen(sendbuf) - 1);
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
					 * We'll do it every 20 minutes to be safe.
					 */
					timeout.tv_sec = 1200;
					timeout.tv_usec = 0;
					DBGP2("idling...");
					FD_ZERO(&fdset);
					FD_SET(sockfd, &fdset);
					FD_SET(threadfd, &fdset);
					res = select(MAX(sockfd + 1, threadfd + 1), &fdset, NULL, NULL, NULL);
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
					} else {
						break;
					}
					recvbuf[numbytes] = '\0';
					DBGP2("imap_thread() received: %s", recvbuf);
					if (strlen(recvbuf) > 2) {
						unsigned long messages, recent;
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
									mail->messages = messages;
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
						if (recent > 0 || (buf && strstr(buf, " FETCH ")) || timeout.tv_sec == 0 || force_check) {
							// re-check messages and unseen
							if (imap_command(sockfd, "DONE\r\n", recvbuf, "a5 OK")) {
								fail++;
								break;
							}
							strncpy(sendbuf, "a2 STATUS ", MAXDATASIZE);
							strncat(sendbuf, mail->folder, MAXDATASIZE - strlen(sendbuf) - 1);
							strncat(sendbuf, " (MESSAGES UNSEEN)\r\n",
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
					}
					imap_unseen_command(mail, old_unseen, old_messages);
					fail = 0;
					old_unseen = mail->unseen;
					old_messages = mail->messages;
				}
			} else {
				strncpy(sendbuf, "a3 logout\r\n", MAXDATASIZE);
				if (send(sockfd, sendbuf, strlen(sendbuf), 0) == -1) {
					perror("send a3");
					fail++;
					break;
				}
				timeout.tv_sec = 60;	// 60 second timeout i guess
				timeout.tv_usec = 0;
				FD_ZERO(&fdset);
				FD_SET(sockfd, &fdset);
				res = select(sockfd + 1, &fdset, NULL, NULL, &timeout);
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
					ERR("IMAP logout failed: %s", recvbuf);
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

int pop3_command(int sockfd, const char *command, char *response, const char *verify)
{
	struct timeval timeout;
	fd_set fdset;
	int res, numbytes = 0;
	if (send(sockfd, command, strlen(command), 0) == -1) {
		perror("send");
		return -1;
	}
	timeout.tv_sec = 60;	// 60 second timeout i guess
	timeout.tv_usec = 0;
	FD_ZERO(&fdset);
	FD_SET(sockfd, &fdset);
	res = select(sockfd + 1, &fdset, NULL, NULL, &timeout);
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

void *pop3_thread(void *arg)
{
	int sockfd, numbytes;
	char recvbuf[MAXDATASIZE];
	char sendbuf[MAXDATASIZE];
	char *reply;
	unsigned int fail = 0;
	unsigned long old_unseen = ULONG_MAX;
	struct stat stat_buf;
	struct hostent he, *he_res = 0;
	int he_errno;
	char hostbuff[2048];
	struct sockaddr_in their_addr;	// connector's address information
	struct mail_s *mail = (struct mail_s *)arg;

#ifdef HAVE_GETHOSTBYNAME_R
	if (gethostbyname_r(mail->host, &he, hostbuff, sizeof(hostbuff), &he_res, &he_errno)) {	// get the host info
		ERR("POP3 gethostbyname_r: %s", hstrerror(h_errno));
		exit(1);
	}
#else /* HAVE_GETHOSTBYNAME_R */
	if ((he_res = gethostbyname(mail->host)) == NULL) {	// get the host info
		herror("gethostbyname");
		exit(1);
	}
#endif /* HAVE_GETHOSTBYNAME_R */
	while (fail < mail->retries) {
		struct timeval timeout;
		int res;
		fd_set fdset;

		if (fail > 0) {
			ERR("Trying POP3 connection again for %s@%s (try %u/%u)",
					mail->user, mail->host, fail + 1, mail->retries);
		}
		do {
			if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
				perror("socket");
				fail++;
				break;
			}

			// host byte order
			their_addr.sin_family = AF_INET;
			// short, network byte order
			their_addr.sin_port = htons(mail->port);
			their_addr.sin_addr = *((struct in_addr *) he_res->h_addr);
			// zero the rest of the struct
			memset(&(their_addr.sin_zero), '\0', 8);

			if (connect(sockfd, (struct sockaddr *) &their_addr,
						sizeof(struct sockaddr)) == -1) {
				perror("connect");
				fail++;
				break;
			}

			timeout.tv_sec = 60;	// 60 second timeout i guess
			timeout.tv_usec = 0;
			FD_ZERO(&fdset);
			FD_SET(sockfd, &fdset);
			res = select(sockfd + 1, &fdset, NULL, NULL, &timeout);
			if (res > 0) {
				if ((numbytes = recv(sockfd, recvbuf, MAXDATASIZE - 1, 0)) == -1) {
					perror("recv");
					fail++;
					break;
				}
			} else {
				ERR("POP3 connection failed: timeout\n");
				fail++;
				break;
			}
			DBGP2("pop3_thread received: %s", recvbuf);
			recvbuf[numbytes] = '\0';
			if (strstr(recvbuf, "+OK ") != recvbuf) {
				ERR("POP3 connection failed, probably not a POP3 server");
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
				ERR("POP3 server login failed: %s", recvbuf);
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
				ERR("Error parsing POP3 response: %s", recvbuf);
				fail++;
				break;
			} else {
				timed_thread_lock(mail->p_timed_thread);
				sscanf(reply, "%lu %lu", &mail->unseen, &mail->used);
				timed_thread_unlock(mail->p_timed_thread);
			}
			
			strncpy(sendbuf, "QUIT\r\n", MAXDATASIZE);
			if (pop3_command(sockfd, sendbuf, recvbuf, "+OK")) {
				ERR("POP3 logout failed: %s", recvbuf);
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

