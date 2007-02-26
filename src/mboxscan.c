/*
 * $Id$
 * 
 * Licence: http://www.opensource.org/licenses/bsd-license.php
 * author: mac@calmar.ws
 * 
 * Modified for use in Conky by Brenden Matthews
 *
 * Description:
 * scanning from top to bottom on a mbox
 * The output as follows:
 * F: FROM_LENGHT S: SUBJECT_LENGHT
 * (PRINT_MAILS or -n NR times)
 */

#include "conky.h"
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mboxscan.h"

#define FROM_WIDTH 10
#define SUBJECT_WIDTH 22
#define PRINT_MAILS 5

struct ring_list {
	char *from;
	char *subject;
	struct ring_list *previous;
	struct ring_list *next;
};

void mbox_scan(char *args, char *output, size_t max_len)
{
	int i, u, flag;
	int from_width, subject_width, print_mails;
	char buf[text_buffer_size];

	char *substr = strstr(args, "-n");
	if (substr) {
		if (sscanf(substr, "-n %i", &print_mails) != 1) {
			print_mails = PRINT_MAILS;
		}
	} else {
		print_mails = PRINT_MAILS;
	}
	substr = strstr(args, "-fw");
	if (substr) {
		if (sscanf(substr, "-fw %i", &from_width) != 1) {
			from_width = FROM_WIDTH;
		}
	} else {
		from_width = FROM_WIDTH;
	}
	substr = strstr(args, "-sw");
	if (substr) {
		if (sscanf(substr, "-sw %i", &subject_width) != 1) {
			subject_width = SUBJECT_WIDTH;
		}
	} else {
		subject_width = SUBJECT_WIDTH;
	}
	char current_mail_spool[text_buffer_size];
	if (args[strlen(args) - 1] == '"') { // encapsulated with "'s
		// find first occurrence of "
		strncpy(current_mail_spool, args, text_buffer_size);
		char *start = strchr(current_mail_spool, '"') + 1;
		start[(long)(strrchr(current_mail_spool, '"') - start)] = '\0';
		strncpy(current_mail_spool, start, text_buffer_size);
	} else {
		char *tmp = strtok(args, " ");
		char *start = tmp;
		while (tmp) {
			tmp = strtok(NULL, " ");
			if (tmp) {
				start = tmp;
			}
		}
		strncpy(current_mail_spool, start, text_buffer_size);
	}
	//printf("'%s', %i, %i, %i\n", current_mail_spool, subject_width, from_width, print_mails);
	if (strlen(current_mail_spool) < 1) {
		CRIT_ERR("Usage: ${mboxscan [-n <number of messages to print>] [-fw <from width>] [-sw <subject width>] mbox}");
	}

	struct stat statbuf;

	if (stat(current_mail_spool, &statbuf)) {
		CRIT_ERR("can't stat %s: %s", current_mail_spool, strerror(errno));
	}
	/* end - argument checking */

	/* build up double-linked ring-list to hold data, while scanning down the mbox */
	struct ring_list *curr=0, *prev=0, *start=0;

	for (i = 0; i < print_mails; i++) {
		curr = (struct ring_list *)malloc(sizeof(struct ring_list));
		curr->from = (char *)malloc(sizeof(char[from_width + 1]));
		curr->subject = (char *)malloc(sizeof(char[subject_width + 1]));
		curr->from[0] = '\0';
		curr->subject[0] = '\0';

		if (i == 0)
			start = curr;
		if (i > 0) {
			curr->previous = prev;
			prev->next = curr;
		}
		prev = curr;
	}

	/* connect end to start for an endless loop-ring */
	start->previous = curr;
	curr->next = start;

	/* mbox */
	FILE *fp;

	fp = fopen(current_mail_spool, "r");
	if (!fp) {
		return;
	}

	flag = 1;		/* frist find a "From " to set it to 0 for header-sarchings */
	while (!feof(fp)) {
		if (fgets(buf, text_buffer_size, fp) == NULL)
			break;

		if (strncmp(buf, "From ", 5) == 0) {
			curr = curr->next;

			/* skip until \n */
			while (strchr(buf, '\n') == NULL && !feof(fp))
				fgets(buf, text_buffer_size, fp);

			flag = 0;	/* in the headers now */
			continue;
		}

		if (flag == 1) {	/* in the body, so skip */
			continue;
		}

		if (buf[0] == '\n') {
			/* beyond the headers now (empty line), skip until \n */
			/* then search for new mail ("From ") */
			while (strchr(buf, '\n') == NULL && !feof(fp))
				fgets(buf, text_buffer_size, fp);
			flag = 1;	/* in the body now */
			continue;
		}

		if ((strncmp(buf, "X-Status: ", 10) == 0)
		    || (strncmp(buf, "Status: R", 9) == 0)) {

			/* Mail was read or something, so skip that message */
			flag = 1;	/* search for next From */
			curr->subject[0] = '0';
			curr->from[0] = '0';
			curr = curr->previous;	/* (will get current again on new 'From ' finding) */
			/* Skip until \n */
			while (strchr(buf, '\n') == NULL && !feof(fp))
				fgets(buf, text_buffer_size, fp);
			continue;
		}

		/* that covers ^From: and ^from: */
		if (strncmp(buf + 1, "rom: ", 5) == 0) {

			i = 0;
			u = 6;	/* no "From: " string needed, so skip */
			while (1) {

				if (buf[u] == '"') {	/* no quotes around names */
					u++;
					continue;
				}

				if (buf[u] == '<' && i > 1) {	/* some are: From: <foo@bar.com> */

					curr->from[i] = '\0';
					/* skip until \n */
					while (strchr(buf, '\n') == NULL && !feof(fp))
						fgets(buf, text_buffer_size, fp);
					break;
				}

				if (buf[u] == '\n') {
					curr->from[i] = '\0';
					break;
				}

				if (buf[u] == '\0') {
					curr->from[i] = '\0';
					break;
				}

				if (i >= from_width) {
					curr->from[i] = '\0';
					/* skip until \n */
					while (strchr(buf, '\n') == NULL && !feof(fp))
						fgets(buf, text_buffer_size, fp);
					break;
				}

				/* nothing special so just set it */
				curr->from[i++] = buf[u++];
			}
		}

		/* that covers ^Subject and ^subject */
		if (strncmp(buf + 1, "ubject: ", 8) == 0) {

			i = 0;
			u = 9;	/* no "Subject: " string needed, so skip */
			while (1) {

				if (buf[u] == '\n') {
					curr->subject[i] = '\0';
					break;
				}
				if (buf[u] == '\0') {
					curr->subject[i] = '\0';
					break;
				}
				if (i >= subject_width) {
					curr->subject[i] = '\0';

					/* skip until \n */
					while (strchr(buf, '\n') == NULL && !feof(fp))
						fgets(buf, text_buffer_size, fp);
					break;
				}

				/* nothing special so just set it */
				curr->subject[i++] = buf[u++];
			}
		}

	}

	fclose(fp);

	i = print_mails;
	output[0] = '\0';
	while (curr->from[0] != '\0') {
		snprintf(buf, text_buffer_size, "F: %-*s S: %-*s\n", from_width, curr->from, subject_width, curr->subject);
		strncat(output, buf, max_len - strlen(output));
/*		printf("F: %-*s", from_width, curr->from);
		printf(" S: %-*s\n", subject_width, curr->subject);*/
		free(curr->from);
		free(curr->subject);
		struct ring_list *old = curr;
		free(old);
		curr = curr->previous;
		if (--i == 0) {
			break;
		}
	}
}
