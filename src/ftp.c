/*
 * ftp.c: basic handling of an FTP command connection to check for
 *        directory availability. No transfer is needed.
 *
 *  Reference: RFC 959
 *
 *  $Id$
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <resolv.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

#include "ftp.h"

/* #define DEBUG_FTP 1  */
#ifdef STANDALONE
#define DEBUG_FTP 1
#endif

static int ftpFd = -1;
static struct sockaddr_in ftpAddr;
static char hostname[100];
static int ftpPassive = 1;
static int dataFd = -1;

#define FTP_COMMAND_OK          200
#define FTP_SYNTAX_ERROR        500
#define FTP_GET_PASSWD          331

/*
 * Initialize the FTP handling.
 */

void initFtp(void)
{
	gethostname(hostname, sizeof(hostname));
}

/*
 * Parsing of the server answer, we just extract the code.
 * return 0 for errors
 *     +XXX for last line of response
 *     -XXX for response to be continued
 */
int parseFtpResponse(char *buf, int len)
{
	int val = 0;

	if (len < 3)
		return (-1);
	if ((*buf >= '0') && (*buf <= '9'))
		val = val * 10 + (*buf - '0');
	else
		return (0);
	buf++;
	if ((*buf >= '0') && (*buf <= '9'))
		val = val * 10 + (*buf - '0');
	else
		return (0);
	buf++;
	if ((*buf >= '0') && (*buf <= '9'))
		val = val * 10 + (*buf - '0');
	else
		return (0);
	buf++;
	if (*buf == '-')
		return (-val);
	return (val);
}

/*
 * Read the response from the FTP server after a command.
 * Returns the code number
 *
 */
int readFtpResponse(char *buf, int size)
{
	char *ptr, *end;
	int len;
	int res = -1;

	if (size <= 0)
		return (-1);

      get_more:
	if ((len = recv(ftpFd, buf, size - 1, 0)) < 0) {
		close(ftpFd);
		ftpFd = -1;
		ftpFd = -1;
		return (-1);
	}
	if (len == 0) {
		return (-1);
	}

	end = &buf[len];
	*end = 0;
#ifdef DEBUG_FTP
	printf(buf);
#endif
	ptr = buf;
	while (ptr < end) {
		res = parseFtpResponse(ptr, end - ptr);
		if (res > 0)
			break;
		if (res == 0) {
#ifdef DEBUG_FTP
			fprintf(stderr, "readFtpResponse failed: %s\n",
				ptr);
#endif
			return (-1);
		}
		while ((ptr < end) && (*ptr != '\n'))
			ptr++;
		if (ptr >= end) {
#ifdef DEBUG_FTP
			fprintf(stderr,
				"readFtpResponse: unexpected end %s\n",
				buf);
#endif
			return ((-res) / 100);
		}
		if (*ptr != '\r')
			ptr++;
	}

	if (res < 0)
		goto get_more;

#ifdef DEBUG_FTP
	printf("Got %d\n", res);
#endif
	return (res / 100);
}

/*
 * Get the response from the FTP server after a command.
 * Returns the code number
 *
 */
int getFtpResponse(void)
{
	char buf[16 * 1024 + 1];

/**************
    fd_set rfd;
    struct timeval tv;
    int res;

    tv.tv_sec = 10;
    tv.tv_usec = 0;
    FD_ZERO(&rfd);
    FD_SET(ftpFd, &rfd);
    res = select(ftpFd + 1, &rfd, NULL, NULL, &tv);
    if (res <= 0) return(res);
 **************/

	return (readFtpResponse(buf, 16 * 1024));
}

/*
 * Check if there is a response from the FTP server after a command.
 * Returns the code number, or 0
 */
int checkFtpResponse(void)
{
	char buf[1024 + 1];
	fd_set rfd;
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&rfd);
	FD_SET(ftpFd, &rfd);
	switch (select(ftpFd + 1, &rfd, NULL, NULL, &tv)) {
	case 0:
		return (0);
	case -1:
#ifdef DEBUG_FTP
		perror("select");
#endif
		return (-1);

	}

	return (readFtpResponse(buf, 1024));
}

/*
 * Send the user authentification
 */

int sendUser(void)
{
	char buf[200];
	int len;
	int res;

	len = snprintf(buf, sizeof(buf), "USER anonymous\r\n");
#ifdef DEBUG_FTP
	printf(buf);
#endif
	res = send(ftpFd, buf, len, 0);
	if (res < 0)
		return (res);
	return (0);
}

