#ifndef LINUX_H_
#define LINUX_H_

#include "common.h"

void get_ibm_acpi_fan(char *buf, size_t client_buffer_size);
void get_ibm_acpi_temps(void);
void get_ibm_acpi_volume(char *buf, size_t client_buffer_size);
void get_ibm_acpi_brightness(char *buf, size_t client_buffer_size);
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

struct ibm_acpi_struct {
	int temps[8];
};

struct ibm_acpi_struct ibm_acpi;

int interface_up(const char *dev);
char *get_ioscheduler(char *);
int get_laptop_mode(void);
void update_gateway_info(void);

enum { PB_BATT_STATUS, PB_BATT_PERCENT, PB_BATT_TIME };
void get_powerbook_batt_info(char *, size_t, int);

#endif /*LINUX_H_*/
