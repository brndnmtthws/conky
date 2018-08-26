/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2018 Brenden Matthews, Philip Kovacs, et. al.
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

#include <ctype.h>
#include <time.h>
#include <array>
#include <cmath>
#include <mutex>
#include <string>
#include "ccurl_thread.h"
#include "config.h"
#include "conky.h"
#include "logging.h"
#include "temphelper.h"
#include "text_object.h"
#include "weather.h"

static void curl_owm_weather(void);
static size_t read_owm_weather_data_cb(char *, size_t, size_t, char *);

static char c_desc[256] = {""};
static char c_temp[256] = {""};
static char c_wind[256] = {""};
static char c_pressure[256] = {""};
static char c_humidity[256] = {""};
static unsigned int got_temp = 0U;
static unsigned int weather_updates = 1U;

/* {"coord":{"lon":-0.13,"lat":51.51},"weather":[{"id":803,"main":"Clouds","description":"broken clouds","icon":"04d"}],"base":"stations","main":{"temp":12.05,"pressure":1000,"humidity":70,"temp_min":12.05,"temp_max":12.05,"sea_level":1038.34,"grnd_level":1030.73},"wind":{"speed":3.82,"deg":8.50131},"clouds":{"all":64},"dt":1476114783,"sys":{"message":0.011,"country":"GB","sunrise":1476080264,"sunset":1476119749},"id":2643743,"name":"London","cod":200} */

static size_t read_owm_weather_data_cb(char *data, size_t size, size_t nmemb, char *p) {
  (void)p;
  unsigned int y = 0;
  unsigned int z = 0;
  char *ptr = nullptr;
  size_t sz = nmemb * size;
  size_t x = 0;
  char *desc_ptr = c_desc;
  char *temp_ptr = c_temp;
  char *wind_ptr = c_wind;
  char *pressure_ptr = c_pressure;
  char *humidity_ptr = c_humidity;

  for (ptr = data; *ptr; ptr++, x++) {
    if (x+7 < sz) { /* Verifying up to *(ptr+7) */
      if ('d' == *ptr && 'e' == *(ptr+1)
          && 's' == *(ptr+2) && 'c' == *(ptr+3) && x+40 < sz) { /* "description":"broken clouds" */
        for (; *ptr && *(ptr+14) && z < 29; z++, ptr++) {
          if ('"' == *(ptr+14)) {
            *desc_ptr++ = '\0';
            break;
          }
          if (isalpha((unsigned char) *(ptr+14)) || ' ' == *(ptr+14)) {
            *desc_ptr++ = *(ptr+14);
          }
        }
      } /* "temp":12.05 */
      if ('t' == *ptr && 0U == got_temp && 'e' == *(ptr+1)
          && 'm' == *(ptr+2) && 'p' == *(ptr+3)) {
        if ('-' == *(ptr+6)) {
          *temp_ptr++ = '-';
          if (isdigit((unsigned char) *(ptr+7))) {
            *temp_ptr++ = *(ptr+7);
            if (isdigit((unsigned char) *(ptr+8))) {
              *temp_ptr++ = *(ptr+8);
            }
            if (isdigit((unsigned char) *(ptr+9))) {
              *temp_ptr++ = *(ptr+9);
            }
            *temp_ptr = '\0';
            got_temp = 1U;
          }
        } else {
          if (isdigit((unsigned char) *(ptr+6))) {
            *temp_ptr++ = *(ptr+6);
            if (isdigit((unsigned char) *(ptr+7))) {
              *temp_ptr++ = *(ptr+7);
            }
            if (isdigit((unsigned char) *(ptr+8))) {
              *temp_ptr++ = *(ptr+8);
            }
            *temp_ptr = '\0';
            got_temp = 1U;
          }
        }
      }  /* "speed":3.82 */
      if ('s' == *ptr && 'p' == *(ptr+1) && 'e' == *(ptr+2)
          && 'e' == *(ptr+3) && x+9 < sz) {
        if (isdigit((unsigned char) *(ptr+7))) {
          *wind_ptr++ = *(ptr+7);
          if (isdigit((unsigned char) *(ptr+8))) {
            *wind_ptr++ = *(ptr+8);
          }
        }
        *wind_ptr = '\0';
      } /* "pressure":1000 */
      if ('p' == *ptr && 'r' == *(ptr+1) && 'e' == *(ptr+2) &&
          'r' == *(ptr+6) && x+14 < sz) {
        if (isdigit((unsigned char) *(ptr+10))) {
          *pressure_ptr++ = *(ptr+10);
        }
        if (isdigit((unsigned char) *(ptr+11))) {
          *pressure_ptr++ = *(ptr+11);
        }
        if (isdigit((unsigned char) *(ptr+12))) {
          *pressure_ptr++ = *(ptr+12);
        }
        if (isdigit((unsigned char) *(ptr+13))) {
          *pressure_ptr++ = *(ptr+13);
        }
        if (isdigit((unsigned char) *(ptr+14))) {
          *pressure_ptr++ = *(ptr+14);
        }
        *pressure_ptr = '\0';
      } /* "humidity":70 */
      if ('h' == *ptr && 'u' == *(ptr+1) && 'm' == *(ptr+2) &&
          't' == *(ptr+6) && x+12) {
        if (isdigit((unsigned char) *(ptr+10))) {
          *humidity_ptr++ = *(ptr+10);
        }
        if (isdigit((unsigned char) *(ptr+11))) {
          *humidity_ptr++ = *(ptr+11);
        }
        if (isdigit((unsigned char) *(ptr+12))) {
          *humidity_ptr++ = *(ptr+12);
        }
        *humidity_ptr = '\0';
      }
    }
  }
  return sz;
}

