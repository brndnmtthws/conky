/* libmpdclient
   (c)2003-2004 by Warren Dukes (shank@mercury.chem.pitt.edu)
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
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/param.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/signal.h>

#ifndef MPD_NO_IPV6
#ifdef AF_INET6
#define MPD_HAVE_IPV6
#endif
#endif

static char sigpipe = 0;

#define COMMAND_LIST	1
#define COMMAND_LIST_OK	2

#ifdef MPD_HAVE_IPV6
int mpd_ipv6Supported()
{
	int s;
	s = socket(AF_INET6, SOCK_STREAM, 0);
	if (s == -1)
		return 0;
	close(s);
	return 1;
}
#endif


char *mpd_sanitizeArg(const char *arg)
{
	size_t i;
	int count = 0;
	char *ret;

	for (i = 0; i < strlen(arg); i++) {
		if (arg[i] == '"' || arg[i] == '\\')
			count++;
	}

	ret = malloc(strlen(arg) + count + 1);

	count = 0;
	for (i = 0; i < strlen(arg) + 1; i++) {
		if (arg[i] == '"' || arg[i] == '\\') {
			ret[i + count] = '\\';
			count++;
		}
		ret[i + count] = arg[i];
	}

	return ret;
}

mpd_ReturnElement *mpd_newReturnElement(const char *name,
					const char *value)
{
	mpd_ReturnElement *ret = malloc(sizeof(mpd_ReturnElement));

	ret->name = strdup(name);
	ret->value = strdup(value);

	return ret;
}

void mpd_freeReturnElement(mpd_ReturnElement * re)
{
	free(re->name);
	free(re->value);
	free(re);
}

void mpd_setConnectionTimeout(mpd_Connection * connection, float timeout)
{
	connection->timeout.tv_sec = (int) timeout;
	connection->timeout.tv_usec = (int) (timeout * 1e6 -
					     connection->timeout.tv_sec *
					     1000000 + 0.5);
}

mpd_Connection *mpd_newConnection(const char *host, int port,
				  float timeout)
{
	int err;
	struct hostent *he;
	struct sockaddr *dest;
#ifdef HAVE_SOCKLEN_T
	socklen_t destlen;
#else
	int destlen;
#endif
	struct sockaddr_in sin;
	char *rt;
	char *output;
	mpd_Connection *connection = malloc(sizeof(mpd_Connection));
	struct timeval tv;
	fd_set fds;
        struct sigaction sapipe;           /* definition of signal action */

#ifdef MPD_HAVE_IPV6
	struct sockaddr_in6 sin6;
#endif
	strcpy(connection->buffer, "");
	connection->buflen = 0;
	connection->bufstart = 0;
	strcpy(connection->errorStr, "");
	connection->error = 0;
	connection->doneProcessing = 0;
	connection->commandList = 0;
	connection->listOks = 0;
	connection->doneListOk = 0;
	connection->returnElement = NULL;
	sigpipe = 0;

	if (!(he = gethostbyname(host))) {
		snprintf(connection->errorStr, MPD_BUFFER_MAX_LENGTH,
			 "host \"%s\" not found", host);
		connection->error = MPD_ERROR_UNKHOST;
		return connection;
	}

	memset(&sin, 0, sizeof(struct sockaddr_in));
	/*dest.sin_family = he->h_addrtype; */
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
#ifdef MPD_HAVE_IPV6
	memset(&sin6, 0, sizeof(struct sockaddr_in6));
	sin6.sin6_family = AF_INET6;
	sin6.sin6_port = htons(port);
#endif
	switch (he->h_addrtype) {
	case AF_INET:
		memcpy((char *) &sin.sin_addr.s_addr, (char *) he->h_addr,
		       he->h_length);
		dest = (struct sockaddr *) &sin;
		destlen = sizeof(struct sockaddr_in);
		break;
#ifdef MPD_HAVE_IPV6
	case AF_INET6:
		if (!mpd_ipv6Supported()) {
			strcpy(connection->errorStr,
			       "no IPv6 suuport but a "
			       "IPv6 address found\n");
			connection->error = MPD_ERROR_SYSTEM;
			return connection;
		}
		memcpy((char *) &sin6.sin6_addr.s6_addr,
		       (char *) he->h_addr, he->h_length);
		dest = (struct sockaddr *) &sin6;
		destlen = sizeof(struct sockaddr_in6);
		break;
#endif
	default:
		strcpy(connection->errorStr,
		       "address type is not IPv4 or " "IPv6\n");
		connection->error = MPD_ERROR_SYSTEM;
		return connection;
		break;
	}

	if ((connection->sock =
	     socket(dest->sa_family, SOCK_STREAM, 0)) < 0) {
		strcpy(connection->errorStr, "problems creating socket");
		connection->error = MPD_ERROR_SYSTEM;
		return connection;
	}

	mpd_setConnectionTimeout(connection, timeout);
        
	/* install the signal handler */
	sapipe.sa_handler = mpd_signalHandler;
