/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*- */

#ifndef RSS_H_
#define RSS_H_

void rss_scan_arg(struct text_object *, const char *);
void rss_print_info(struct text_object *, char *, int);
void rss_free_obj_info(struct text_object *);

void rss_free_info(void);

#endif /*RSS_H_*/
