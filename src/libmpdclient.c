/* libmpdclient
   (c)2003-2006 by Warren Dukes (warren.dukes@gmail.com)
   This project's homepage is: http://www.musicpd.org

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of the Music Player Daemon nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "libmpdclient.h"

#include <errno.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/param.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#ifdef WIN32
#  include <ws2tcpip.h>
#  include <winsock.h>
#else
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <sys/socket.h>
#  include <netdb.h>
#endif

#ifndef MSG_DONTWAIT
#  define MSG_DONTWAIT 0
#endif

#ifndef MPD_NO_GAI
#  ifdef AI_ADDRCONFIG
#    define MPD_HAVE_GAI
#  endif
#endif

#define COMMAND_LIST    1
#define COMMAND_LIST_OK 2

#ifdef WIN32
#  define SELECT_ERRNO_IGNORE   (errno == WSAEINTR || errno == WSAEINPROGRESS)
#  define SENDRECV_ERRNO_IGNORE SELECT_ERRNO_IGNORE
#else
#  define SELECT_ERRNO_IGNORE   (errno == EINTR)
#  define SENDRECV_ERRNO_IGNORE (errno == EINTR || errno == EAGAIN)
#  define winsock_dll_error(c)  0
#  define closesocket(s)        close(s)
#  define WSACleanup()          do { /* nothing */ } while (0)
#endif

