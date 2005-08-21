/*
 * Conky, a system monitor, based on torsmo
 *
 * This program is licensed under BSD license, read COPYING
 *
 *  $Id$
 */
 
 /*
 
 okay, nothing here right now.  thanks for coming out
 
 */

#include "conky.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 3490 // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

void client()
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct hostent *he;
	struct sockaddr_in their_addr; // connector's address information 
	if ((he=gethostbyname("localhost")) == NULL) {  // get the host info 
		perror("gethostbyname");
		exit(1);
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	their_addr.sin_family = AF_INET;    // host byte order 
	their_addr.sin_port = htons(PORT);  // short, network byte order 
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(their_addr.sin_zero), '\0', 8);  // zero the rest of the struct 

	if (connect(sockfd, (struct sockaddr *)&their_addr,
	    sizeof(struct sockaddr)) == -1) {
		    perror("connect");
		    exit(1);
	    }

	    if ((numbytes=recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
		    perror("recv");
		    exit(1);
	    }

	    buf[numbytes] = '\0';

	    printf("Received: %s",buf);

	    close(sockfd);

	    return 0;
}
