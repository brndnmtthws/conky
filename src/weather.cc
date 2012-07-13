/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
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

#include "config.h"
#include "conky.h"
#include "logging.h"
#include "weather.h"
#include "temphelper.h"
#include "text_object.h"
#include "ccurl_thread.h"
#include <time.h>
#include <ctype.h>
#include <array>
#include <cmath>
#include <mutex>
#include <string>
#ifdef BUILD_WEATHER_XOAP
#include <libxml/parser.h>
#include <libxml/xpath.h>
#endif /* BUILD_WEATHER_XOAP */

/* WEATHER data */
class weather {
	void parse_token(const char *token);
#ifdef BUILD_WEATHER_XOAP
	void parse_cc(xmlXPathContextPtr xpathCtx);
	void parse_weather_xml(const std::string &data);
#endif

public:
	std::string lastupd;
#ifdef BUILD_WEATHER_XOAP
	std::string xoap_t;
	std::string icon;
#endif /* BUILD_WEATHER_XOAP */
	int temp;
	int dew;
	int cc;
	int bar;
	int wind_s;
	int wind_d;
	int hmid;
	int wc;

	weather()
		: temp(0), dew(0), cc(0), bar(0), wind_s(0), wind_d(0), hmid(0), wc(0)
	{}

	weather(const std::string &);
};

#ifdef BUILD_WEATHER_XOAP
#define FORECAST_DAYS 5
struct weather_forecast_day {
	std::string icon;
	std::string xoap_t;
	std::string day;
	std::string date;
	int hi;
	int low;
	int wind_s;
	int wind_d;
	int hmid;
	int ppcp;

	weather_forecast_day()
		: hi(0), low(0), wind_s(0), wind_d(0), hmid(0), ppcp(0)
	{}
};

class weather_forecast: public std::array<weather_forecast_day, FORECAST_DAYS> {
	void parse_df(xmlXPathContextPtr xpathCtx);

public:
	weather_forecast() = default;

	weather_forecast(const std::string &);
};


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
void weather_forecast::parse_df(xmlXPathContextPtr xpathCtx)
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
						(*this)[k].hi = atoi(content);
						break;
					case 1:
						(*this)[k].low = atoi(content);
						break;
					case 2:
						(*this)[k].icon = content;
					case 3:
						(*this)[k].xoap_t = content;
						break;
					case 4:
						(*this)[k].wind_s = atoi(content);
						break;
					case 5:
						(*this)[k].wind_d = atoi(content);
						break;
					case 6:
						(*this)[k].ppcp = atoi(content);
						break;
					case 7:
						(*this)[k].hmid = atoi(content);
					}
				} else if (nodes->nodeTab[j]->type == XML_ATTRIBUTE_NODE) {
					content = (char *)xmlNodeGetContent(nodes->nodeTab[k]);
					switch(i) {
					case 8:
						(*this)[k].day = content;
						break;
					case 9:
						(*this)[k].date = content;
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

weather_forecast::weather_forecast(const std::string &data)
{
	xmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;

	if (!(doc = xmlReadMemory(data.c_str(), data.length(), "", NULL, 0))) {
		NORM_ERR("weather_forecast: can't read xml data");
		return;
	}

	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL) {
	        NORM_ERR("weather_forecast: unable to create new XPath context");
		xmlFreeDoc(doc);
		return;
	}

	parse_df(xpathCtx);
	xmlXPathFreeContext(xpathCtx);
	xmlFreeDoc(doc);
	return;
}

void weather::parse_cc(xmlXPathContextPtr xpathCtx)
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
					lastupd = content;
					break;
				case 1:
					temp = atoi(content);
					break;
				case 2:
					xoap_t = content;
					break;
				case 3:
					bar = atoi(content);
					break;
				case 4:
					wind_s = atoi(content);
					break;
				case 5:
					wind_d = atoi(content);
					break;
				case 6:
					hmid = atoi(content);
					break;
				case 7:
					icon = content;
			}
			xmlFree(content);
		}
		xmlXPathFreeObject(xpathObj);
	}
	return;
}

