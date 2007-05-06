/*
 * rss.c
 * RSS stuff
 *
 * $Id$
 */

#include <mrss.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

GList*
get_rss_info(char *uri, int count)
{
        mrss_t *data;
        mrss_item_t *item;
        mrss_error_t ret;
	GList *titles = NULL;
	int i = 0;

        ret = mrss_parse_url(uri, &data);

        if (ret) {
		titles = g_list_append(titles, mrss_strerror(ret));
		return titles;
	}

        for (item = data->item; item; item = item->next) {
		char *tmp = strdup(item->title);
		titles = g_list_append(titles, tmp);
		
		if ((count > 0) && (++i > count - 1))
			goto cleanup;
        }

cleanup:
	mrss_free(data);

	return titles;
}
