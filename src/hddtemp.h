#ifndef HDDTEMP_H_
#define HDDTEMP_H_

int scan_hddtemp(const char *arg, char **dev, char **addr, int *port);
char *get_hddtemp_info(char *dev, char *addr, int port);

#endif /*HDDTEMP_H_*/
