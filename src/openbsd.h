/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*- */

#ifndef OPENBSD_H_
#define OPENBSD_H_

#include "common.h"
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/sensors.h>
#include <machine/apmvar.h>

void update_obsd_sensors(void);
void get_obsd_vendor(char *buf, size_t client_buffer_size);
void get_obsd_product(char *buf, size_t client_buffer_size);

#define OBSD_MAX_SENSORS 256
struct obsd_sensors_struct {
       int device;
       float temp[MAXSENSORDEVICES][OBSD_MAX_SENSORS];
       unsigned int fan[MAXSENSORDEVICES][OBSD_MAX_SENSORS];
       float volt[MAXSENSORDEVICES][OBSD_MAX_SENSORS];
};
struct obsd_sensors_struct obsd_sensors;

#if defined(i386) || defined(__i386__)
typedef struct apm_power_info *apm_info_t;
#endif

#endif /*OPENBSD_H_*/
#ifndef OPENBSD_H_
#define OPENBSD_H_

#include "common.h"
#include <sys/sysctl.h>
#include <sys/sensors.h>
#include <machine/apmvar.h>

void update_obsd_sensors(void);
void get_obsd_vendor(char *buf, size_t client_buffer_size);
void get_obsd_product(char *buf, size_t client_buffer_size);

#define OBSD_MAX_SENSORS 256
struct obsd_sensors_struct {
	int device;
	float temp[MAXSENSORDEVICES][OBSD_MAX_SENSORS];
	unsigned int fan[MAXSENSORDEVICES][OBSD_MAX_SENSORS];
	float volt[MAXSENSORDEVICES][OBSD_MAX_SENSORS];
};
struct obsd_sensors_struct obsd_sensors;

#if defined(i386) || defined(__i386__)
typedef struct apm_power_info *apm_info_t;
#endif

#endif /*OPENBSD_H_*/
