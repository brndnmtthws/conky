#ifndef HDDTEMP_H_
#define HDDTEMP_H_

int scan_hddtemp(const char *arg, char **dev, char **addr, int *port, char **temp);
char *get_hddtemp_info(char *dev, char *addr, int port, char *unit);

#endif /*HDDTEMP_H_*/
