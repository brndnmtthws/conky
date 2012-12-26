/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
 *
 * Conky, a system monitor, based on torsmo
 *
 * Copyright (c) 2008 Asbjørn Zweidorff Kjær
 * Copyright (c) 2005-2012 Brenden Matthews, Philip Kovacs, et. al.
 *	(see AUTHORS)
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "eve.h"
#include "config.h"
#include "logging.h"
#include "text_object.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlwriter.h>

#include <curl/curl.h>
#include <curl/easy.h>

#include <time.h>

#include "conky.h"

#define MAXCHARS 4
#define EVE_UPDATE_DELAY 60

typedef struct {
	char *charid;
	char *skillname;
	char *time;
	char *lastOutput;

	struct tm ends;
	struct tm cache;

	time_t delay;

	int level;
	int skill;
} Character;

struct xmlData {
	char *data;
	size_t size;
};

struct eve_data {
	char apikey[65];
	char charid[21];
	char userid[21];
};

int num_chars = 0;
Character eveCharacters[MAXCHARS];

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	size_t realsize = 0;
	struct xmlData *data = 0;
	data = (struct xmlData *)stream;
	realsize = size * nmemb;

	data->data = (char *)realloc(data->data, data->size + realsize + 1);
	if (data->data) {
		memcpy(&(data->data[data->size]), ptr, realsize);
		data->size += realsize;
		data->data[data->size] = '\0';
	}

	return realsize;
}

int parseTrainingXml(char *data, Character * s)
{
	char *skill, *level, *ends, *cache;
	xmlNodePtr n;
	xmlDocPtr doc = 0;
	xmlNodePtr root = 0;
	struct tm end_tm, cache_tm;

	if (!data)
		return 1;

	doc = xmlReadMemory(data, strlen(data), "", NULL, 0);
	root = xmlDocGetRootElement(doc);
	for (n = root->children; n; n = n->next) {
		if (n->type == XML_ELEMENT_NODE) {
			if (!strcasecmp((const char *)n->name, "error")) {
				return 1;
			} else if (!strcasecmp((const char *)n->name, "result")) {
				xmlNodePtr c;
				for (c = n->children; c; c = c->next) {
					if (!strcasecmp((const char *)c->name, "trainingEndTime")) {
						ends = (char *)c->children->content;
					} else if (!strcasecmp((const char *)c->name, "trainingTypeID")) {
						if (c->children->content)
							skill = (char *)c->children->content;
					} else if (!strcasecmp((const char *)c->name, "trainingToLevel")) {
						level = (char *)c->children->content;
					}
				}
			} else if (!strcasecmp((const char *)n->name, "cachedUntil")) {
				cache = (char *)n->children->content;
			}
		}
	}

	strptime(ends, "%Y-%m-%d %H:%M:%S", &end_tm);
	strptime(cache, "%Y-%m-%d %H:%M:%S", &cache_tm);
	s->skill = atoi(skill);
	s->level = atoi(level);
	s->ends = end_tm;
	s->cache = cache_tm;

	xmlFreeDoc(doc);
	return 0;
}

static char *getXmlFromAPI(const char *userid, const char *apikey, const char *charid, const char *url)
{
	struct curl_httppost *post = NULL;
	struct curl_httppost *last = NULL;
	struct xmlData chr;
	char *content;
	CURL *curl_handle;
	int rc = 0;

	chr.data = NULL;
	chr.size = 0;

	curl_handle = curl_easy_init();
	curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chr);

	if (userid != NULL && apikey != NULL && charid != NULL) {
		curl_formadd(&post, &last, CURLFORM_COPYNAME, "userID", CURLFORM_COPYCONTENTS, userid, CURLFORM_END);
		curl_formadd(&post, &last, CURLFORM_COPYNAME, "apiKey", CURLFORM_COPYCONTENTS, apikey, CURLFORM_END);
		curl_formadd(&post, &last, CURLFORM_COPYNAME, "characterID", CURLFORM_COPYCONTENTS, charid, CURLFORM_END);

		curl_easy_setopt(curl_handle, CURLOPT_HTTPPOST, post);
	}

	if ((rc = curl_easy_perform(curl_handle)) != CURLE_OK) {
		return NULL;
	}

	content = strdup(chr.data);
	curl_easy_cleanup(curl_handle);

	return content;
}

static void init_eve(void)
{
	int i;

	for (i = 0; i < MAXCHARS; i++) {
		eveCharacters[i].charid = NULL;
		eveCharacters[i].skillname = NULL;
		eveCharacters[i].time = NULL;
		eveCharacters[i].level = 0;
		eveCharacters[i].skill = 0;
		eveCharacters[i].delay = 0;
	}
}

static int isCacheValid(struct tm cached)
{
	struct timeval tv;
	struct timezone tz;
	double offset = 0;
	time_t now = 0;
	time_t cache = 0;
	double diff = 0;

	gettimeofday(&tv, &tz);
	offset = (double)(tz.tz_minuteswest * 60);
	now = time(NULL);
	cache = mktime(&cached);
	diff = difftime(cache, now);

	if (diff < offset)
		return 0;
	else
		return 1;
}

