/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*- */

#ifndef HDDTEMP_H_
#define HDDTEMP_H_

int scan_hddtemp(const char *arg, char **dev, char **addr, int *port);
char *get_hddtemp_info(char *dev, char *addr, int port);

#endif /*HDDTEMP_H_*/
