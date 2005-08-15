/*
 * Conky, a system monitor, based on torsmo
 *
 * This program is licensed under BSD license, read COPYING
 *
 *  $Id$
 */

#include "conky.h"
#include <string.h>
#include "ftp.h"
#include <math.h>
#include <time.h>
#include <pthread.h>
#include "metarinfo.h"

/* for threads */
static int status = 0;


int calculateRelativeHumidity(int temp, int dew)
{
	float rh = 0.0;
	rh = 100.0 * powf((112 - (0.1 * temp) + dew) /
			  (112 + (0.9 * temp)), 8);
	return (int) rh;
}

int calculateWindChill(int temperatureC, int windKn)
{
	double windKmh = windKn * 1.852;
	return (int) (13.112 + 0.6215 * temperatureC -
		      11.37 * powf(windKmh,
				   .16) +
		      0.3965 * temperatureC * powf(windKmh, .16));
}

/*Should no longer be needed
int knTokph(int knSpeed)
{
	return (knSpeed * 1.852);
}
*/
const char *calculateWindDirectionString(int degree)
{
	if (degree < 22.5) {
		return "North";
	} else if (degree < 67.5) {
		return "Northeast";
	} else if (degree < 112.5) {
		return "East";
	} else if (degree < 157.5) {
		return "Southeast";
	} else if (degree < 202.5) {
		return "South";
	} else if (degree < 247.5) {
		return "Southwest";
	} else if (degree < 292.5) {
		return "West";
	} else if (degree < 337.5) {
		return "Northwest";
	} else {
		return "North";
	}
}
const char *calculateShortWindDirectionString(int degree)
{
	if (degree < 22.5) {
		return "N";
	} else if (degree < 67.5) {
		return "NE";
	} else if (degree < 112.5) {
		return "E";
	} else if (degree < 157.5) {
		return "SE";
	} else if (degree < 202.5) {
		return "S";
	} else if (degree < 247.5) {
		return "SW";
	} else if (degree < 292.5) {
		return "W";
	} else if (degree < 337.5) {
		return "NW";
	} else {
		return "N";
	}
}

void ftpData(void *userData, const char *data, int len)
{
	if (userData) {
	}			// do nothing to make stupid warning go away
	if (len <= 0)
		return;

	if (data != NULL) {
		line = strdup(data);
	}
}

extern unsigned int sleep(unsigned int);

void *fetch_ftp()
{
	// try three times if it fails
	int tries = 0;
	do {
		if (tries > 0) {
			/* this happens too often, so i'll leave it commented fprintf(stderr, "METAR update failed for some reason, retry %i\n", tries); */
			sleep(15);	/* give it some time in case the server is busy or down */
		}

		tries++;
		if (strlen(metar_station) != 8) {
			ERR("You didn't supply a valid METAR station code\n");
			return NULL;
		}
		if (metar_server == NULL)
			metar_server = strdup("weather.noaa.gov");
		if (metar_path == NULL)
			metar_path =
			    strdup("/data/observations/metar/stations");

		int res;
		initFtp();
		res = connectFtp(metar_server, 0);
		if (res < 0) {
			ERR("Couldn't connect to %s, retrying\n",
			    metar_server);
			continue;
		}
		res = changeFtpDirectory(metar_path);
		if (res < 0) {
			ERR("Metar update failed (couldn't CWD to %s)\n",
			    metar_path);
			disconnectFtp();
			continue;
		}
		if (res == 0) {
			ERR("Metar update failed\n");
			continue;
		}
		if (getFtp(ftpData, NULL, metar_station) < 0) {
			ERR("Failed to get file %s\n", metar_station);
		}

		disconnectFtp();
		if (line != NULL) {
			dcdNetMETAR(line, &data);
			free(line);
			line = NULL;
			ftp_ok = 1;
			metar_worked = 1;
		} else {
			ftp_ok = 0;
		}

	} while (ftp_ok == 0 && tries < 3);
	status = 1;
	return NULL;
}

static pthread_t thread1;

void update_metar()
{
	int iret1;
	if (!status) {
		status = 2;
		iret1 = pthread_create(&thread1, NULL, fetch_ftp, NULL);
	} else if (status == 2) {	/* thread is still running.  let's kill it and start again */
		pthread_cancel(thread1);
		status = 2;
		iret1 = pthread_create(&thread1, NULL, fetch_ftp, NULL);
	} else if (status == 1) {		/* status must be 1 */
		pthread_join(thread1, NULL);
		status = 2;
		iret1 = pthread_create(&thread1, NULL, fetch_ftp, NULL);
	}

}