static char *formatTime(struct tm *ends)
{
	struct timeval tv;
	struct timezone tz;
	double offset = 0;
	time_t now = 0;
	time_t tEnds = 0;
	long lin = 0;
	long lie = 0;
	long diff = 0;
	
	gettimeofday(&tv, &tz);
	offset = (double)(tz.tz_minuteswest * 60);
	now = time(NULL);
	tEnds = mktime(ends);
	lin = (long)now;
	lin += (long)offset;
	lie = (long)tEnds;
	diff = (lie - lin);

	if (diff > 0) {
		int days = (int)(diff / 60 / 60 / 24);
		int hours = (int)((diff / 60 / 60) - (days * 24));
		int minutes = (int)((diff / 60) - ((hours * 60) + (days * 60 * 24)));
		int seconds = (int)(diff - ((minutes * 60) + (hours * 60 * 60) + (days * 60 * 60 * 24)));
		char *output = (char*) malloc(100 * sizeof(char));

		if (days > 0)
			sprintf(output, "%dd, %dh, %02dm and %02ds", days, hours, minutes, seconds);
		else if (hours > 0)
			sprintf(output, "%dh, %02dm and %02ds", hours, minutes, seconds);
		else
			sprintf(output, "%02dm and %02ds", minutes, seconds);

		return output;
	} else {
		char *output = strdup("Done");
		return output;
	}
}

static void writeSkilltree(char *content, const char *filename)
{
	FILE *fp = fopen(filename, "w");
	if (fwrite(content, sizeof(char), strlen(content), fp) < strlen(content))
		NORM_ERR("skill tree write failed");
	fclose(fp);
}

static char *getSkillname(const char *file, int skillid)
{
	char *skilltree;
	char *skill = NULL;
	xmlNodePtr n;
	xmlDocPtr doc = 0;
	xmlNodePtr root = 0;

	skilltree = getXmlFromAPI(NULL, NULL, NULL, EVEURL_SKILLTREE);
	writeSkilltree(skilltree, file);
	free(skilltree);

	doc = xmlReadFile(file, NULL, 0);
	unlink(file);
	if (!doc)
		return NULL;

	root = xmlDocGetRootElement(doc);

	for (n = root->children; n; n = n->next) {
		xmlNodePtr o;
		for (o = n->children; o; o = o->next) {
			xmlNodePtr p;
			for (p = o->children; p; p = p->next) {
				xmlNodePtr q;
				for (q = p->children; q; q = q->next) {
					xmlNodePtr r;
					for (r = q->children; r; r = r->next) {
						xmlElementPtr ele = (xmlElementPtr) r;
						xmlAttrPtr attr = (xmlAttrPtr) ele->attributes;
						char *mySkill = NULL;
						int id;

						while (attr != NULL) {
							if (!strcasecmp((const char *)attr->name, "typeName")) {
								mySkill = strdup((const char *)attr->children->content);
							} else if (!strcasecmp((const char *)attr->name, "typeID")) {
								id = atoi((const char *)attr->children->content);
							}
							attr = attr->next;
						}

						if (id == skillid) {
							skill = mySkill;
							goto END;
						}

						free(mySkill);
					}
				}
			}
		}
	}
      END:
	xmlFreeDoc(doc);

	return skill;
}

static char *eve(char *userid, char *apikey, char *charid)
{
	Character *chr = NULL;
	char skillfile[] = "/tmp/.cesfXXXXXX";
	int i = 0;
	char *output = 0;
	char *timel = 0;
	char *skill = 0;
	char *content = 0;
	time_t now = 0;
	char *error = 0;
	int tmp_fd, old_umask;


	for (i = 0; i < MAXCHARS; i++) {
		if (eveCharacters[i].charid != NULL) {
			if (strcasecmp(eveCharacters[i].charid, charid) == 0) {
				chr = &eveCharacters[i];
				break;
			}
		}
	}

	if (!chr) {
		if (num_chars == MAXCHARS - 1)
			return NULL;
		chr = &eveCharacters[num_chars];
		chr->charid = strdup(charid);
		num_chars++;
	}

	if (chr->delay > 0) {
		now = time(NULL);
		if (now < chr->delay) {
			output = strdup("Server error");
			return output;
		} else
			chr->delay = 0;
	}

	if (isCacheValid(chr->cache)) {
		output = (char *)malloc(200 * sizeof(char));
		timel = strdup(formatTime(&chr->ends));
		sprintf(output, EVE_OUTPUT_FORMAT, chr->skillname, chr->level, timel);
		free(timel);
		return output;
	} else {
		content = getXmlFromAPI(userid, apikey, charid, EVEURL_TRAINING);
		if (content == NULL) {
			error = strdup("Server error");
			now = time(NULL);
			now += (time_t) 1800;
			chr->delay = now;
			return error;
		}

		if (parseTrainingXml(content, chr)) {
			output = strdup("API error");
			return output;
		}

		timel = formatTime(&chr->ends);
		old_umask = umask(0066);
		tmp_fd = mkstemp(skillfile);
		umask(old_umask);
		if (tmp_fd == -1) {
			error = strdup("Cannot create temporary file");
			return error;
		}
		close(tmp_fd);
		skill = getSkillname(skillfile, chr->skill);

		chr->skillname = strdup(skill);

		output = (char *)malloc(200 * sizeof(char));
		sprintf(output, EVE_OUTPUT_FORMAT, chr->skillname, chr->level, timel);
		free(skill);
		return output;
	}

}

void scan_eve(struct text_object *obj, const char *arg)
{
	int argc;
	struct eve_data *ed;

	ed = (struct eve_data *) malloc(sizeof(struct eve_data));
	memset(ed, 0, sizeof(struct eve_data));

	argc = sscanf(arg, "%20s %64s %20s", ed->userid, ed->apikey, ed->charid);

	init_eve();
	obj->data.opaque = ed;
}

void print_eve(struct text_object *obj, char *p, int p_max_size)
{
	struct eve_data *ed = (struct eve_data *) obj->data.opaque;

	if (!ed)
		return;

	snprintf(p, p_max_size, "%s", eve(ed->userid, ed->apikey, ed->charid));
}

void free_eve(struct text_object *obj)
{
	free_and_zero(obj->data.opaque);
}
