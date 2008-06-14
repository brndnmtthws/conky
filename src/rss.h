#ifndef RSS_H_
#define RSS_H_

#include "prss.h"

PRSS *get_rss_info(char *uri, int delay);
void init_rss_info(void);
void free_rss_info(void);

#endif /*RSS_H_*/
