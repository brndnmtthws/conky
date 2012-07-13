/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
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
 * Copyright (c) 2005-2012 Brenden Matthews, Philip Kovacs, et. al.
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

#include "mail.h"

#include "conky.h"
#include "common.h"
#include "logging.h"
#include "text_object.h"

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
#include <inttypes.h>

#include <dirent.h>
#include <termios.h>

#include <cmath>
#include <mutex>
#include <sstream>

#include "update-cb.hh"

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

std::pair<std::string, bool>
priv::current_mail_spool_setting::do_convert(lua::state &l, int index)
{
	auto ret = Base::do_convert(l, index);
	if(ret.second)
		ret.first = variable_substitute(ret.first);
	return ret;
}

priv::current_mail_spool_setting current_mail_spool;

namespace {
	enum { DEFAULT_MAIL_INTERVAL = 300 /*seconds*/ };

	enum { MP_USER, MP_PASS, MP_FOLDER, MP_COMMAND, MP_HOST, MP_PORT };

	struct mail_result {
		unsigned long unseen;
		unsigned long used;
		unsigned long messages;

		mail_result()
			: unseen(0), used(0), messages(0)
		{}
	};

	class mail_cb: public conky::callback<mail_result, std::string, std::string, std::string,
											std::string, std::string, in_port_t> {
		typedef conky::callback<mail_result, std::string, std::string, std::string,
								std::string, std::string, in_port_t> Base;

	protected:
		addrinfo *ai;

		uint16_t fail;
		uint16_t retries;

		void resolve_host()
		{
			struct addrinfo hints;
			char portbuf[8];

			memset(&hints, 0, sizeof(struct addrinfo));
			hints.ai_family = AF_UNSPEC;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;
			snprintf(portbuf, 8, "%" SCNu16, get<MP_PORT>());

			if(int res = getaddrinfo(get<MP_HOST>().c_str(), portbuf, &hints, &ai))
				throw std::runtime_error(std::string("IMAP getaddrinfo: ") + gai_strerror(res));
		}

		int connect()
		{
			for (struct addrinfo *rp = ai; rp != NULL; rp = rp->ai_next) {
				int sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
				if (sockfd == -1) {
					continue;
				}
				if (::connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) {
					return sockfd;
				}
				close(sockfd);
			}
			throw std::runtime_error("Unable to connect to mail server");
		}

		virtual void merge(callback_base &&other)
		{
			mail_cb &&o = dynamic_cast<mail_cb &&>(other);
			if(retries < o.retries) {
				retries = o.retries;
				fail = 0;
			}

			Base::merge(std::move(other));
		}

		mail_cb(uint32_t period, const Tuple &tuple, uint16_t retries_)
			: Base(period, false, tuple, true), ai(NULL), fail(0), retries(retries_)
		{}

		~mail_cb()
		{
			if(ai)
				freeaddrinfo(ai);
		}
	};

	struct mail_param_ex: public mail_cb::Tuple {
		uint16_t retries;
		uint32_t period;

		mail_param_ex()
			: retries(0), period(1)
		{}
	};

	class imap_cb: public mail_cb {
		typedef mail_cb Base;

		void check_status(char *recvbuf);
		void unseen_command(unsigned long old_unseen, unsigned long old_messages);

	protected:
		virtual void work();

	public:
		imap_cb(uint32_t period, const Tuple &tuple, uint16_t retries_)
			: Base(period, tuple, retries_)
		{}
	};

	class pop3_cb: public mail_cb {
		typedef mail_cb Base;

	protected:
		virtual void work();

	public:
		pop3_cb(uint32_t period, const Tuple &tuple, uint16_t retries_)
			: Base(period, tuple, retries_)
		{}
	};

	struct mail_param_ex *global_mail;
}

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
				if (!fgets(buf, 128, fp))
					break;
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
	char mbox[256];
	struct local_mail_s *locmail;

	if (!arg) {
		n1 = 9.5;
		strncpy(mbox, current_mail_spool.get(*state).c_str(), sizeof(mbox));
	} else {
		if (sscanf(arg, "%s %f", mbox, &n1) != 2) {
			n1 = 9.5;
			strncpy(mbox, arg, sizeof(mbox));
		}
	}

	std::string dst = variable_substitute(mbox);

	locmail = (struct local_mail_s*)malloc(sizeof(struct local_mail_s));
	memset(locmail, 0, sizeof(struct local_mail_s));
	locmail->mbox = strndup(dst.c_str(), text_buffer_size.get(*state));
	locmail->interval = n1;
	obj->data.opaque = locmail;
}