//	sapipe.sa_mask = 0;
	sapipe.sa_flags = 0;
	sapipe.sa_restorer = NULL;
	sigaction(SIGPIPE,&sapipe,NULL);

	/* connect stuff */
	{
		int flags = fcntl(connection->sock, F_GETFL, 0);
		fcntl(connection->sock, F_SETFL, flags | O_NONBLOCK);

		if (connect(connection->sock, dest, destlen) < 0
		    && errno != EINPROGRESS) {
			snprintf(connection->errorStr,
				 MPD_BUFFER_MAX_LENGTH,
				 "problems connecting to \"%s\" on port"
				 " %i", host, port);
			connection->error = MPD_ERROR_CONNPORT;
			return connection;
		}
	}

	while (!(rt = strstr(connection->buffer, "\n"))) {
		tv.tv_sec = connection->timeout.tv_sec;
		tv.tv_usec = connection->timeout.tv_usec;
		FD_ZERO(&fds);
		FD_SET(connection->sock, &fds);
		if ((err =
		     select(connection->sock + 1, &fds, NULL, NULL,
			    &tv)) == 1) {
			int readed;
			readed = recv(connection->sock,
				      &(connection->
					buffer[connection->buflen]),
				      MPD_BUFFER_MAX_LENGTH -
				      connection->buflen, 0);
			if (readed <= 0) {
				snprintf(connection->errorStr,
					 MPD_BUFFER_MAX_LENGTH,
					 "problems getting a response from"
					 " \"%s\" on port %i", host, port);
				connection->error = MPD_ERROR_NORESPONSE;
				return connection;
			}
			connection->buflen += readed;
			connection->buffer[connection->buflen] = '\0';
			tv.tv_sec = connection->timeout.tv_sec;
			tv.tv_usec = connection->timeout.tv_usec;
		} else if (err < 0) {
			switch (errno) {
			case EINTR:
				continue;
			default:
				snprintf(connection->errorStr,
					 MPD_BUFFER_MAX_LENGTH,
					 "problems connecting to \"%s\" on port"
					 " %i", host, port);
				connection->error = MPD_ERROR_CONNPORT;
				return connection;
			}
		} else {
			snprintf(connection->errorStr,
				 MPD_BUFFER_MAX_LENGTH,
				 "timeout in attempting to get a response from"
				 " \"%s\" on port %i", host, port);
			connection->error = MPD_ERROR_NORESPONSE;
			return connection;
		}
	}

	*rt = '\0';
	output = strdup(connection->buffer);
	strcpy(connection->buffer, rt + 1);
	connection->buflen = strlen(connection->buffer);

	if (strncmp
	    (output, MPD_WELCOME_MESSAGE, strlen(MPD_WELCOME_MESSAGE))) {
		free(output);
		snprintf(connection->errorStr, MPD_BUFFER_MAX_LENGTH,
			 "mpd not running on port %i on host \"%s\"", port,
			 host);
		connection->error = MPD_ERROR_NOTMPD;
		return connection;
	}

	{
		char *test;
		char *version[3];
		char *tmp = &output[strlen(MPD_WELCOME_MESSAGE)];
		char *search = ".";
		int i;

		for (i = 0; i < 3; i++) {
			char *tok;
			if (i == 3)
				search = " ";
			version[i] = strtok_r(tmp, search, &tok);
			if (!version[i]) {
				free(output);
				snprintf(connection->errorStr,
					 MPD_BUFFER_MAX_LENGTH,
					 "error parsing version number at "
					 "\"%s\"",
					 &output[strlen
						 (MPD_WELCOME_MESSAGE)]);
				connection->error = MPD_ERROR_NOTMPD;
				return connection;
			}
			connection->version[i] =
			    strtol(version[i], &test, 10);
			if (version[i] == test || *test != '\0') {
				free(output);
				snprintf(connection->errorStr,
					 MPD_BUFFER_MAX_LENGTH,
					 "error parsing version number at "
					 "\"%s\"",
					 &output[strlen
						 (MPD_WELCOME_MESSAGE)]);
				connection->error = MPD_ERROR_NOTMPD;
				return connection;
			}
			tmp = NULL;
		}
	}

	free(output);

	connection->doneProcessing = 1;

	return connection;
}

void mpd_clearError(mpd_Connection * connection)
{
	connection->error = 0;
	connection->errorStr[0] = '\0';
}

void mpd_closeConnection(mpd_Connection * connection)
{
	close(connection->sock);
	if (connection->returnElement)
		free(connection->returnElement);
	free(connection);
}

void mpd_executeCommand(mpd_Connection * connection, char *command)
{
	int ret;
	struct timeval tv;
	fd_set fds;
	char *commandPtr = command;
	int commandLen = strlen(command);

	if (!connection->doneProcessing && !connection->commandList) {
		strcpy(connection->errorStr,
		       "not done processing current command");
		connection->error = 1;
		return;
	}

	mpd_clearError(connection);

	FD_ZERO(&fds);
	FD_SET(connection->sock, &fds);
	tv.tv_sec = connection->timeout.tv_sec;
	tv.tv_usec = connection->timeout.tv_usec;

	if (sigpipe) {
		perror("");
		snprintf(connection->errorStr, MPD_BUFFER_MAX_LENGTH, "got SIGINT");
		connection->error = MPD_ERROR_SENDING;
		return;
	}
	while ((ret = select(connection->sock + 1, NULL, &fds, NULL, &tv) == 1) || (ret == -1 && errno == EINTR)) {
		if (sigpipe) {
			perror("");
			snprintf(connection->errorStr, MPD_BUFFER_MAX_LENGTH, "got SIGINT");
			connection->error = MPD_ERROR_SENDING;
			return;
		} else {
			ret = send(connection->sock, commandPtr, commandLen, MSG_DONTWAIT);
			if (ret <= 0) {
				if (ret == EAGAIN || ret == EINTR)
					continue;
				snprintf(connection->errorStr, MPD_BUFFER_MAX_LENGTH, "problems giving command \"%s\"", command);
				connection->error = MPD_ERROR_SENDING;
				return;
			} else {
				commandPtr += ret;
				commandLen -= ret;
			}
			if (commandLen <= 0)
				break;
		}
	}

	if (commandLen > 0) {
		perror("");
		snprintf(connection->errorStr, MPD_BUFFER_MAX_LENGTH,
			 "timeout sending command \"%s\"", command);
		connection->error = MPD_ERROR_TIMEOUT;
		return;
	}

	if (!connection->commandList)
		connection->doneProcessing = 0;
	else if (connection->commandList == COMMAND_LIST_OK) {
		connection->listOks++;
	}
}

