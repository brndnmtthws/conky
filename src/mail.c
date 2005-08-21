/*
 * Conky, a system monitor, based on torsmo
 *
 * This program is licensed under BSD license, read COPYING
 *
 *  $Id$
 */

#include "conky.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>

char *current_mail_spool;

static time_t last_mail_mtime;
static double last_mail_update;

void update_mail_count()
{
	struct stat buf;

	if (current_mail_spool == NULL)
		return;

	/* TODO: use that fine file modification notify on Linux 2.4 */

	/* don't check mail so often (9.5s is minimum interval) */
	if (current_update_time - last_mail_update < 9.5)
		return;
	else
		last_mail_update = current_update_time;

	if (stat(current_mail_spool, &buf)) {
		static int rep;
		if (!rep) {
			ERR("can't stat %s: %s", current_mail_spool,
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
		info.mail_count = 0;
		info.new_mail_count = 0;

		dirname =
		    (char *) malloc(sizeof(char) *
				    (strlen(current_mail_spool) + 5));
		if (!dirname) {
			ERR("malloc");
			return;
		}
		strcpy(dirname, current_mail_spool);
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
				info.mail_count++;
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
				info.new_mail_count++;
				info.mail_count++;
			}
			dirent = readdir(dir);
		}
		closedir(dir);

		free(dirname);
		return;
	}
#endif
	/* mbox format */


	if (buf.st_mtime != last_mail_mtime) {
		/* yippee, modification time has changed, let's read mail count! */
		static int rep;
		FILE *fp;
		int reading_status = 0;

		/* could lock here but I don't think it's really worth it because
		 * this isn't going to write mail spool */

		info.new_mail_count = 0;
		info.mail_count = 0;

		fp = open_file(current_mail_spool, &rep);
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
					info.mail_count++;

					if (reading_status)
						info.new_mail_count++;
					else
						reading_status = 1;
				}
			} else {
				if (reading_status
				    && strncmp(buf, "X-Mozilla-Status:",
					       17) == 0) {
					/* check that mail isn't already read */
					if (strchr(buf + 21, '0'))
						info.new_mail_count++;

					reading_status = 0;
					continue;
				}
				if (reading_status
				    && strncmp(buf, "Status:", 7) == 0) {
					/* check that mail isn't already read */
					if (strchr(buf + 7, 'R') == NULL)
						info.new_mail_count++;

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
			info.new_mail_count++;

		last_mail_mtime = buf.st_mtime;
	}
}