#define PRINT_MAILS_GENERATOR(x) \
void print_##x##mails(struct text_object *obj, char *p, int p_max_size) \
{ \
	struct local_mail_s *locmail = (struct local_mail_s *)obj->data.opaque; \
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
	struct local_mail_s *locmail = (struct local_mail_s *)obj->data.opaque;

	if (!locmail)
		return;

	free_and_zero(locmail->mbox);
	free_and_zero(obj->data.opaque);
}

#define MAXDATASIZE 1000

namespace {
	enum mail_type {
		POP3_TYPE, IMAP_TYPE
	};
}

std::unique_ptr<mail_param_ex> parse_mail_args(mail_type type, const char *arg)
{
	using std::get;

	std::unique_ptr<mail_param_ex> mail;
	char *tmp;
	char host[128];
	char user[128];
	char pass[128];

	if (sscanf(arg, "%128s %128s %128s", host, user, pass)
			!= 3) {
		if (type == POP3_TYPE) {
			NORM_ERR("Scanning POP3 args failed");
		} else if (type == IMAP_TYPE) {
			NORM_ERR("Scanning IMAP args failed");
		}
		return mail;
	}

	// see if password needs prompting
	if (pass[0] == '*' && pass[1] == '\0') {
		int fp = fileno(stdin);
		struct termios term;

		tcgetattr(fp, &term);
		term.c_lflag &= ~ECHO;
		tcsetattr(fp, TCSANOW, &term);
		printf("Enter mailbox password (%s@%s): ", user, host);
		if (scanf("%128s", pass) != 1)
			pass[0] = 0;
		printf("\n");
		term.c_lflag |= ECHO;
		tcsetattr(fp, TCSANOW, &term);
	}

	mail.reset(new mail_param_ex);
	get<MP_HOST>(*mail) = host;
	get<MP_USER>(*mail) = user;
	get<MP_PASS>(*mail) = pass;

	// now we check for optional args
	tmp = (char*)strstr(arg, "-r ");
	if (tmp) {
		tmp += 3;
		sscanf(tmp, "%" SCNu16, &mail->retries);
	} else {
		mail->retries = 5;	// 5 retries after failure
	}

	float interval = DEFAULT_MAIL_INTERVAL;
	tmp = (char*)strstr(arg, "-i ");
	if (tmp) {
		tmp += 3;
		sscanf(tmp, "%f", &interval);
	}
	mail->period = std::max(lround(interval/active_update_interval()), 1l);

	tmp = (char*)strstr(arg, "-p ");
	if (tmp) {
		tmp += 3;
		sscanf(tmp, "%" SCNu16, &get<MP_PORT>(*mail));
	} else {
		if (type == POP3_TYPE) {
			get<MP_PORT>(*mail) = 110;	// default pop3 port
		} else if (type == IMAP_TYPE) {
			get<MP_PORT>(*mail) = 143;	// default imap port
		}
	}
	if (type == IMAP_TYPE) {
		tmp = (char*)strstr(arg, "-f ");
		if (tmp) {
			int len = 0;
			tmp += 3;
			if (tmp[0] == '\'') {
				len = (char*)strstr(tmp + 1, "'") - tmp - 1;
				tmp++;
			}
			get<MP_FOLDER>(*mail).assign(tmp, len);
		} else {
			get<MP_FOLDER>(*mail) = "INBOX";	// default imap inbox
		}
	}
	tmp = (char*)strstr(arg, "-e ");
	if (tmp) {
		int len = 0;
		tmp += 3;

		if (tmp[0] == '\'') {
			len = (char*)strstr(tmp + 1, "'") - tmp - 1;
		}
		get<MP_COMMAND>(*mail).assign(tmp + 1, len);
	}

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
		return;
	}
	// process
	obj->data.opaque = parse_mail_args(IMAP_TYPE, arg).release();
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
		return;
	}
	// process
	obj->data.opaque = parse_mail_args(POP3_TYPE, arg).release();
}