/*
 * Send the password authentification
 */

int sendPasswd(void)
{
	char buf[200];
	int len;
	int res;

	len =
	    snprintf(buf, sizeof(buf), "PASS mirrorfind@%s\r\n", hostname);
#ifdef DEBUG_FTP
	printf(buf);
#endif
	res = send(ftpFd, buf, len, 0);
	if (res < 0)
		return (res);
	return (0);
}

/*
 * Send a QUIT
 */

int sendQuit(void)
{
	char buf[200];
	int len;
	int res;

	len = snprintf(buf, sizeof(buf), "QUIT\r\n");
#ifdef DEBUG_FTP
	printf(buf);
#endif
	res = send(ftpFd, buf, len, 0);
	return (0);
}

/*
 * Connecting to the server, port 21 by default.
 */

int connectFtp(const char *server, int port)
{
	struct hostent *hp;
	int res;

	/*
	 * do the blocking DNS query.
	 */
	hp = gethostbyname(server);
	if (hp == NULL)
		return (-1);

	/*
	 * Prepare the socket
	 */
	memset(&ftpAddr, 0, sizeof(ftpAddr));
	ftpAddr.sin_family = AF_INET;
	memcpy(&ftpAddr.sin_addr, hp->h_addr_list[0], hp->h_length);
	if (port == 0)
		port = 21;
	ftpAddr.sin_port = htons(port);
	ftpFd = socket(AF_INET, SOCK_STREAM, 0);
	if (ftpFd < 0)
		return (-1);

	/*
	 * Do the connect.
	 */
	if (connect(ftpFd, (struct sockaddr *) &ftpAddr,
		    sizeof(struct sockaddr_in)) < 0) {
		close(ftpFd);
		ftpFd = -1;
		ftpFd = -1;
		return (-1);
	}

	/*
	 * Wait for the HELLO from the server.
	 */
	res = getFtpResponse();
	if (res != 2) {
		close(ftpFd);
		ftpFd = -1;
		ftpFd = -1;
		return (-1);
	}

	/*
	 * State diagram for the login operation on the FTP server
	 *
	 * Reference: RFC 959
	 *
	 *                       1
	 * +---+   USER    +---+------------->+---+
	 * | B |---------->| W | 2       ---->| E |
	 * +---+           +---+------  |  -->+---+
	 *                  | |       | | |
	 *                3 | | 4,5   | | |
	 *    --------------   -----  | | |
	 *   |                      | | | |
	 *   |                      | | | |
	 *   |                 ---------  |
	 *   |               1|     | |   |
	 *   V                |     | |   |
	 * +---+   PASS    +---+ 2  |  ------>+---+
	 * |   |---------->| W |------------->| S |
	 * +---+           +---+   ---------->+---+
	 *                  | |   | |     |
	 *                3 | |4,5| |     |
	 *    --------------   --------   |
	 *   |                    | |  |  |
	 *   |                    | |  |  |
	 *   |                 -----------
	 *   |             1,3|   | |  |
	 *   V                |  2| |  |
	 * +---+   ACCT    +---+--  |   ----->+---+
	 * |   |---------->| W | 4,5 -------->| F |
	 * +---+           +---+------------->+---+
	 */
	res = sendUser();
	if (res < 0) {
		close(ftpFd);
		ftpFd = -1;
		ftpFd = -1;
		return (-1);
	}
	res = getFtpResponse();
	switch (res) {
	case 2:
		return (0);
	case 3:
		break;
	case 1:
	case 4:
	case 5:
	case -1:
	default:
		close(ftpFd);
		ftpFd = -1;
		ftpFd = -1;
		return (-1);
	}
	res = sendPasswd();
	if (res < 0) {
		close(ftpFd);
		ftpFd = -1;
		ftpFd = -1;
		return (-1);
	}
	res = getFtpResponse();
	switch (res) {
	case 2:
		return (0);
	case 3:
		fprintf(stderr,
			"FTP server asking for ACCNT on anonymous\n");
	case 1:
	case 4:
	case 5:
	case -1:
	default:
		close(ftpFd);
		ftpFd = -1;
		ftpFd = -1;
		return (-1);
	}

	return (0);
}

/*
 * Check an FTP directory on the server
 */