void mpd_getNextReturnElement(mpd_Connection * connection)
{
	char *output = NULL;
	char *rt = NULL;
	char *name = NULL;
	char *value = NULL;
	fd_set fds;
	struct timeval tv;
	char *tok = NULL;
	int readed;
	char *bufferCheck = NULL;
	int err;

	if (connection->returnElement)
		mpd_freeReturnElement(connection->returnElement);
	connection->returnElement = NULL;

	if (connection->doneProcessing || (connection->listOks &&
					   connection->doneListOk)) {
		strcpy(connection->errorStr,
		       "already done processing current command");
		connection->error = 1;
		return;
	}

	bufferCheck = connection->buffer + connection->bufstart;
	while (connection->bufstart >= connection->buflen ||
	       !(rt = strstr(bufferCheck, "\n"))) {
		if (connection->buflen >= MPD_BUFFER_MAX_LENGTH) {
			memmove(connection->buffer,
				connection->buffer +
				connection->bufstart,
				connection->buflen - connection->bufstart +
				1);
			bufferCheck -= connection->bufstart;
			connection->buflen -= connection->bufstart;
			connection->bufstart = 0;
		}
		if (connection->buflen >= MPD_BUFFER_MAX_LENGTH) {
			strcpy(connection->errorStr, "buffer overrun");
			connection->error = MPD_ERROR_BUFFEROVERRUN;
			connection->doneProcessing = 1;
			connection->doneListOk = 0;
			return;
		}
		bufferCheck += connection->buflen - connection->bufstart;
		tv.tv_sec = connection->timeout.tv_sec;
		tv.tv_usec = connection->timeout.tv_usec;
		FD_ZERO(&fds);
		FD_SET(connection->sock, &fds);
		if ((err =
		     select(connection->sock + 1, &fds, NULL, NULL,
			    &tv) == 1)) {
			readed =
			    recv(connection->sock,
				 connection->buffer + connection->buflen,
				 MPD_BUFFER_MAX_LENGTH -
				 connection->buflen, MSG_DONTWAIT);
			if (readed < 0
			    && (errno == EAGAIN || errno == EINTR)) {
				continue;
			}
			if (readed <= 0) {
				strcpy(connection->errorStr,
				       "connection" " closed");
				connection->error = MPD_ERROR_CONNCLOSED;
				connection->doneProcessing = 1;
				connection->doneListOk = 0;
				return;
			}
			connection->buflen += readed;
			connection->buffer[connection->buflen] = '\0';
		} else if (err < 0 && errno == EINTR)
			continue;
		else {
			strcpy(connection->errorStr, "connection timeout");
			connection->error = MPD_ERROR_TIMEOUT;
			connection->doneProcessing = 1;
			connection->doneListOk = 0;
			return;
		}
	}

	*rt = '\0';
	output = connection->buffer + connection->bufstart;
	connection->bufstart = rt - connection->buffer + 1;

	if (strcmp(output, "OK") == 0) {
		if (connection->listOks > 0) {
			strcpy(connection->errorStr,
			       "expected more list_OK's");
			connection->error = 1;
		}
		connection->listOks = 0;
		connection->doneProcessing = 1;
		connection->doneListOk = 0;
		return;
	}

	if (strcmp(output, "list_OK") == 0) {
		if (!connection->listOks) {
			strcpy(connection->errorStr,
			       "got an unexpected list_OK");
			connection->error = 1;
		} else {
			connection->doneListOk = 1;
			connection->listOks--;
		}
		return;
	}

	if (strncmp(output, "ACK", strlen("ACK")) == 0) {
		char *test;
		char *needle;
		int val;

		strcpy(connection->errorStr, output);
		connection->error = MPD_ERROR_ACK;
		connection->errorCode = MPD_ACK_ERROR_UNK;
		connection->errorAt = MPD_ERROR_AT_UNK;
		connection->doneProcessing = 1;
		connection->doneListOk = 0;

		needle = strchr(output, '[');
		if (!needle)
			return;
		val = strtol(needle + 1, &test, 10);
		if (*test != '@')
			return;
		connection->errorCode = val;
		val = strtol(test + 1, &test, 10);
		if (*test != ']')
			return;
		connection->errorAt = val;
		return;
	}

	name = strtok_r(output, ":", &tok);
	if (name && (value = strtok_r(NULL, "", &tok)) && value[0] == ' ') {
		connection->returnElement =
		    mpd_newReturnElement(name, &(value[1]));
	} else {
		if (!name || !value) {
			snprintf(connection->errorStr,
				 MPD_BUFFER_MAX_LENGTH,
				 "error parsing: %s", output);
		} else {
			snprintf(connection->errorStr,
				 MPD_BUFFER_MAX_LENGTH,
				 "error parsing: %s:%s", name, value);
		}
		connection->errorStr[MPD_BUFFER_MAX_LENGTH] = '\0';
		connection->error = 1;
	}
}

void mpd_finishCommand(mpd_Connection * connection)
{
	while (!connection->doneProcessing) {
		if (connection->doneListOk)
			connection->doneListOk = 0;
		mpd_getNextReturnElement(connection);
	}
}

void mpd_finishListOkCommand(mpd_Connection * connection)
{
	while (!connection->doneProcessing && connection->listOks &&
	       !connection->doneListOk) {
		mpd_getNextReturnElement(connection);
	}
}

int mpd_nextListOkCommand(mpd_Connection * connection)
{
	mpd_finishListOkCommand(connection);
	if (!connection->doneProcessing)
		connection->doneListOk = 0;
	if (connection->listOks == 0 || connection->doneProcessing)
		return -1;
	return 0;
}