void weather::parse_weather_xml(const std::string &data)
{
	xmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;

	if (!(doc = xmlReadMemory(data.c_str(), data.length(), "", NULL, 0))) {
		NORM_ERR("weather: can't read xml data");
		return;
	}

	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL) {
	        NORM_ERR("weather: unable to create new XPath context");
		xmlFreeDoc(doc);
		return;
	}

	parse_cc(xpathCtx);
	xmlXPathFreeContext(xpathCtx);
	xmlFreeDoc(doc);
	return;
}
#endif /* BUILD_WEATHER_XOAP */

/*
 * Horrible hack to avoid using regexes
 *
 */

void weather::parse_token(const char *token) {

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
						wc=i+1;
						break;
					}
				}
				return;
			}

			//Check for CB
			if (!strcmp(token, "CB")) {
				cc = 8;
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
							wc=i+1;
							break;
						}
					}
					return;
				}
			}

			//Check for NCD or NSC
			if ((!strcmp(token, "NCD")) || (!strcmp(token, "NSC"))) {
				cc = 1;
				return;
			}

			//Check for TCU
			if (!strcmp(token, "TCU")) {
				cc = 7;
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
							wc=i+1;
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
				cc = 1;
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
					temp=atoi(token);

					//4th and 5th digits gives the dew point temperature
					dew=atoi(&token[3]);

					//Compute humidity
					hmid = rel_humidity(dew, temp);

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
						bar = (int)(atoi(&token[1])*0.338637526f);
						return;
					}

					//Last 4 digits is pressure im mbar
					bar = atoi(&token[1]);
					return;
				}
			}

			//Check if token is a modified weather condition
			if ((token[0] == '+') || (token[0] == '-')) {
				for(i=0; i<NUM_WM_CODES; i++) {
					if (!strncmp(&token[1], WM_CODES[i], 2)) {
						for(i=0; i<NUM_WC_CODES; i++) {
							if (!strncmp(&token[3], WC_CODES[i], 2)) {
								wc=i+1;
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
							cc=i+1;
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
					temp = atoi(token);

					//5th and 6th digits gives the dew point temperature
					dew = -atoi(&token[4]);

					//Compute humidity
					hmid = rel_humidity(dew, temp);

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
				wind_d=atoi(s_tmp);

				//4th and 5th digit are wind speed in knots (convert to km/hr)
				wind_s = (int)(atoi(&token[3])*1.852);

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
						temp = -atoi(&token[1]);

						//6th and 7th digits gives the dew point temperature
						dew = -atoi(&token[5]);

						//Compute humidity
						hmid = rel_humidity(dew, temp);

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
				wind_d=atoi(s_tmp);

				//4th and 5th digit are wind speed in m/s (convert to km/hr)
				wind_s = (int)(atoi(&token[3])*3.6);

				return;
			}

		default:

			//printf("token : %s\n", token);
			break;
	}
}

weather::weather(const std::string &data)
	: temp(0), dew(0), cc(0), bar(0), wind_s(0), wind_d(0), hmid(0), wc(0)
{
#ifdef BUILD_WEATHER_XOAP
	//Check if it is an xml file
	if ( strncmp(data.c_str(), "<?xml ", 6) == 0 ) {
		parse_weather_xml(data);
	} else
#endif /* BUILD_WEATHER_XOAP */
	{
		//We assume its a text file
		char s_tmp[256];
		char lastupd_[32];
		const char delim[] = " ";

		//Divide time stamp and metar data
		if (sscanf(data.c_str(), "%[^'\n']\n%[^'\n']", lastupd_, s_tmp) == 2) {
			lastupd = lastupd_;

			//Process all tokens
			char *p_tok = NULL;
			char *p_save = NULL;

			if ((strtok_r(s_tmp, delim, &p_save)) != NULL) {

				//Jump first token, must be icao
				p_tok = strtok_r(NULL, delim, &p_save);

				do {

					parse_token(p_tok);
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

namespace {
	template<typename Result>
	class weather_cb: public curl_callback<Result> {
		typedef curl_callback<Result> Base;

	protected:
		virtual void process_data()
		{
			Result tmp(Base::data);

			std::unique_lock<std::mutex> lock(Base::result_mutex);
			Base::result = tmp;
		}

	public:
		weather_cb(uint32_t period, const std::string &uri)
			: Base(period, typename Base::Tuple(uri))
		{}

	};
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
static void weather_forecast_process_info(char *p, int p_max_size, const
		std::string &uri, unsigned int day, char *data_type, int interval)
{
	uint32_t period = std::max(lround(interval/active_update_interval()), 1l);

	auto cb = conky::register_cb<weather_cb<weather_forecast>>(period, uri);

	std::lock_guard<std::mutex> lock(cb->result_mutex);
	const weather_forecast &data = cb->get_result();
	if (strcmp(data_type, "hi") == EQUAL) {
		temp_print(p, p_max_size, data[day].hi, TEMP_CELSIUS);
	} else if (strcmp(data_type, "low") == EQUAL) {
		temp_print(p, p_max_size, data[day].low, TEMP_CELSIUS);
	} else if (strcmp(data_type, "icon") == EQUAL) {
		strncpy(p, data[day].icon.c_str(), p_max_size);
	} else if (strcmp(data_type, "forecast") == EQUAL) {
		strncpy(p, data[day].xoap_t.c_str(), p_max_size);
	} else if (strcmp(data_type, "wind_speed") == EQUAL) {
		snprintf(p, p_max_size, "%d", data[day].wind_s);
	} else if (strcmp(data_type, "wind_dir") == EQUAL) {
		wind_deg_to_dir(p, p_max_size, data[day].wind_d);
	} else if (strcmp(data_type, "wind_dir_DEG") == EQUAL) {
		snprintf(p, p_max_size, "%d", data[day].wind_d);
	} else if (strcmp(data_type, "humidity") == EQUAL) {
		snprintf(p, p_max_size, "%d", data[day].hmid);
	} else if (strcmp(data_type, "precipitation") == EQUAL) {
		snprintf(p, p_max_size, "%d", data[day].ppcp);
	} else if (strcmp(data_type, "day") == EQUAL) {
		strncpy(p, data[day].day.c_str(), p_max_size);
	} else if (strcmp(data_type, "date") == EQUAL) {
		strncpy(p, data[day].date.c_str(), p_max_size);
	}

}
#endif /* BUILD_WEATHER_XOAP */

static void weather_process_info(char *p, int p_max_size, const std::string &uri, char *data_type, int interval)
{
	static const char *wc[] = {
		"", "drizzle", "rain", "hail", "soft hail",
		"snow", "snow grains", "fog", "haze", "smoke",
		"mist", "dust", "sand", "funnel cloud tornado",
		"dust/sand", "squall", "sand storm", "dust storm"
	};

	uint32_t period = std::max(lround(interval/active_update_interval()), 1l);

	auto cb = conky::register_cb<weather_cb<weather>>(period, uri);

	std::lock_guard<std::mutex> lock(cb->result_mutex);
	const weather *data = &cb->get_result();
	if (strcmp(data_type, "last_update") == EQUAL) {
		strncpy(p, data->lastupd.c_str(), p_max_size);
	} else if (strcmp(data_type, "temperature") == EQUAL) {
		temp_print(p, p_max_size, data->temp, TEMP_CELSIUS);
	} else if (strcmp(data_type, "cloud_cover") == EQUAL) {
#ifdef BUILD_WEATHER_XOAP
		if (data->xoap_t[0] != '\0') {
			char *s = p;
			strncpy(p, data->xoap_t.c_str(), p_max_size);
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
		strncpy(p, data->icon.c_str(), p_max_size);
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
namespace {
	std::string xoap_cc;
	std::string xoap_df;
}
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
		if ((dayf == 0) && (xoap_cc.length() != 0)) {
			strcat(uri, locID);
			strcat(uri, xoap_cc.c_str());
		} else if ((dayf == 1) && (xoap_df.length() != 0)) {
			strcat(uri, locID);
			strcat(uri, xoap_df.c_str());
		} else {
			return 0;
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

	std::string xoap = to_real_path(XOAP_FILE);
	fp = fopen(xoap.c_str(), "r");
	if (fp != NULL) {
		if (fscanf(fp, "%10s %16s", par, key) == 2) {
			xoap_cc = std::string("?cc=*&link=xoap&prod=xoap&par=")
				+ par + "&key=" + key + "&unit=m";

			/* TODO: Use FORECAST_DAYS instead of 5 */
			xoap_df = std::string("?dayf=5&link=xoap&prod=xoap&par=")
				+ par + "&key=" + key + "&unit=m";
		}
		fclose(fp);
	}
	free(par);
	free(key);
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
	free_and_zero(obj->data.opaque);
}
