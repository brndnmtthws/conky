/*
 * mldonkey.c: MLDonkey stuff for Conky
 *
 *  $Id$
 */
 
#include <arpa/inet.h>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include "conky.h"

int64 buf_to_int(char *buf, int pos, int size);
void int_to_buf(int64 i, char *buf, int pos, int size);

#define BUF16_TO_INT(buf, pos) buf_to_int((buf), (pos), 2)
#define BUF32_TO_INT(buf, pos) buf_to_int((buf), (pos), 4)
#define BUF64_TO_INT(buf, pos) buf_to_int((buf), (pos), 8)

#define INT_TO_BUF16(i, buf, pos) int_to_buf((i), (buf), (pos), 2)
#define INT_TO_BUF32(i, buf, pos) int_to_buf((i), (buf), (pos), 4)
#define INT_TO_BUF64(i, buf, pos) int_to_buf((i), (buf), (pos), 8)

mldonkey_info mlinfo;
mldonkey_config mlconfig;

/* Call this function to update the information about mldonkey.
 * Note that the function will not reconnect to mldonkey if the
 * pointer to the mldonkey_config has not changed. As it uses static
 * data, it cannot be used in a multithreaded env.
 * Returns 1 if connected and info filled, 0 if connected but not filled,
 * -1 otherwise. */

enum to_gui {
	CoreProtocol,		/* 0 */
	Options_info,
	RESERVED2,
	DefineSearches,
	Result_info,
	Search_result,
	Search_waiting,
	File_info,
	File_downloaded,
	File_availability,
	File_source,		/* 10 */
	Server_busy,
	Server_user,
	Server_state,
	Server_info,
	Client_info,
	Client_state,
	Client_friend,
	Client_file,
	Console,
	Network_info,		/* 20 */
	User_info,
	Room_info,
	Room_message,
	Room_add_user,
	Client_stats,
	Server_info_v2,
	MessageFromClient,
	ConnectedServers,
	DownloadFiles,
	DownloadedFiles,	/* 30 */
	Room_info_v2,
	Room_remove_user,
	Shared_file_info,
	Shared_file_upload,
	Shared_file_unshared,
	Add_section_option,
	Client_stats_v2,
	Add_plugin_option,
	Client_stats_v3,
	File_info_v2,		/* 40 */
	DownloadFiles_v2,
	DownloadedFiles_v2,
	File_info_v3,
	DownloadFiles_v3,
	DownloadedFiles_v3,
	File_downloaded_v2,
	BadPassword,
	Shared_file_info_v2,
	Client_stats_v4,	/* 49 */
};

#define MLDONKEY_DISCONNECTED   0
#define MLDONKEY_CONNECTING     1
#define MLDONKEY_AUTHENTICATING 2
#define MLDONKEY_CONNECTED      3

#define MAX_MESSAGE_LEN 65000
static int write_pos = 0;
static char write_buf[MAX_MESSAGE_LEN];
static char read_buf[MAX_MESSAGE_LEN];
static int read_pos;
static int mldonkey_sock = -1;
static int mldonkey_state = MLDONKEY_DISCONNECTED;
static mldonkey_config *old_config = NULL;

/* int64 ------------------------------ */

int64 buf_to_int(char *buf, int pos, int size)
{
	int i;
	int64 res = 0;

	for (i = 0; i < size; i++) {
		res += (buf[pos + i] & 0xFF) << (8 * i);
	}
	return res;
}

void int_to_buf(int64 i, char *buf, int pos, int size)
{
	int j;

	for (j = 0; j < size; j++) {
		buf[pos + j] = (i & (-1)) >> (8 * j);
	}
}

/* Write operations --------------------- */

void init_message()
{
	write_pos = 0;
}

void write_int8(int code)
{
	write_buf[write_pos++] = code;
}

void write_opcode(int code)
{
	write_buf[write_pos++] = code;
}

void write_int16(int code)
{
	INT_TO_BUF16(code, write_buf, write_pos);
	write_pos += 2;
}

void write_int32(int code)
{
	INT_TO_BUF32(code, write_buf, write_pos);
	write_pos += 4;
}

void write_int64(int64 code)
{
	INT_TO_BUF64(code, write_buf, write_pos);
	write_pos += 8;
}

void write_string(char *str)
{
	if (str == NULL) {
		write_int16(0);
	} else {
		int len = strlen(str);
		write_int16(len);
		memcpy((void *) (write_buf + write_pos), (void *) str,
		       (size_t) len);
		write_pos += len;
	}
}


int write_message(char *mtype)
{
	char header[4];

	INT_TO_BUF32(write_pos, header, 0);
	if (4 != write(mldonkey_sock, header, 4) ||
	    write_pos != write(mldonkey_sock, (void *) write_buf,
			       (size_t) write_pos)) {
		ERR("Error in transmitting %s\n", mtype);
		write_pos = 0;

		/* Immediatly close the connection */
		close(mldonkey_sock);
		mldonkey_state = MLDONKEY_DISCONNECTED;
		mldonkey_sock = -1;
		return -1;
	} else {
		write_pos = 0;
		return 0;
	}
}


/* Read operations ----------------------------*/

int read_int8()
{
	return read_buf[read_pos++];
}

int read_int16()
{
	int i = BUF16_TO_INT(read_buf, read_pos);
	read_pos += 2;
	return i;
}

int read_int32()
{
	int i = BUF32_TO_INT(read_buf, read_pos);
	read_pos += 4;
	return i;
}

int64 read_int64()
{
	int64 i = BUF64_TO_INT(read_buf, read_pos);
	read_pos += 8;
	return i;
}