void mpd_sendStatusCommand(mpd_Connection * connection)
{
	mpd_executeCommand(connection, "status\n");
}

mpd_Status *mpd_getStatus(mpd_Connection * connection)
{
	mpd_Status *status;

	/*mpd_executeCommand(connection,"status\n");

	   if(connection->error) return NULL; */

	if (connection->doneProcessing || (connection->listOks &&
					   connection->doneListOk)) {
		return NULL;
	}

	if (!connection->returnElement)
		mpd_getNextReturnElement(connection);

	status = malloc(sizeof(mpd_Status));
	status->volume = -1;
	status->repeat = 0;
	status->random = 0;
	status->playlist = -1;
	status->playlistLength = -1;
	status->state = -1;
	status->song = 0;
	status->elapsedTime = 0;
	status->totalTime = 0;
	status->bitRate = 0;
	status->sampleRate = 0;
	status->bits = 0;
	status->channels = 0;
	status->crossfade = -1;
	status->error = NULL;
	status->updatingDb = 0;

	if (connection->error) {
		free(status);
		return NULL;
	}
	while (connection->returnElement) {
		mpd_ReturnElement *re = connection->returnElement;
		if (strcmp(re->name, "volume") == 0) {
			status->volume = atoi(re->value);
		} else if (strcmp(re->name, "repeat") == 0) {
			status->repeat = atoi(re->value);
		} else if (strcmp(re->name, "random") == 0) {
			status->random = atoi(re->value);
		} else if (strcmp(re->name, "playlist") == 0) {
			status->playlist = strtol(re->value, NULL, 10);
		} else if (strcmp(re->name, "playlistlength") == 0) {
			status->playlistLength = atoi(re->value);
		} else if (strcmp(re->name, "bitrate") == 0) {
			status->bitRate = atoi(re->value);
		} else if (strcmp(re->name, "state") == 0) {
			if (strcmp(re->value, "play") == 0) {
				status->state = MPD_STATUS_STATE_PLAY;
			} else if (strcmp(re->value, "stop") == 0) {
				status->state = MPD_STATUS_STATE_STOP;
			} else if (strcmp(re->value, "pause") == 0) {
				status->state = MPD_STATUS_STATE_PAUSE;
			} else {
				status->state = MPD_STATUS_STATE_UNKNOWN;
			}
		} else if (strcmp(re->name, "song") == 0) {
			status->song = atoi(re->value);
		} else if (strcmp(re->name, "songid") == 0) {
			status->songid = atoi(re->value);
		} else if (strcmp(re->name, "time") == 0) {
			char *tok;
			char *copy;
			char *temp;
			copy = strdup(re->value);
			temp = strtok_r(copy, ":", &tok);
			if (temp) {
				status->elapsedTime = atoi(temp);
				temp = strtok_r(NULL, "", &tok);
				if (temp)
					status->totalTime = atoi(temp);
			}
			free(copy);
		} else if (strcmp(re->name, "error") == 0) {
			status->error = strdup(re->value);
		} else if (strcmp(re->name, "xfade") == 0) {
			status->crossfade = atoi(re->value);
		} else if (strcmp(re->name, "updating_db") == 0) {
			status->updatingDb = atoi(re->value);
		} else if (strcmp(re->name, "audio") == 0) {
			char *tok;
			char *copy;
			char *temp;
			copy = strdup(re->value);
			temp = strtok_r(copy, ":", &tok);
			if (temp) {
				status->sampleRate = atoi(temp);
				temp = strtok_r(NULL, ":", &tok);
				if (temp) {
					status->bits = atoi(temp);
					temp = strtok_r(NULL, "", &tok);
					if (temp)
						status->channels =
						    atoi(temp);
				}
			}
			free(copy);
		}

		mpd_getNextReturnElement(connection);
		if (connection->error) {
			free(status);
			return NULL;
		}
	}

	if (connection->error) {
		free(status);
		return NULL;
	} else if (status->state < 0) {
		strcpy(connection->errorStr, "state not found");
		connection->error = 1;
		free(status);
		return NULL;
	}

	return status;
}

void mpd_freeStatus(mpd_Status * status)
{
	if (status->error)
		free(status->error);
	free(status);
}

void mpd_sendStatsCommand(mpd_Connection * connection)
{
	mpd_executeCommand(connection, "stats\n");
}

mpd_Stats *mpd_getStats(mpd_Connection * connection)
{
	mpd_Stats *stats;

	/*mpd_executeCommand(connection,"stats\n");

	   if(connection->error) return NULL; */

	if (connection->doneProcessing || (connection->listOks &&
					   connection->doneListOk)) {
		return NULL;
	}

	if (!connection->returnElement)
		mpd_getNextReturnElement(connection);

	stats = malloc(sizeof(mpd_Stats));
	stats->numberOfArtists = 0;
	stats->numberOfAlbums = 0;
	stats->numberOfSongs = 0;
	stats->uptime = 0;
	stats->dbUpdateTime = 0;
	stats->playTime = 0;
	stats->dbPlayTime = 0;

	if (connection->error) {
		free(stats);
		return NULL;
	}
	while (connection->returnElement) {
		mpd_ReturnElement *re = connection->returnElement;
		if (strcmp(re->name, "artists") == 0) {
			stats->numberOfArtists = atoi(re->value);
		} else if (strcmp(re->name, "albums") == 0) {
			stats->numberOfAlbums = atoi(re->value);
		} else if (strcmp(re->name, "songs") == 0) {
			stats->numberOfSongs = atoi(re->value);
		} else if (strcmp(re->name, "uptime") == 0) {
			stats->uptime = strtol(re->value, NULL, 10);
		} else if (strcmp(re->name, "db_update") == 0) {
			stats->dbUpdateTime = strtol(re->value, NULL, 10);
		} else if (strcmp(re->name, "playtime") == 0) {
			stats->playTime = strtol(re->value, NULL, 10);
		} else if (strcmp(re->name, "db_playtime") == 0) {
			stats->dbPlayTime = strtol(re->value, NULL, 10);
		}

		mpd_getNextReturnElement(connection);
		if (connection->error) {
			free(stats);
			return NULL;
		}
	}

	if (connection->error) {
		free(stats);
		return NULL;
	}

	return stats;
}