int changeFtpDirectory(char *directory)
{
	char buf[400];
	int len;
	int res;

	/*
	 * Expected response code for CWD:
	 *
	 * CWD
	 *     250
	 *     500, 501, 502, 421, 530, 550
	 */
	len = snprintf(buf, sizeof(buf), "CWD %s\r\n", directory);
#ifdef DEBUG_FTP
	printf(buf);
#endif
	res = send(ftpFd, buf, len, 0);
	if (res < 0)
		return (res);
	res = getFtpResponse();
	if (res == 4) {
		close(ftpFd);
		ftpFd = -1;
		ftpFd = -1;
		return (-1);
	}
	if (res == 2)
		return (1);
	if (res == 5) {
		return (0);
	}
	return (0);
}

/*
 * dataConnectFtp
 */
int dataConnectFtp()
{
	char buf[200];
	int len, i;
	int res;
	unsigned char ad[6], *cur, *adp, *portp;
	unsigned int temp[6];
	struct sockaddr_in dataAddr;
	socklen_t dataAddrLen;

	dataFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (dataFd < 0) {
		fprintf(stderr,
			"dataConnectFtp: failed to create socket\n");
	}
	dataAddrLen = sizeof(dataAddr);
	memset(&dataAddr, 0, dataAddrLen);
	dataAddr.sin_family = AF_INET;

	if (ftpPassive) {
		len = snprintf(buf, sizeof(buf), "PASV\r\n");
#ifdef DEBUG_FTP
		printf(buf);
#endif
		res = send(ftpFd, buf, len, 0);
		if (res < 0) {
			close(dataFd);
			dataFd = -1;
			return (res);
		}
		res = readFtpResponse(buf, sizeof(buf) - 1);
		if (res != 2) {
			if (res == 5) {
				close(dataFd);
				dataFd = -1;
				return (-1);
			} else {
				/*
				 * retry with an active connection
				 */
				close(dataFd);
				dataFd = -1;
				ftpPassive = 0;
			}
		}
		cur = &buf[4];
		while (((*cur < '0') || (*cur > '9')) && *cur != '\0')
			cur++;
		if (sscanf
		    (cur, "%d,%d,%d,%d,%d,%d", &temp[0], &temp[1],
		     &temp[2], &temp[3], &temp[4], &temp[5]) != 6) {
			fprintf(stderr, "Invalid answer to PASV\n");
			close(dataFd);
			dataFd = -1;
			return (-1);
		}
		for (i = 0; i < 6; i++)
			ad[i] = (unsigned char) (temp[i] & 0xff);
		memcpy(&dataAddr.sin_addr, &ad[0], 4);
		memcpy(&dataAddr.sin_port, &ad[4], 2);
		if (connect
		    (dataFd, (struct sockaddr *) &dataAddr,
		     dataAddrLen) < 0) {
			fprintf(stderr,
				"Failed to create a data connection\n");
			close(dataFd);
			dataFd = -1;
			return (-1);
		}
	} else {
		getsockname(dataFd, (struct sockaddr *) &dataAddr,
			    &dataAddrLen);
		dataAddr.sin_port = 0;
		if (bind
		    (dataFd, (struct sockaddr *) &dataAddr,
		     dataAddrLen) < 0) {
			fprintf(stderr, "Failed to bind a port\n");
			close(dataFd);
			dataFd = -1;
			return (-1);
		}
		getsockname(dataFd, (struct sockaddr *) &dataAddr,
			    &dataAddrLen);

		if (listen(dataFd, 1) < 0) {
			fprintf(stderr, "Could not listen on port %d\n",
				ntohs(dataAddr.sin_port));
			close(dataFd);
			dataFd = -1;
			return (-1);
		}
		adp = (unsigned char *) &dataAddr.sin_addr;
		portp = (unsigned char *) &dataAddr.sin_port;
		len =
		    snprintf(buf, sizeof(buf),
			     "PORT %d,%d,%d,%d,%d,%d\r\n", adp[0] & 0xff,
			     adp[1] & 0xff, adp[2] & 0xff, adp[3] & 0xff,
			     portp[0] & 0xff, portp[1] & 0xff);
		buf[sizeof(buf) - 1] = 0;
#ifdef DEBUG_FTP
		printf(buf);
#endif

		res = send(ftpFd, buf, len, 0);
		if (res < 0) {
			close(dataFd);
			dataFd = -1;
			return (res);
		}
		res = getFtpResponse();
		if (res != 2) {
			close(dataFd);
			dataFd = -1;
			return (-1);
		}
	}
	return (dataFd);

}

/*
 * dataConnectEndFtp
 */
int dataConnectEndFtp()
{
	int res;

	close(dataFd);
	dataFd = -1;
	res = getFtpResponse();
	if (res != 2) {
		close(dataFd);
		dataFd = -1;
		close(ftpFd);
		ftpFd = -1;
		return (-1);
	}
	return (0);
}

