/* Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2009 Brenden Matthews, Philip Kovacs, et. al.
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

#include "conky.h"
#include "logging.h"
#include "weather.h"
#include <time.h>
#include <ctype.h>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#define MAX_LOCATIONS 3

/* Possible sky conditions */
#define NUM_CC_CODES 7
const char *CC_CODES[NUM_CC_CODES] =
  {"SKC", "CLR", "FEW", "SCT", "BKN", "OVC", "TCU"};

/* Possible weather conditions */
#define NUM_WC_CODES 17
const char *WC_CODES[NUM_WC_CODES] =
  {"DZ", "RA", "GR", "GS", "SN", "SG", "FG", "HZ", "FU", "BR", "DU", "SA",
   "FC", "PO", "SQ", "SS", "DS"};

/*
 * TODO: This could be made common with the one used in prss.c
 *
 */

struct WMemoryStruct {
	char *memory;
	size_t size;
};

typedef struct location_ {
	char *uri;
	int last_update;
	PWEATHER *data;
} location;

int num_locations = 0;
location locations[MAX_LOCATIONS];

/*
 * TODO: This could be made common with the one used in prss.c
 *
 */

size_t WWriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
	size_t realsize = size * nmemb;
	struct WMemoryStruct *mem = (struct WMemoryStruct *) data;

	mem->memory = (char *) realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory) {
		memcpy(&(mem->memory[mem->size]), ptr, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;
	}
	return realsize;
}

int weather_delay(int *last, int delay)
{
	time_t now = time(NULL);

	if ((!*last) || (now >= *last + delay)) {
		*last = now;
		return 1;
	}

	return 0;
}

void init_weather_info(void)
{
	int i;

	for (i = 0; i < MAX_LOCATIONS; i++) {
		locations[i].uri = NULL;
		locations[i].data = NULL;
		locations[i].last_update = 0;
	}
}

void free_weather_info(void)
{
	int i;

	for (i = 0; i < num_locations; i++) {
		if (locations[i].uri != NULL) {
			free(locations[i].uri);
		}
	}
}

int rel_humidity(int dew_point, int air) {
  const float a = 17.27f;
  const float b = 237.7f;

  float g = a*dew_point/(b+dew_point);
  return (int)(100.f*expf(g-a*air/(b+air)));
}

/*
 * Horrible hack to avoid using regexes
 *
 */