void mpd_freeStats(mpd_Stats * stats)
{
	free(stats);
}

void mpd_initSong(mpd_Song * song)
{
	song->file = NULL;
	song->artist = NULL;
	song->album = NULL;
	song->track = NULL;
	song->title = NULL;
	song->name = NULL;
	song->time = MPD_SONG_NO_TIME;
	song->pos = MPD_SONG_NO_NUM;
	song->id = MPD_SONG_NO_ID;
}

void mpd_finishSong(mpd_Song * song)
{
	if (song->file)
		free(song->file);
	if (song->artist)
		free(song->artist);
	if (song->album)
		free(song->album);
	if (song->title)
		free(song->title);
	if (song->track)
		free(song->track);
	if (song->name)
		free(song->name);
}

mpd_Song *mpd_newSong()
{
	mpd_Song *ret = malloc(sizeof(mpd_Song));

	mpd_initSong(ret);

	return ret;
}

void mpd_freeSong(mpd_Song * song)
{
	mpd_finishSong(song);
	free(song);
}

mpd_Song *mpd_songDup(mpd_Song * song)
{
	mpd_Song *ret = mpd_newSong();

	if (song->file)
		ret->file = strdup(song->file);
	if (song->artist)
		ret->artist = strdup(song->artist);
	if (song->album)
		ret->album = strdup(song->album);
	if (song->title)
		ret->title = strdup(song->title);
	if (song->track)
		ret->track = strdup(song->track);
	if (song->name)
		ret->name = strdup(song->name);
	ret->time = song->time;
	ret->pos = song->pos;
	ret->id = song->id;

	return ret;
}

void mpd_initDirectory(mpd_Directory * directory)
{
	directory->path = NULL;
}

void mpd_finishDirectory(mpd_Directory * directory)
{
	if (directory->path)
		free(directory->path);
}

mpd_Directory *mpd_newDirectory()
{
	mpd_Directory *directory = malloc(sizeof(mpd_Directory));;

	mpd_initDirectory(directory);

	return directory;
}

void mpd_freeDirectory(mpd_Directory * directory)
{
	mpd_finishDirectory(directory);

	free(directory);
}

mpd_Directory *mpd_directoryDup(mpd_Directory * directory)
{
	mpd_Directory *ret = mpd_newDirectory();

	if (directory->path)
		ret->path = strdup(directory->path);

	return ret;
}

void mpd_initPlaylistFile(mpd_PlaylistFile * playlist)
{
	playlist->path = NULL;
}

void mpd_finishPlaylistFile(mpd_PlaylistFile * playlist)
{
	if (playlist->path)
		free(playlist->path);
}

mpd_PlaylistFile *mpd_newPlaylistFile()
{
	mpd_PlaylistFile *playlist = malloc(sizeof(mpd_PlaylistFile));

	mpd_initPlaylistFile(playlist);

	return playlist;
}

void mpd_freePlaylistFile(mpd_PlaylistFile * playlist)
{
	mpd_finishPlaylistFile(playlist);
	free(playlist);
}

mpd_PlaylistFile *mpd_playlistFileDup(mpd_PlaylistFile * playlist)
{
	mpd_PlaylistFile *ret = mpd_newPlaylistFile();

	if (playlist->path)
		ret->path = strdup(playlist->path);

	return ret;
}

void mpd_initInfoEntity(mpd_InfoEntity * entity)
{
	entity->info.directory = NULL;
}

void mpd_finishInfoEntity(mpd_InfoEntity * entity)
{
	if (entity->info.directory) {
		if (entity->type == MPD_INFO_ENTITY_TYPE_DIRECTORY) {
			mpd_freeDirectory(entity->info.directory);
		} else if (entity->type == MPD_INFO_ENTITY_TYPE_SONG) {
			mpd_freeSong(entity->info.song);
		} else if (entity->type ==
			   MPD_INFO_ENTITY_TYPE_PLAYLISTFILE) {
			mpd_freePlaylistFile(entity->info.playlistFile);
		}
	}
}

mpd_InfoEntity *mpd_newInfoEntity()
{
	mpd_InfoEntity *entity = malloc(sizeof(mpd_InfoEntity));

	mpd_initInfoEntity(entity);

	return entity;
}

void mpd_freeInfoEntity(mpd_InfoEntity * entity)
{
	mpd_finishInfoEntity(entity);
	free(entity);
}

void mpd_sendInfoCommand(mpd_Connection * connection, char *command)
{
	mpd_executeCommand(connection, command);
}

