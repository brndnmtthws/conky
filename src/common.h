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

#ifndef _COMMON_H
#define _COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include "text_object.h"
#include "setting.hh"

char* readfile(const char* filename, int* total_read, char showerror);

void print_to_bytes(struct text_object *, char *, int);

void strfold(char *start, int count);
int check_mount(struct text_object *);
void prepare_update(void);
int update_uptime(void);
int update_meminfo(void);
int update_net_stats(void);
int update_cpu_usage(void);
int update_total_processes(void);
int update_uname(void);
int update_threads(void);
int update_running_processes(void);
void update_stuff(void);
char get_freq(char *, size_t, const char *, int, unsigned int);
void print_voltage_mv(struct text_object *, char *, int);
void print_voltage_v(struct text_object *, char *, int);
int update_load_average(void);
void free_all_processes(void);
struct process *get_first_process(void);
void get_cpu_count(void);
double get_time(void);

/* Converts '~/...' paths to '/home/blah/...'
 * It's similar to variable_substitute, except only cheques for $HOME and ~/ in path */
std::string to_real_path(const std::string &source);
FILE *open_file(const char *file, int *reported);
int open_fifo(const char *file, int *reported);
std::string variable_substitute(std::string s);

void format_seconds(char *buf, unsigned int n, long t);
void format_seconds_short(char *buf, unsigned int n, long t);

int round_to_int_temp(float);

unsigned int round_to_int(float);

extern conky::simple_config_setting<bool> no_buffers;

int open_acpi_temperature(const char *name);
double get_acpi_temperature(int fd);
void get_acpi_ac_adapter(char *p_client_buffer, size_t client_buffer_size, const char *adapter);
void get_acpi_fan(char *, size_t);
void get_battery_stuff(char *buf, unsigned int n, const char *bat, int item);
int get_battery_perct(const char *bat);
double get_battery_perct_bar(struct text_object *);
void get_battery_short_status(char *buf, unsigned int n, const char *bat);

void scan_no_update(struct text_object *, const char *);
void print_no_update(struct text_object *, char *, int);
void free_no_update(struct text_object *);

void scan_loadavg_arg(struct text_object *, const char *);
void print_loadavg(struct text_object *, char *, int);
#ifdef BUILD_X11
void scan_loadgraph_arg(struct text_object *, const char *);
double loadgraphval(struct text_object *);
#endif /* BUILD_X11 */

uint8_t cpu_percentage(struct text_object *);
double cpu_barval(struct text_object *);

void print_mem(struct text_object *, char *, int);
void print_memwithbuffers(struct text_object *, char *, int);
void print_memeasyfree(struct text_object *, char *, int);
void print_memfree(struct text_object *, char *, int);
void print_memmax(struct text_object *, char *, int);
void print_memdirty(struct text_object *, char *, int);
void print_swap(struct text_object *, char *, int);
void print_swapfree(struct text_object *, char *, int);
void print_swapmax(struct text_object *, char *, int);
uint8_t mem_percentage(struct text_object *);
double mem_barval(struct text_object *);
double mem_with_buffers_barval(struct text_object *);
uint8_t swap_percentage(struct text_object *);
double swap_barval(struct text_object *);

void print_kernel(struct text_object *, char *, int);
void print_machine(struct text_object *, char *, int);
void print_nodename(struct text_object *, char *, int);
void print_nodename_short(struct text_object *, char *, int);
void print_sysname(struct text_object *, char *, int);

#if defined(__DragonFly__)
void print_version(struct text_object *obj, char *p, int p_max_size);
#endif


void print_uptime(struct text_object *, char *, int);
void print_uptime_short(struct text_object *, char *, int);

void print_processes(struct text_object *, char *, int);
void print_running_processes(struct text_object *, char *, int);
void print_running_threads(struct text_object *, char *, int);
void print_threads(struct text_object *, char *, int);

void print_buffers(struct text_object *, char *, int);
void print_cached(struct text_object *, char *, int);

void print_evaluate(struct text_object *, char *, int);

int if_empty_iftest(struct text_object *);
int if_existing_iftest(struct text_object *);
int if_running_iftest(struct text_object *);

#ifndef __OpenBSD__
void print_acpitemp(struct text_object *, char *, int);
void free_acpitemp(struct text_object *);
#endif /* !__OpenBSD__ */

void print_freq(struct text_object *, char *, int);
void print_freq_g(struct text_object *, char *, int);

#ifndef __OpenBSD__
void print_acpifan(struct text_object *, char *, int);
void print_acpiacadapter(struct text_object *, char *, int);
void print_battery(struct text_object *, char *, int);
void print_battery_time(struct text_object *, char *, int);
uint8_t battery_percentage(struct text_object *);
void print_battery_short(struct text_object *, char *, int);
#endif /* !__OpenBSD__ */

void print_blink(struct text_object *, char *, int);
void print_include(struct text_object *, char *, int);

void print_updates(struct text_object *, char *, int);
int updatenr_iftest(struct text_object *);

#ifdef BUILD_CURL
void print_stock(struct text_object *, char *, int);
void free_stock(struct text_object *);
#endif /* BUILD_CURL */
#endif /* _COMMON_H */
