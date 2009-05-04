/* conky support for information from sony_laptop kernel module
 *   information from sony_laptop kernel module
 *   /sys/devices/platform/sony-laptop
 *   I mimicked the methods from ibm.c
 * Yeon-Hyeong Yang <lbird94@gmail.com> */

#ifndef _SONY_H
#define _SONY_H

#include <sys/types.h>

void get_sony_fanspeed(char *buf, size_t client_buffer_size);

#endif /* _SONY_H */
