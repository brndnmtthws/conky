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

#ifndef _NET_STAT_H
#define _NET_STAT_H

#include <sys/socket.h>	/* struct sockaddr */

#ifdef BUILD_IPV6
struct v6addr {
	struct in6_addr addr;
	unsigned int netmask;
	char scope;
	struct v6addr *next;
};
#endif /* BUILD_IPV6 */

struct net_stat {
	/* interface name, e.g. wlan0, eth0, ... */
	char *dev;
	/* set to 1, if interface is up */
	int up;
	/* network traffic read on last call in order to calculate how much
	 * was received or transmitted since the last call. contains -1 if
	 * it was never read before. in bytes */
	long long last_read_recv, last_read_trans;
	/* total received and transmitted data statistics in bytes */
	long long recv, trans;
	/* averaged network speed in bytes / second */
	double recv_speed, trans_speed;
	/* struct with at least the member sa_data which is a const * containing
	 * the socket address.
	 * @see http://pubs.opengroup.org/onlinepubs/7908799/xns/syssocket.h.html */
	struct sockaddr addr;
#ifdef BUILD_IPV6
	struct v6addr *v6addrs;
	bool v6show_nm;
	bool v6show_sc;
#endif /* BUILD_IPV6 */
#if defined(__linux__)
	char addrs[17 * MAX_NET_INTERFACES + 1];
#endif /* __linux__ */
	/* network speeds between two conky calls in bytes per second.
	 * An average over these samples is calculated in recv_speed and
	 * trans_speed */
	double net_rec[15], net_trans[15];
	// wireless extensions
	char essid[32];
	int channel;
	char freq[16];
	char bitrate[16];
	char mode[16];
	int link_qual;
	int link_qual_max;
	char ap[18];
};

extern struct net_stat netstats[];

struct net_stat *get_net_stat(const char *, void *, void *);

void parse_net_stat_arg(struct text_object *, const char *, void *);
void parse_net_stat_bar_arg(struct text_object *, const char *, void *);
void print_downspeed(struct text_object *, char *, int);
void print_downspeedf(struct text_object *, char *, int);
void print_upspeed(struct text_object *, char *, int);
void print_upspeedf(struct text_object *, char *, int);
void print_totaldown(struct text_object *, char *, int);
void print_totalup(struct text_object *, char *, int);
void print_addr(struct text_object *, char *, int);
#ifdef __linux__
void print_addrs(struct text_object *, char *, int);
#ifdef BUILD_IPV6
void print_v6addrs(struct text_object *, char *, int);
#endif /* BUILD_IPV6 */
#endif /* __linux__ */
#ifdef BUILD_X11
void parse_net_stat_graph_arg(struct text_object *, const char *, void *);
double downspeedgraphval(struct text_object *);
double upspeedgraphval(struct text_object *);
#endif /* BUILD_X11 */
#ifdef BUILD_WLAN
void print_wireless_essid(struct text_object *, char *, int);
void print_wireless_channel(struct text_object *, char *, int);
void print_wireless_frequency(struct text_object *, char *, int);
void print_wireless_mode(struct text_object *, char *, int);
void print_wireless_bitrate(struct text_object *, char *, int);
void print_wireless_ap(struct text_object *, char *, int);
void print_wireless_link_qual(struct text_object *, char *, int);
void print_wireless_link_qual_max(struct text_object *, char *, int);
void print_wireless_link_qual_perc(struct text_object *, char *, int);
double wireless_link_barval(struct text_object *);
#endif /* BUILD_WLAN */

void clear_net_stats(void);
void clear_net_stats(net_stat*);

void parse_if_up_arg(struct text_object *, const char *);
int interface_up(struct text_object *);
void free_if_up(struct text_object *);

void free_dns_data(struct text_object *);
int update_dns_data(void);
void parse_nameserver_arg(struct text_object *, const char *);
void print_nameserver(struct text_object *, char *, int);

#endif /* _NET_STAT_H */
