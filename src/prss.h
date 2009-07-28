/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 *
 * Copyright (c) 2007 Mikko Sysikaski <mikko.sysikaski@gmail.com>
 *					  Toni Spets <toni.spets@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#ifndef PRSS_H
#define PRSS_H

#include <libxml/parser.h>

typedef struct PRSS_Item_ {
	char *title;
	char *link;
	char *description;
	char *category;
	char *pubdate;
	char *guid;
} PRSS_Item;

typedef struct PRSS_ {
	xmlDocPtr _data;
	char *version;

	char *title;
	char *link;
	char *description;
	char *language;
	char *generator;
	char *managingeditor;
	char *webmaster;
	char *docs;
	char *lastbuilddate;
	char *pubdate;
	char *copyright;
	char *ttl;

	PRSS_Item *items;
	int item_count;
} PRSS;

/* Functions for parsing RSS-data */
void prss_parse_data(void *result, const char *xml_data);

/* // Works wrong currently when called from application!
PRSS *prss_parse_doc(xmlDocPtr doc); */

/* Frees the PRSS-stucture returned by prss_parse_*.
 * The memory area pointed by data becomes invalid
 * after call to this function. */
void prss_free(PRSS *data);

#endif /* PRSS_H */
