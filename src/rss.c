/*
 * rss.c
 * RSS stuff (prss version)
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "prss.h"
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

PRSS* save = NULL;

struct MemoryStruct {
	char *memory;
	size_t size;
};

size_t
WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)data;

	mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory) {
		memcpy(&(mem->memory[mem->size]), ptr, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;
	}
	return realsize;
}


int rss_delay(int delay)
{
	static int wait = 0;
	time_t now = time(NULL);

	// make it minutes
	if(delay < 1) delay = 1;
	delay *= 60;

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
	CURL *curl = NULL;
	CURLcode res;
	struct MemoryStruct chunk;
	chunk.memory = NULL;
	chunk.size = 0;

	if(!rss_delay(delay))
		return save; // wait for delay to pass

	if(save != NULL)
		prss_free(save); // clean up old data

	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, uri);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "conky-rss/1.0");

		res = curl_easy_perform(curl);
		if(chunk.size) {
			save = prss_parse_data(chunk.memory);
			free(chunk.memory);
		}

		curl_easy_cleanup(curl);
	}

	return save;
}
