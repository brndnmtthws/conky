/*
* Conky, a system monitor, based on torsmo
*
* This program is licensed under BSD license, read COPYING
*
*  $Id$
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

static pthread_t daemon_thread;
static int daemon_status = 0;
static char *data;

/* okay, heres how it will basically work.
* when something connects, it will first send the conkyrc on the local (daemonized) server
* after this, it will simply continue to send all the buffered text to the remote client
* http://analyser.oli.tudelft.nl/beej/mirror/net/html/
*  http://www.kegel.com/c10k.html
*/

#define MYPORT 3490    // the port users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold

void sigchld_handler(int s)
{
	while(wait(NULL) > 0);
}

void *daemon_loop()
{
	/* do something */



	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct sockaddr_in my_addr;    // my address information
	struct sockaddr_in their_addr; // connector's address information
	int sin_size;
	struct sigaction sa;
	int yes=1;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}
	
	my_addr.sin_family = AF_INET;         // host byte order
	my_addr.sin_port = htons(MYPORT);     // short, network byte order
	my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
	memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct

	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr))
		   == -1) {
		perror("bind");
		exit(1);
		   }

		   if (listen(sockfd, BACKLOG) == -1) {
			   perror("listen");
			   exit(1);
		   }

		   sa.sa_handler = sigchld_handler; // reap all dead processes
		   sigemptyset(&sa.sa_mask);
		   sa.sa_flags = SA_RESTART;
		   if (sigaction(SIGCHLD, &sa, NULL) == -1) {
			   perror("sigaction");
			   exit(1);
		   }

		   while(1) {  // main accept() loop
			   sin_size = sizeof(struct sockaddr_in);
			   if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr,
				&sin_size)) == -1) {
					perror("accept");
					continue;
				}
				printf("server: got connection from %s\n",
				       inet_ntoa(their_addr.sin_addr));
				if (!fork()) { // this is the child process
					close(sockfd); // child doesn't need the listener
					if (send(new_fd, data, 14, 0) == -1)
						perror("send");
					close(new_fd);
					exit(0);
				}
				close(new_fd);  // parent doesn't need this
		   }

		   return 0;
}

void daemon_run(const char *s)
{
	/* create thread, keep an eye on it */
	data = s;
	int iret;
	if (!daemon_status) {
		daemon_status = 1;
		iret = pthread_create(&daemon_thread, NULL, daemon_loop, NULL);
	} else if (daemon_status == 1) {
		/* thread is still running, we'll just wait for it to finish for now */
		pthread_join(daemon_thread, NULL);
		daemon_status = 0;
	} else {
		/* something else */
	}
}
