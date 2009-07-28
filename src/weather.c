/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
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
 * vim: ts=4 sw=4 noet ai cindent syntax=c
 *
 */

/*
 * TODO: Add weather forecast info from weather.com
 *
 */

#include "conky.h"
#include "logging.h"
#include "weather.h"
#include "temphelper.h"
#include "ccurl_thread.h"
#include <time.h>
#include <ctype.h>
#ifdef MATH
#include <math.h>
#endif /* MATH */
#ifdef XOAP
#include <libxml/parser.h>
#include <libxml/xpath.h>

/* Xpath expressions for XOAP xml parsing */
#define NUM_XPATH_EXPRESSIONS 7
const char *xpath_expression[NUM_XPATH_EXPRESSIONS] = {
	"/weather/cc/lsup", "/weather/cc/tmp", "/weather/cc/t",
	"/weather/cc/bar/r", "/weather/cc/wind/s", "/weather/cc/wind/d",
	"/weather/cc/hmid"
};
#endif /* XOAP */

/* Possible sky conditions */
#define NUM_CC_CODES 6
const char *CC_CODES[NUM_CC_CODES] = {
	"SKC", "CLR", "FEW", "SCT", "BKN", "OVC"
};

/* Possible weather modifiers */
#define NUM_WM_CODES 9
const char *WM_CODES[NUM_WM_CODES] = {
	"VC", "MI", "BC", "PR", "TS", "BL",
	"SH", "DR", "FZ"
};

/* Possible weather conditions */
#define NUM_WC_CODES 17
const char *WC_CODES[NUM_WC_CODES] = {
	"DZ", "RA", "GR", "GS", "SN", "SG",
	"FG", "HZ", "FU", "BR", "DU", "SA",
	"FC", "PO", "SQ", "SS", "DS"
};

static ccurl_location_t *locations_head = 0;

void weather_free_info(void)
{
	ccurl_free_locations(&locations_head);
}

int rel_humidity(int dew_point, int air) {
	const float a = 17.27f;
	const float b = 237.7f;

	float diff = a*(dew_point/(b+dew_point)-air/(b+air));
#ifdef MATH
	return (int)(100.f*expf(diff));
#else
	return (int)(16.666667163372f*(6.f+diff*(6.f+diff*(3.f+diff))));
#endif /* MATH */
}

#ifdef XOAP
static void parse_cc(PWEATHER *res, xmlXPathContextPtr xpathCtx)
{
	int i;
	char *content;
	xmlXPathObjectPtr xpathObj;

	for (i = 0; i < NUM_XPATH_EXPRESSIONS; i++) {
	  xpathObj = xmlXPathEvalExpression((xmlChar *)xpath_expression[i], xpathCtx);
		if ((xpathObj != NULL) && (xpathObj->nodesetval->nodeTab[0]->type == XML_ELEMENT_NODE)) {
		  content = (char *)xmlNodeGetContent(xpathObj->nodesetval->nodeTab[0]);
		  switch(i) {
		       case 0:
			    strncpy(res->lastupd, content, 31);
		       break;
		       case 1:
			    res->temp = atoi(content);
		       break;
		       case 2:
			    if(res->xoap_t[0] == '\0') {
				strncpy(res->xoap_t, content, 31);
				}
		       break;
		       case 3:
			    res->bar = atoi(content);
		       break;
		       case 4:
			    res->wind_s = atoi(content);
		       break;
		       case 5:
			    if (isdigit((char)content[0])) {
			        res->wind_d = atoi(content);
			    }
			    break;
		       case 6:
			    res->hmid = atoi(content);
		  }
		  xmlFree(content);
		}
		xmlXPathFreeObject(xpathObj);
	}
	return;
}

static void parse_weather_xml(PWEATHER *res, const char *data)
{
	xmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;

	if (!(doc = xmlReadMemory(data, strlen(data), "", NULL, 0))) {
		ERR("weather: can't read xml data");
		return;
	}

	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL) {
	        ERR("weather: unable to create new XPath context");
		xmlFreeDoc(doc);
		return;
	}

	parse_cc(res, xpathCtx);
	xmlXPathFreeContext(xpathCtx);
	xmlFreeDoc(doc);
	return;
}
#endif /* XOAP */

/*
 * Horrible hack to avoid using regexes
 *
 */