#ifdef WIN32
static int winsock_dll_error(mpd_Connection *connection)
{
	WSADATA wsaData;
	if ((WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0 ||
			LOBYTE(wsaData.wVersion) != 2 ||
			HIBYTE(wsaData.wVersion) != 2 ) {
		snprintf(connection->errorStr,MPD_BUFFER_MAX_LENGTH,
				"Could not find usable WinSock DLL.");
		connection->error = MPD_ERROR_SYSTEM;
		return 1;
	}
	return 0;
}

static int do_connect_fail(mpd_Connection *connection,
                           const struct sockaddr *serv_addr, int addrlen)
{
	int iMode = 1; /* 0 = blocking, else non-blocking */
	ioctlsocket(connection->sock, FIONBIO, (u_long FAR*) &iMode);
	return (connect(connection->sock,serv_addr,addrlen) == SOCKET_ERROR
			&& WSAGetLastError() != WSAEWOULDBLOCK);
}
#else /* !WIN32 (sane operating systems) */
static int do_connect_fail(mpd_Connection *connection,
                           const struct sockaddr *serv_addr, int addrlen)
{
	int flags = fcntl(connection->sock, F_GETFL, 0);
	fcntl(connection->sock, F_SETFL, flags | O_NONBLOCK);
	return (connect(connection->sock,serv_addr,addrlen)<0 &&
				errno!=EINPROGRESS);
}
#endif /* !WIN32 */

#ifdef MPD_HAVE_GAI
static int mpd_connect(mpd_Connection * connection, const char * host, int port,
                       float timeout)
{
	int error;
	char service[20];
	struct addrinfo hints;
	struct addrinfo *res = NULL;
	struct addrinfo *addrinfo = NULL;

	/**
	 * Setup hints
	 */
	hints.ai_flags     = AI_ADDRCONFIG;
	hints.ai_family    = PF_UNSPEC;
	hints.ai_socktype  = SOCK_STREAM;
	hints.ai_protocol  = IPPROTO_TCP;
	hints.ai_addrlen   = 0;
	hints.ai_addr      = NULL;
	hints.ai_canonname = NULL;
	hints.ai_next      = NULL;

	snprintf(service, sizeof(service), "%d", port);

	error = getaddrinfo(host, service, &hints, &addrinfo);

	if (error) {
		snprintf(connection->errorStr,MPD_BUFFER_MAX_LENGTH,
				"host \"%s\" not found: %s",host, gai_strerror(error));
		connection->error = MPD_ERROR_UNKHOST;
		return -1;
	}

	for (res = addrinfo; res; res = res->ai_next) {
		/* create socket */
		connection->sock = socket(res->ai_family, SOCK_STREAM, res->ai_protocol);
		if (connection->sock < 0) {
			snprintf(connection->errorStr, MPD_BUFFER_MAX_LENGTH,
			         "problems creating socket: %s",
			         strerror(errno));
			connection->error = MPD_ERROR_SYSTEM;
			freeaddrinfo(addrinfo);
			return -1;
		}

		mpd_setConnectionTimeout(connection,timeout);

		/* connect stuff */
 		if (do_connect_fail(connection, res->ai_addr, res->ai_addrlen)) {
 			/* try the next address family */
 			closesocket(connection->sock);
 			connection->sock = -1;
 			continue;
		}
	}
	freeaddrinfo(addrinfo);

	if (connection->sock < 0) {
		snprintf(connection->errorStr,MPD_BUFFER_MAX_LENGTH,
				"problems connecting to \"%s\" on port"
				" %i: %s",host,port, strerror(errno));
		connection->error = MPD_ERROR_CONNPORT;

		return -1;
	}

	return 0;
}
#else /* !MPD_HAVE_GAI */
static int mpd_connect(mpd_Connection * connection, const char * host, int port,
                       float timeout)
{
	struct hostent * he;
	struct sockaddr * dest;
	int destlen;
	struct sockaddr_in sin;

	if(!(he=gethostbyname(host))) {
		snprintf(connection->errorStr,MPD_BUFFER_MAX_LENGTH,
				"host \"%s\" not found",host);
		connection->error = MPD_ERROR_UNKHOST;
		return -1;
	}

	memset(&sin,0,sizeof(struct sockaddr_in));
	/*dest.sin_family = he->h_addrtype;*/
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	switch(he->h_addrtype) {
	case AF_INET:
		memcpy((char *)&sin.sin_addr.s_addr,(char *)he->h_addr,
				he->h_length);
		dest = (struct sockaddr *)&sin;
		destlen = sizeof(struct sockaddr_in);
		break;
	default:
		strcpy(connection->errorStr,"address type is not IPv4\n");
		connection->error = MPD_ERROR_SYSTEM;
		return -1;
		break;
	}

	if((connection->sock = socket(dest->sa_family,SOCK_STREAM,0))<0) {
		strcpy(connection->errorStr,"problems creating socket");
		connection->error = MPD_ERROR_SYSTEM;
		return -1;
	}

	mpd_setConnectionTimeout(connection,timeout);

	/* connect stuff */
	if (do_connect_fail(connection, dest, destlen)) {
		snprintf(connection->errorStr,MPD_BUFFER_MAX_LENGTH,
				"problems connecting to \"%s\" on port"
				" %i",host,port);
		connection->error = MPD_ERROR_CONNPORT;
		return -1;
	}

	return 0;
}
#endif /* !MPD_HAVE_GAI */

char * mpdTagItemKeys[MPD_TAG_NUM_OF_ITEM_TYPES] =
{
	"Artist",
	"Album",
	"Title",
	"Track",
	"Name",
	"Genre",
	"Date",
	"Composer",
	"Performer",
	"Comment",
	"Disc",
	"filename"
};

static char * mpd_sanitizeArg(const char * arg) {
	size_t i;
	char * ret;
	register const char *c;
	register char *rc;

	/* instead of counting in that loop above, just
	 * use a bit more memory and half running time
	 */
	ret = malloc(strlen(arg) * 2 + 1);

	c = arg;
	rc = ret;
	for(i = strlen(arg)+1; i != 0; --i) {
		if(*c=='"' || *c=='\\')
			*rc++ = '\\';
		*(rc++) = *(c++);
	}

	return ret;
}

static mpd_ReturnElement * mpd_newReturnElement(const char * name, const char * value)
{
	mpd_ReturnElement * ret = malloc(sizeof(mpd_ReturnElement));

	ret->name = strdup(name);
	ret->value = strdup(value);

	return ret;
}

static void mpd_freeReturnElement(mpd_ReturnElement * re) {
	free(re->name);
	free(re->value);
	free(re);
}

void mpd_setConnectionTimeout(mpd_Connection * connection, float timeout) {
	connection->timeout.tv_sec = (int)timeout;
	connection->timeout.tv_usec = (int)(timeout*1e6 -
	                                    connection->timeout.tv_sec*1000000 +
					    0.5);
}

static int mpd_parseWelcome(mpd_Connection * connection, const char * host, int port,
                            char * rt, char * output) {
	char * tmp;
	char * test;
	int i;

	if(strncmp(output,MPD_WELCOME_MESSAGE,strlen(MPD_WELCOME_MESSAGE))) {
		snprintf(connection->errorStr,MPD_BUFFER_MAX_LENGTH,
				"mpd not running on port %i on host \"%s\"",
				port,host);
		connection->error = MPD_ERROR_NOTMPD;
		return 1;
	}

	tmp = &output[strlen(MPD_WELCOME_MESSAGE)];

	for(i=0;i<3;i++) {
		if(tmp) connection->version[i] = strtol(tmp,&test,10);

		if (!tmp || (test[0] != '.' && test[0] != '\0')) {
			snprintf(connection->errorStr,
			         MPD_BUFFER_MAX_LENGTH,
			         "error parsing version number at "
			         "\"%s\"",
			         &output[strlen(MPD_WELCOME_MESSAGE)]);
			connection->error = MPD_ERROR_NOTMPD;
			return 1;
		}
		tmp = ++test;
	}

	return 0;
}

mpd_Connection * mpd_newConnection(const char * host, int port, float timeout) {
	int err;
	char * rt;
	char * output =  NULL;
	mpd_Connection * connection = malloc(sizeof(mpd_Connection));
	struct timeval tv;
	fd_set fds;
	strcpy(connection->buffer,"");
	connection->buflen = 0;
	connection->bufstart = 0;
	strcpy(connection->errorStr,"");
	connection->error = 0;
	connection->doneProcessing = 0;
	connection->commandList = 0;
	connection->listOks = 0;
	connection->doneListOk = 0;
	connection->returnElement = NULL;
	connection->request = NULL;

	if (winsock_dll_error(connection))
		return connection;

	if (mpd_connect(connection, host, port, timeout) < 0)
		return connection;

	while(!(rt = strstr(connection->buffer,"\n"))) {
		tv.tv_sec = connection->timeout.tv_sec;
		tv.tv_usec = connection->timeout.tv_usec;
		FD_ZERO(&fds);
		FD_SET(connection->sock,&fds);
		if((err = select(connection->sock+1,&fds,NULL,NULL,&tv)) == 1) {
			int readed;
			readed = recv(connection->sock,
					&(connection->buffer[connection->buflen]),
					MPD_BUFFER_MAX_LENGTH-connection->buflen,0);
			if(readed<=0) {
				snprintf(connection->errorStr,MPD_BUFFER_MAX_LENGTH,
						"problems getting a response from"
						" \"%s\" on port %i : %s",host,
						port, strerror(errno));
				connection->error = MPD_ERROR_NORESPONSE;
				return connection;
			}
			connection->buflen+=readed;
			connection->buffer[connection->buflen] = '\0';
		}
		else if(err<0) {
 			if (SELECT_ERRNO_IGNORE)
				continue;
			snprintf(connection->errorStr,
					MPD_BUFFER_MAX_LENGTH,
					"problems connecting to \"%s\" on port"
					" %i",host,port);
			connection->error = MPD_ERROR_CONNPORT;
			return connection;
		}
		else {
			snprintf(connection->errorStr,MPD_BUFFER_MAX_LENGTH,
					"timeout in attempting to get a response from"
					" \"%s\" on port %i",host,port);
			connection->error = MPD_ERROR_NORESPONSE;
			return connection;
		}
	}

	*rt = '\0';
	output = strdup(connection->buffer);
	strcpy(connection->buffer,rt+1);
	connection->buflen = strlen(connection->buffer);

	if(mpd_parseWelcome(connection,host,port,rt,output) == 0) connection->doneProcessing = 1;

	free(output);

	return connection;
}

void mpd_clearError(mpd_Connection * connection) {
	connection->error = 0;
	connection->errorStr[0] = '\0';
}

void mpd_closeConnection(mpd_Connection * connection) {
	closesocket(connection->sock);
	if(connection->returnElement) free(connection->returnElement);
	if(connection->request) free(connection->request);
	free(connection);
	WSACleanup();
}

static void mpd_executeCommand(mpd_Connection * connection, char * command) {
	int ret;
	struct timeval tv;
	fd_set fds;
	char * commandPtr = command;
	int commandLen = strlen(command);

	if(!connection->doneProcessing && !connection->commandList) {
		strcpy(connection->errorStr,"not done processing current command");
		connection->error = 1;
		return;
	}

	mpd_clearError(connection);

	FD_ZERO(&fds);
	FD_SET(connection->sock,&fds);
	tv.tv_sec = connection->timeout.tv_sec;
	tv.tv_usec = connection->timeout.tv_usec;

	while((ret = select(connection->sock+1,NULL,&fds,NULL,&tv)==1) ||
			(ret==-1 && SELECT_ERRNO_IGNORE)) {
		ret = send(connection->sock,commandPtr,commandLen,MSG_DONTWAIT);
		if(ret<=0)
		{
			if (SENDRECV_ERRNO_IGNORE) continue;
			snprintf(connection->errorStr,MPD_BUFFER_MAX_LENGTH,
			         "problems giving command \"%s\"",command);
			connection->error = MPD_ERROR_SENDING;
			return;
		}
		else {
			commandPtr+=ret;
			commandLen-=ret;
		}

		if(commandLen<=0) break;
	}

	if(commandLen>0) {
		perror("");
		snprintf(connection->errorStr,MPD_BUFFER_MAX_LENGTH,
		         "timeout sending command \"%s\"",command);
		connection->error = MPD_ERROR_TIMEOUT;
		return;
	}

	if(!connection->commandList) connection->doneProcessing = 0;
	else if(connection->commandList == COMMAND_LIST_OK) {
		connection->listOks++;
	}
}

static void mpd_getNextReturnElement(mpd_Connection * connection) {
	char * output = NULL;
	char * rt = NULL;
	char * name = NULL;
	char * value = NULL;
	fd_set fds;
	struct timeval tv;
	char * tok = NULL;
	int readed;
	char * bufferCheck = NULL;
	int err;
	int pos;

	if(connection->returnElement) mpd_freeReturnElement(connection->returnElement);
	connection->returnElement = NULL;

	if(connection->doneProcessing || (connection->listOks &&
	   connection->doneListOk))
	{
		strcpy(connection->errorStr,"already done processing current command");
		connection->error = 1;
		return;
	}

	bufferCheck = connection->buffer+connection->bufstart;
	while(connection->bufstart>=connection->buflen ||
			!(rt = strchr(bufferCheck,'\n'))) {
		if(connection->buflen>=MPD_BUFFER_MAX_LENGTH) {
			memmove(connection->buffer,
					connection->buffer+
					connection->bufstart,
					connection->buflen-
					connection->bufstart+1);
			connection->buflen-=connection->bufstart;
			connection->bufstart = 0;
		}
		if(connection->buflen>=MPD_BUFFER_MAX_LENGTH) {
			strcpy(connection->errorStr,"buffer overrun");
			connection->error = MPD_ERROR_BUFFEROVERRUN;
			connection->doneProcessing = 1;
			connection->doneListOk = 0;
			return;
		}
		bufferCheck = connection->buffer+connection->buflen;
		tv.tv_sec = connection->timeout.tv_sec;
		tv.tv_usec = connection->timeout.tv_usec;
		FD_ZERO(&fds);
		FD_SET(connection->sock,&fds);
		if((err = select(connection->sock+1,&fds,NULL,NULL,&tv) == 1)) {
			readed = recv(connection->sock,
					connection->buffer+connection->buflen,
					MPD_BUFFER_MAX_LENGTH-connection->buflen,
					MSG_DONTWAIT);
			if(readed<0 && SENDRECV_ERRNO_IGNORE) {
				continue;
			}
			if(readed<=0) {
				strcpy(connection->errorStr,"connection"
				       " closed");
				connection->error = MPD_ERROR_CONNCLOSED;
				connection->doneProcessing = 1;
				connection->doneListOk = 0;
				return;
			}
			connection->buflen+=readed;
			connection->buffer[connection->buflen] = '\0';
		}
		else if(err<0 && SELECT_ERRNO_IGNORE) continue;
		else {
			strcpy(connection->errorStr,"connection timeout");
			connection->error = MPD_ERROR_TIMEOUT;
			connection->doneProcessing = 1;
			connection->doneListOk = 0;
			return;
		}
	}

	*rt = '\0';
	output = connection->buffer+connection->bufstart;
	connection->bufstart = rt - connection->buffer + 1;

	if(strcmp(output,"OK")==0) {
		if(connection->listOks > 0) {
			strcpy(connection->errorStr, "expected more list_OK's");
			connection->error = 1;
		}
		connection->listOks = 0;
		connection->doneProcessing = 1;
		connection->doneListOk = 0;
		return;
	}

	if(strcmp(output, "list_OK") == 0) {
		if(!connection->listOks) {
			strcpy(connection->errorStr,
					"got an unexpected list_OK");
			connection->error = 1;
		}
		else {
			connection->doneListOk = 1;
			connection->listOks--;
		}
		return;
	}

	if(strncmp(output,"ACK",strlen("ACK"))==0) {
		char * test;
		char * needle;
		int val;

		strcpy(connection->errorStr, output);
		connection->error = MPD_ERROR_ACK;
		connection->errorCode = MPD_ACK_ERROR_UNK;
		connection->errorAt = MPD_ERROR_AT_UNK;
		connection->doneProcessing = 1;
		connection->doneListOk = 0;

		needle = strchr(output, '[');
		if(!needle) return;
		val = strtol(needle+1, &test, 10);
		if(*test != '@') return;
		connection->errorCode = val;
		val = strtol(test+1, &test, 10);
		if(*test != ']') return;
		connection->errorAt = val;
		return;
	}

	tok = strchr(output, ':');
	if (!tok) return;
	pos = tok - output;
	value = ++tok;
	name = output;
	name[pos] = '\0';

	if(value[0]==' ') {
		connection->returnElement = mpd_newReturnElement(name,&(value[1]));
	}
	else {
		snprintf(connection->errorStr,MPD_BUFFER_MAX_LENGTH,
					"error parsing: %s:%s",name,value);
		connection->errorStr[MPD_BUFFER_MAX_LENGTH] = '\0';
		connection->error = 1;
	}
}

void mpd_finishCommand(mpd_Connection * connection) {
	while(!connection->doneProcessing) {
		if(connection->doneListOk) connection->doneListOk = 0;
		mpd_getNextReturnElement(connection);
	}
}

static void mpd_finishListOkCommand(mpd_Connection * connection) {
	while(!connection->doneProcessing && connection->listOks &&
			!connection->doneListOk)
	{
		mpd_getNextReturnElement(connection);
	}
}

int mpd_nextListOkCommand(mpd_Connection * connection) {
	mpd_finishListOkCommand(connection);
	if(!connection->doneProcessing) connection->doneListOk = 0;
	if(connection->listOks == 0 || connection->doneProcessing) return -1;
	return 0;
}

void mpd_sendStatusCommand(mpd_Connection * connection) {
	mpd_executeCommand(connection,"status\n");
}

mpd_Status * mpd_getStatus(mpd_Connection * connection) {
	mpd_Status * status;

	/*mpd_executeCommand(connection,"status\n");

	if(connection->error) return NULL;*/

	if(connection->doneProcessing || (connection->listOks &&
	   connection->doneListOk))
	{
		return NULL;
	}

	if(!connection->returnElement) mpd_getNextReturnElement(connection);

	status = malloc(sizeof(mpd_Status));
	status->volume = -1;
	status->repeat = 0;
	status->random = 0;
	status->playlist = -1;
	status->playlistLength = -1;
	status->state = -1;
	status->song = 0;
	status->songid = 0;
	status->elapsedTime = 0;
	status->totalTime = 0;
	status->bitRate = 0;
	status->sampleRate = 0;
	status->bits = 0;
	status->channels = 0;
	status->crossfade = -1;
	status->error = NULL;
	status->updatingDb = 0;

	if(connection->error) {
		free(status);
		return NULL;
	}
	while(connection->returnElement) {
		mpd_ReturnElement * re = connection->returnElement;
		if(strcmp(re->name,"volume")==0) {
			status->volume = atoi(re->value);
		}
		else if(strcmp(re->name,"repeat")==0) {
			status->repeat = atoi(re->value);
		}
		else if(strcmp(re->name,"random")==0) {
			status->random = atoi(re->value);
		}
		else if(strcmp(re->name,"playlist")==0) {
			status->playlist = strtol(re->value,NULL,10);
		}
		else if(strcmp(re->name,"playlistlength")==0) {
			status->playlistLength = atoi(re->value);
		}
		else if(strcmp(re->name,"bitrate")==0) {
			status->bitRate = atoi(re->value);
		}
		else if(strcmp(re->name,"state")==0) {
			if(strcmp(re->value,"play")==0) {
				status->state = MPD_STATUS_STATE_PLAY;
			}
			else if(strcmp(re->value,"stop")==0) {
				status->state = MPD_STATUS_STATE_STOP;
			}
			else if(strcmp(re->value,"pause")==0) {
				status->state = MPD_STATUS_STATE_PAUSE;
			}
			else {
				status->state = MPD_STATUS_STATE_UNKNOWN;
			}
		}
		else if(strcmp(re->name,"song")==0) {
			status->song = atoi(re->value);
		}
		else if(strcmp(re->name,"songid")==0) {
			status->songid = atoi(re->value);
		}
		else if(strcmp(re->name,"time")==0) {
			char * tok = strchr(re->value,':');
			/* the second strchr below is a safety check */
			if (tok && (strchr(tok,0) > (tok+1))) {
				/* atoi stops at the first non-[0-9] char: */
				status->elapsedTime = atoi(re->value);
				status->totalTime = atoi(tok+1);
			}
		}
		else if(strcmp(re->name,"error")==0) {
			status->error = strdup(re->value);
		}
		else if(strcmp(re->name,"xfade")==0) {
			status->crossfade = atoi(re->value);
		}
		else if(strcmp(re->name,"updating_db")==0) {
			status->updatingDb = atoi(re->value);
		}
		else if(strcmp(re->name,"audio")==0) {
			char * tok = strchr(re->value,':');
			if (tok && (strchr(tok,0) > (tok+1))) {
				status->sampleRate = atoi(re->value);
				status->bits = atoi(++tok);
				tok = strchr(tok,':');
				if (tok && (strchr(tok,0) > (tok+1)))
					status->channels = atoi(tok+1);
			}
		}

		mpd_getNextReturnElement(connection);
		if(connection->error) {
			free(status);
			return NULL;
		}
	}

	if(connection->error) {
		free(status);
		return NULL;
	}
	else if(status->state<0) {
		strcpy(connection->errorStr,"state not found");
		connection->error = 1;
		free(status);
		return NULL;
	}

	return status;
}

void mpd_freeStatus(mpd_Status * status) {
	if(status->error) free(status->error);
	free(status);
}

void mpd_sendStatsCommand(mpd_Connection * connection) {
	mpd_executeCommand(connection,"stats\n");
}

mpd_Stats * mpd_getStats(mpd_Connection * connection) {
	mpd_Stats * stats;

	/*mpd_executeCommand(connection,"stats\n");

	if(connection->error) return NULL;*/

	if(connection->doneProcessing || (connection->listOks &&
	   connection->doneListOk))
	{
		return NULL;
	}

	if(!connection->returnElement) mpd_getNextReturnElement(connection);

	stats = malloc(sizeof(mpd_Stats));
	stats->numberOfArtists = 0;
	stats->numberOfAlbums = 0;
	stats->numberOfSongs = 0;
	stats->uptime = 0;
	stats->dbUpdateTime = 0;
	stats->playTime = 0;
	stats->dbPlayTime = 0;

	if(connection->error) {
		free(stats);
		return NULL;
	}
	while(connection->returnElement) {
		mpd_ReturnElement * re = connection->returnElement;
		if(strcmp(re->name,"artists")==0) {
			stats->numberOfArtists = atoi(re->value);
		}
		else if(strcmp(re->name,"albums")==0) {
			stats->numberOfAlbums = atoi(re->value);
		}
		else if(strcmp(re->name,"songs")==0) {
			stats->numberOfSongs = atoi(re->value);
		}
		else if(strcmp(re->name,"uptime")==0) {
			stats->uptime = strtol(re->value,NULL,10);
		}
		else if(strcmp(re->name,"db_update")==0) {
			stats->dbUpdateTime = strtol(re->value,NULL,10);
		}
		else if(strcmp(re->name,"playtime")==0) {
			stats->playTime = strtol(re->value,NULL,10);
		}
		else if(strcmp(re->name,"db_playtime")==0) {
			stats->dbPlayTime = strtol(re->value,NULL,10);
		}

		mpd_getNextReturnElement(connection);
		if(connection->error) {
			free(stats);
			return NULL;
		}
	}

	if(connection->error) {
		free(stats);
		return NULL;
	}

	return stats;
}

void mpd_freeStats(mpd_Stats * stats) {
	free(stats);
}

static void mpd_initSong(mpd_Song * song) {
	song->file = NULL;
	song->artist = NULL;
	song->album = NULL;
	song->track = NULL;
	song->title = NULL;
	song->name = NULL;
	song->date = NULL;
	/* added by Qball */
	song->genre = NULL;
	song->composer = NULL;
	song->disc = NULL;
	song->comment = NULL;

	song->time = MPD_SONG_NO_TIME;
	song->pos = MPD_SONG_NO_NUM;
	song->id = MPD_SONG_NO_ID;
}

static void mpd_finishSong(mpd_Song * song) {
	if(song->file) free(song->file);
	if(song->artist) free(song->artist);
	if(song->album) free(song->album);
	if(song->title) free(song->title);
	if(song->track) free(song->track);
	if(song->name) free(song->name);
	if(song->date) free(song->date);
	if(song->genre) free(song->genre);
	if(song->composer) free(song->composer);
	if(song->disc) free(song->disc);
	if(song->comment) free(song->comment);
}

mpd_Song * mpd_newSong(void) {
	mpd_Song * ret = malloc(sizeof(mpd_Song));

	mpd_initSong(ret);

	return ret;
}

void mpd_freeSong(mpd_Song * song) {
	mpd_finishSong(song);
	free(song);
}

mpd_Song * mpd_songDup(mpd_Song * song) {
	mpd_Song * ret = mpd_newSong();

	if(song->file) ret->file = strdup(song->file);
	if(song->artist) ret->artist = strdup(song->artist);
	if(song->album) ret->album = strdup(song->album);
	if(song->title) ret->title = strdup(song->title);
	if(song->track) ret->track = strdup(song->track);
	if(song->name) ret->name = strdup(song->name);
	if(song->date) ret->date = strdup(song->date);
	if(song->genre) ret->genre= strdup(song->genre);
	if(song->composer) ret->composer= strdup(song->composer);
	if(song->disc) ret->disc = strdup(song->disc);
	if(song->comment) ret->comment = strdup(song->comment);
	ret->time = song->time;
	ret->pos = song->pos;
	ret->id = song->id;

	return ret;
}

static void mpd_initDirectory(mpd_Directory * directory) {
	directory->path = NULL;
}

static void mpd_finishDirectory(mpd_Directory * directory) {
	if(directory->path) free(directory->path);
}

mpd_Directory * mpd_newDirectory(void) {
	mpd_Directory * directory = malloc(sizeof(mpd_Directory));;

	mpd_initDirectory(directory);

	return directory;
}

void mpd_freeDirectory(mpd_Directory * directory) {
	mpd_finishDirectory(directory);

	free(directory);
}

mpd_Directory * mpd_directoryDup(mpd_Directory * directory) {
	mpd_Directory * ret = mpd_newDirectory();

	if(directory->path) ret->path = strdup(directory->path);

	return ret;
}

static void mpd_initPlaylistFile(mpd_PlaylistFile * playlist) {
	playlist->path = NULL;
}

static void mpd_finishPlaylistFile(mpd_PlaylistFile * playlist) {
	if(playlist->path) free(playlist->path);
}

mpd_PlaylistFile * mpd_newPlaylistFile(void) {
	mpd_PlaylistFile * playlist = malloc(sizeof(mpd_PlaylistFile));

	mpd_initPlaylistFile(playlist);

	return playlist;
}

void mpd_freePlaylistFile(mpd_PlaylistFile * playlist) {
	mpd_finishPlaylistFile(playlist);
	free(playlist);
}

mpd_PlaylistFile * mpd_playlistFileDup(mpd_PlaylistFile * playlist) {
	mpd_PlaylistFile * ret = mpd_newPlaylistFile();

	if(playlist->path) ret->path = strdup(playlist->path);

	return ret;
}

static void mpd_initInfoEntity(mpd_InfoEntity * entity) {
	entity->info.directory = NULL;
}

static void mpd_finishInfoEntity(mpd_InfoEntity * entity) {
	if(entity->info.directory) {
		if(entity->type == MPD_INFO_ENTITY_TYPE_DIRECTORY) {
			mpd_freeDirectory(entity->info.directory);
		}
		else if(entity->type == MPD_INFO_ENTITY_TYPE_SONG) {
			mpd_freeSong(entity->info.song);
		}
		else if(entity->type == MPD_INFO_ENTITY_TYPE_PLAYLISTFILE) {
			mpd_freePlaylistFile(entity->info.playlistFile);
		}
	}
}

mpd_InfoEntity * mpd_newInfoEntity(void) {
	mpd_InfoEntity * entity = malloc(sizeof(mpd_InfoEntity));

	mpd_initInfoEntity(entity);

	return entity;
}

void mpd_freeInfoEntity(mpd_InfoEntity * entity) {
	mpd_finishInfoEntity(entity);
	free(entity);
}

static void mpd_sendInfoCommand(mpd_Connection * connection, char * command) {
	mpd_executeCommand(connection,command);
}

mpd_InfoEntity * mpd_getNextInfoEntity(mpd_Connection * connection) {
	mpd_InfoEntity * entity = NULL;

	if(connection->doneProcessing || (connection->listOks &&
	   connection->doneListOk))
	{
		return NULL;
	}

	if(!connection->returnElement) mpd_getNextReturnElement(connection);

	if(connection->returnElement) {
		if(strcmp(connection->returnElement->name,"file")==0) {
			entity = mpd_newInfoEntity();
			entity->type = MPD_INFO_ENTITY_TYPE_SONG;
			entity->info.song = mpd_newSong();
			entity->info.song->file =
				strdup(connection->returnElement->value);
		}
		else if(strcmp(connection->returnElement->name,
					"directory")==0) {
			entity = mpd_newInfoEntity();
			entity->type = MPD_INFO_ENTITY_TYPE_DIRECTORY;
			entity->info.directory = mpd_newDirectory();
			entity->info.directory->path =
				strdup(connection->returnElement->value);
		}
		else if(strcmp(connection->returnElement->name,"playlist")==0) {
			entity = mpd_newInfoEntity();
			entity->type = MPD_INFO_ENTITY_TYPE_PLAYLISTFILE;
			entity->info.playlistFile = mpd_newPlaylistFile();
			entity->info.playlistFile->path =
				strdup(connection->returnElement->value);
		}
		else if(strcmp(connection->returnElement->name, "cpos") == 0){
			entity = mpd_newInfoEntity();
			entity->type = MPD_INFO_ENTITY_TYPE_SONG;
			entity->info.song = mpd_newSong();
			entity->info.song->pos = atoi(connection->returnElement->value);
		}
		else {
			connection->error = 1;
			strcpy(connection->errorStr,"problem parsing song info");
			return NULL;
		}
	}
	else return NULL;

	mpd_getNextReturnElement(connection);
	while(connection->returnElement) {
		mpd_ReturnElement * re = connection->returnElement;

		if(strcmp(re->name,"file")==0) return entity;
		else if(strcmp(re->name,"directory")==0) return entity;
		else if(strcmp(re->name,"playlist")==0) return entity;
		else if(strcmp(re->name,"cpos")==0) return entity;

		if(entity->type == MPD_INFO_ENTITY_TYPE_SONG &&
				strlen(re->value)) {
			if(!entity->info.song->artist &&
					strcmp(re->name,"Artist")==0) {
				entity->info.song->artist = strdup(re->value);
			}
			else if(!entity->info.song->album &&
					strcmp(re->name,"Album")==0) {
				entity->info.song->album = strdup(re->value);
			}
			else if(!entity->info.song->title &&
					strcmp(re->name,"Title")==0) {
				entity->info.song->title = strdup(re->value);
			}
			else if(!entity->info.song->track &&
					strcmp(re->name,"Track")==0) {
				entity->info.song->track = strdup(re->value);
			}
			else if(!entity->info.song->name &&
					strcmp(re->name,"Name")==0) {
				entity->info.song->name = strdup(re->value);
			}
			else if(entity->info.song->time==MPD_SONG_NO_TIME &&
					strcmp(re->name,"Time")==0) {
				entity->info.song->time = atoi(re->value);
			}
			else if(entity->info.song->pos==MPD_SONG_NO_NUM &&
					strcmp(re->name,"Pos")==0) {
				entity->info.song->pos = atoi(re->value);
			}
			else if(entity->info.song->id==MPD_SONG_NO_ID &&
					strcmp(re->name,"Id")==0) {
				entity->info.song->id = atoi(re->value);
			}
			else if(!entity->info.song->date &&
					strcmp(re->name, "Date") == 0) {
				entity->info.song->date = strdup(re->value);
			}
			else if(!entity->info.song->genre &&
					strcmp(re->name, "Genre") == 0) {
				entity->info.song->genre = strdup(re->value);
			}
			else if(!entity->info.song->composer &&
					strcmp(re->name, "Composer") == 0) {
				entity->info.song->composer = strdup(re->value);
			}
			else if(!entity->info.song->disc &&
					strcmp(re->name, "Disc") == 0) {
				entity->info.song->disc = strdup(re->value);
			}
			else if(!entity->info.song->comment &&
					strcmp(re->name, "Comment") == 0) {
				entity->info.song->comment = strdup(re->value);
			}
		}
		else if(entity->type == MPD_INFO_ENTITY_TYPE_DIRECTORY) {
		}
		else if(entity->type == MPD_INFO_ENTITY_TYPE_PLAYLISTFILE) {
		}

		mpd_getNextReturnElement(connection);
	}

	return entity;
}

static char * mpd_getNextReturnElementNamed(mpd_Connection * connection,
		const char * name)
{
	if(connection->doneProcessing || (connection->listOks &&
				connection->doneListOk))
	{
		return NULL;
	}

	mpd_getNextReturnElement(connection);
	while(connection->returnElement) {
		mpd_ReturnElement * re = connection->returnElement;

		if(strcmp(re->name,name)==0) return strdup(re->value);
		mpd_getNextReturnElement(connection);
	}

	return NULL;
}

char * mpd_getNextTag(mpd_Connection * connection,int table) {
	if(table >= 0 && table < MPD_TAG_NUM_OF_ITEM_TYPES)
	{
		return mpd_getNextReturnElementNamed(connection,mpdTagItemKeys[table]);
	}
	return NULL;
}

char * mpd_getNextArtist(mpd_Connection * connection) {
	return mpd_getNextReturnElementNamed(connection,"Artist");
}

char * mpd_getNextAlbum(mpd_Connection * connection) {
	return mpd_getNextReturnElementNamed(connection,"Album");
}

void mpd_sendPlaylistInfoCommand(mpd_Connection * connection, int songPos) {
	char * string = malloc(strlen("playlistinfo")+25);
	sprintf(string,"playlistinfo \"%i\"\n",songPos);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendPlaylistIdCommand(mpd_Connection * connection, int id) {
	char * string = malloc(strlen("playlistid")+25);
	sprintf(string, "playlistid \"%i\"\n", id);
	mpd_sendInfoCommand(connection, string);
	free(string);
}

void mpd_sendPlChangesCommand(mpd_Connection * connection, long long playlist) {
	char * string = malloc(strlen("plchanges")+25);
	sprintf(string,"plchanges \"%lld\"\n",playlist);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendPlChangesPosIdCommand(mpd_Connection * connection, long long playlist) {
	char * string = malloc(strlen("plchangesposid")+25);
	sprintf(string,"plchangesposid \"%lld\"\n",playlist);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendListallCommand(mpd_Connection * connection, const char * dir) {
	char * sDir = mpd_sanitizeArg(dir);
	char * string = malloc(strlen("listall")+strlen(sDir)+5);
	sprintf(string,"listall \"%s\"\n",sDir);
	mpd_sendInfoCommand(connection,string);
	free(string);
	free(sDir);
}

void mpd_sendListallInfoCommand(mpd_Connection * connection, const char * dir) {
	char * sDir = mpd_sanitizeArg(dir);
	char * string = malloc(strlen("listallinfo")+strlen(sDir)+5);
	sprintf(string,"listallinfo \"%s\"\n",sDir);
	mpd_sendInfoCommand(connection,string);
	free(string);
	free(sDir);
}

void mpd_sendLsInfoCommand(mpd_Connection * connection, const char * dir) {
	char * sDir = mpd_sanitizeArg(dir);
	char * string = malloc(strlen("lsinfo")+strlen(sDir)+5);
	sprintf(string,"lsinfo \"%s\"\n",sDir);
	mpd_sendInfoCommand(connection,string);
	free(string);
	free(sDir);
}

void mpd_sendCurrentSongCommand(mpd_Connection * connection) {
	mpd_executeCommand(connection,"currentsong\n");
}

void mpd_sendSearchCommand(mpd_Connection * connection, int table,
		const char * str)
{
	char st[10];
	char * string;
	char * sanitStr = mpd_sanitizeArg(str);
	if(table == MPD_TABLE_ARTIST) strcpy(st,"artist");
	else if(table == MPD_TABLE_ALBUM) strcpy(st,"album");
	else if(table == MPD_TABLE_TITLE) strcpy(st,"title");
	else if(table == MPD_TABLE_FILENAME) strcpy(st,"filename");
	else {
		connection->error = 1;
		strcpy(connection->errorStr,"unknown table for search");
		return;
	}
	string = malloc(strlen("search")+strlen(sanitStr)+strlen(st)+6);
	sprintf(string,"search %s \"%s\"\n",st,sanitStr);
	mpd_sendInfoCommand(connection,string);
	free(string);
	free(sanitStr);
}

void mpd_sendFindCommand(mpd_Connection * connection, int table,
		const char * str)
{
	char st[10];
	char * string;
	char * sanitStr = mpd_sanitizeArg(str);
	if(table == MPD_TABLE_ARTIST) strcpy(st,"artist");
	else if(table == MPD_TABLE_ALBUM) strcpy(st,"album");
	else if(table == MPD_TABLE_TITLE) strcpy(st,"title");
	else {
		connection->error = 1;
		strcpy(connection->errorStr,"unknown table for find");
		return;
	}
	string = malloc(strlen("find")+strlen(sanitStr)+strlen(st)+6);
	sprintf(string,"find %s \"%s\"\n",st,sanitStr);
	mpd_sendInfoCommand(connection,string);
	free(string);
	free(sanitStr);
}

void mpd_sendListCommand(mpd_Connection * connection, int table,
		const char * arg1)
{
	char st[10];
	char * string;
	if(table == MPD_TABLE_ARTIST) strcpy(st,"artist");
	else if(table == MPD_TABLE_ALBUM) strcpy(st,"album");
	else {
		connection->error = 1;
		strcpy(connection->errorStr,"unknown table for list");
		return;
	}
	if(arg1) {
		char * sanitArg1 = mpd_sanitizeArg(arg1);
		string = malloc(strlen("list")+strlen(sanitArg1)+strlen(st)+6);
		sprintf(string,"list %s \"%s\"\n",st,sanitArg1);
		free(sanitArg1);
	}
	else {
		string = malloc(strlen("list")+strlen(st)+3);
		sprintf(string,"list %s\n",st);
	}
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendAddCommand(mpd_Connection * connection, const char * file) {
	char * sFile = mpd_sanitizeArg(file);
	char * string = malloc(strlen("add")+strlen(sFile)+5);
	sprintf(string,"add \"%s\"\n",sFile);
	mpd_executeCommand(connection,string);
	free(string);
	free(sFile);
}

void mpd_sendDeleteCommand(mpd_Connection * connection, int songPos) {
	char * string = malloc(strlen("delete")+25);
	sprintf(string,"delete \"%i\"\n",songPos);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendDeleteIdCommand(mpd_Connection * connection, int id) {
	char * string = malloc(strlen("deleteid")+25);
	sprintf(string, "deleteid \"%i\"\n", id);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendSaveCommand(mpd_Connection * connection, const char * name) {
	char * sName = mpd_sanitizeArg(name);
	char * string = malloc(strlen("save")+strlen(sName)+5);
	sprintf(string,"save \"%s\"\n",sName);
	mpd_executeCommand(connection,string);
	free(string);
	free(sName);
}

void mpd_sendLoadCommand(mpd_Connection * connection, const char * name) {
	char * sName = mpd_sanitizeArg(name);
	char * string = malloc(strlen("load")+strlen(sName)+5);
	sprintf(string,"load \"%s\"\n",sName);
	mpd_executeCommand(connection,string);
	free(string);
	free(sName);
}

void mpd_sendRmCommand(mpd_Connection * connection, const char * name) {
	char * sName = mpd_sanitizeArg(name);
	char * string = malloc(strlen("rm")+strlen(sName)+5);
	sprintf(string,"rm \"%s\"\n",sName);
	mpd_executeCommand(connection,string);
	free(string);
	free(sName);
}

void mpd_sendShuffleCommand(mpd_Connection * connection) {
	mpd_executeCommand(connection,"shuffle\n");
}

void mpd_sendClearCommand(mpd_Connection * connection) {
	mpd_executeCommand(connection,"clear\n");
}

void mpd_sendPlayCommand(mpd_Connection * connection, int songPos) {
	char * string = malloc(strlen("play")+25);
	sprintf(string,"play \"%i\"\n",songPos);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendPlayIdCommand(mpd_Connection * connection, int id) {
	char * string = malloc(strlen("playid")+25);
	sprintf(string,"playid \"%i\"\n",id);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendStopCommand(mpd_Connection * connection) {
	mpd_executeCommand(connection,"stop\n");
}

void mpd_sendPauseCommand(mpd_Connection * connection, int pauseMode) {
	char * string = malloc(strlen("pause")+25);
	sprintf(string,"pause \"%i\"\n",pauseMode);
	mpd_executeCommand(connection,string);
	free(string);
}

void mpd_sendNextCommand(mpd_Connection * connection) {
	mpd_executeCommand(connection,"next\n");
}

void mpd_sendMoveCommand(mpd_Connection * connection, int from, int to) {
	char * string = malloc(strlen("move")+25);
	sprintf(string,"move \"%i\" \"%i\"\n",from,to);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendMoveIdCommand(mpd_Connection * connection, int id, int to) {
	char * string = malloc(strlen("moveid")+25);
	sprintf(string, "moveid \"%i\" \"%i\"\n", id, to);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendSwapCommand(mpd_Connection * connection, int song1, int song2) {
	char * string = malloc(strlen("swap")+25);
	sprintf(string,"swap \"%i\" \"%i\"\n",song1,song2);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendSwapIdCommand(mpd_Connection * connection, int id1, int id2) {
	char * string = malloc(strlen("swapid")+25);
	sprintf(string, "swapid \"%i\" \"%i\"\n", id1, id2);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendSeekCommand(mpd_Connection * connection, int song, int time) {
	char * string = malloc(strlen("seek")+25);
	sprintf(string,"seek \"%i\" \"%i\"\n",song,time);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendSeekIdCommand(mpd_Connection * connection, int id, int time) {
	char * string = malloc(strlen("seekid")+25);
	sprintf(string,"seekid \"%i\" \"%i\"\n",id,time);
	mpd_sendInfoCommand(connection,string);
	free(string);
}

void mpd_sendUpdateCommand(mpd_Connection * connection, char * path) {
	char * sPath = mpd_sanitizeArg(path);
	char * string = malloc(strlen("update")+strlen(sPath)+5);
	sprintf(string,"update \"%s\"\n",sPath);
	mpd_sendInfoCommand(connection,string);
	free(string);
	free(sPath);
}

int mpd_getUpdateId(mpd_Connection * connection) {
	char * jobid;
	int ret = 0;

	jobid = mpd_getNextReturnElementNamed(connection,"updating_db");
	if(jobid) {
		ret = atoi(jobid);
		free(jobid);
	}

	return ret;
}

void mpd_sendPrevCommand(mpd_Connection * connection) {
	mpd_executeCommand(connection,"previous\n");
}

void mpd_sendRepeatCommand(mpd_Connection * connection, int repeatMode) {
	char * string = malloc(strlen("repeat")+25);
	sprintf(string,"repeat \"%i\"\n",repeatMode);
	mpd_executeCommand(connection,string);
	free(string);
}

void mpd_sendRandomCommand(mpd_Connection * connection, int randomMode) {
	char * string = malloc(strlen("random")+25);
	sprintf(string,"random \"%i\"\n",randomMode);
	mpd_executeCommand(connection,string);
	free(string);
}

void mpd_sendSetvolCommand(mpd_Connection * connection, int volumeChange) {
	char * string = malloc(strlen("setvol")+25);
	sprintf(string,"setvol \"%i\"\n",volumeChange);
	mpd_executeCommand(connection,string);
	free(string);
}

void mpd_sendVolumeCommand(mpd_Connection * connection, int volumeChange) {
	char * string = malloc(strlen("volume")+25);
	sprintf(string,"volume \"%i\"\n",volumeChange);
	mpd_executeCommand(connection,string);
	free(string);
}

void mpd_sendCrossfadeCommand(mpd_Connection * connection, int seconds) {
	char * string = malloc(strlen("crossfade")+25);
	sprintf(string,"crossfade \"%i\"\n",seconds);
	mpd_executeCommand(connection,string);
	free(string);
}

void mpd_sendPasswordCommand(mpd_Connection * connection, const char * pass) {
	char * sPass = mpd_sanitizeArg(pass);
	char * string = malloc(strlen("password")+strlen(sPass)+5);
	sprintf(string,"password \"%s\"\n",sPass);
	mpd_executeCommand(connection,string);
	free(string);
	free(sPass);
}

void mpd_sendCommandListBegin(mpd_Connection * connection) {
	if(connection->commandList) {
		strcpy(connection->errorStr,"already in command list mode");
		connection->error = 1;
		return;
	}
	connection->commandList = COMMAND_LIST;
	mpd_executeCommand(connection,"command_list_begin\n");
}

void mpd_sendCommandListOkBegin(mpd_Connection * connection) {
	if(connection->commandList) {
		strcpy(connection->errorStr,"already in command list mode");
		connection->error = 1;
		return;
	}
	connection->commandList = COMMAND_LIST_OK;
	mpd_executeCommand(connection,"command_list_ok_begin\n");
	connection->listOks = 0;
}

void mpd_sendCommandListEnd(mpd_Connection * connection) {
	if(!connection->commandList) {
		strcpy(connection->errorStr,"not in command list mode");
		connection->error = 1;
		return;
	}
	connection->commandList = 0;
	mpd_executeCommand(connection,"command_list_end\n");
}

void mpd_sendOutputsCommand(mpd_Connection * connection) {
	mpd_executeCommand(connection,"outputs\n");
}

mpd_OutputEntity * mpd_getNextOutput(mpd_Connection * connection) {
	mpd_OutputEntity * output = NULL;

	if(connection->doneProcessing || (connection->listOks &&
				connection->doneListOk))
	{
		return NULL;
	}

	if(connection->error) return NULL;

	output = malloc(sizeof(mpd_OutputEntity));
	output->id = -10;
	output->name = NULL;
	output->enabled = 0;

	if(!connection->returnElement) mpd_getNextReturnElement(connection);

	while(connection->returnElement) {
		mpd_ReturnElement * re = connection->returnElement;
		if(strcmp(re->name,"outputid")==0) {
			if(output!=NULL && output->id>=0) return output;
			output->id = atoi(re->value);
		}
		else if(strcmp(re->name,"outputname")==0) {
			output->name = strdup(re->value);
		}
		else if(strcmp(re->name,"outputenabled")==0) {
			output->enabled = atoi(re->value);
		}

		mpd_getNextReturnElement(connection);
		if(connection->error) {
			free(output);
			return NULL;
		}

	}

	return output;
}

void mpd_sendEnableOutputCommand(mpd_Connection * connection, int outputId) {
	char * string = malloc(strlen("enableoutput")+25);
	sprintf(string,"enableoutput \"%i\"\n",outputId);
	mpd_executeCommand(connection,string);
	free(string);
}

void mpd_sendDisableOutputCommand(mpd_Connection * connection, int outputId) {
	char * string = malloc(strlen("disableoutput")+25);
	sprintf(string,"disableoutput \"%i\"\n",outputId);
	mpd_executeCommand(connection,string);
	free(string);
}

void mpd_freeOutputElement(mpd_OutputEntity * output) {
	free(output->name);
	free(output);
}

/**
 * mpd_sendNotCommandsCommand
 * odd naming, but it gets the not allowed commands
 */

void mpd_sendNotCommandsCommand(mpd_Connection * connection) {
	mpd_executeCommand(connection,"notcommands\n");
}

/**
 * mpd_sendCommandsCommand
 * odd naming, but it gets the allowed commands
 */

void mpd_sendCommandsCommand(mpd_Connection * connection) {
	mpd_executeCommand(connection,"commands\n");
}
/**
 * Get the next returned command
 */
char * mpd_getNextCommand(mpd_Connection * connection) {
	return mpd_getNextReturnElementNamed(connection,"command");
}

void mpd_startSearch(mpd_Connection * connection,int exact) {
	if(connection->request) {
		/* search/find allready in progress */
		/* TODO: set error here?  */
		return;
	}
	if(exact){
		connection->request = strdup("find");
	}
	else{
		connection->request = strdup("search");
	}
}


void mpd_startFieldSearch(mpd_Connection * connection,int field) {
	if(connection->request) {
		/* search/find allready in progress */
		/* TODO: set error here?  */
		return;
	}
	if(field < 0 || field >= MPD_TAG_NUM_OF_ITEM_TYPES) {
		/* set error here */
		return;
	}

	connection->request = malloc(sizeof(char)*(
				/* length of the field name */
				strlen(mpdTagItemKeys[field])+
				/* "list"+space+\0 */
				6
				));
	sprintf(connection->request, "list %s", mpdTagItemKeys[field]);
}



void mpd_addConstraintSearch(mpd_Connection *connection,
		int field,
		char *name)
{
	char *arg = NULL;
	if(!connection->request){
		return;
	}
	if(name == NULL) {
		return;
	}
	if(field < 0 || field >= MPD_TAG_NUM_OF_ITEM_TYPES) {
		return;
	}
	/* clean up the query */
	arg = mpd_sanitizeArg(name);
	/* create space for the query */
	connection->request = realloc(connection->request, (
			 /* length of the old string */
			 strlen(connection->request)+
			 /* space between */
			 1+
			 /* length of the field name */
			 strlen(mpdTagItemKeys[field])+
			 /* space plus starting " */
			 2+
			 /* length of search term */
			 strlen(arg)+
			 /* closign " +\0 that is added sprintf */
			 2
			)*sizeof(char));
	/* and form the query */
	sprintf(connection->request, "%s %s \"%s\"",
			connection->request,
			mpdTagItemKeys[field],
			arg);
	free(arg);
}


void mpd_commitSearch(mpd_Connection *connection)
{
	if(connection->request)
	{
		int length = strlen(connection->request);
		/* fixing up the string for mpd to like */
		connection->request = realloc(connection->request,
				(length+	/* old length */
				 2		/* closing \n and \0 */
				)*sizeof(char));
		connection->request[length] = '\n';
		connection->request[length+1] = '\0';
		/* and off we go */
		mpd_sendInfoCommand(connection, connection->request);
		/* clean up a bit */
		free(connection->request);
		connection->request = NULL;
	}
}

/**
 * @param connection a MpdConnection
 * @param path	the path to the playlist.
 *
 * List the content, with full metadata, of a stored playlist.
 *
 */
void mpd_sendListPlaylistInfoCommand(mpd_Connection *connection, char *path)
{
	char *arg = mpd_sanitizeArg(path);
	char *query = malloc(strlen("listplaylistinfo")+strlen(arg)+5);
	sprintf(query, "listplaylistinfo \"%s\"\n",arg);
	mpd_sendInfoCommand(connection, query);
	free(arg);
	free(query);
}

/**
 * @param connection a MpdConnection
 * @param path	the path to the playlist.
 *
 * List the content of a stored playlist.
 *
 */
void mpd_sendListPlaylistCommand(mpd_Connection *connection, char *path)
{
	char *arg = mpd_sanitizeArg(path);
	char *query = malloc(strlen("listplaylist")+strlen(arg)+5);
	sprintf(query, "listplaylist \"%s\"\n",arg);
	mpd_sendInfoCommand(connection, query);
	free(arg);
	free(query);
}
