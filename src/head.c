#include "config.h"
#include "conky.h"
#include "logging.h"
#include "tail.h" /* MAX_TAIL_LINES */

int init_head_object(struct text_object *obj, const char *arg)
{
	char buf[64];
	int n1, n2;
	FILE *fp;
	int numargs;

	if (!arg) {
		ERR("head needs arguments");
		return 1;
	}

	numargs = sscanf(arg, "%63s %i %i", buf, &n1, &n2);

	if (numargs < 2 || numargs > 3) {
		ERR("incorrect number of arguments given to tail object");
		return 1;
	}

	if (n1 < 1 || n1 > MAX_TAIL_LINES) {
		ERR("invalid arg for tail, number of lines must be "
				"between 1 and %i", MAX_TAIL_LINES);
		return 1;
	}

	if ((fp = fopen(buf, "r")) == NULL) {
		ERR("head logfile does not exist, or you do not have "
				"correct permissions");
		return 1;
	}

	obj->data.tail.logfile = malloc(text_buffer_size);
	strcpy(obj->data.tail.logfile, buf);
	obj->data.tail.wantedlines = n1;
	obj->data.tail.interval = update_interval * 2;
	fclose(fp);

	/* XXX: the following implies update_interval >= 1 ?! */
	if (numargs == 3 && (n2 < 1 || n2 < update_interval)) {
		ERR("tail interval must be greater than "
		    "0 and "PACKAGE_NAME"'s interval, ignoring");
	} else if (numargs == 3) {
			obj->data.tail.interval = n2;
	}
	/* asumming all else worked */
	obj->data.tail.buffer = malloc(text_buffer_size * 20);
	return 0;
}

static long fwd_fcharfind(FILE *fp, char val, unsigned int step)
{
#define BUFSZ 0x1000
	long ret = -1;
	unsigned int count = 0;
	static char buf[BUFSZ];
	long orig_pos = ftell(fp);
	long buf_pos = -1;
	long buf_size = BUFSZ;
	char *cur_found = NULL;

	while (count < step) {
		if (cur_found == NULL) {
			buf_size = fread(buf, 1, buf_size, fp);
			buf_pos = 0;
		}
		cur_found = memchr(buf + buf_pos, val, buf_size - buf_pos);
		if (cur_found != NULL) {
			buf_pos = cur_found - buf + 1;
			count++;
		} else {
			if (feof(fp)) {
				break;
			}
		}
	}
	if (count == step) {
		ret = ftell(fp) - buf_size + buf_pos - 1;
	}
	fseek(fp, orig_pos, SEEK_SET);
	return ret;
}

int print_head_object(struct text_object *obj, char *p, size_t p_max_size)
{
	FILE *fp;
	long nl = 0;
	int iter;

	if (current_update_time - obj->data.tail.last_update < obj->data.tail.interval) {
		snprintf(p, p_max_size, "%s", obj->data.tail.buffer);
		return 0;
	}

	obj->data.tail.last_update = current_update_time;

	fp = fopen(obj->data.tail.logfile, "rt");
	if (fp == NULL) {
		/* Send one message, but do not consistently spam
		 * on missing logfiles. */
		if (obj->data.tail.readlines != 0) {
			ERR("head logfile failed to open");
			strcpy(obj->data.tail.buffer, "Logfile Missing");
		}
		obj->data.tail.readlines = 0;
		snprintf(p, p_max_size, "Logfile Missing");
	} else {
		obj->data.tail.readlines = 0;
		for (iter = obj->data.tail.wantedlines; iter > 0;
				iter--) {
			nl = fwd_fcharfind(fp, '\n', iter);
			if (nl >= 0) {
				break;
			}
		}
		obj->data.tail.readlines = iter;
		/* Make sure nl is at least 1 byte smaller than the
		 * buffer max size. */
		if (nl > (long) ((text_buffer_size * 20) - 1)) {
			nl = text_buffer_size * 20 - 1;
		}
		nl = fread(obj->data.tail.buffer, 1, nl, fp);
		fclose(fp);
		if (nl > 0) {
			/* Clean up trailing newline, make sure the buffer
			 * is null terminated. */
			if (obj->data.tail.buffer[nl - 1] == '\n') {
				obj->data.tail.buffer[nl - 1] = '\0';
			} else {
				obj->data.tail.buffer[nl] = '\0';
			}
			snprintf(p, p_max_size, "%s",
					obj->data.tail.buffer);
		} else {
			strcpy(obj->data.tail.buffer, "Logfile Empty");
			snprintf(p, p_max_size, "Logfile Empty");
		}	/* nl > 0 */
	}		/* if fp == null */
	return 0;
}
