/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*- */

#ifndef HDDTEMP_H_
#define HDDTEMP_H_

void set_hddtemp_host(const char *);
void set_hddtemp_port(const char *);
void update_hddtemp(void);
void free_hddtemp(void);
int get_hddtemp_info(const char *, short *, char *);

#endif /*HDDTEMP_H_*/
