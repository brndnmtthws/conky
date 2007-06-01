#ifndef PRSS_H
#define PRSS_H

#include <libxml/parser.h>

typedef struct PRSS_Item_ {
	char* title;
	char* link;
	char* description;
	char* category;
	char* pubdate;
} PRSS_Item;

typedef struct PRSS_ {
	xmlDocPtr _data;
	
	char* title;
	char* link;
	char* description;
	char* language;

	PRSS_Item* items;
	int item_count;
} PRSS;

/* Functions for parsing RSS-data */
PRSS* prss_parse_data(const char *xml_data);
PRSS* prss_parse_file(const char *xml_file);

/* Frees the PRSS-stucture returned by prss_parse_*.
 * The memory area pointed by data becomes invalid
 * after call to this function. */
void prss_free(PRSS* data);

#endif	// PRSS_H