static inline void parse_token(PWEATHER *res, char *token) {

  int i;
  char s_tmp[64];

  //Check all tokens 2 chars long
  if (strlen(token) == 2 ) {

    //Check if token is a weather condition
    for (i=0; i<2; i++) {
      if (!isalpha(token[i])) break;
    }
    if (i==2) {
      for(i=0; i<NUM_WC_CODES; i++) {
	if (!strncmp(token, WC_CODES[i], 2)) {
	  res->wc=i+1;
	  break;
	}
      }
      return;
    }

    //Check for CB
    if (!strcmp(token, "CB")) {
      res->cc = 8;
      return;
    }

  }

  //Check all tokens 3 chars long
  if (strlen(token) == 3 ) {

    //Check if token is a modified weather condition
    if ((token[0] == '+') || (token[0] == '-')) {
      for (i=1; i<3; i++) {
	if (!isalpha(token[i])) break;
      }
      if (i==3) {
	for(i=0; i<NUM_WC_CODES; i++) {
	  if (!strncmp(&token[1], WC_CODES[i], 2)) {
	    res->wc=i+1;
	    break;
	  }
	}
	return;
      }
    }

    //Check for NCD
    if (!strcmp(token, "NCD")) {
      res->cc = 1;
      return;
    }

  }

  //Check all tokens 4 chars long
  if (strlen(token) == 4 ) {

    //Check if token is an icao
    for (i=0; i<4; i++) {
      if (!isalpha(token[i])) break;
    }
    if (i==4) return;

  }

  //Check all tokens 5 chars long
  if (strlen(token) == 5 ) {

    //Check for CAVOK
    if (!strcmp(token, "CAVOK")) {
      res->cc = 1;
      return;
    }

    //Check if token is the temperature
    for (i=0; i<2; i++) {
      if (!isdigit(token[i])) break;
    }
    if ((i==2) && (token[2] == '/')) {
      for (i=3; i<5; i++) {
	if (!isdigit(token[i])) break;
      }
      if (i==5) {
	//First 2 digits gives the air temperature
	res->tmpC=atoi(token);

	//4th and 5th digits gives the dew point temperature
	res->dew=atoi(&token[3]);

	//Compute humidity
	res->hmid = rel_humidity(res->dew, res->tmpC);

	//Convert to Fahrenheit (faster here than in conky.c)
	res->tmpF = (res->tmpC*9)/5 + 32;

	return;
      }
    }

    //Check if token is the pressure
    if ((token[0] == 'Q') || (token[0] == 'A')) {
      for (i=1; i<5; i++) {
	if (!isdigit(token[i])) break;
      }
      if (i==5) {
	if (token[0] == 'A') {
	  //Convert inches of mercury to mbar
	  res->bar = (int)(atoi(&token[1])*0.338637526f);
	  return;
	}

	//Last 4 digits is pressure im mbar
	res->bar = atoi(&token[1]);
	return;
      }
    }
  }

  //Check all tokens 6 chars long
  if (strlen(token) == 6 ) {

    //Check if token is the cloud cover
    for (i=0; i<3; i++) {
      if (!isalpha(token[i])) break;
    }
    if (i==3) {
      for (i=3; i<6; i++) {
	if (!isdigit(token[i])) break;
      }
      if (i==6) {
	//Check if first 3 digits gives the cloud cover condition
	for(i=0; i<NUM_CC_CODES; i++) {
	  if (!strncmp(token, CC_CODES[i], 3)) {
	    res->cc=i+1;
	    break;
	  }
	}
	return;
      }
    }

    //Check if token is positive temp and negative dew
    for (i=0; i<2; i++) {
      if (!isdigit(token[i])) break;
    }
    if ((i==2) && (token[2] == '/')  && (token[3] == 'M')) {
      for (i=4; i<6; i++) {
	if (!isdigit(token[i])) break;
      }
      if (i==6) {
	  //1st and 2nd digits gives the temperature
	  res->tmpC = atoi(token);

	  //5th and 6th digits gives the dew point temperature
	  res->dew = -atoi(&token[4]);

	  //Compute humidity
	  res->hmid = rel_humidity(res->dew, res->tmpC);

	  //Convert to Fahrenheit (faster here than in conky.c)
	  res->tmpF = (res->tmpC*9)/5 + 32;

	  return;
	}
    }
  }

  //Check all tokens 7 chars long
  if (strlen(token) == 7 ) {

    //Check if token is the observation time
    for (i=0; i<6; i++) {
      if (!isdigit(token[i])) break;
    }
    if ((i==6) && (token[6] == 'Z')) return;

    //Check if token is the wind speed/direction in knots
    for (i=0; i<5; i++) {
      if (!isdigit(token[i])) break;
    }
    if ((i==5) && (token[5] == 'K') &&  (token[6] == 'T')) {

      //First 3 digits are wind direction
      strncpy(s_tmp, token, 3);
      res->wind_d=atoi(s_tmp);

      //4th and 5th digit are wind speed in knots (convert to km/hr)
      res->wind_s = (int)(atoi(&token[3])*1.852);

      return;
    }

    //Check if token is negative temperature
    if ((token[0] == 'M') && (token[4] == 'M')) {
      for (i=1; i<3; i++) {
	if (!isdigit(token[i])) break;
      }
    if ((i==3) && (token[3] == '/')) {
	for (i=5; i<7; i++) {
	  if (!isdigit(token[i])) break;
	}
	if (i==7) {
	  //2nd and 3rd digits gives the temperature
	  res->tmpC = -atoi(&token[1]);

	  //6th and 7th digits gives the dew point temperature
	  res->dew = -atoi(&token[5]);

	  //Compute humidity
	  res->hmid = rel_humidity(res->dew, res->tmpC);

	  //Convert to Fahrenheit (faster here than in conky.c)
	  res->tmpF = (res->tmpC*9)/5 + 32;

	  return;
	}
      }
    }

    //Check if token is wind variability
    for (i=0; i<3; i++) {
      if (!isdigit(token[i])) break;
    }
    if ((i==3) && (token[3] == 'V')) {
      for (i=4; i<7; i++) {
	if (!isdigit(token[i])) break;
      }
      if (i==7) return;
    }

  }

  //Check all tokens 8 chars long
  if (strlen(token) == 8 ) {

    //Check if token is the wind speed/direction in m/s
    for (i=0; i<5; i++) {
      if (!isdigit(token[i])) break;
    }
    if ((i==5)&&(token[5] == 'M')&&(token[6] == 'P')&&(token[7] == 'S')) {

      //First 3 digits are wind direction
      strncpy(s_tmp, token, 3);
      res->wind_d=atoi(s_tmp);

      //4th and 5th digit are wind speed in m/s (convert to km/hr)
      res->wind_s = (int)(atoi(&token[3])*3.6);

      return;
    }

  }

  //printf("token : %s\n", token);
}

static inline PWEATHER *parse_weather(const char *data)
{
  char s_tmp[256];
  const char delim[] = " ";

  PWEATHER *res = malloc(sizeof(PWEATHER));
  memset(res, 0, sizeof(PWEATHER));

  //Divide time stamp and metar data
  if (sscanf(data, "%[^'\n']\n%[^'\n']", res->lastupd, s_tmp) == 2) {
    
    //Process all tokens
    char *p_tok = NULL;
    char *p_save = NULL;

    if ((p_tok = strtok_r(s_tmp, delim, &p_save)) != NULL) {
      do {

	parse_token(res, p_tok);
	p_tok = strtok_r(NULL, delim, &p_save);
      
      } while (p_tok != NULL);
    }
      return res;
  }
  else {
    return NULL;
  }
}

PWEATHER *get_weather_info(char *uri, int delay)
{
  CURL *curl = NULL;
  CURLcode res;

  // pointers to struct
  location *curloc = NULL;
  PWEATHER *curdata = NULL;
  int *last_update = 0;

  int i;

  // curl temps
  struct WMemoryStruct chunk;

  chunk.memory = NULL;
  chunk.size = 0;

  // first seek for the uri in list
  for (i = 0; i < num_locations; i++) {
    if (locations[i].uri != NULL) {
      if (!strcmp(locations[i].uri, uri)) {
	curloc = &locations[i];
	break;
      }
    }
  }

  if (!curloc) { // new location
    if (num_locations == MAX_LOCATIONS) {
      return NULL;
    }
    curloc = &locations[num_locations];
    curloc->uri = strndup(uri, text_buffer_size);
    num_locations++;
  }

  last_update = &curloc->last_update;
  curdata = curloc->data;

  // wait for delay to pass
  if (!weather_delay(last_update, delay)) {
    return curdata;
  }

  // clean up old data
  if (curdata != NULL) {
    free(curdata);
    curdata = NULL;
  }

  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, uri);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WWriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "conky-weather/1.0");

    res = curl_easy_perform(curl);
    if (chunk.size) {
      curdata = parse_weather(chunk.memory);
      free(chunk.memory);
    } else {
      ERR("No data from server");
    }
    
    curl_easy_cleanup(curl);
  }

  curloc->data = curdata;

  return curdata;
}
