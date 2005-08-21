/*
 * ftp.h: interface for basic handling of an FTP command connection
 *        to check for directory availability. No transfer is needed.
 *
 *  Reference: RFC 959
 *
 *  $Id$
 */

#ifndef __MIRRORS_FTP_H__
#define __MIRRORS_FTP_H__
typedef void (*ftpListCallback) (void *userData,
				 const char *filename, const char *attrib,
				 const char *owner, const char *group,
				 unsigned long size, int links, int year,
				 const char *month, int day, int minute);

typedef void (*ftpDataCallback) (void *userData,
				 const char *data, int len);


extern void initFtp(void);
extern int connectFtp(const char *server, int port);
extern int changeFtpDirectory(char *directory);
extern int disconnectFtp(void);
int getFtp(ftpDataCallback, void *, const char *);

#endif				/* __MIRRORS_FTP_H__ */