void print_owm_weather_temp(struct text_object *obj, char *p, unsigned int p_max_size) {
  (void)obj;
  if (1U != weather_updates) {
    --weather_updates;
    snprintf(p, p_max_size, "%s", c_temp);
    return;
  }
  curl_owm_weather();
  snprintf(p, p_max_size, "%s", c_temp);
}

void print_owm_weather_desc(struct text_object *obj, char *p, unsigned int p_max_size) {
  (void)obj;
  if (1U != weather_updates) {
    --weather_updates;
    snprintf(p, p_max_size, "%s", c_desc);
    return;
  }
  curl_owm_weather();
  snprintf(p, p_max_size, "%s", c_desc);
}

void print_owm_weather_wind(struct text_object *obj, char *p, unsigned int p_max_size) {
  (void)obj;
  if (1U != weather_updates) {
    --weather_updates;
    snprintf(p, p_max_size, "%s", c_wind);
    return;
  }
  curl_owm_weather();
  snprintf(p, p_max_size, "%s", c_wind);
}

void print_owm_weather_pressure(struct text_object *obj, char *p, unsigned int p_max_size) {
  (void)obj;
  if (1U != weather_updates) {
    --weather_updates;
    snprintf(p, p_max_size, "%s", c_pressure);
    return;
  }
  curl_owm_weather();
  snprintf(p, p_max_size, "%s", c_pressure);
}

void print_owm_weather_humidity(struct text_object *obj, char *p, unsigned int p_max_size) {
  (void)obj;
  if (1U != weather_updates) {
    --weather_updates;
    snprintf(p, p_max_size, "%s", c_humidity);
    return;
  }
  curl_owm_weather();
  snprintf(p, p_max_size, "%s", c_humidity);
}

static void curl_owm_weather(void) {
  static const char *const da_url = "http://api.openweathermap.org/data/2.5/weather?q=";
  CURL *curl = nullptr;
  CURLcode res;
  char temp[256];

  if (0 == strcmp(owm_weather_town.get(*state).c_str(), "")) {
    snprintf(c_desc, sizeof(c_desc) - 1, "%s", "owm_town requires argument, e.g: London,uk");
    snprintf(c_wind, sizeof(c_wind) - 1, "%s", "owm_town requires argument, e.g: London,uk");
    snprintf(c_temp, sizeof(c_temp) - 1, "%s", "owm_town requires argument, e.g: London,uk");
    snprintf(c_pressure, sizeof(c_pressure) - 1, "%s", "owm_town requires argument, e.g: London,uk");
    snprintf(c_humidity, sizeof(c_humidity) - 1, "%s", "owm_town requires argument, e.g: London,uk");
    return;
  }
  if (0 == strcmp(owm_weather_units.get(*state).c_str(), "")) {
    snprintf(c_desc, sizeof(c_desc) - 1, "%s", "owm_units requires argument e.g: metric/imperial");
    snprintf(c_wind, sizeof(c_wind) - 1, "%s", "owm_units requires argument e.g: metric/imperial");
    snprintf(c_temp, sizeof(c_temp) - 1, "%s", "owm_units requires argument e.g: metric/imperial");
    snprintf(c_pressure, sizeof(c_pressure) - 1, "%s", "owm_units requires argument e.g: metric/imperial");
    snprintf(c_humidity, sizeof(c_humidity) - 1, "%s", "owm_units requires argument e.g: metric/imperial");
    return;
  }
  if (0 == strcmp(owm_weather_api.get(*state).c_str(), "")) {
    snprintf(c_desc, sizeof(c_desc) - 1, "%s", "owm_api requires api key, register yourself and get an api key");
    snprintf(c_wind, sizeof(c_wind) - 1, "%s", "owm_api requires api key, register yourself and get an api key");
    snprintf(c_temp, sizeof(c_temp) - 1, "%s", "owm_api requires api key, register yourself and get an api key");
    snprintf(c_pressure, sizeof(c_pressure) - 1, "%s", "owm_api requires api key, register yourself and get an api key");
    snprintf(c_humidity, sizeof(c_humidity) - 1, "%s", "owm_api requires api key, register yourself and get an api key");
    return;
  }
  snprintf(temp, sizeof(temp) - 1, "%s%s%s%s%s%s", da_url, owm_weather_town.get(*state).c_str(),
    "&units=", owm_weather_units.get(*state).c_str(), "&APPID=", owm_weather_api.get(*state).c_str());
  got_temp = 0U;

  curl_global_init(CURL_GLOBAL_ALL);
  if (nullptr == (curl = curl_easy_init())) {
    goto error;
  }
  curl_easy_setopt(curl, CURLOPT_URL, temp);
  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, read_owm_weather_data_cb);

  res = curl_easy_perform(curl);
  if (CURLE_OK != res) {
    goto error;
  }
  weather_updates = owm_weather_update.get(*state);

error:
  if (nullptr != curl) {
    curl_easy_cleanup(curl);
  }
  curl_global_cleanup();
  return;
}
