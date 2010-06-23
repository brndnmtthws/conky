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

#include "conky.h"
#include "logging.h"
#include "mysql.h"

#include <mysql.h>

struct mysql_conn mysql_settings;

void print_mysql(struct text_object *obj, char *p, int p_max_size) {
	MYSQL *conn = mysql_init(NULL);

	if(mysql_settings.db == NULL)
		mysql_settings.db = strdup("mysql");
	if(conn == NULL) {
		NORM_ERR("Can't initialize MySQL");
		mysql_library_end();
		return;
	}
	if (!mysql_real_connect(conn, mysql_settings.host, mysql_settings.user, mysql_settings.password, mysql_settings.db, mysql_settings.port, NULL, 0)) {
		NORM_ERR("MySQL: %s",  mysql_error(conn));
		mysql_close(conn);
		mysql_library_end();
		return;
	}
	if(mysql_query(conn, obj->data.s)) {
		NORM_ERR("MySQL: %s", mysql_error(conn));
		mysql_close(conn);
		mysql_library_end();
		return;
	}
	MYSQL_RES *res = mysql_use_result(conn);
	if(res == NULL) {
		NORM_ERR("MySQL: %s", mysql_error(conn));
		mysql_close(conn);
		mysql_library_end();
		return;
	}
	MYSQL_ROW row = mysql_fetch_row(res);
	if(row) {
		snprintf(p, p_max_size, "%s", row[0]);
	} else {
		NORM_ERR("MySQL: '%s' returned no results", obj->data.s);
	}
	mysql_free_result(res);
	mysql_close(conn);
	mysql_library_end();
}

void free_mysql(struct text_object *obj) {
	free(obj->data.s);
}

void free_mysql_global() {
	free_and_zero(mysql_settings.host);
	free_and_zero(mysql_settings.user);
	free_and_zero(mysql_settings.password);
	free_and_zero(mysql_settings.db);
}

void mysql_set_host(const char *host) {
	free(mysql_settings.host);
	mysql_settings.host = strdup(host);
}

void mysql_set_port(const char *port) {
	mysql_settings.port = strtol(port, 0, 0);
	if(mysql_settings.port < 1 || mysql_settings.port > 0xffff)
		mysql_settings.port = 0;
}

void mysql_set_user(const char *user) {
	free(mysql_settings.user);
	mysql_settings.user = strdup(user);
}

void mysql_set_password(const char *password) {
	free_and_zero(mysql_settings.password);
	if(password && strlen(password) > 2 && password[0] == '"' && password[strlen(password)-1] == '"') {
		mysql_settings.password = strdup(password+1);
		mysql_settings.password[strlen(password)-2] = 0;
	} else
		mysql_settings.password = NULL;
}

void mysql_set_db(const char *db) {
	free(mysql_settings.db);
	mysql_settings.db = strdup(db);
}
