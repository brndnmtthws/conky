/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2010 Brenden Matthews, Philip Kovacs, et. al.
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

#include "config.h"
#include "conky.h"
#include "logging.h"
#include "weather.h"
#include "temphelper.h"
#include "text_object.h"
#include "ccurl_thread.h"
#include <time.h>
#include <ctype.h>
#include <mutex>
#ifdef MATH
#include <math.h>
#endif /* MATH */
#ifdef BUILD_WEATHER_XOAP
#include <libxml/parser.h>
#include <libxml/xpath.h>
#endif /* BUILD_WEATHER_XOAP */

/* WEATHER data */
typedef struct PWEATHER_ {
	char lastupd[32];
#ifdef BUILD_WEATHER_XOAP
	char xoap_t[32];
	char icon[3];
#endif /* BUILD_WEATHER_XOAP */
	int temp;
	int dew;
	int cc;
	int bar;
	int wind_s;
	int wind_d;
	int hmid;
	int wc;
} PWEATHER;

#ifdef BUILD_WEATHER_XOAP
#define FORECAST_DAYS 5
typedef struct PWEATHER_FORECAST_ {
	int hi[FORECAST_DAYS];
	int low[FORECAST_DAYS];
	char icon[FORECAST_DAYS][3];
	char xoap_t[FORECAST_DAYS][32];
	char day[FORECAST_DAYS][9];
	char date[FORECAST_DAYS][7];
	int wind_s[FORECAST_DAYS];
	int wind_d[FORECAST_DAYS];
	int hmid[FORECAST_DAYS];
	int ppcp[FORECAST_DAYS];
} PWEATHER_FORECAST;

/* Xpath expressions for BUILD_WEATHER_XOAP xml parsing */
#define NUM_XPATH_EXPRESSIONS_CC 8
const char *xpath_expression_cc[NUM_XPATH_EXPRESSIONS_CC] = {
	"/weather/cc/lsup", "/weather/cc/tmp", "/weather/cc/t",
	"/weather/cc/bar/r", "/weather/cc/wind/s", "/weather/cc/wind/d",
	"/weather/cc/hmid", "/weather/cc/icon"
};

#define NUM_XPATH_EXPRESSIONS_DF 10
const char *xpath_expression_df[NUM_XPATH_EXPRESSIONS_DF] = {
	"/weather/dayf/day[*]/hi", "/weather/dayf/day[*]/low",
	"/weather/dayf/day[*]/part[1]/icon", "/weather/dayf/day[*]/part[1]/t",
	"/weather/dayf/day[*]/part[1]/wind/s","/weather/dayf/day[*]/part[1]/wind/d",
	"/weather/dayf/day[*]/part[1]/ppcp", "/weather/dayf/day[*]/part[1]/hmid",
	"/weather/dayf/day[*]/@t", "/weather/dayf/day[*]/@dt"
};
#endif /* BUILD_WEATHER_XOAP */

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

static ccurl_location_list locations_cc;
#ifdef BUILD_WEATHER_XOAP
static ccurl_location_list locations_df;
#endif

struct weather_data {
	char uri[128];
	char data_type[32];
	int interval;
};

#ifdef BUILD_WEATHER_XOAP
struct weather_forecast_data {
	char uri[128];
	unsigned int day;
	char data_type[32];
	int interval;
};
#endif