/*
 * parseListFtp
 */

int parseListFtp(const char *list, ftpListCallback callback,
		 void *userData)
{
	const char *cur = list;
	char filename[151];
	char attrib[11];
	char owner[11];
	char group[11];
	char month[4];
	int year = 0;
	int minute = 0;
	int hour = 0;
	int day = 0;
	unsigned long size = 0;
	int links = 0;
	int i;

	if (!strncmp(cur, "total", 5)) {
		cur += 5;
		while (*cur == ' ')
			cur++;
		while ((*cur >= '0') && (*cur <= '9'))
			links = (links * 10) + (*cur++ - '0');
		while ((*cur == ' ') || (*cur == '\n') || (*cur == '\r'))
			cur++;
		return (cur - list);
	} else if (*list == '+') {
		return (0);
	} else {
		while ((*cur == ' ') || (*cur == '\n') || (*cur == '\r'))
			cur++;
		if (*cur == 0)
			return (0);
		i = 0;
		while (*cur != ' ') {
			if (i < 10)
				attrib[i++] = *cur;
			cur++;
			if (*cur == 0)
				return (0);
		}
		attrib[10] = 0;
		while (*cur == ' ')
			cur++;
		if (*cur == 0)
			return (0);
		while ((*cur >= '0') && (*cur <= '9'))
			links = (links * 10) + (*cur++ - '0');
		while (*cur == ' ')
			cur++;
		if (*cur == 0)
			return (0);
		i = 0;
		while (*cur != ' ') {
			if (i < 10)
				owner[i++] = *cur;
			cur++;
			if (*cur == 0)
				return (0);
		}
		owner[i] = 0;
		while (*cur == ' ')
			cur++;
		if (*cur == 0)
			return (0);
		i = 0;
		while (*cur != ' ') {
			if (i < 10)
				group[i++] = *cur;
			cur++;
			if (*cur == 0)
				return (0);
		}
		group[i] = 0;
		while (*cur == ' ')
			cur++;
		if (*cur == 0)
			return (0);
		while ((*cur >= '0') && (*cur <= '9'))
			size = (size * 10) + (*cur++ - '0');
		while (*cur == ' ')
			cur++;
		if (*cur == 0)
			return (0);
		i = 0;
		while (*cur != ' ') {
			if (i < 3)
				month[i++] = *cur;
			cur++;
			if (*cur == 0)
				return (0);
		}
		month[i] = 0;
		while (*cur == ' ')
			cur++;
		if (*cur == 0)
			return (0);
		while ((*cur >= '0') && (*cur <= '9'))
			day = (day * 10) + (*cur++ - '0');
		while (*cur == ' ')
			cur++;
		if (*cur == 0)
			return (0);
		if ((cur[1] == 0) || (cur[2] == 0))
			return (0);
		if ((cur[1] == ':') || (cur[2] == ':')) {
			while ((*cur >= '0') && (*cur <= '9'))
				hour = (hour * 10) + (*cur++ - '0');
			if (*cur == ':')
				cur++;
			while ((*cur >= '0') && (*cur <= '9'))
				minute = (minute * 10) + (*cur++ - '0');
		} else {
			while ((*cur >= '0') && (*cur <= '9'))
				year = (year * 10) + (*cur++ - '0');
		}
		while (*cur == ' ')
			cur++;
		if (*cur == 0)
			return (0);
		i = 0;
		while ((*cur != '\n') && (*cur != '\r')) {
			if (i < 150)
				filename[i++] = *cur;
			cur++;
			if (*cur == 0)
				return (0);
		}
		filename[i] = 0;
		if ((*cur != '\n') && (*cur != '\r'))
			return (0);
		while ((*cur == '\n') || (*cur == '\r'))
			cur++;
	}
	if (callback != NULL) {
		callback(userData, filename, attrib, owner, group, size,
			 links, year, month, day, minute);
	}
	return (cur - list);
}

/*
 * listFtp
 */
