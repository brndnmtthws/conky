#ifndef _LINUX_H
#define _LINUX_H

#include "common.h"

const char *get_disk_protect_queue(const char *);

struct i8k_struct {
	char *version;
	char *bios;
	char *serial;
	char *cpu_temp;
	char *left_fan_status;
	char *right_fan_status;
	char *left_fan_rpm;
	char *right_fan_rpm;
	char *ac_status;
	char *buttons_status;
};

struct i8k_struct i8k;

int interface_up(const char *dev);
char *get_ioscheduler(char *);
int get_laptop_mode(void);
void update_gateway_info(void);

enum { PB_BATT_STATUS, PB_BATT_PERCENT, PB_BATT_TIME };
void get_powerbook_batt_info(char *, size_t, int);

#endif /* _LINUX_H */
