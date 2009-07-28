/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*- */

#ifndef _IBM_H
#define _IBM_H

#include <sys/types.h>

struct ibm_acpi_struct {
	int temps[8];
};

struct ibm_acpi_struct ibm_acpi;

void get_ibm_acpi_fan(char *buf, size_t client_buffer_size);
void get_ibm_acpi_temps(void);
void get_ibm_acpi_volume(char *buf, size_t client_buffer_size);
void get_ibm_acpi_brightness(char *buf, size_t client_buffer_size);

#endif /* _IBM_H */
