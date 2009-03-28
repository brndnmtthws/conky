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

char *get_ioscheduler(char *);
int get_laptop_mode(void);
void update_gateway_info(void);

enum { PB_BATT_STATUS, PB_BATT_PERCENT, PB_BATT_TIME };
void get_powerbook_batt_info(char *, size_t, int);

int open_sysfs_sensor(const char *dir, const char *dev, const char *type, int n,
	int *divisor, char *devtype);

#define open_i2c_sensor(dev, type, n, divisor, devtype) \
	open_sysfs_sensor("/sys/bus/i2c/devices/", dev, type, n, divisor, devtype)
#define open_platform_sensor(dev, type, n, divisor, devtype) \
	open_sysfs_sensor("/sys/bus/platform/devices/", dev, type, n, divisor, devtype)
#define open_hwmon_sensor(dev, type, n, divisor, devtype) \
	open_sysfs_sensor("/sys/class/hwmon/", dev, type, n, divisor, devtype)

double get_sysfs_info(int *fd, int arg, char *devtype, char *type);

#endif /* _LINUX_H */