mpd_InfoEntity *mpd_getNextInfoEntity(mpd_Connection * connection)
{
	mpd_InfoEntity *entity = NULL;

	if (connection->doneProcessing || (connection->listOks &&
					   connection->doneListOk)) {
		return NULL;
	}

	if (!connection->returnElement)
		mpd_getNextReturnElement(connection);

	if (connection->returnElement) {
		if (strcmp(connection->returnElement->name, "file") == 0) {
			entity = mpd_newInfoEntity();
			entity->type = MPD_INFO_ENTITY_TYPE_SONG;
			entity->info.song = mpd_newSong();
			entity->info.song->file =
			    strdup(connection->returnElement->value);
		} else
		    if (strcmp
			(connection->returnElement->name,
			 "directory") == 0) {
			entity = mpd_newInfoEntity();
			entity->type = MPD_INFO_ENTITY_TYPE_DIRECTORY;
			entity->info.directory = mpd_newDirectory();
			entity->info.directory->path =
			    strdup(connection->returnElement->value);
		} else
		    if (strcmp(connection->returnElement->name, "playlist")
			== 0) {
			entity = mpd_newInfoEntity();
			entity->type = MPD_INFO_ENTITY_TYPE_PLAYLISTFILE;
			entity->info.playlistFile = mpd_newPlaylistFile();
			entity->info.playlistFile->path =
			    strdup(connection->returnElement->value);
		} else {
			connection->error = 1;
			strcpy(connection->errorStr,
			       "problem parsing song info");
			return NULL;
		}
	} else
		return NULL;

	mpd_getNextReturnElement(connection);
	while (connection->returnElement) {
		mpd_ReturnElement *re = connection->returnElement;

		if (strcmp(re->name, "file") == 0)
			return entity;
		else if (strcmp(re->name, "directory") == 0)
			return entity;
		else if (strcmp(re->name, "playlist") == 0)
			return entity;

		if (entity->type == MPD_INFO_ENTITY_TYPE_SONG
		    && strlen(re->value)) {
			if (!entity->info.song->artist
			    && strcmp(re->name, "Artist") == 0) {
				entity->info.song->artist =
				    strdup(re->value);
			} else if (!entity->info.song->album
				   && strcmp(re->name, "Album") == 0) {
				entity->info.song->album =
				    strdup(re->value);
			} else if (!entity->info.song->title
				   && strcmp(re->name, "Title") == 0) {
				entity->info.song->title =
				    strdup(re->value);
			} else if (!entity->info.song->track
				   && strcmp(re->name, "Track") == 0) {
				entity->info.song->track =
				    strdup(re->value);
			} else if (!entity->info.song->name
				   && strcmp(re->name, "Name") == 0) {
				entity->info.song->name =
				    strdup(re->value);
			} else if (entity->info.song->time ==
				   MPD_SONG_NO_TIME
				   && strcmp(re->name, "Time") == 0) {
				entity->info.song->time = atoi(re->value);
			} else if (entity->info.song->pos ==
				   MPD_SONG_NO_NUM
				   && strcmp(re->name, "Pos") == 0) {
				entity->info.song->pos = atoi(re->value);
			} else if (entity->info.song->id == MPD_SONG_NO_ID
				   && strcmp(re->name, "Id") == 0) {
				entity->info.song->id = atoi(re->value);
			}
		} else if (entity->type == MPD_INFO_ENTITY_TYPE_DIRECTORY) {
		} else if (entity->type ==
			   MPD_INFO_ENTITY_TYPE_PLAYLISTFILE) {
		}

		mpd_getNextReturnElement(connection);
	}

	return entity;
}

char *mpd_getNextReturnElementNamed(mpd_Connection * connection,
				    const char *name)
{
	if (connection->doneProcessing || (connection->listOks &&
					   connection->doneListOk)) {
		return NULL;
	}

	mpd_getNextReturnElement(connection);
	while (connection->returnElement) {
		mpd_ReturnElement *re = connection->returnElement;

		if (strcmp(re->name, name) == 0)
			return strdup(re->value);
		mpd_getNextReturnElement(connection);
	}

	return NULL;
}

char *mpd_getNextArtist(mpd_Connection * connection)
{
	return mpd_getNextReturnElementNamed(connection, "Artist");
}

char *mpd_getNextAlbum(mpd_Connection * connection)
{
	return mpd_getNextReturnElementNamed(connection, "Album");
}

void mpd_sendPlaylistInfoCommand(mpd_Connection * connection, int songPos)
{
	char *string = malloc(strlen("playlistinfo") + 25);
	sprintf(string, "playlistinfo \"%i\"\n", songPos);
	mpd_sendInfoCommand(connection, string);
	free(string);
}

void mpd_sendPlaylistIdCommand(mpd_Connection * connection, int id)
{
	char *string = malloc(strlen("playlistid") + 25);
	sprintf(string, "playlistid \"%i\"\n", id);
	mpd_sendInfoCommand(connection, string);
	free(string);
}

void
mpd_sendPlChangesCommand(mpd_Connection * connection, long long playlist)
{
	char *string = malloc(strlen("plchanges") + 25);
	sprintf(string, "plchanges \"%lld\"\n", playlist);
	mpd_sendInfoCommand(connection, string);
	free(string);
}

void mpd_sendListallCommand(mpd_Connection * connection, const char *dir)
{
	char *sDir = mpd_sanitizeArg(dir);
	char *string = malloc(strlen("listall") + strlen(sDir) + 5);
	sprintf(string, "listall \"%s\"\n", sDir);
	mpd_sendInfoCommand(connection, string);
	free(string);
	free(sDir);
}

void
mpd_sendListallInfoCommand(mpd_Connection * connection, const char *dir)
{
	char *sDir = mpd_sanitizeArg(dir);
	char *string = malloc(strlen("listallinfo") + strlen(sDir) + 5);
	sprintf(string, "listallinfo \"%s\"\n", sDir);
	mpd_sendInfoCommand(connection, string);
	free(string);
	free(sDir);
}

void mpd_sendLsInfoCommand(mpd_Connection * connection, const char *dir)
{
	char *sDir = mpd_sanitizeArg(dir);
	char *string = malloc(strlen("lsinfo") + strlen(sDir) + 5);
	sprintf(string, "lsinfo \"%s\"\n", sDir);
	mpd_sendInfoCommand(connection, string);
	free(string);
	free(sDir);
}