static inline void parse_token(PWEATHER *res, char *token) {

	int i;
	char s_tmp[64];

	switch (strlen(token)) {

		//Check all tokens 2 chars long
		case 2:

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

			break;

			//Check all tokens 3 chars long
		case 3:

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

			//Check for NCD or NSC
			if ((!strcmp(token, "NCD")) || (!strcmp(token, "NSC"))) {
				res->cc = 1;
				return;
			}

			//Check for TCU
			if (!strcmp(token, "TCU")) {
				res->cc = 7;
				return;
			}

			break;

			//Check all tokens 4 chars long
		case 4:

			//Check if token is a modified weather condition
			for(i=0; i<NUM_WM_CODES; i++) {
				if (!strncmp(token, WM_CODES[i], 2)) {
					for(i=0; i<NUM_WC_CODES; i++) {
						if (!strncmp(&token[2], WC_CODES[i], 2)) {
							res->wc=i+1;
							return;
						}
					}
					break;
				}
			}

			break;

			//Check all tokens 5 chars long
		case 5:

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
					res->temp=atoi(token);

					//4th and 5th digits gives the dew point temperature
					res->dew=atoi(&token[3]);

					//Compute humidity
					res->hmid = rel_humidity(res->dew, res->temp);

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

			//Check if token is a modified weather condition
			if ((token[0] == '+') || (token[0] == '-')) {
				for(i=0; i<NUM_WM_CODES; i++) {
					if (!strncmp(&token[1], WM_CODES[i], 2)) {
						for(i=0; i<NUM_WC_CODES; i++) {
							if (!strncmp(&token[3], WC_CODES[i], 2)) {
								res->wc=i+1;
								return;
							}
						}
						break;
					}
				}
			}
			break;

			//Check all tokens 6 chars long
		case 6:

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
					res->temp = atoi(token);

					//5th and 6th digits gives the dew point temperature
					res->dew = -atoi(&token[4]);

					//Compute humidity
					res->hmid = rel_humidity(res->dew, res->temp);

					return;
				}
			}

			break;

			//Check all tokens 7 chars long
		case 7:

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
				s_tmp[3]='\0';
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
						res->temp = -atoi(&token[1]);

						//6th and 7th digits gives the dew point temperature
						res->dew = -atoi(&token[5]);

						//Compute humidity
						res->hmid = rel_humidity(res->dew, res->temp);

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

			break;

			//Check all tokens 8 chars long
		case 8:

			//Check if token is the wind speed/direction in m/s
			for (i=0; i<5; i++) {
				if (!isdigit(token[i])) break;
			}
			if ((i==5)&&(token[5] == 'M')&&(token[6] == 'P')&&(token[7] == 'S')) {

				//First 3 digits are wind direction
				strncpy(s_tmp, token, 3);
				s_tmp[3]='\0';
				res->wind_d=atoi(s_tmp);

				//4th and 5th digit are wind speed in m/s (convert to km/hr)
				res->wind_s = (int)(atoi(&token[3])*3.6);

				return;
			}

		default:

			//printf("token : %s\n", token);
			break;
	}
}

void parse_weather(void *result, const char *data)
{
	PWEATHER *res = (PWEATHER*)result;
	/* Reset results */
	memset(res, 0, sizeof(PWEATHER));

#ifdef XOAP
	//Check if it is an xml file
	if ( strncmp(data, "<?xml ", 6) == 0 ) {
		parse_weather_xml(res, data);
	} else
#endif /* XOAP */
	{
		//We assume its a text file
		char s_tmp[256];
		const char delim[] = " ";

		//Divide time stamp and metar data
		if (sscanf(data, "%[^'\n']\n%[^'\n']", res->lastupd, s_tmp) == 2) {

			//Process all tokens
			char *p_tok = NULL;
			char *p_save = NULL;

			if ((strtok_r(s_tmp, delim, &p_save)) != NULL) {

				//Jump first token, must be icao
				p_tok = strtok_r(NULL, delim, &p_save);

				do {

					parse_token(res, p_tok);
					p_tok = strtok_r(NULL, delim, &p_save);

				} while (p_tok != NULL);
			}
			return;
		}
		else {
			return;
		}
	}
}