void weather_free_info(void)
{
	ccurl_free_locations(locations_cc);
#ifdef BUILD_WEATHER_XOAP
	ccurl_free_locations(locations_df);
#endif
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

#ifdef BUILD_WEATHER_XOAP
static void parse_df(PWEATHER_FORECAST *res, xmlXPathContextPtr xpathCtx)
{
	int i, j, k;
	char *content = NULL;
	xmlXPathObjectPtr xpathObj;

	xpathObj = xmlXPathEvalExpression((const xmlChar *)"/error/err", xpathCtx);
	if (xpathObj && xpathObj->nodesetval && xpathObj->nodesetval->nodeNr > 0 &&
			xpathObj->nodesetval->nodeTab[0]->type == XML_ELEMENT_NODE) {
		content = (char *)xmlNodeGetContent(xpathObj->nodesetval->nodeTab[0]);
		NORM_ERR("XOAP error: %s", content);
		xmlFree(content);
		xmlXPathFreeObject(xpathObj);
		return;
	}
	xmlXPathFreeObject(xpathObj);

	for (i = 0; i < NUM_XPATH_EXPRESSIONS_DF; i++) {
		xpathObj = xmlXPathEvalExpression((const xmlChar *)xpath_expression_df[i], xpathCtx);
		if (xpathObj != NULL) {
			xmlNodeSetPtr nodes = xpathObj->nodesetval;
			k = 0;
			for (j = 0; j < nodes->nodeNr; ++j) {
				if (nodes->nodeTab[j]->type == XML_ELEMENT_NODE) {
					content = (char *)xmlNodeGetContent(nodes->nodeTab[k]);
					switch(i) {
					case 0:
						res->hi[k] = atoi(content);
						break;
					case 1:
						res->low[k] = atoi(content);
						break;
					case 2:
						strncpy(res->icon[k], content, 2);
					case 3:
						strncpy(res->xoap_t[k], content, 31);
						break;
					case 4:
						res->wind_s[k] = atoi(content);
						break;
					case 5:
						res->wind_d[k] = atoi(content);
						break;
					case 6:
						res->ppcp[k] = atoi(content);
						break;
					case 7:
						res->hmid[k] = atoi(content);
					}
				} else if (nodes->nodeTab[j]->type == XML_ATTRIBUTE_NODE) {
					content = (char *)xmlNodeGetContent(nodes->nodeTab[k]);
					switch(i) {
					case 8:
						strncpy(res->day[k], content, 8);
						break;
					case 9:
						strncpy(res->date[k], content, 6);
					}
				}
				xmlFree(content);
				if (++k == FORECAST_DAYS) break;
			}
		}
		xmlXPathFreeObject(xpathObj);
	}
	return;
}

static void parse_weather_forecast_xml(PWEATHER_FORECAST *res, const char *data)
{
	xmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;

	if (!(doc = xmlReadMemory(data, strlen(data), "", NULL, 0))) {
		NORM_ERR("weather_forecast: can't read xml data");
		return;
	}

	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL) {
	        NORM_ERR("weather_forecast: unable to create new XPath context");
		xmlFreeDoc(doc);
		return;
	}

	parse_df(res, xpathCtx);
	xmlXPathFreeContext(xpathCtx);
	xmlFreeDoc(doc);
	return;
}