void mpd_sendCurrentSongCommand(mpd_Connection * connection)
{
	mpd_executeCommand(connection, "currentsong\n");
}

void
mpd_sendSearchCommand(mpd_Connection * connection, int table,
		      const char *str)
{
	char st[10];
	char *string;
	char *sanitStr = mpd_sanitizeArg(str);
	if (table == MPD_TABLE_ARTIST)
		strcpy(st, "artist");
	else if (table == MPD_TABLE_ALBUM)
		strcpy(st, "album");
	else if (table == MPD_TABLE_TITLE)
		strcpy(st, "title");
	else if (table == MPD_TABLE_FILENAME)
		strcpy(st, "filename");
	else {
		connection->error = 1;
		strcpy(connection->errorStr, "unknown table for search");
		return;
	}
	string =
	    malloc(strlen("search") + strlen(sanitStr) + strlen(st) + 6);
	sprintf(string, "search %s \"%s\"\n", st, sanitStr);
	mpd_sendInfoCommand(connection, string);
	free(string);
	free(sanitStr);
}

void
mpd_sendFindCommand(mpd_Connection * connection, int table,
		    const char *str)
{
	char st[10];
	char *string;
	char *sanitStr = mpd_sanitizeArg(str);
	if (table == MPD_TABLE_ARTIST)
		strcpy(st, "artist");
	else if (table == MPD_TABLE_ALBUM)
		strcpy(st, "album");
	else if (table == MPD_TABLE_TITLE)
		strcpy(st, "title");
	else {
		connection->error = 1;
		strcpy(connection->errorStr, "unknown table for find");
		return;
	}
	string =
	    malloc(strlen("find") + strlen(sanitStr) + strlen(st) + 6);
	sprintf(string, "find %s \"%s\"\n", st, sanitStr);
	mpd_sendInfoCommand(connection, string);
	free(string);
	free(sanitStr);
}

void
mpd_sendListCommand(mpd_Connection * connection, int table,
		    const char *arg1)
{
	char st[10];
	char *string;
	if (table == MPD_TABLE_ARTIST)
		strcpy(st, "artist");
	else if (table == MPD_TABLE_ALBUM)
		strcpy(st, "album");
	else {
		connection->error = 1;
		strcpy(connection->errorStr, "unknown table for list");
		return;
	}
	if (arg1) {
		char *sanitArg1 = mpd_sanitizeArg(arg1);
		string =
		    malloc(strlen("list") + strlen(sanitArg1) +
			   strlen(st) + 6);
		sprintf(string, "list %s \"%s\"\n", st, sanitArg1);
		free(sanitArg1);
	} else {
		string = malloc(strlen("list") + strlen(st) + 3);
		sprintf(string, "list %s\n", st);
	}
	mpd_sendInfoCommand(connection, string);
	free(string);
}

void mpd_sendAddCommand(mpd_Connection * connection, const char *file)
{
	char *sFile = mpd_sanitizeArg(file);
	char *string = malloc(strlen("add") + strlen(sFile) + 5);
	sprintf(string, "add \"%s\"\n", sFile);
	mpd_executeCommand(connection, string);
	free(string);
	free(sFile);
}

void mpd_sendDeleteCommand(mpd_Connection * connection, int songPos)
{
	char *string = malloc(strlen("delete") + 25);
	sprintf(string, "delete \"%i\"\n", songPos);
	mpd_sendInfoCommand(connection, string);
	free(string);
}

void mpd_sendDeleteIdCommand(mpd_Connection * connection, int id)
{
	char *string = malloc(strlen("deleteid") + 25);
	sprintf(string, "deleteid \"%i\"\n", id);
	mpd_sendInfoCommand(connection, string);
	free(string);
}

void mpd_sendSaveCommand(mpd_Connection * connection, const char *name)
{
	char *sName = mpd_sanitizeArg(name);
	char *string = malloc(strlen("save") + strlen(sName) + 5);
	sprintf(string, "save \"%s\"\n", sName);
	mpd_executeCommand(connection, string);
	free(string);
	free(sName);
}

void mpd_sendLoadCommand(mpd_Connection * connection, const char *name)
{
	char *sName = mpd_sanitizeArg(name);
	char *string = malloc(strlen("load") + strlen(sName) + 5);
	sprintf(string, "load \"%s\"\n", sName);
	mpd_executeCommand(connection, string);
	free(string);
	free(sName);
}

void mpd_sendRmCommand(mpd_Connection * connection, const char *name)
{
	char *sName = mpd_sanitizeArg(name);
	char *string = malloc(strlen("rm") + strlen(sName) + 5);
	sprintf(string, "rm \"%s\"\n", sName);
	mpd_executeCommand(connection, string);
	free(string);
	free(sName);
}

void mpd_sendShuffleCommand(mpd_Connection * connection)
{
	mpd_executeCommand(connection, "shuffle\n");
}

void mpd_sendClearCommand(mpd_Connection * connection)
{
	mpd_executeCommand(connection, "clear\n");
}

void mpd_sendPlayCommand(mpd_Connection * connection, int songPos)
{
	char *string = malloc(strlen("play") + 25);
	sprintf(string, "play \"%i\"\n", songPos);
	mpd_sendInfoCommand(connection, string);
	free(string);
}

void mpd_sendPlayIdCommand(mpd_Connection * connection, int id)
{
	char *string = malloc(strlen("playid") + 25);
	sprintf(string, "playid \"%i\"\n", id);
	mpd_sendInfoCommand(connection, string);
	free(string);
}

void mpd_sendStopCommand(mpd_Connection * connection)
{
	mpd_executeCommand(connection, "stop\n");
}

void mpd_sendPauseCommand(mpd_Connection * connection, int pauseMode)
{
	char *string = malloc(strlen("pause") + 25);
	sprintf(string, "pause \"%i\"\n", pauseMode);
	mpd_executeCommand(connection, string);
	free(string);
}

