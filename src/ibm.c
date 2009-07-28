/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 *
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2007 Toni Spets
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

#include "conky.h"
#include "config.h"
#include "ibm.h"
#include "logging.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

/* Here come the IBM ACPI-specific things. For reference, see
 * http://ibm-acpi.sourceforge.net/README
 * If IBM ACPI is installed, /proc/acpi/ibm contains the following files:
bay
beep
bluetooth
brightness
cmos
dock
driver
ecdump
fan
hotkey
led
light
thermal
video
volume
 * The content of these files is described in detail in the aforementioned
 * README - some of them also in the following functions accessing them.
 * Peter Tarjan (ptarjan@citromail.hu) */

#define IBM_ACPI_DIR "/proc/acpi/ibm"

/* get fan speed on IBM/Lenovo laptops running the ibm acpi.
 * /proc/acpi/ibm/fan looks like this (3 lines):
status:         disabled
speed:          2944
commands:       enable, disable
 * Peter Tarjan (ptarjan@citromail.hu) */

void get_ibm_acpi_fan(char *p_client_buffer, size_t client_buffer_size)
{
	FILE *fp;
	unsigned int speed = 0;
	char fan[128];

	if (!p_client_buffer || client_buffer_size <= 0) {
		return;
	}

	snprintf(fan, 127, "%s/fan", IBM_ACPI_DIR);

	fp = fopen(fan, "r");
	if (fp != NULL) {
		while (!feof(fp)) {
			char line[256];

			if (fgets(line, 255, fp) == NULL) {
				break;
			}
			if (sscanf(line, "speed: %u", &speed)) {
				break;
			}
		}
	} else {
		CRIT_ERR(NULL, NULL, "can't open '%s': %s\nYou are not using the IBM ACPI. Remove "
			"ibm* from your "PACKAGE_NAME" config file.", fan, strerror(errno));
	}

	fclose(fp);
	snprintf(p_client_buffer, client_buffer_size, "%d", speed);
}

/* get the measured temperatures from the temperature sensors
 * on IBM/Lenovo laptops running the ibm acpi.
 * There are 8 values in /proc/acpi/ibm/thermal, and according to
 * http://ibm-acpi.sourceforge.net/README
 * these mean the following (at least on an IBM R51...)
 * 0:  CPU (also on the T series laptops)
 * 1:  Mini PCI Module (?)
 * 2:  HDD (?)
 * 3:  GPU (also on the T series laptops)
 * 4:  Battery (?)
 * 5:  N/A
 * 6:  Battery (?)
 * 7:  N/A
 * I'm not too sure about those with the question mark, but the values I'm
 * reading from *my* thermal file (on a T42p) look realistic for the
 * hdd and the battery.
 * #5 and #7 are always -128.
 * /proc/acpi/ibm/thermal looks like this (1 line):
temperatures:   41 43 31 46 33 -128 29 -128
 * Peter Tarjan (ptarjan@citromail.hu) */

static double last_ibm_acpi_temp_time;
void get_ibm_acpi_temps(void)
{

	FILE *fp;
	char thermal[128];

	/* don't update too often */
	if (current_update_time - last_ibm_acpi_temp_time < 10.00) {
		return;
	}
	last_ibm_acpi_temp_time = current_update_time;

	/* if (!p_client_buffer || client_buffer_size <= 0) {
		return;
	} */

	snprintf(thermal, 127, "%s/thermal", IBM_ACPI_DIR);
	fp = fopen(thermal, "r");

	if (fp != NULL) {
		while (!feof(fp)) {
			char line[256];

			if (fgets(line, 255, fp) == NULL) {
				break;
			}
			if (sscanf(line, "temperatures: %d %d %d %d %d %d %d %d",
					&ibm_acpi.temps[0], &ibm_acpi.temps[1], &ibm_acpi.temps[2],
					&ibm_acpi.temps[3], &ibm_acpi.temps[4], &ibm_acpi.temps[5],
					&ibm_acpi.temps[6], &ibm_acpi.temps[7])) {
				break;
			}
		}
	} else {
		CRIT_ERR(NULL, NULL, "can't open '%s': %s\nYou are not using the IBM ACPI. Remove "
			"ibm* from your "PACKAGE_NAME" config file.", thermal, strerror(errno));
	}

	fclose(fp);
}

/* get volume (0-14) on IBM/Lenovo laptops running the ibm acpi.
 * "Volume" here is none of the mixer volumes, but a "master of masters"
 * volume adjusted by the IBM volume keys.
 * /proc/acpi/ibm/fan looks like this (4 lines):
level:          4
mute:           off
commands:       up, down, mute
commands:       level <level> (<level> is 0-15)
 * Peter Tarjan (ptarjan@citromail.hu) */

void get_ibm_acpi_volume(char *p_client_buffer, size_t client_buffer_size)
{
	FILE *fp;
	char volume[128];
	unsigned int vol = -1;
	char mute[3] = "";

	if (!p_client_buffer || client_buffer_size <= 0) {
		return;
	}

	snprintf(volume, 127, "%s/volume", IBM_ACPI_DIR);

	fp = fopen(volume, "r");
	if (fp != NULL) {
		while (!feof(fp)) {
			char line[256];
			unsigned int read_vol = -1;

			if (fgets(line, 255, fp) == NULL) {
				break;
			}
			if (sscanf(line, "level: %u", &read_vol)) {
				vol = read_vol;
				continue;
			}
			if (sscanf(line, "mute: %s", mute)) {
				break;
			}
		}
	} else {
		CRIT_ERR(NULL, NULL, "can't open '%s': %s\nYou are not using the IBM ACPI. Remove "
			"ibm* from your "PACKAGE_NAME" config file.", volume, strerror(errno));
	}

	fclose(fp);

	if (strcmp(mute, "on") == 0) {
		snprintf(p_client_buffer, client_buffer_size, "%s", "mute");
		return;
	} else {
		snprintf(p_client_buffer, client_buffer_size, "%d", vol);
		return;
	}
}

/* static FILE *fp = NULL; */

/* get LCD brightness on IBM/Lenovo laptops running the ibm acpi.
 * /proc/acpi/ibm/brightness looks like this (3 lines):
level:          7
commands:       up, down
commands:       level <level> (<level> is 0-7)
 * Peter Tarjan (ptarjan@citromail.hu) */

void get_ibm_acpi_brightness(char *p_client_buffer, size_t client_buffer_size)
{
	FILE *fp;
	unsigned int brightness = 0;
	char filename[128];

	if (!p_client_buffer || client_buffer_size <= 0) {
		return;
	}

	snprintf(filename, 127, "%s/brightness", IBM_ACPI_DIR);

	fp = fopen(filename, "r");
	if (fp != NULL) {
		while (!feof(fp)) {
			char line[256];

			if (fgets(line, 255, fp) == NULL) {
				break;
			}
			if (sscanf(line, "level: %u", &brightness)) {
				break;
			}
		}
	} else {
		CRIT_ERR(NULL, NULL, "can't open '%s': %s\nYou are not using the IBM ACPI. Remove "
			"ibm* from your "PACKAGE_NAME" config file.", filename, strerror(errno));
	}

	fclose(fp);

	snprintf(p_client_buffer, client_buffer_size, "%d", brightness);
}

