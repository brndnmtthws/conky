/* doesn't work, feel free to finish this */
#include "conky.h"
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
			knp =
			    (kstat_named_t *) kstat_data_lookup(ksp,
								"boot_time");
			if (knp != NULL) {
				return get_time() -
				    (double) knp->value.ui32;
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