int listFtp(ftpListCallback callback, void *userData)
{
	char buf[4096 + 1];
	int len, res;
	int index = 0, base;
	fd_set rfd, efd;
	struct timeval tv;

	dataFd = dataConnectFtp();

	len = snprintf(buf, sizeof(buf), "LIST -L\r\n");
#ifdef DEBUG_FTP
	printf(buf);
#endif
	res = send(ftpFd, buf, len, 0);
	if (res < 0) {
		close(dataFd);
		dataFd = -1;
		return (res);
	}
	res = readFtpResponse(buf, sizeof(buf) - 1);
	if (res != 1) {
		close(dataFd);
		dataFd = -1;
		return (-res);
	}

	do {
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		FD_ZERO(&rfd);
		FD_SET(dataFd, &rfd);
		FD_ZERO(&efd);
		FD_SET(dataFd, &efd);
		res = select(dataFd + 1, &rfd, NULL, &efd, &tv);
		if (res < 0) {
#ifdef DEBUG_FTP
			perror("select");
#endif
			close(dataFd);
			dataFd = -1;
			return (-1);
		}
		if (res == 0) {
			res = checkFtpResponse();
			if (res < 0) {
				close(dataFd);
				dataFd = -1;
				dataFd = -1;
				return (-1);
			}
			if (res == 2) {
				close(dataFd);
				dataFd = -1;
				return (0);
			}

			continue;
		}

		if ((len =
		     read(dataFd, &buf[index],
			  sizeof(buf) - (index + 1))) < 0) {
#ifdef DEBUG_FTP
			perror("read");
#endif
			close(dataFd);
			dataFd = -1;
			dataFd = -1;
			return (-1);
		}
#ifdef DEBUG_FTP
		write(1, &buf[index], len);
#endif
		index += len;
		buf[index] = 0;
		base = 0;
		do {
			res = parseListFtp(&buf[base], callback, userData);
			base += res;
		} while (res > 0);

		memmove(&buf[0], &buf[base], index - base);
		index -= base;
	} while (len != 0);
	dataConnectEndFtp();
	return (0);
}

/*
 * getFtpSocket:
 */

int getFtpSocket(const char *filename)
{
	char buf[300];
	int res, len;
	if (filename == NULL)
		return (-1);
	dataFd = dataConnectFtp();

	len = snprintf(buf, sizeof(buf), "TYPE I\r\n");
#ifdef DEBUG_FTP
	printf(buf);
#endif
	res = send(ftpFd, buf, len, 0);
	if (res < 0) {
		close(dataFd);
		dataFd = -1;
		return (res);
	}
	res = readFtpResponse(buf, sizeof(buf) - 1);
	if (res != 2) {
		close(dataFd);
		dataFd = -1;
		return (-res);
	}
	len = snprintf(buf, sizeof(buf), "RETR %s\r\n", filename);
#ifdef DEBUG_FTP
	printf(buf);
#endif
	res = send(ftpFd, buf, len, 0);
	if (res < 0) {
		close(dataFd);
		dataFd = -1;
		return (res);
	}
	res = readFtpResponse(buf, sizeof(buf) - 1);
	if (res != 1) {
		close(dataFd);
		dataFd = -1;
		return (-res);
	}
	return (dataFd);
}

/*
 * closeFtpSocket
 */

int closeFtpSocket()
{
	return (dataConnectEndFtp());
}

/*
 * listFtp
 */
int getFtp(ftpDataCallback callback, void *userData, const char *filename)
{
	char buf[4096];
	int len = 0, res;
	fd_set rfd;
	struct timeval tv;

	if (filename == NULL)
		return (-1);
	if (callback == NULL)
		return (-1);
	if (getFtpSocket(filename) < 0)
		return (-1);

	do {
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		FD_ZERO(&rfd);
		FD_SET(dataFd, &rfd);
		res = select(dataFd + 1, &rfd, NULL, NULL, &tv);
		if (res < 0) {
#ifdef DEBUG_FTP
			perror("select");
#endif
			close(dataFd);
			dataFd = -1;
			return (-1);
		}
		if (res == 0) {
			res = checkFtpResponse();
			if (res < 0) {
				close(dataFd);
				dataFd = -1;
				dataFd = -1;
				return (-1);
			}
			if (res == 2) {
				close(dataFd);
				dataFd = -1;
				return (0);
			}

			continue;
		}
		if ((len = read(dataFd, &buf, sizeof(buf))) < 0) {
			callback(userData, buf, len);
			close(dataFd);
			dataFd = -1;
			ftpFd = -1;
			return (-1);
		}
		callback(userData, buf, len);
	} while (len != 0);

	return (closeFtpSocket());
}

/*
 * Disconnect from the FTP server.
 */

int disconnectFtp(void)
{
	if (ftpFd < 0)
		return (-1);
	sendQuit();
	close(ftpFd);
	ftpFd = -1;
	ftpFd = -1;
	return (0);
}
