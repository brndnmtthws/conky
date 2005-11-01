/* -------------------------------------------------------
 * test-portmon.c: unit testing for libtcp-portmon library
 * Philip Kovacs (kovacsp3@comcast.net) 2005
 * ------------------------------------------------------*/

#include <signal.h>
#include <unistd.h>
#include "libtcp-portmon.h"

volatile int g_signal;

tcp_port_monitor_collection_t *p_collection = NULL;

void set_terminate(int); 

int main()
{
   tcp_port_monitor_t * p_monitor;
   int i;
   char buf[256];

   g_signal=0;

   (void) signal(SIGINT,set_terminate);

   if ( (p_collection = create_tcp_port_monitor_collection()) == NULL) {
	fprintf(stderr,"error: create_tcp_port_monitor_collection()\n");
   	exit(1);
   }

   if ( (p_monitor = create_tcp_port_monitor( 1, 65535 )) == NULL) {
	fprintf(stderr,"error: create_tcp_port_monitor()\n");
	exit(1);
   }

   if ( (insert_tcp_port_monitor_into_collection( p_collection, p_monitor )) != 0 ) {
	 fprintf(stderr,"error: insert_tcp_port_monitor_into_collection()\n");
	 exit(1);
   }
	
   while (1)
   {
      update_tcp_port_monitor_collection( p_collection );

      if ( (p_monitor = find_tcp_port_monitor( p_collection, 1, 65535 )) == NULL ) {
	   fprintf(stderr,"error: find_tcp_port_monitor()\n");
	   exit(1);
      }

      peek_tcp_port_monitor( p_monitor, COUNT, 0, buf, sizeof(buf)-1 );
      fprintf( stdout, "\n(%d,%d) COUNT=%s\n", 
			p_monitor->port_range_begin, p_monitor->port_range_end, buf );
      for ( i=0; i<p_monitor->hash.size; i++ )
      {
 	   fprintf( stdout, "(%d,%d) -- connection %d -- ", 
			p_monitor->port_range_begin, p_monitor->port_range_end, i );
 	   peek_tcp_port_monitor( p_monitor, REMOTEIP, i, buf, sizeof(buf)-1 );
      	   fprintf( stdout, "%s ", buf );
 	   peek_tcp_port_monitor( p_monitor, REMOTEHOST, i, buf, sizeof(buf)-1 );
	   fprintf( stdout, "(%s) ", buf );
 	   peek_tcp_port_monitor( p_monitor, REMOTEPORT, i, buf, sizeof(buf)-1 );
	   fprintf( stdout, ": %s ", buf );
	   peek_tcp_port_monitor( p_monitor, LOCALIP, i, buf, sizeof(buf)-1 );
	   fprintf( stdout, "on %s ", buf );
 	   peek_tcp_port_monitor( p_monitor, LOCALHOST, i, buf, sizeof(buf)-1 );
	   fprintf( stdout, "(%s) ", buf );
 	   peek_tcp_port_monitor( p_monitor, LOCALPORT, i, buf, sizeof(buf)-1 );
	   fprintf( stdout, ": %s\n", buf );
	   /*
 	   peek_tcp_port_monitor( p_monitor, LOCALSERVICE, i, buf, sizeof(buf)-1 );
	   fprintf( stdout, "(%s)\n", buf );
	   */
      }
 
      printf("\n<< PRESS CNTL-C TO EXIT >>\n");

      sleep(3);

      if ( g_signal && p_collection ) {
	   destroy_tcp_port_monitor_collection(p_collection);
	   break;
      }
   }

   printf("bye\n");
   return(0);
}

void set_terminate(int sig)
{
   g_signal=1;
}
