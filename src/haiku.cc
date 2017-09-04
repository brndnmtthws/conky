/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
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

#include "haiku.h"
#include "net_stat.h"

void prepare_update()
{
}

int update_uptime()
{
	info.uptime = (double)system_time() / 1000000.0;
	return 0;
}

int check_mount(struct text_object *obj)
{
	/* stub */
	(void)obj;
	return 0;
}

int update_meminfo()
{
	// TODO
	return 1;
}

int update_net_stats()
{
	// TODO
	return 1;
}

int update_total_processes()
{
	// TODO
	return 1;
}

int update_running_processes()
{
	// TODO
	return 1;
}

void get_cpu_count(void)
{
	system_info si;

	if (get_system_info(&si) != B_OK) {
		fprintf(stderr, "Cannot get_system_info\n");
		info.cpu_count = 0;
		return;
	}
	info.cpu_count = si.cpu_count;

	//XXX: info.cpu_usage = 
}

int update_cpu_usage()
{
	// TODO
	return 1;
}

int update_load_average()
{
	// TODO
	return 1;
}

double get_acpi_temperature(int fd)
{
	return -1;
}

void get_battery_stuff(char *buf, unsigned int n, const char *bat, int item)
{
	// TODO
}

int get_battery_perct(const char *bat)
{
	/*
	int batcapacity;

	get_battery_stats(NULL, &batcapacity, NULL, NULL);
	return batcapacity;
	*/
	// TODO
	return 0;
}

double get_battery_perct_bar(struct text_object *obj)
{
	int batperct = get_battery_perct(obj->data.s);
	return batperct;
}

int open_acpi_temperature(const char *name)
{
	return -1;
}

void get_acpi_ac_adapter(char *p_client_buffer, size_t client_buffer_size, const char *adapter)
{
	(void) adapter; // only linux uses this

	if (!p_client_buffer || client_buffer_size <= 0) {
		return;
	}

	/* not implemented */
	memset(p_client_buffer, 0, client_buffer_size);
}

/* char *get_acpi_fan() */
void get_acpi_fan(char *p_client_buffer, size_t client_buffer_size)
{
	if (!p_client_buffer || client_buffer_size <= 0) {
		return;
	}

	/* not implemented */
	memset(p_client_buffer, 0, client_buffer_size);
}

/* void */
char get_freq(char *p_client_buffer, size_t client_buffer_size, const char *p_format,
		int divisor, unsigned int cpu)
{
	int freq;
	char *freq_sysctl;

	if (!p_client_buffer || client_buffer_size <= 0 || !p_format
			|| divisor <= 0) {
		return 0;
	}
	return 0;
	// TODO
//	return 1;
}

int update_diskio(void)
{
	return 1;
}

void get_top_info(void)
{
	// TODO
}

void get_battery_short_status(char *buffer, unsigned int n, const char *bat)
{
	// TODO
}

int get_entropy_avail(unsigned int *val)
{
	return 1;
}

int get_entropy_poolsize(unsigned int *val)
{
	return 1;
}
