/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2021 Brenden Matthews, Philip Kovacs, et. al.
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

#ifndef OCTOPRINT_H_
#define OCTOPRINT_H_

#include "conky.h"
// #include "json/json.h"

//general variable format: ${octoprint_user printer_id <optional index>}

// /api/currentuser
void print_octoprint_user(struct text_object *, char *, unsigned int);
// /api/version
void print_octoprint_version(struct text_object *, char *, unsigned int);
void print_octoprint_longversion(struct text_object *, char *, unsigned int);
// /api/server
void print_octoprint_safemode(struct text_object *, char *, unsigned int);
// /api/connection
void print_octoprint_conn_baud(struct text_object *, char *, unsigned int);
void print_octoprint_conn_port(struct text_object *, char *, unsigned int);
void print_octoprint_conn_state(struct text_object *, char *, unsigned int);
// /api/files
// /api/files/<location>
// /api/files/<location>/<file>
void print_octoprint_local_used(struct text_object *, char *, unsigned int);
void print_octoprint_local_free(struct text_object *, char *, unsigned int);
void print_octoprint_local_total(struct text_object *, char *, unsigned int);
void print_octoprint_local_filecount(struct text_object *, char *, unsigned int);
void print_octoprint_sdcard_filecount(struct text_object *, char *, unsigned int);
// /api/job
void print_octoprint_job_name(struct text_object *, char *, unsigned int);
void print_octoprint_job_progress(struct text_object *, char *, unsigned int);
uint8_t octoprint_job_progress_pct(struct text_object *);
double octoprint_job_progress_barval(struct text_object *);

void print_octoprint_job_time(struct text_object *, char *, unsigned int);
void print_octoprint_job_time_left(struct text_object *, char *, unsigned int);
void print_octoprint_job_state(struct text_object *, char *, unsigned int);
void print_octoprint_job_user(struct text_object *, char *, unsigned int);
// /plugin/logging/logs
void print_octoprint_logs_used(struct text_object *, char *, unsigned int);
void print_octoprint_logs_free(struct text_object *, char *, unsigned int);
void print_octoprint_logs_total(struct text_object *, char *, unsigned int);
// /api/printer  -- use single endpoint for efficiency
void print_octoprint_printer_state(struct text_object *, char *, unsigned int);
void print_octoprint_printer_error(struct text_object *, char *, unsigned int);

void print_octoprint_temperature(struct text_object *, char *, unsigned int);
void print_octoprint_target_temp(struct text_object *, char *, unsigned int);
double octoprint_temperature(struct text_object *);
double octoprint_target_temp(struct text_object *);

void print_octoprint_sdcard_ready(struct text_object *, char *, unsigned int);

void octoprint_parse_arg(struct text_object *, const char *);
void octoprint_free_obj_info(struct text_object *);

#endif