void weather_process_info(char *p, int p_max_size, char *uri, char *data_type, int interval)
{
	static const char *wc[] = {
		"", "drizzle", "rain", "hail", "soft hail",
		"snow", "snow grains", "fog", "haze", "smoke",
		"mist", "dust", "sand", "funnel cloud tornado",
		"dust/sand", "squall", "sand storm", "dust storm"
	};
	PWEATHER *data;

	ccurl_location_t *curloc = ccurl_find_location(&locations_head, uri);
	if (!curloc->p_timed_thread) {
		curloc->result = malloc(sizeof(PWEATHER));
		memset(curloc->result, 0, sizeof(PWEATHER));
		curloc->process_function = &parse_weather;
		ccurl_init_thread(curloc, interval);
		if (!curloc->p_timed_thread) {
			ERR("error setting up weather thread");
		}
	}

	timed_thread_lock(curloc->p_timed_thread);
	data = (PWEATHER*)curloc->result;
	if (strcmp(data_type, "last_update") == EQUAL) {
		strncpy(p, data->lastupd, p_max_size);
	} else if (strcmp(data_type, "temperature") == EQUAL) {
		temp_print(p, p_max_size, data->temp, TEMP_CELSIUS);
	} else if (strcmp(data_type, "cloud_cover") == EQUAL) {
#ifdef XOAP
		if (data->xoap_t[0] != '\0') {
			strncpy(p, data->xoap_t, p_max_size);
		} else
#endif /* XOAP */
			if (data->cc == 0) {
				strncpy(p, "", p_max_size);
			} else if (data->cc < 3) {
				strncpy(p, "clear", p_max_size);
			} else if (data->cc < 5) {
				strncpy(p, "partly cloudy", p_max_size);
			} else if (data->cc == 5) {
				strncpy(p, "cloudy", p_max_size);
			} else if (data->cc == 6) {
				strncpy(p, "overcast", p_max_size);
			} else if (data->cc == 7) {
				strncpy(p, "towering cumulus", p_max_size);
			} else  {
				strncpy(p, "cumulonimbus", p_max_size);
			}
	} else if (strcmp(data_type, "pressure") == EQUAL) {
		snprintf(p, p_max_size, "%d", data->bar);
	} else if (strcmp(data_type, "wind_speed") == EQUAL) {
		snprintf(p, p_max_size, "%d", data->wind_s);
	} else if (strcmp(data_type, "wind_dir") == EQUAL) {
		if ((data->wind_d >= 349) || (data->wind_d < 12)) {
			strncpy(p, "N", p_max_size);
		} else if (data->wind_d < 33) {
			strncpy(p, "NNE", p_max_size);
		} else if (data->wind_d < 57) {
			strncpy(p, "NE", p_max_size);
		} else if (data->wind_d < 79) {
			strncpy(p, "ENE", p_max_size);
		} else if (data->wind_d < 102) {
			strncpy(p, "E", p_max_size);
		} else if (data->wind_d < 124) {
			strncpy(p, "ESE", p_max_size);
		} else if (data->wind_d < 147) {
			strncpy(p, "SE", p_max_size);
		} else if (data->wind_d < 169) {
			strncpy(p, "SSE", p_max_size);
		} else if (data->wind_d < 192) {
			strncpy(p, "S", p_max_size);
		} else if (data->wind_d < 214) {
			strncpy(p, "SSW", p_max_size);
		} else if (data->wind_d < 237) {
			strncpy(p, "SW", p_max_size);
		} else if (data->wind_d < 259) {
			strncpy(p, "WSW", p_max_size);
		} else if (data->wind_d < 282) {
			strncpy(p, "W", p_max_size);
		} else if (data->wind_d < 304) {
			strncpy(p, "WNW", p_max_size);
		} else if (data->wind_d < 327) {
			strncpy(p, "NW", p_max_size);
		} else if (data->wind_d < 349) {
			strncpy(p, "NNW", p_max_size);
		};
	} else if (strcmp(data_type, "wind_dir_DEG") == EQUAL) {
		snprintf(p, p_max_size, "%d", data->wind_d);

	} else if (strcmp(data_type, "humidity") == EQUAL) {
		snprintf(p, p_max_size, "%d", data->hmid);
	} else if (strcmp(data_type, "weather") == EQUAL) {
		strncpy(p, wc[data->wc], p_max_size);
	}

	timed_thread_unlock(curloc->p_timed_thread);
}

#ifdef XOAP

/* xoap suffix for weather from weather.com */
static char *xoap = NULL;

/*
 * TODO: make the xoap keys file readable from the config file
 *       make the keys directly readable from the config file
 *       make the xoap keys file giveable as a command line option
 */
void load_xoap_keys(void)
{
	FILE *fp;
	char *par = (char *) malloc(11 * sizeof(char));
	char *key = (char *) malloc(17 * sizeof(char));

	xoap = (char *) malloc(64 * sizeof(char));
	to_real_path(xoap, XOAP_FILE);
	fp = fopen(xoap, "r");
	if (fp != NULL) {
		if (fscanf(fp, "%10s %16s", par, key) == 2) {
			strcpy(xoap, "?cc=*&link=xoap&prod=xoap&par=");
			strcat(xoap, par);
			strcat(xoap, "&key=");
			strcat(xoap, key);
			strcat(xoap, "&unit=m");
		} else {
			free(xoap);
			xoap = NULL;
		}
		fclose(fp);
	} else {
		free(xoap);
		xoap = NULL;
	}
	free(par);
	free(key);
}
#endif /* XOAP */

int process_weather_uri(char *uri, char *locID)
{
	/* locID MUST BE upper-case */
	char *tmp_p = locID;
	while (*tmp_p) {
		*tmp_p = toupper(*tmp_p);
		tmp_p++;
	}

	/* Construct complete uri */
#ifdef XOAP
	if (strstr(uri, "xoap.weather.com")) {
		if (xoap != NULL) {
			strcat(uri, locID);
			strcat(uri, xoap);
		} else {
			free(uri);
			uri = NULL;
		}
	} else 
#endif /* XOAP */
	if (strstr(uri, "weather.noaa.gov")) {
		strcat(uri, locID);
		strcat(uri, ".TXT");
	} else  if (!strstr(uri, "localhost") && !strstr(uri, "127.0.0.1")) {
		return -1;
	}
	return 0;
}

