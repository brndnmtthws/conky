/*
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2007 Brenden Matthews, Philip Kovacs, et. al. (see AUTHORS)
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
 *  $Id$
 */

#include <sys/stat.h>
#include <sys/time.h>

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "conky.h"

char *current_mail_spool;

void update_mail_count(struct local_mail_s *mail)
{
	struct stat buf;

	if (mail == NULL)
		return;

	/* TODO: use that fine file modification notify on Linux 2.4 */

	/* don't check mail so often (9.5s is minimum interval) */
	if (current_update_time - mail->last_update < 9.5)
		return;
	else
		mail->last_update = current_update_time;

	if (stat(mail->box, &buf)) {
		static int rep;
		if (!rep) {
			ERR("can't stat %s: %s", mail->box,
			    strerror(errno));
			rep = 1;
		}
		return;
	}
#if HAVE_DIRENT_H
	/* maildir format */
	if (S_ISDIR(buf.st_mode)) {
		DIR *dir;
		char *dirname;
		struct dirent *dirent;

		mail->mail_count = mail->new_mail_count = 0;
		dirname =
		    (char *) malloc(sizeof(char) *
				    (strlen(mail->box) + 5));
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
	if (buf.st_mtime != mail->last_mtime) {
		/* yippee, modification time has changed, let's read mail count! */
		static int rep;
		FILE *fp;
		int reading_status = 0;

		/* could lock here but I don't think it's really worth it because
		 * this isn't going to write mail spool */

		mail->new_mail_count = mail->mail_count = 0;

		fp = open_file(mail->box, &rep);
		if (!fp)
			return;

		/* NOTE: adds mail as new if there isn't Status-field at all */

		while (!feof(fp)) {
			char buf[128];
			if (fgets(buf, 128, fp) == NULL)
				break;

			if (strncmp(buf, "From ", 5) == 0) {
				/* ignore MAILER-DAEMON */
				if (strncmp(buf + 5, "MAILER-DAEMON ", 14)
				    != 0) {
					mail->mail_count++;

					if (reading_status)
						mail->new_mail_count++;
					else
						reading_status = 1;
				}
			} else {
				if (reading_status
				    && strncmp(buf, "X-Mozilla-Status:",
					       17) == 0) {
					/* check that mail isn't already read */
					if (strchr(buf + 21, '0'))
						mail->new_mail_count++;

					reading_status = 0;
					continue;
				}
				if (reading_status
				    && strncmp(buf, "Status:", 7) == 0) {
					/* check that mail isn't already read */
					if (strchr(buf + 7, 'R') == NULL)
						mail->new_mail_count++;

					reading_status = 0;
					continue;
				}
			}

			/* skip until \n */
			while (strchr(buf, '\n') == NULL && !feof(fp))
				fgets(buf, 128, fp);
		}

		fclose(fp);

		if (reading_status)
			mail->new_mail_count++;

		mail->last_mtime = buf.st_mtime;
	}
}
