/*
 * Conky, a system monitor, based on torsmo
 *
 * This program is licensed under BSD license, read COPYING
 *
 *  $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int calculateRelativeHumidity(int, int);
int calculateWindChill(int, int);
//int knTokph(int);
const char *calculateWindDirectionString(int);
const char *calculateShortWindDirectionString(int);

char *line = NULL;
char *metar_station = NULL;
char *metar_server = NULL;
char *metar_path = NULL;
char ftp_ok = 0;
char metar_worked = 0;

Decoded_METAR data;
