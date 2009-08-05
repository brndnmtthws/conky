/*
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
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * vim: ts=4 sw=4 noet ai cindent syntax=c
 *
 */

#include "conky.h"
#include "prss.h"
#include "logging.h"
#include <libxml/parser.h>
#include <libxml/tree.h>

#ifndef PARSE_OPTIONS
#define PARSE_OPTIONS 0
#endif

void prss_parse_doc(PRSS *result, xmlDocPtr doc);

void prss_parse_data(void *result, const char *xml_data)
{
	PRSS *data = (PRSS*)result;

	xmlDocPtr doc = xmlReadMemory(xml_data, strlen(xml_data), "", NULL,
		PARSE_OPTIONS);

	if (!doc) {
		return;
	}

	prss_parse_doc(data, doc);
	xmlFreeDoc(doc);
}

void free_rss_items(PRSS *data)
{
	for (int i = 0; i < data->item_count; i++) {
#define CLEAR(a) if (data->items[i].a) { free(data->items[i].a); data->items[i].a = 0; }
		CLEAR(title);
		CLEAR(link);
		CLEAR(description);
		CLEAR(category);
		CLEAR(pubDate);
		CLEAR(guid);
#undef CLEAR
	}
	free(data->items);
	data->item_count = 0;
	data->items = 0;
}

void prss_free(PRSS *data)
{
	if (!data) {
		return;
	}
	if (data->version) {
		free(data->version);
		data->version = 0;
	}
	if (data->items) {
		free_rss_items(data);
	}
	data->version = 0;
	data->items = 0;
#define CLEAR(a) if (data->a) { free(data->a); data->a = 0; }
	CLEAR(title);
	CLEAR(link);
	CLEAR(description);
	CLEAR(language);
	CLEAR(pubDate);
	CLEAR(lastBuildDate);
	CLEAR(generator);
	CLEAR(docs);
	CLEAR(managingEditor);
	CLEAR(webMaster);
	CLEAR(copyright);
	CLEAR(ttl);
#undef CLEAR
}

static inline void prss_null_item(PRSS_Item *i)
{
	memset(i, 0, sizeof(PRSS_Item));
}

static inline void read_item(PRSS_Item *res, xmlNodePtr data)
{
	prss_null_item(res);

	for (; data; data = data->next) {
		xmlNodePtr child;

		if (data->type != XML_ELEMENT_NODE) {
			continue;
		}
		child = data->children;

		if (!child) {
			continue;
		}

#define ASSIGN(a) if (strcasecmp((const char*)data->name, #a) == EQUAL) { \
			if (res->a) free(res->a); \
			res->a = strdup((const char*)child->content); \
			continue; \
		}
		ASSIGN(title);
		ASSIGN(link);
		ASSIGN(description);
		ASSIGN(category);
		ASSIGN(pubDate);
		ASSIGN(guid);
#undef ASSIGN
	}
}
static inline void read_element(PRSS *res, xmlNodePtr n)
{
	xmlNodePtr child;

	if (n->type != XML_ELEMENT_NODE) {
		return;
	}
	child = n->children;

	if (!child) {
		return;
	}

#define ASSIGN(a) if (strcasecmp((const char*)n->name, #a) == EQUAL) { \
		if (res->a) free(res->a); \
		res->a = strdup((const char*)child->content); \
		return; \
	}
	ASSIGN(title);
	ASSIGN(link);
	ASSIGN(description);
	ASSIGN(language);
	ASSIGN(pubDate);
	ASSIGN(lastBuildDate);
	ASSIGN(generator);
	ASSIGN(docs);
	ASSIGN(managingEditor);
	ASSIGN(webMaster);
	ASSIGN(copyright);
	ASSIGN(ttl);
#undef ASSIGN
	if (!strcasecmp((const char*)n->name, "item")) {
		read_item(&res->items[res->item_count++], n->children);
	}
}

static inline int parse_rss_2_0(PRSS *res, xmlNodePtr root)
{
	xmlNodePtr channel = root->children;
	xmlNodePtr n;
	int items = 0;

	DBGP("parsing rss 2.0 or <1 doc");

	while (channel && (channel->type != XML_ELEMENT_NODE
				|| strcmp((const char *) channel->name, "channel"))) {
		channel = channel->next;
	}
	if (!channel) {
		return 0;
	}

	for (n = channel->children; n; n = n->next) {
		if (n->type == XML_ELEMENT_NODE &&
				!strcmp((const char *) n->name, "item")) {
			++items;
		}
	}

	if (res->version) free(res->version);
	res->version = strndup("2.0", text_buffer_size);
	if (res->items) free_rss_items(res);
	res->items = malloc(items * sizeof(PRSS_Item));
	res->item_count = 0;

	for (n = channel->children; n; n = n->next) {
		read_element(res, n);
	}

	return 1;
}
static inline int parse_rss_1_0(PRSS *res, xmlNodePtr root)
{
	int items = 0;
	xmlNodePtr n;

	DBGP("parsing rss 1.0 doc");

	for (n = root->children; n; n = n->next) {
		if (n->type == XML_ELEMENT_NODE) {
			if (!strcmp((const char *) n->name, "item")) {
				++items;
			} else if (!strcmp((const char *) n->name, "channel")) {
				xmlNodePtr i;

				for (i = n->children; i; i = i->next) {
					read_element(res, i);
				}
			}
		}
	}

	if (res->version) free(res->version);
	res->version = strndup("1.0", text_buffer_size);
	if (res->items) free_rss_items(res);
	res->items = malloc(items * sizeof(PRSS_Item));
	res->item_count = 0;

	for (n = root->children; n; n = n->next) {
		if (n->type == XML_ELEMENT_NODE &&
				!strcmp((const char *) n->name, "item")) {
			read_item(&res->items[res->item_count++], n->children);
		}
	}

	return 1;
}

void prss_parse_doc(PRSS *result, xmlDocPtr doc)
{
	/* FIXME: doc shouldn't be freed after failure when called explicitly from
	 * program! */

	xmlNodePtr root = xmlDocGetRootElement(doc);

	do {
		if (root->type == XML_ELEMENT_NODE) {
			if (!strcmp((const char *) root->name, "RDF")) {
				// RSS 1.0 document
				parse_rss_1_0(result, root);
				return;
			} else if (!strcmp((const char *) root->name, "rss")) {
				// RSS 2.0 or <1.0 document
				parse_rss_2_0(result, root);
				return;
			}
		}
		root = root->next;
	} while (root);
	return;
}