char *read_string()
{
	char *buf;
	int len;

	len = BUF16_TO_INT(read_buf, read_pos);
	read_pos += 2;

	buf = (char *) malloc((size_t) len + 1);
	memmove(read_buf + read_pos, buf, len);
	buf[len] = 0;
	read_pos += len;

	return buf;
}

/* protocol impl. ----------------------------- */

void close_sock();

/* This function returns the number of messages read, 0 if it blocks,
-1 on error. */
int cut_messages(int reinit)
{
	int nread;
	static int toread = 0;
	static int pos = 0;

	if (reinit) {
		toread = 0;
		pos = 0;
		read_pos = 0;
		return 0;
	}

	while (1) {
		if (toread == 0) {
			nread =
			    read(mldonkey_sock, read_buf + pos, 4 - pos);
			if (nread <= 0) {
				if (errno == EAGAIN) {
					return 0;
				} else {
					close_sock();
					pos = 0;
					toread = 0;
					return -1;
				}
			}
			pos += nread;
			if (pos == 4) {
				toread = BUF32_TO_INT(read_buf, 0);
				pos = 0;
			}
		} else {
			nread =
			    read(mldonkey_sock, read_buf + pos,
				 toread - pos);
			if (nread <= 0) {
				if (errno == EAGAIN)
					return 0;
				else {
					pos = 0;
					toread = 0;
					close_sock();
					return -1;
				}
			}
			pos += nread;
			if (pos == toread) {
				/* We have one message !!! */
				int old_pos = pos;
				read_pos = 0;
				pos = 0;
				toread = 0;

				return old_pos;
			}
		}
	}
}

void close_sock()
{
	old_config = NULL;
	if (mldonkey_sock >= 0)
		close(mldonkey_sock);
	mldonkey_sock = -1;
	mldonkey_state = MLDONKEY_DISCONNECTED;
	cut_messages(1);
}

int mldonkey_connect(mldonkey_config * config)
{
	if (config != old_config) {
		struct sockaddr_in sa;
		int retcode;
		close_sock();


		old_config = config;
		/* resolve hostname */
		memset(&sa, 0, sizeof(sa));

		if (config->mldonkey_hostname == NULL)
			config->mldonkey_hostname = "127.0.0.1";
		if (config->mldonkey_hostname[0] >= '0' &&
		    config->mldonkey_hostname[0] <= '9') {
#ifdef HAS_INET_ATON
			if (inet_aton
			    (config->mldonkey_hostname, &sa.sin_addr) == 0)
				return -1;
#else

			sa.sin_addr.s_addr =
			    inet_addr(config->mldonkey_hostname);
			if (sa.sin_addr.s_addr == (unsigned int) -1)
				return -1;
#endif

		} else {
			struct hostent *hp;
			hp = gethostbyname(config->mldonkey_hostname);
			if (hp == (struct hostent *) NULL)
				return -1;
			sa.sin_addr.s_addr =
			    (unsigned long) hp->h_addr_list[0];
		}

		sa.sin_port = htons(config->mldonkey_port);
		sa.sin_family = AF_INET;

		if ((mldonkey_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
			ERR("Opening socket");
			close_sock();
			return -1;
		}

		if (connect
		    (mldonkey_sock, (struct sockaddr *) &sa,
		     sizeof(sa)) < 0) {
			if (errno != EAGAIN && errno != EINTR
			    && errno != EINPROGRESS
			    && errno != EWOULDBLOCK) {
//        ERR("Connection failed");
				close_sock();
				return -1;
			}
		}

		retcode = fcntl(mldonkey_sock, F_GETFL, 0);
		if (retcode == -1 ||
		    fcntl(mldonkey_sock, F_SETFL,
			  retcode | O_NONBLOCK) == -1) {
			return -1;
		}


		mldonkey_state = MLDONKEY_CONNECTING;
		return 0;
	}

	return 0;
}

int mldonkey_can_read()
{
	return cut_messages(0);
}

int mldonkey_info_message(mldonkey_info * info)
{
	int opcode = read_int16();

	switch (opcode) {

	case CoreProtocol:
		init_message();

		write_int16(0);	/* GUI protocol */
		write_int32(10);	/* Version 10 ! */
		write_message("GuiProtocol");

		write_int16(47);	/* GUI protocol */

		write_int16(1);
		write_int32(1);
		write_int8(1);
		write_message("GuiExtensions");

		init_message();
		write_int16(5);	/* Password */
		write_string(old_config->mldonkey_password);
		write_message("Password");

		break;

	case BadPassword:
		ERR("Bad Password\n");
		close_sock();
		break;

	case Client_stats:
	case Client_stats_v2:
	case Client_stats_v3:
		ERR("Client stats format too old...\n");
		break;

	case Client_stats_v4:
		mldonkey_state = MLDONKEY_CONNECTED;

		info->upload_counter = read_int64();
		info->download_counter = read_int64();
		info->shared_counter = read_int64();
		info->nshared_files = read_int32();
		info->tcp_upload_rate = read_int32();
		info->tcp_download_rate = read_int32();
		info->udp_upload_rate = read_int32();
		info->udp_download_rate = read_int32();
		info->ndownloading_files = read_int32();
		info->ndownloaded_files = read_int32();

		break;
	}

	return 0;
}

int get_mldonkey_status(mldonkey_config * config, mldonkey_info * info)
{
	if (mldonkey_connect(config) >= 0) {
		while (mldonkey_can_read() > 0) {
			mldonkey_info_message(info);
		}
	}
	return mldonkey_state;
}
