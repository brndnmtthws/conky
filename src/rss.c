/*
 * rss.c
 * RSS stuff (prss version)
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "prss.h"

PRSS* save = NULL;

int rss_delay(int delay)
{
	static int wait = 0;
	time_t now = time(NULL);

	if(!wait) {
		wait = now + delay;
		return 1;
	}

	if(now >= wait + delay) {
		wait = now + delay;
		return 1;
	}

	return 0;
}

PRSS*
get_rss_info(char *uri, int delay)
{
	if(!rss_delay(delay))
		return save; // wait for delay to pass

	if(save != NULL)
		prss_free(save); // clean up old data

	save = prss_parse_file("test.xml");
	//assert(save);

	return save;
}
