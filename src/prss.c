#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "prss.h"

#ifndef PARSE_OPTIONS
#define PARSE_OPTIONS 0
#endif

static PRSS* get_data(xmlDocPtr doc);

PRSS* prss_parse_data(const char* xml_data)
{
	xmlDocPtr doc = xmlReadMemory(xml_data, strlen(xml_data), "", NULL, PARSE_OPTIONS);
	if (!doc)
		return NULL;
	
	PRSS* data = get_data(doc);
	return data;
}
PRSS* prss_parse_file(const char* xml_file)
{
	xmlDocPtr doc = xmlReadFile(xml_file, NULL, PARSE_OPTIONS);
	if (!doc)
		return NULL;
	
	PRSS* data = get_data(doc);
	return data;
}
void prss_free(PRSS* data)
{
	xmlFreeDoc(data->_data);
	free(data->items);
}

static inline void read_item(PRSS_Item* res, xmlNodePtr data)
{
	res->title = res->link = res->description = 0;
	for(; data; data = data->next) {
		if (data->type != XML_ELEMENT_NODE)
			continue;
		xmlNodePtr child = data->children;
		if (!child)
			continue;
	
		if (!strcmp((char*)data->name, "title")) {
			res->title = (char*)child->content;
		} else if (!strcmp((char*)data->name, "link")) {
			res->link = (char*)child->content;
		} else if (!strcmp((char*)data->name, "description")) {
			res->description = (char*)child->content;
		} else if (!strcmp((char*)data->name, "category")) {
			res->category = (char*)child->content;
		} else if (!strcmp((char*)data->name, "pubDate")) {
			res->pubdate = (char*)child->content;
		}
	}
}

PRSS* get_data(xmlDocPtr doc)
{
	PRSS* result = malloc(sizeof(PRSS));
	xmlNodePtr channel = xmlDocGetRootElement(doc)->children->next;
	if (!channel) {
		fprintf(stderr, "Got root? No!\n");
		return NULL;
	}
	result->_data = doc;
	result->title = result->link = result->description = result->language = NULL;
	
	/* Get item count */
	int items = 0;
	xmlNodePtr n;
	for(n = channel->children; n; n = n->next)
		if (n->type==XML_ELEMENT_NODE && !strcmp((char*)n->name, "item"))
			++items;

	result->item_count = items;
	result->items = malloc(items*sizeof(PRSS_Item));

	int cur_item = 0;
	for(n = channel->children; n; n = n->next) {
		if (n->type != XML_ELEMENT_NODE)
			continue;
		xmlNodePtr child = n->children;
		if (!child)
			continue;
		
		if (!strcmp((char*)n->name, "title")) {
			result->title = (char*)child->content;
		} else if (!strcmp((char*)n->name, "link")) {
			result->link = (char*)child->content;
		} else if (!strcmp((char*)n->name, "description")) {
			result->description = (char*)child->content;
		} else if (!strcmp((char*)n->name, "language")) {
			result->language = (char*)child->content;
		} else if (!strcmp((char*)n->name, "item")) {
			read_item(&result->items[cur_item++], n->children);
		}
	}
	
	return result;
}
