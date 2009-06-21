#define _GNU_SOURCE
#include "config.h"
#include <stdio.h>
#include <sys/types.h>
#include "defconfig.h"

#if defined(HAVE_FOPENCOOKIE)
#define COOKIE_LEN_T	size_t
#define COOKIE_RET_T	ssize_t
#else
#define COOKIE_LEN_T	int
#define COOKIE_RET_T	int
#endif

static COOKIE_RET_T
conf_read(void *cookie, char *buf, COOKIE_LEN_T size)
{
	static int col = 0, row = 0;
	COOKIE_LEN_T i = 0;
	const char *conf[] = defconfig;

	(void)cookie;

	while (i < size) {
		if (!(conf[row]))		/* end of rows */
			break;
		if (!(conf[row][col])) {	/* end of line */
			row++;
			col = 0;
			continue;
		}
		buf[i++] = conf[row][col++];
	}
	return i;
}

#if defined(HAVE_FOPENCOOKIE)
static cookie_io_functions_t conf_cookie = {
	.read = &conf_read,
	.write = NULL,
	.seek = NULL,
	.close = NULL,
};
FILE *conf_cookie_open(void)
{
	return fopencookie(NULL, "r", conf_cookie);
}
#elif defined(HAVE_FUNOPEN)
FILE *conf_cookie_open(void)
{
	return funopen(NULL, &conf_read, NULL, NULL, NULL);
}
#else
FILE *conf_cookie_open(void) { return NULL; }
#endif
