/* Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
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
 */

/* doesn't work, feel free to finish this */
#include "conky.h"
#include "common.h"
#include <kstat.h>

static kstat_ctl_t *kstat;
static int kstat_updated;

static void update_kstat()
{
	if (kstat == NULL) {
		kstat = kstat_open();
		if (kstat == NULL) {
			ERR("can't open kstat: %s", strerror(errno));
		}
	}

	if (kstat_chain_update(kstat) == -1) {
		perror("kstat_chain_update");
		return;
	}
}

void prepare_update()
{
	kstat_updated = 0;
}

double get_uptime()
{
	kstat_t *ksp;

	update_kstat();

	ksp = kstat_lookup(kstat, "unix", -1, "system_misc");
	if (ksp != NULL) {
		if (kstat_read(kstat, ksp, NULL) >= 0) {
			kstat_named_t *knp;

			knp = (kstat_named_t *) kstat_data_lookup(ksp, "boot_time");
			if (knp != NULL) {
				return get_time() - (double) knp->value.ui32;
			}
		}
	}
}

void update_meminfo()
{
	/* TODO */
}

int check_mount(char *s)
{
	/* stub */
	return 0;
}