namespace {
	class mail_setting: public conky::simple_config_setting<std::string> {
		typedef conky::simple_config_setting<std::string> Base;

		mail_type type;

	protected:
		virtual void lua_setter(lua::state &l, bool init)
		{
			lua::stack_sentry s(l, -2);

			Base::lua_setter(l, init);

			if(init && !global_mail) {
				const std::string &t = do_convert(l, -1).first;
				if(t.size())
					global_mail = parse_mail_args(type, t.c_str()).release();
			}

			++s;
		}

		virtual void cleanup(lua::state &l)
		{
			lua::stack_sentry s(l, -1);

			delete global_mail;
			global_mail = NULL;

			l.pop();
		}

	public:
		mail_setting(const std::string &name, mail_type type_)
			: Base(name), type(type_)
		{}
	};

	mail_setting imap("imap", IMAP_TYPE);
	mail_setting pop3("pop3", POP3_TYPE);
}

void free_mail_obj(struct text_object *obj)
{
	if (!obj->data.opaque)
		return;

	if (obj->data.opaque != global_mail) {
		mail_param_ex *mail = static_cast<mail_param_ex *>(obj->data.opaque);
		delete mail;
		obj->data.opaque = 0;
	}
}

static void command(int sockfd, const std::string &cmd, char *response, const char *verify)
{
	struct timeval fetchtimeout;
	fd_set fdset;
	ssize_t total = 0; 
	int numbytes = 0;

	if (send(sockfd, cmd.c_str(), cmd.length(), 0) == -1)
		throw std::runtime_error("send: " + strerror_r(errno));
	DBGP2("command()  command: %s", cmd.c_str());

	while(1) {
		fetchtimeout.tv_sec = 60;	// 60 second timeout i guess
		fetchtimeout.tv_usec = 0;
		FD_ZERO(&fdset);
		FD_SET(sockfd, &fdset);

		if(select(sockfd + 1, &fdset, NULL, NULL, &fetchtimeout) == 0)
			throw std::runtime_error("select: read timeout");

		if ((numbytes = recv(sockfd, response + total, MAXDATASIZE - 1 - total, 0)) == -1)
			throw std::runtime_error("recv: " + strerror_r(errno));

		total += numbytes;
		response[total] = '\0';
		DBGP2("command() received: %s", response);

		if (strstr(response, verify) != NULL)
			return;

		if(numbytes == 0)
			throw std::runtime_error("Unexpected response from server");
	}
}

void imap_cb::check_status(char *recvbuf)
{
	char *reply;
	reply = (char*)strstr(recvbuf, " (MESSAGES ");
	if (!reply || strlen(reply) < 2)
		std::runtime_error("Unexpected response from server");

	reply += 2;
	*strchr(reply, ')') = '\0';

	std::lock_guard<std::mutex> lock(result_mutex);
	if(sscanf(reply, "MESSAGES %lu UNSEEN %lu", &result.messages, &result.unseen) != 2)
		throw std::runtime_error(std::string("Error parsing response: ") + recvbuf);
}

void imap_cb::unseen_command(unsigned long old_unseen, unsigned long old_messages)
{
	if (!get<MP_COMMAND>().empty() && (result.unseen > old_unseen
				|| (result.messages > old_messages && result.unseen > 0))) {
		if (system(get<MP_COMMAND>().c_str()) == -1) {
			perror("system()");
		}
	}
}