static void parse_cc(PWEATHER *res, xmlXPathContextPtr xpathCtx)
{
	int i;
	char *content;
	xmlXPathObjectPtr xpathObj;

	xpathObj = xmlXPathEvalExpression((const xmlChar *)"/error/err", xpathCtx);
	if (xpathObj && xpathObj->nodesetval && xpathObj->nodesetval->nodeNr > 0 &&
			xpathObj->nodesetval->nodeTab[0]->type == XML_ELEMENT_NODE) {
		content = (char *)xmlNodeGetContent(xpathObj->nodesetval->nodeTab[0]);
		NORM_ERR("XOAP error: %s", content);
		xmlFree(content);
		xmlXPathFreeObject(xpathObj);
		return;
	}
	xmlXPathFreeObject(xpathObj);

	for (i = 0; i < NUM_XPATH_EXPRESSIONS_CC; i++) {
		xpathObj = xmlXPathEvalExpression((const xmlChar *)xpath_expression_cc[i], xpathCtx);
		if (xpathObj && xpathObj->nodesetval && xpathObj->nodesetval->nodeNr >0 &&
				xpathObj->nodesetval->nodeTab[0]->type ==
				XML_ELEMENT_NODE) {
			content = (char *)xmlNodeGetContent(xpathObj->nodesetval->nodeTab[0]);
			switch(i) {
				case 0:
					strncpy(res->lastupd, content, 31);
					break;
				case 1:
					res->temp = atoi(content);
					break;
				case 2:
					strncpy(res->xoap_t, content, 31);
					break;
				case 3:
					res->bar = atoi(content);
					break;
				case 4:
					res->wind_s = atoi(content);
					break;
				case 5:
					res->wind_d = atoi(content);
					break;
				case 6:
					res->hmid = atoi(content);
					break;
				case 7:
					strncpy(res->icon, content, 2);
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
		NORM_ERR("weather: can't read xml data");
		return;
	}

	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL) {
	        NORM_ERR("weather: unable to create new XPath context");
		xmlFreeDoc(doc);
		return;
	}

	parse_cc(res, xpathCtx);
	xmlXPathFreeContext(xpathCtx);
	xmlFreeDoc(doc);
	return;
}
#endif /* BUILD_WEATHER_XOAP */

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

#ifdef BUILD_WEATHER_XOAP
void parse_weather_forecast(void *result, const char *data)
{
	PWEATHER_FORECAST *res = (PWEATHER_FORECAST*)result;
	/* Reset results */
	memset(res, 0, sizeof(PWEATHER_FORECAST));

	//Check if it is an xml file
	if ( strncmp(data, "<?xml ", 6) == 0 ) {
		parse_weather_forecast_xml(res, data);
	}
}
#endif /* BUILD_WEATHER_XOAP */

void parse_weather(void *result, const char *data)
{
	PWEATHER *res = (PWEATHER*)result;
	/* Reset results */
	memset(res, 0, sizeof(PWEATHER));

#ifdef BUILD_WEATHER_XOAP
	//Check if it is an xml file
	if ( strncmp(data, "<?xml ", 6) == 0 ) {
		parse_weather_xml(res, data);
	} else
#endif /* BUILD_WEATHER_XOAP */
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

void wind_deg_to_dir(char *p, int p_max_size, int wind_deg) {
	if ((wind_deg >= 349) || (wind_deg < 12)) {
		strncpy(p, "N", p_max_size);
	} else if (wind_deg < 33) {
		strncpy(p, "NNE", p_max_size);
	} else if (wind_deg < 57) {
		strncpy(p, "NE", p_max_size);
	} else if (wind_deg < 79) {
		strncpy(p, "ENE", p_max_size);
	} else if (wind_deg < 102) {
		strncpy(p, "E", p_max_size);
	} else if (wind_deg < 124) {
		strncpy(p, "ESE", p_max_size);
	} else if (wind_deg < 147) {
		strncpy(p, "SE", p_max_size);
	} else if (wind_deg < 169) {
		strncpy(p, "SSE", p_max_size);
	} else if (wind_deg < 192) {
		strncpy(p, "S", p_max_size);
	} else if (wind_deg < 214) {
		strncpy(p, "SSW", p_max_size);
	} else if (wind_deg < 237) {
		strncpy(p, "SW", p_max_size);
	} else if (wind_deg < 259) {
		strncpy(p, "WSW", p_max_size);
	} else if (wind_deg < 282) {
			strncpy(p, "W", p_max_size);
	} else if (wind_deg < 304) {
		strncpy(p, "WNW", p_max_size);
	} else if (wind_deg < 327) {
		strncpy(p, "NW", p_max_size);
	} else if (wind_deg < 349) {
		strncpy(p, "NNW", p_max_size);
	};
}

#ifdef BUILD_WEATHER_XOAP
static void weather_forecast_process_info(char *p, int p_max_size, char *uri, unsigned int day, char *data_type, int interval)
{
	PWEATHER_FORECAST *data;

	ccurl_location_ptr curloc = ccurl_find_location(locations_df, uri);
	if (!curloc->p_timed_thread) {
		curloc->result = (char*)malloc(sizeof(PWEATHER_FORECAST));
		memset(curloc->result, 0, sizeof(PWEATHER_FORECAST));
		curloc->process_function = std::bind(parse_weather_forecast,
				std::placeholders::_1, std::placeholders::_2);
		ccurl_init_thread(curloc, interval);
		if (!curloc->p_timed_thread) {
			NORM_ERR("error setting up weather_forecast thread");
		}
	}

	std::lock_guard<std::mutex> lock(curloc->p_timed_thread->mutex());
	data = (PWEATHER_FORECAST*)curloc->result;
	if (strcmp(data_type, "hi") == EQUAL) {
		temp_print(p, p_max_size, data->hi[day], TEMP_CELSIUS);
	} else if (strcmp(data_type, "low") == EQUAL) {
		temp_print(p, p_max_size, data->low[day], TEMP_CELSIUS);
	} else if (strcmp(data_type, "icon") == EQUAL) {
		strncpy(p, data->icon[day], p_max_size);
	} else if (strcmp(data_type, "forecast") == EQUAL) {
		strncpy(p, data->xoap_t[day], p_max_size);
	} else if (strcmp(data_type, "wind_speed") == EQUAL) {
		snprintf(p, p_max_size, "%d", data->wind_s[day]);
	} else if (strcmp(data_type, "wind_dir") == EQUAL) {
		wind_deg_to_dir(p, p_max_size, data->wind_d[day]);
	} else if (strcmp(data_type, "wind_dir_DEG") == EQUAL) {
		snprintf(p, p_max_size, "%d", data->wind_d[day]);
	} else if (strcmp(data_type, "humidity") == EQUAL) {
		snprintf(p, p_max_size, "%d", data->hmid[day]);
	} else if (strcmp(data_type, "precipitation") == EQUAL) {
		snprintf(p, p_max_size, "%d", data->ppcp[day]);
	} else if (strcmp(data_type, "day") == EQUAL) {
		strncpy(p, data->day[day], p_max_size);
	} else if (strcmp(data_type, "date") == EQUAL) {
		strncpy(p, data->date[day], p_max_size);
	}

}
#endif /* BUILD_WEATHER_XOAP */

static void weather_process_info(char *p, int p_max_size, char *uri, char *data_type, int interval)
{
	static const char *wc[] = {
		"", "drizzle", "rain", "hail", "soft hail",
		"snow", "snow grains", "fog", "haze", "smoke",
		"mist", "dust", "sand", "funnel cloud tornado",
		"dust/sand", "squall", "sand storm", "dust storm"
	};
	PWEATHER *data;

	ccurl_location_ptr curloc = ccurl_find_location(locations_cc, uri);
	if (!curloc->p_timed_thread) {
		curloc->result = (char*)malloc(sizeof(PWEATHER));
		memset(curloc->result, 0, sizeof(PWEATHER));
		curloc->process_function = std::bind(parse_weather,
				std::placeholders::_1, std::placeholders::_2);
		ccurl_init_thread(curloc, interval);
		if (!curloc->p_timed_thread) {
			NORM_ERR("error setting up weather thread");
		}
	}

	std::lock_guard<std::mutex> lock(curloc->p_timed_thread->mutex());
	data = (PWEATHER*)curloc->result;
	if (strcmp(data_type, "last_update") == EQUAL) {
		strncpy(p, data->lastupd, p_max_size);
	} else if (strcmp(data_type, "temperature") == EQUAL) {
		temp_print(p, p_max_size, data->temp, TEMP_CELSIUS);
	} else if (strcmp(data_type, "cloud_cover") == EQUAL) {
#ifdef BUILD_WEATHER_XOAP
		if (data->xoap_t[0] != '\0') {
			char *s = p;
			strncpy(p, data->xoap_t, p_max_size);
			while (*s) {
				*s = tolower(*s);
				s++;
			}
		} else
#endif /* BUILD_WEATHER_XOAP */
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
#ifdef BUILD_WEATHER_XOAP
	} else if (strcmp(data_type, "icon") == EQUAL) {
		strncpy(p, data->icon, p_max_size);
#endif /* BUILD_WEATHER_XOAP */
	} else if (strcmp(data_type, "pressure") == EQUAL) {
		snprintf(p, p_max_size, "%d", data->bar);
	} else if (strcmp(data_type, "wind_speed") == EQUAL) {
		snprintf(p, p_max_size, "%d", data->wind_s);
	} else if (strcmp(data_type, "wind_dir") == EQUAL) {
		wind_deg_to_dir(p, p_max_size, data->wind_d);
	} else if (strcmp(data_type, "wind_dir_DEG") == EQUAL) {
		snprintf(p, p_max_size, "%d", data->wind_d);

	} else if (strcmp(data_type, "humidity") == EQUAL) {
		snprintf(p, p_max_size, "%d", data->hmid);
	} else if (strcmp(data_type, "weather") == EQUAL) {
		strncpy(p, wc[data->wc], p_max_size);
	}

}

#ifdef BUILD_WEATHER_XOAP
/* xoap suffix for weather from weather.com */
static char *xoap_cc = NULL;
static char *xoap_df = NULL;
#endif /* BUILD_WEATHER_XOAP */

static int process_weather_uri(char *uri, char *locID, int dayf UNUSED_ATTR)
{
	/* locID MUST BE upper-case */
	char *tmp_p = locID;

	while (*tmp_p) {
		*tmp_p = toupper(*tmp_p);
		tmp_p++;
	}

	/* Construct complete uri */
#ifdef BUILD_WEATHER_XOAP
	if (strstr(uri, "xoap.weather.com")) {
		if ((dayf == 0) && (xoap_cc != NULL)) {
			strcat(uri, locID);
			strcat(uri, xoap_cc);
		} else if ((dayf == 1) && (xoap_df != NULL)) {
			strcat(uri, locID);
			strcat(uri, xoap_df);
		} else {
			free(uri);
			uri = NULL;
		}
	} else
#endif /* BUILD_WEATHER_XOAP */
	if (strstr(uri, "weather.noaa.gov")) {
		strcat(uri, locID);
		strcat(uri, ".TXT");
	} else  if (!strstr(uri, "localhost") && !strstr(uri, "127.0.0.1")) {
		return -1;
	}
	return 0;
}

#ifdef BUILD_WEATHER_XOAP

/*
 * TODO: make the xoap keys file readable from the config file
 *       make the keys directly readable from the config file
 *       make the xoap keys file giveable as a command line option
 */
void load_xoap_keys(void)
{
	FILE *fp;
	char *par  = (char *) malloc(11 * sizeof(char));
	char *key  = (char *) malloc(17 * sizeof(char));
	char *xoap = (char *) malloc(64 * sizeof(char));

	to_real_path(xoap, XOAP_FILE);
	fp = fopen(xoap, "r");
	if (fp != NULL) {
		if (fscanf(fp, "%10s %16s", par, key) == 2) {
			xoap_cc = (char *) malloc(128 * sizeof(char));
			xoap_df = (char *) malloc(128 * sizeof(char));

			strcpy(xoap_cc, "?cc=*&link=xoap&prod=xoap&par=");
			strcat(xoap_cc, par);
			strcat(xoap_cc, "&key=");
			strcat(xoap_cc, key);
			strcat(xoap_cc, "&unit=m");

			/* TODO: Use FORECAST_DAYS instead of 5 */
			strcpy(xoap_df, "?dayf=5&link=xoap&prod=xoap&par=");
			strcat(xoap_df, par);
			strcat(xoap_df, "&key=");
			strcat(xoap_df, key);
			strcat(xoap_df, "&unit=m");
		}
		fclose(fp);
	}
	free(par);
	free(key);
	free(xoap);
}

void scan_weather_forecast_arg(struct text_object *obj, const char *arg, void *free_at_crash)
{
	int argc;
	struct weather_forecast_data *wfd;
	float interval = 0;
	char *locID = (char *) malloc(9 * sizeof(char));

	wfd = (struct weather_forecast_data *)malloc(sizeof(struct weather_forecast_data));
	memset(wfd, 0, sizeof(struct weather_forecast_data));

	argc = sscanf(arg, "%119s %8s %1u %31s %f", wfd->uri, locID, &wfd->day, wfd->data_type, &interval);

	if (argc < 4) {
		free(locID);
		free(wfd);
		CRIT_ERR(obj, free_at_crash, "wrong number of arguments for $weather_forecast");
	}
	if (process_weather_uri(wfd->uri, locID, 1)) {
		free(locID);
		free(wfd);
		CRIT_ERR(obj, free_at_crash, \
				"could not recognize the weather forecast uri");
	}

	/* Limit the day between 0 (today) and FORECAST_DAYS */
	if (wfd->day >= FORECAST_DAYS) {
		wfd->day = FORECAST_DAYS-1;
	}

	/* Limit the data retrieval interval to 3 hours and an half */
	if (interval < 210) {
		interval = 210;
	}

	/* Convert to seconds */
	wfd->interval = interval * 60;
	free(locID);

	DBGP("weather_forecast: fetching %s for day %d from %s every %d seconds", \
			wfd->data_type, wfd->day, wfd->uri, wfd->interval);

	obj->data.opaque = wfd;
}

void print_weather_forecast(struct text_object *obj, char *p, int p_max_size)
{
	struct weather_forecast_data *wfd = (struct weather_forecast_data *)obj->data.opaque;

	if (!wfd || !wfd->uri) {
		NORM_ERR("error processing weather forecast data, check that you have a valid XOAP key if using XOAP.");
		return;
	}
	weather_forecast_process_info(p, p_max_size, wfd->uri, wfd->day, wfd->data_type, wfd->interval);
}
#endif /* BUILD_WEATHER_XOAP */

void scan_weather_arg(struct text_object *obj, const char *arg, void *free_at_crash)
{
	int argc;
	struct weather_data *wd;
	char *locID = (char *) malloc(9 * sizeof(char));
	float interval = 0;

	wd = (struct weather_data *)malloc(sizeof(struct weather_data));
	memset(wd, 0, sizeof(struct weather_data));

	argc = sscanf(arg, "%119s %8s %31s %f", wd->uri, locID, wd->data_type, &interval);

	if (argc < 3) {
		free(locID);
		free(wd);
		CRIT_ERR(obj, free_at_crash, "wrong number of arguments for $weather");
	}
	if (process_weather_uri(wd->uri, locID, 0)) {
		free(locID);
		free(wd);
		CRIT_ERR(obj, free_at_crash, \
				"could not recognize the weather uri");
	}

	/* Limit the data retrieval interval to half hour min */
	if (interval < 30) {
		interval = 30;
	}

	/* Convert to seconds */
	wd->interval = interval * 60;
	free(locID);

	DBGP("weather: fetching %s from %s every %d seconds", \
			wd->data_type, wd->uri, wd->interval);

	obj->data.opaque = wd;
}

void print_weather(struct text_object *obj, char *p, int p_max_size)
{
	struct weather_data *wd = (struct weather_data *)obj->data.opaque;

	if (!wd || !wd->uri) {
		NORM_ERR("error processing weather data, check that you have a valid XOAP key if using XOAP.");
		return;
	}
	weather_process_info(p, p_max_size, wd->uri, wd->data_type, wd->interval);
}

void free_weather(struct text_object *obj)
{
	if (obj->data.opaque) {
		free(obj->data.opaque);
		obj->data.opaque = NULL;
	}
}
