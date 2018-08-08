/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
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

#include "conky.h"
#include "logging.h"
#include "mysql.h"

#include <mysql.h>

#include "setting.hh"

namespace {
conky::simple_config_setting<std::string> host("mysql_host", "localhost",
                                               false);
conky::range_config_setting<in_port_t> port("mysql_port", 0, 0xffff, 0, false);
conky::simple_config_setting<std::string> user("mysql_user", "root", false);
conky::simple_config_setting<std::string> password("mysql_password",
                                                   std::string(), false);
conky::simple_config_setting<std::string> db("mysql_db", "mysql", false);
}  // namespace

void mysql_finish(MYSQL *conn, MYSQL_RES *res) {
  if (nullptr != res) { mysql_free_result(res); }
  mysql_close(conn);
  mysql_library_end();
}

void print_mysql(struct text_object *obj, char *p, unsigned int p_max_size) {
  MYSQL *conn = mysql_init(nullptr);
  MYSQL_RES *res = nullptr;

  if (conn == nullptr) {
    NORM_ERR("Can't initialize MySQL");
    mysql_library_end();
    return;
  }
  if (!mysql_real_connect(conn, host.get(*state).c_str(),
                          user.get(*state).c_str(),
                          password.get(*state).c_str(), db.get(*state).c_str(),
                          port.get(*state), nullptr, 0)) {
    NORM_ERR("MySQL: %s", mysql_error(conn));
    mysql_finish(conn, res);
    return;
  }
  if (mysql_query(conn, obj->data.s)) {
    NORM_ERR("MySQL: %s", mysql_error(conn));
    mysql_finish(conn, res);
    return;
  }
  res = mysql_use_result(conn);
  if (res == nullptr) {
    NORM_ERR("MySQL: %s", mysql_error(conn));
    mysql_finish(conn, res);
    return;
  }
  MYSQL_ROW row = mysql_fetch_row(res);
  if (row) {
    snprintf(p, p_max_size, "%s", row[0]);
  } else {
    NORM_ERR("MySQL: '%s' returned no results", obj->data.s);
  }
  mysql_finish(conn, res);
}