void imap_cb::work()
{
	int sockfd, numbytes;
	char recvbuf[MAXDATASIZE];
	unsigned long old_unseen = ULONG_MAX;
	unsigned long old_messages = ULONG_MAX;
	bool has_idle = false;

	while (fail < retries) {
		struct timeval fetchtimeout;
		int res;
		fd_set fdset;

		if(not ai)
			resolve_host();

		try {
			sockfd = connect();

			command(sockfd, "", recvbuf, "* OK");

			command(sockfd, "abc CAPABILITY\r\n", recvbuf, "abc OK");
			if (strstr(recvbuf, " IDLE ") != NULL)
				has_idle = true;

			std::ostringstream str;
			str << "a1 login " << get<MP_USER>() << " {" << get<MP_PASS>().length() << "}\r\n";
			command(sockfd, str.str(), recvbuf, "+");

			command(sockfd, get<MP_PASS>() + "\r\n", recvbuf, "a1 OK");

			command(sockfd, "a2 STATUS \"" + get<MP_FOLDER>() + "\" (MESSAGES UNSEEN)\r\n",
					recvbuf, "a2 OK");
			check_status(recvbuf);

			unseen_command(old_unseen, old_messages);
			fail = 0;
			old_unseen = result.unseen;
			old_messages = result.messages;

			if(not has_idle) {
				try { command(sockfd, "a3 LOGOUT\r\n", recvbuf, "a3 OK"); }
				catch(std::runtime_error &) {}
				close(sockfd);
				return;
			}

			command(sockfd, "a4 SELECT \"" + get<MP_FOLDER>() + "\"\r\n", recvbuf, "a4 OK");

			command(sockfd, "a5 IDLE\r\n", recvbuf, "+ idling");
			recvbuf[0] = '\0';

			while (1) {
				/*
				 * RFC 2177 says we have to re-idle every 29 minutes.
				 * We'll do it every 20 minutes to be safe.
				 */
				fetchtimeout.tv_sec = 1200;
				fetchtimeout.tv_usec = 0;
				DBGP2("idling...");
				FD_ZERO(&fdset);
				FD_SET(sockfd, &fdset);
				FD_SET(donefd(), &fdset);
				res = select(std::max(sockfd, donefd()) + 1, &fdset, NULL, NULL, &fetchtimeout);
				if ((res == -1 && errno == EINTR) || FD_ISSET(donefd(), &fdset)) {
					try {
						command(sockfd, "DONE\r\n", recvbuf, "a5 OK");
						command(sockfd, "a3 LOGOUT\r\n", recvbuf, "a3 OK");
					}
					catch(std::runtime_error &) {}
					close(sockfd);
					return;
				} else if (res > 0) {
					if((numbytes = recv(sockfd, recvbuf, MAXDATASIZE - 1, 0)) == -1)
						throw std::runtime_error("recv idling");
				} else
					throw std::runtime_error("");

				recvbuf[numbytes] = '\0';
				DBGP2("imap_thread() received: %s", recvbuf);
				unsigned long messages, recent = 0;
				bool force_check = 0;
				if (strlen(recvbuf) > 2) {
					char *buf = recvbuf;
					buf = (char*)strstr(buf, "EXISTS");
					while (buf && strlen(buf) > 1 && strstr(buf + 1, "EXISTS")) {
						buf = (char*)strstr(buf + 1, "EXISTS");
					}
					if (buf) {
						// back up until we reach '*'
						while (buf >= recvbuf && buf[0] != '*') {
							buf--;
						}
						if (sscanf(buf, "* %lu EXISTS\r\n", &messages) == 1) {
							std::lock_guard<std::mutex> lock(result_mutex);
							if (result.messages != messages) {
								force_check = 1;
								result.messages = messages;
							}
						}
					}
					buf = recvbuf;
					buf = (char*)strstr(buf, "RECENT");
					while (buf && strlen(buf) > 1 && strstr(buf + 1, "RECENT")) {
						buf = (char*)strstr(buf + 1, "RECENT");
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
				}
				/* check if we got a BYE from server */
				if (strstr(recvbuf, "* BYE")) {
					// need to re-connect
					throw std::runtime_error("");
				}

				/*
				 * check if we got a FETCH from server, recent was
				 * something other than 0, or we had a timeout
				 */
				if (recent > 0 || strstr(recvbuf, " FETCH ") || fetchtimeout.tv_sec == 0 || force_check) {
					// re-check messages and unseen
					command(sockfd, "DONE\r\n", recvbuf, "a5 OK");

					command(sockfd, "a2 STATUS \"" + get<MP_FOLDER>()
							+ "\" (MESSAGES UNSEEN)\r\n", recvbuf, "a2 OK");

					check_status(recvbuf);

					command(sockfd, "a5 IDLE\r\n", recvbuf, "+ idling");
				}
				unseen_command(old_unseen, old_messages);
				fail = 0;
				old_unseen = result.unseen;
				old_messages = result.messages;
			}
		}
		catch(std::runtime_error &e) {
			if(sockfd != -1)
				close(sockfd);
			freeaddrinfo(ai);
			ai = NULL;

			++fail;
			if(*e.what())
				NORM_ERR("Error while communicating with IMAP server: %s", e.what());
			NORM_ERR("Trying IMAP connection again for %s@%s (try %u/%u)",
					get<MP_USER>().c_str(), get<MP_HOST>().c_str(), fail+1, retries);
			sleep(fail); /* sleep more for the more failures we have */
		}

		if(is_done())
			return;
	}
}

void print_imap_unseen(struct text_object *obj, char *p, int p_max_size)
{
	struct mail_param_ex *mail = (struct mail_param_ex *)obj->data.opaque;

	if (!mail)
		return;

	auto cb = conky::register_cb<imap_cb>(mail->period, *mail, mail->retries);

	snprintf(p, p_max_size, "%lu", cb->get_result_copy().unseen);
}

void print_imap_messages(struct text_object *obj, char *p, int p_max_size)
{
	struct mail_param_ex *mail = (struct mail_param_ex *)obj->data.opaque;

	if (!mail)
		return;

	auto cb = conky::register_cb<imap_cb>(mail->period, *mail, mail->retries);

	snprintf(p, p_max_size, "%lu", cb->get_result_copy().messages);
}

void pop3_cb::work()
{
	int sockfd;
	char recvbuf[MAXDATASIZE];
	char *reply;
	unsigned long old_unseen = ULONG_MAX;

	while (fail < retries) {
		if(not ai)
			resolve_host();

		try {
			sockfd = connect();

			command(sockfd, "", recvbuf, "+OK ");
			command(sockfd, "USER " + get<MP_USER>() + "\r\n", recvbuf, "+OK ");
			command(sockfd, "PASS " + get<MP_PASS>() + "\r\n", recvbuf, "+OK ");
			command(sockfd, "STAT\r\n", recvbuf, "+OK ");

			// now we get the data
			reply = recvbuf + 4;
			{
				std::lock_guard<std::mutex> lock(result_mutex);
				sscanf(reply, "%lu %lu", &result.unseen, &result.used);
			}

			command(sockfd, "QUIT\r\n", recvbuf, "+OK");

			if (get<MP_COMMAND>().length() > 1 && result.unseen > old_unseen) {
				// new mail goodie
				if (system(get<MP_COMMAND>().c_str()) == -1) {
					perror("system()");
				}
			}
			fail = 0;
			old_unseen = result.unseen;
			return;
		}
		catch(std::runtime_error &e) {
			if(sockfd != -1)
				close(sockfd);
			freeaddrinfo(ai);
			ai = NULL;

			++fail;
			if(*e.what())
				NORM_ERR("Error while communicating with POP3 server: %s", e.what());
			NORM_ERR("Trying POP3 connection again for %s@%s (try %u/%u)",
					get<MP_USER>().c_str(), get<MP_HOST>().c_str(), fail+1, retries);
			sleep(fail); /* sleep more for the more failures we have */
		}

		if(is_done())
			return;
	}
}

void print_pop3_unseen(struct text_object *obj, char *p, int p_max_size)
{
	struct mail_param_ex *mail = (struct mail_param_ex *)obj->data.opaque;

	if (!mail)
		return;

	auto cb = conky::register_cb<pop3_cb>(mail->period, *mail, mail->retries);

	snprintf(p, p_max_size, "%lu", cb->get_result_copy().unseen);
}

void print_pop3_used(struct text_object *obj, char *p, int p_max_size)
{
	struct mail_param_ex *mail = (struct mail_param_ex *)obj->data.opaque;

	if (!mail)
		return;

	auto cb = conky::register_cb<pop3_cb>(mail->period, *mail, mail->retries);

	snprintf(p, p_max_size, "%.1f", cb->get_result_copy().used / 1024.0 / 1024.0);
}