void mpd_sendNextCommand(mpd_Connection * connection)
{
	mpd_executeCommand(connection, "next\n");
}

void mpd_sendMoveCommand(mpd_Connection * connection, int from, int to)
{
	char *string = malloc(strlen("move") + 25);
	sprintf(string, "move \"%i\" \"%i\"\n", from, to);
	mpd_sendInfoCommand(connection, string);
	free(string);
}

void mpd_sendMoveIdCommand(mpd_Connection * connection, int id, int to)
{
	char *string = malloc(strlen("moveid") + 25);
	sprintf(string, "moveid \"%i\" \"%i\"\n", id, to);
	mpd_sendInfoCommand(connection, string);
	free(string);
}

void mpd_sendSwapCommand(mpd_Connection * connection, int song1, int song2)
{
	char *string = malloc(strlen("swap") + 25);
	sprintf(string, "swap \"%i\" \"%i\"\n", song1, song2);
	mpd_sendInfoCommand(connection, string);
	free(string);
}

void mpd_sendSwapIdCommand(mpd_Connection * connection, int id1, int id2)
{
	char *string = malloc(strlen("swapid") + 25);
	sprintf(string, "swapid \"%i\" \"%i\"\n", id1, id2);
	mpd_sendInfoCommand(connection, string);
	free(string);
}

void mpd_sendSeekCommand(mpd_Connection * connection, int song, int time)
{
	char *string = malloc(strlen("seek") + 25);
	sprintf(string, "seek \"%i\" \"%i\"\n", song, time);
	mpd_sendInfoCommand(connection, string);
	free(string);
}

void mpd_sendSeekIdCommand(mpd_Connection * connection, int id, int time)
{
	char *string = malloc(strlen("seekid") + 25);
	sprintf(string, "seekid \"%i\" \"%i\"\n", id, time);
	mpd_sendInfoCommand(connection, string);
	free(string);
}

void mpd_sendUpdateCommand(mpd_Connection * connection, char *path)
{
	char *sPath = mpd_sanitizeArg(path);
	char *string = malloc(strlen("update") + strlen(sPath) + 5);
	sprintf(string, "update \"%s\"\n", sPath);
	mpd_sendInfoCommand(connection, string);
	free(string);
	free(sPath);
}

int mpd_getUpdateId(mpd_Connection * connection)
{
	char *jobid;
	int ret = 0;

	jobid = mpd_getNextReturnElementNamed(connection, "updating_db");
	if (jobid) {
		ret = atoi(jobid);
		free(jobid);
	}

	return ret;
}

void mpd_sendPrevCommand(mpd_Connection * connection)
{
	mpd_executeCommand(connection, "previous\n");
}

void mpd_sendRepeatCommand(mpd_Connection * connection, int repeatMode)
{
	char *string = malloc(strlen("repeat") + 25);
	sprintf(string, "repeat \"%i\"\n", repeatMode);
	mpd_executeCommand(connection, string);
	free(string);
}

void mpd_sendRandomCommand(mpd_Connection * connection, int randomMode)
{
	char *string = malloc(strlen("random") + 25);
	sprintf(string, "random \"%i\"\n", randomMode);
	mpd_executeCommand(connection, string);
	free(string);
}

void mpd_sendSetvolCommand(mpd_Connection * connection, int volumeChange)
{
	char *string = malloc(strlen("setvol") + 25);
	sprintf(string, "setvol \"%i\"\n", volumeChange);
	mpd_executeCommand(connection, string);
	free(string);
}

void mpd_sendVolumeCommand(mpd_Connection * connection, int volumeChange)
{
	char *string = malloc(strlen("volume") + 25);
	sprintf(string, "volume \"%i\"\n", volumeChange);
	mpd_executeCommand(connection, string);
	free(string);
}

void mpd_sendCrossfadeCommand(mpd_Connection * connection, int seconds)
{
	char *string = malloc(strlen("crossfade") + 25);
	sprintf(string, "crossfade \"%i\"\n", seconds);
	mpd_executeCommand(connection, string);
	free(string);
}

void mpd_sendPasswordCommand(mpd_Connection * connection, const char *pass)
{
	char *sPass = mpd_sanitizeArg(pass);
	char *string = malloc(strlen("password") + strlen(sPass) + 5);
	sprintf(string, "password \"%s\"\n", sPass);
	mpd_executeCommand(connection, string);
	free(string);
	free(sPass);
}

void mpd_sendCommandListBegin(mpd_Connection * connection)
{
	if (connection->commandList) {
		strcpy(connection->errorStr,
		       "already in command list mode");
		connection->error = 1;
		return;
	}
	connection->commandList = COMMAND_LIST;
	mpd_executeCommand(connection, "command_list_begin\n");
}

void mpd_sendCommandListOkBegin(mpd_Connection * connection)
{
	if (connection->commandList) {
		strcpy(connection->errorStr,
		       "already in command list mode");
		connection->error = 1;
		return;
	}
	connection->commandList = COMMAND_LIST_OK;
	mpd_executeCommand(connection, "command_list_ok_begin\n");
	connection->listOks = 0;
}

void mpd_sendCommandListEnd(mpd_Connection * connection)
{
	if (!connection->commandList) {
		strcpy(connection->errorStr, "not in command list mode");
		connection->error = 1;
		return;
	}
	connection->commandList = 0;
	mpd_executeCommand(connection, "command_list_end\n");
}

void mpd_signalHandler(int signal)
{
	if (signal == SIGPIPE) {
		fprintf(stderr, "Conky: recieved SIGPIPE from MPD connection\n");
		sigpipe = 1;
	}
}

