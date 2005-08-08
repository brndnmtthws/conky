/*
 * Conky, a system monitor, based on torsmo
 *
 * This program is licensed under BSD license, read COPYING
 *
 *  $Id$
 */

#include <pthread.h>

static pthread_t daemon;
static int daemon_status = 0;

/* okay, heres how it will basically work.
 * when something connects, it will first send the conkyrc on the local (daemonized) server
 * after this, it will simply continue to send all the buffered text to the remote client
 * http://analyser.oli.tudelft.nl/beej/mirror/net/html/
 *  http://www.kegel.com/c10k.html
 */

void *daemon_thread()
{
	/* do something */
	return NULL;
}

static void daemon_loop()
{
	/* create thread, keep an eye on it */
	int iret;
	if (!daemon_status) {
		daemon_status = 1;
		iret = pthread_create(&daemon, NULL, daemon_thread, NULL);
	} else if (daemon_status == 1) {
		/* thread is still running, we'll just wait for it to finish for now */
		pthread_join(daemon, NULL);
		daemon_status = 0;
	} else {
		/* something else */
	}
}
