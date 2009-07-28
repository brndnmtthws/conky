/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*- */

#ifndef RSS_H_
#define RSS_H_

#include "prss.h"

void rss_free_info(void);
void rss_process_info(char *p, int p_max_size, char *uri, char *action, int
		act_par, int interval, unsigned int nrspaces);

#endif /*RSS_H_*/
