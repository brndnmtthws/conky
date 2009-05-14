#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include "defconfig.h"

ssize_t conf_read(void *cookie, char *buf, size_t size)
{
	static int col = 0, row = 0;
	size_t i = 0;
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

#if defined(__linux__)
cookie_io_functions_t conf_cookie = {
	.read = &conf_read,
	.write = NULL,
	.seek = NULL,
	.close = NULL,
};
#endif
