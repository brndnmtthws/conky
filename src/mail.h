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

#ifndef _MAIL_H
#define _MAIL_H

#include "setting.hh"

void parse_local_mail_args(struct text_object *, const char *);

#define PRINT_MAILS_PROTO_GENERATOR(x) \
void print_##x##mails(struct text_object *, char *, int);

PRINT_MAILS_PROTO_GENERATOR()
PRINT_MAILS_PROTO_GENERATOR(new_)
PRINT_MAILS_PROTO_GENERATOR(seen_)
PRINT_MAILS_PROTO_GENERATOR(unseen_)
PRINT_MAILS_PROTO_GENERATOR(flagged_)
PRINT_MAILS_PROTO_GENERATOR(unflagged_)
PRINT_MAILS_PROTO_GENERATOR(forwarded_)
PRINT_MAILS_PROTO_GENERATOR(unforwarded_)
PRINT_MAILS_PROTO_GENERATOR(replied_)
PRINT_MAILS_PROTO_GENERATOR(unreplied_)
PRINT_MAILS_PROTO_GENERATOR(draft_)
PRINT_MAILS_PROTO_GENERATOR(trashed_)

void free_local_mails(struct text_object *obj);

void parse_global_imap_mail_args(const char *);
void parse_global_pop3_mail_args(const char *);

void parse_imap_mail_args(struct text_object *, const char *);
void parse_pop3_mail_args(struct text_object *, const char *);
void free_mail_obj(struct text_object *);
void print_imap_unseen(struct text_object *, char *, int);
void print_imap_messages(struct text_object *, char *, int);
void print_pop3_unseen(struct text_object *, char *, int);
void print_pop3_used(struct text_object *, char *, int);

namespace priv {
	class current_mail_spool_setting: public conky::simple_config_setting<std::string> {
		typedef conky::simple_config_setting<std::string> Base;
		
	protected:
		virtual std::pair<std::string, bool> do_convert(lua::state &l, int index);

	public:
		current_mail_spool_setting()
			: Base("current_mail_spool", "$MAIL", true)
		{}
	};
}

extern priv::current_mail_spool_setting current_mail_spool;

#endif /* _MAIL_H */
