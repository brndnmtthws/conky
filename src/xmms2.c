/*
* xmms2.c: xmms2 stuff for Conky
*
*
*/

#include "conky.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <xmmsclient/xmmsclient.h>

#define CONN_INIT    0
#define CONN_OK      1
#define CONN_NO      2

/* callbacks */

void connection_lost( void *ptr ) {
    ((struct information *)ptr)->xmms2_conn_state = CONN_NO;

    if ( ((struct information *)ptr)->xmms2.status == NULL )
        ((struct information *)ptr)->xmms2.status = malloc( TEXT_BUFFER_SIZE );
    strncpy( ((struct information *)ptr)->xmms2.status, "xmms2 not responding", TEXT_BUFFER_SIZE - 1 );


    if ( ((struct information *)ptr)->xmms2.artist == NULL )
        ((struct information *)ptr)->xmms2.artist = malloc( TEXT_BUFFER_SIZE );
    strncpy( ((struct information *)ptr)->xmms2.artist, "", TEXT_BUFFER_SIZE - 1 );

    if ( ((struct information *)ptr)->xmms2.album == NULL )
        ((struct information *)ptr)->xmms2.album = malloc( TEXT_BUFFER_SIZE );
    strncpy( ((struct information *)ptr)->xmms2.album, "", TEXT_BUFFER_SIZE - 1 );

    if ( ((struct information *)ptr)->xmms2.title == NULL )
        ((struct information *)ptr)->xmms2.title = malloc( TEXT_BUFFER_SIZE );
    strncpy( ((struct information *)ptr)->xmms2.title, "", TEXT_BUFFER_SIZE - 1 );

    if ( ((struct information *)ptr)->xmms2.genre == NULL )
        ((struct information *)ptr)->xmms2.genre = malloc( TEXT_BUFFER_SIZE );
    strncpy( ((struct information *)ptr)->xmms2.genre, "", TEXT_BUFFER_SIZE - 1 );

    if ( ((struct information *)ptr)->xmms2.comment == NULL )
        ((struct information *)ptr)->xmms2.comment = malloc( TEXT_BUFFER_SIZE );
    strncpy( ((struct information *)ptr)->xmms2.comment, "", TEXT_BUFFER_SIZE - 1 );

    if ( ((struct information *)ptr)->xmms2.decoder == NULL )
        ((struct information *)ptr)->xmms2.decoder = malloc( TEXT_BUFFER_SIZE );
    strncpy( ((struct information *)ptr)->xmms2.decoder, "", TEXT_BUFFER_SIZE - 1 );

    if ( ((struct information *)ptr)->xmms2.transport == NULL )
        ((struct information *)ptr)->xmms2.transport = malloc( TEXT_BUFFER_SIZE );
    strncpy( ((struct information *)ptr)->xmms2.transport, "", TEXT_BUFFER_SIZE - 1 );

    if ( ((struct information *)ptr)->xmms2.url == NULL )
        ((struct information *)ptr)->xmms2.url = malloc( TEXT_BUFFER_SIZE );
    strncpy( ((struct information *)ptr)->xmms2.url, "", TEXT_BUFFER_SIZE - 1 );

    if ( ((struct information *)ptr)->xmms2.date == NULL )
        ((struct information *)ptr)->xmms2.date = malloc( TEXT_BUFFER_SIZE );
    strncpy( ((struct information *)ptr)->xmms2.date, "", TEXT_BUFFER_SIZE - 1 );

    ((struct information *)ptr)->xmms2.tracknr = 0;
    ((struct information *)ptr)->xmms2.id = 0;
    ((struct information *)ptr)->xmms2.bitrate = 0;
    ((struct information *)ptr)->xmms2.duration = 0;
    ((struct information *)ptr)->xmms2.elapsed = 0;
    ((struct information *)ptr)->xmms2.size = 0;
    ((struct information *)ptr)->xmms2.progress = 0;
}

void handle_curent_id( xmmsc_result_t *res, void *ptr ) {

    uint current_id;

    if ( xmmsc_result_get_uint( res, &current_id ) ) {

        xmmsc_result_t *res2;
        res2 = xmmsc_medialib_get_info( ((struct information *)ptr)->xmms2_conn, current_id );
        xmmsc_result_wait( res2 );


        if ( ((struct information *)ptr)->xmms2.artist == NULL )
            ((struct information *)ptr)->xmms2.artist = malloc( TEXT_BUFFER_SIZE );

        if ( ((struct information *)ptr)->xmms2.album == NULL )
            ((struct information *)ptr)->xmms2.album = malloc( TEXT_BUFFER_SIZE );

        if ( ((struct information *)ptr)->xmms2.title == NULL )
            ((struct information *)ptr)->xmms2.title = malloc( TEXT_BUFFER_SIZE );

        if ( ((struct information *)ptr)->xmms2.genre == NULL )
            ((struct information *)ptr)->xmms2.genre = malloc( TEXT_BUFFER_SIZE );

        if ( ((struct information *)ptr)->xmms2.comment == NULL )
            ((struct information *)ptr)->xmms2.comment = malloc( TEXT_BUFFER_SIZE );

        if ( ((struct information *)ptr)->xmms2.decoder == NULL )
            ((struct information *)ptr)->xmms2.decoder = malloc( TEXT_BUFFER_SIZE );

        if ( ((struct information *)ptr)->xmms2.transport == NULL )
            ((struct information *)ptr)->xmms2.transport = malloc( TEXT_BUFFER_SIZE );

        if ( ((struct information *)ptr)->xmms2.url == NULL )
            ((struct information *)ptr)->xmms2.url = malloc( TEXT_BUFFER_SIZE );

        if ( ((struct information *)ptr)->xmms2.date == NULL )
            ((struct information *)ptr)->xmms2.date = malloc( TEXT_BUFFER_SIZE );


        ((struct information *)ptr)->xmms2.id = current_id;

        char *temp;
        xmmsc_result_get_dict_entry_string( res2, "artist", &temp );
        if ( temp != NULL ) {
            strncpy( ((struct information *)ptr)->xmms2.artist, temp, TEXT_BUFFER_SIZE - 1 );
        } else {
            strncpy( ((struct information *)ptr)->xmms2.artist, "[Unknown]", TEXT_BUFFER_SIZE - 1 );
        }


        xmmsc_result_get_dict_entry_string( res2, "title", &temp );
        if ( temp != NULL ) {
            strncpy( ((struct information *)ptr)->xmms2.title, temp, TEXT_BUFFER_SIZE - 1 );
        } else {
            strncpy( ((struct information *)ptr)->xmms2.title, "[Unknown]", TEXT_BUFFER_SIZE - 1 );
        }

        xmmsc_result_get_dict_entry_string( res2, "album", &temp );
        if ( temp != NULL ) {
            strncpy( ((struct information *)ptr)->xmms2.album, temp, TEXT_BUFFER_SIZE - 1 );
        } else {
            strncpy( ((struct information *)ptr)->xmms2.album, "[Unknown]", TEXT_BUFFER_SIZE - 1 );
        }


        xmmsc_result_get_dict_entry_string( res2, "genre", &temp );
        if ( temp != NULL ) {

            strncpy( ((struct information *)ptr)->xmms2.genre, temp, TEXT_BUFFER_SIZE - 1 );
        } else {
            strncpy( ((struct information *)ptr)->xmms2.genre, "[Unknown]", TEXT_BUFFER_SIZE - 1 );
        }


        xmmsc_result_get_dict_entry_string( res2, "comment", &temp );
        if ( temp != NULL ) {
            strncpy( ((struct information *)ptr)->xmms2.comment, temp, TEXT_BUFFER_SIZE - 1 );
        } else {
            strncpy( ((struct information *)ptr)->xmms2.comment, "", TEXT_BUFFER_SIZE - 1 );
        }


        xmmsc_result_get_dict_entry_string( res2, "decoder", &temp );
        if ( temp != NULL ) {
            strncpy( ((struct information *)ptr)->xmms2.decoder, temp, TEXT_BUFFER_SIZE - 1 );
        } else {
            strncpy( ((struct information *)ptr)->xmms2.decoder, "[Unknown]", TEXT_BUFFER_SIZE - 1 );
        }


        xmmsc_result_get_dict_entry_string( res2, "transport", &temp );
        if ( temp != NULL ) {
            strncpy( ((struct information *)ptr)->xmms2.transport, temp, TEXT_BUFFER_SIZE - 1 );
        } else {
            strncpy( ((struct information *)ptr)->xmms2.transport, "[Unknown]", TEXT_BUFFER_SIZE - 1 );
        }


        xmmsc_result_get_dict_entry_string( res2, "url", &temp );
        if ( temp != NULL ) {
            strncpy( ((struct information *)ptr)->xmms2.url, temp, TEXT_BUFFER_SIZE - 1 );
        } else {
            strncpy( ((struct information *)ptr)->xmms2.url, "[Unknown]", TEXT_BUFFER_SIZE - 1 );
        }


        xmmsc_result_get_dict_entry_string( res2, "date", &temp );
        if ( temp != NULL ) {
            strncpy( ((struct information *)ptr)->xmms2.date, temp, TEXT_BUFFER_SIZE - 1 );
        } else {
            strncpy( ((struct information *)ptr)->xmms2.date, "????", TEXT_BUFFER_SIZE - 1 );
        }


        int itemp;
        xmmsc_result_get_dict_entry_int( res2, "tracknr", &itemp );
        ((struct information *)ptr)->xmms2.tracknr = itemp;

        xmmsc_result_get_dict_entry_int( res2, "duration", &itemp );
        ((struct information *)ptr)->xmms2.duration = itemp;

        xmmsc_result_get_dict_entry_int( res2, "bitrate", &itemp );
        ((struct information *)ptr)->xmms2.bitrate = itemp / 1000;

        xmmsc_result_get_dict_entry_int( res2, "size", &itemp );
        ((struct information *)ptr)->xmms2.size = (float)itemp / 1048576;

        xmmsc_result_unref( res2 );
    }
}

void handle_playtime( xmmsc_result_t *res, void *ptr ) {
    xmmsc_result_t * res2;
    uint play_time;

    if ( xmmsc_result_iserror( res ) )
        return;

    if ( !xmmsc_result_get_uint( res, &play_time ) )
        return;

    res2 = xmmsc_result_restart( res );
    xmmsc_result_unref( res2 );

    ((struct information *)ptr)->xmms2.elapsed = play_time;
    ((struct information *)ptr)->xmms2.progress = (float) play_time / ((struct information *)ptr)->xmms2.duration;
}

void handle_playback_state_change( xmmsc_result_t *res, void *ptr ) {
    uint pb_state = 0;
    if ( xmmsc_result_iserror( res ) )
        return;

    if ( !xmmsc_result_get_uint( res, &pb_state ) )
        return;

    if ( ((struct information *)ptr)->xmms2.status == NULL )
        ((struct information *)ptr)->xmms2.status = malloc( TEXT_BUFFER_SIZE );

    switch (pb_state) {
    case XMMS_PLAYBACK_STATUS_PLAY:
        strncpy( ((struct information *)ptr)->xmms2.status,
                 "Playing", TEXT_BUFFER_SIZE - 1 );
        break;
    case XMMS_PLAYBACK_STATUS_PAUSE:
        strncpy( ((struct information *)ptr)->xmms2.status,
                 "Paused", TEXT_BUFFER_SIZE - 1 );
        break;
    case XMMS_PLAYBACK_STATUS_STOP:
        strncpy( ((struct information *)ptr)->xmms2.status,
                 "Stopped", TEXT_BUFFER_SIZE - 1 );
        break;
    default:
        strncpy( ((struct information *)ptr)->xmms2.status,
                 "Unknown", TEXT_BUFFER_SIZE - 1 );
    }
}


void update_xmms2() {
    struct information * current_info = &info;

    /* initialize connection */
    if ( current_info->xmms2_conn_state == CONN_INIT ) {

        if ( current_info->xmms2_conn == NULL ) {
            current_info->xmms2_conn = xmmsc_init( "conky" );
        }

        /* did init fail? */
        if ( current_info->xmms2_conn == NULL ) {
            fprintf(stderr,"Conky: xmms2 init failed. %s\n", xmmsc_get_last_error ( current_info->xmms2_conn ));
            fflush(stderr);
            return;
        }

        /* init ok but not connected yet.. */
        current_info->xmms2_conn_state = CONN_NO;

        /* clear all values */
        if ( current_info->xmms2.artist == NULL )
            current_info->xmms2.artist = malloc( TEXT_BUFFER_SIZE );
        strncpy( current_info->xmms2.artist, "", TEXT_BUFFER_SIZE - 1 );

        if ( current_info->xmms2.album == NULL )
            current_info->xmms2.album = malloc( TEXT_BUFFER_SIZE );
        strncpy( current_info->xmms2.album, "", TEXT_BUFFER_SIZE - 1 );

        if ( current_info->xmms2.title == NULL )
            current_info->xmms2.title = malloc( TEXT_BUFFER_SIZE );
        strncpy( current_info->xmms2.title, "", TEXT_BUFFER_SIZE - 1 );

        if ( current_info->xmms2.genre == NULL )
            current_info->xmms2.genre = malloc( TEXT_BUFFER_SIZE );
        strncpy( current_info->xmms2.genre, "", TEXT_BUFFER_SIZE - 1 );

        if ( current_info->xmms2.comment == NULL )
            current_info->xmms2.comment = malloc( TEXT_BUFFER_SIZE );
        strncpy( current_info->xmms2.comment, "", TEXT_BUFFER_SIZE - 1 );

        if ( current_info->xmms2.decoder == NULL )
            current_info->xmms2.decoder = malloc( TEXT_BUFFER_SIZE );
        strncpy( current_info->xmms2.decoder, "", TEXT_BUFFER_SIZE - 1 );

        if ( current_info->xmms2.transport == NULL )
            current_info->xmms2.transport = malloc( TEXT_BUFFER_SIZE );
        strncpy( current_info->xmms2.transport, "", TEXT_BUFFER_SIZE - 1 );

        if ( current_info->xmms2.url == NULL )
            current_info->xmms2.url = malloc( TEXT_BUFFER_SIZE );
        strncpy( current_info->xmms2.url, "", TEXT_BUFFER_SIZE - 1 );

        if ( current_info->xmms2.date == NULL )
            current_info->xmms2.date = malloc( TEXT_BUFFER_SIZE );
        strncpy( current_info->xmms2.date, "", TEXT_BUFFER_SIZE - 1 );


        current_info->xmms2.tracknr = 0;
        current_info->xmms2.id = 0;
        current_info->xmms2.bitrate = 0;
        current_info->xmms2.duration = 0;
        current_info->xmms2.elapsed = 0;
        current_info->xmms2.size = 0;
        current_info->xmms2.progress = 0;

        /*    fprintf(stderr,"Conky: xmms2 init ok.\n");
            fflush(stderr); */
    }

    /* connect */
    if ( current_info->xmms2_conn_state == CONN_NO ) {

        char *path = getenv ( "XMMS_PATH" );
        if ( !xmmsc_connect( current_info->xmms2_conn, path ) ) {
            fprintf(stderr,"Conky: xmms2 connection failed. %s\n",
                    xmmsc_get_last_error ( current_info->xmms2_conn ));
            fflush(stderr);
            current_info->xmms2_conn_state = CONN_NO;
            return;
        }

        /* set callbacks */
        xmmsc_disconnect_callback_set( current_info->xmms2_conn, connection_lost, current_info );
        XMMS_CALLBACK_SET( current_info->xmms2_conn, xmmsc_playback_current_id, handle_curent_id, current_info );
        XMMS_CALLBACK_SET( current_info->xmms2_conn, xmmsc_broadcast_playback_current_id, handle_curent_id, current_info );
        XMMS_CALLBACK_SET( current_info->xmms2_conn, xmmsc_signal_playback_playtime, handle_playtime, current_info );
        XMMS_CALLBACK_SET( current_info->xmms2_conn, xmmsc_broadcast_playback_status, handle_playback_state_change, current_info );

	/* get playback status, it wont be broadcasted untill it chages */
	xmmsc_result_t * res = xmmsc_playback_status( current_info->xmms2_conn );
        xmmsc_result_wait ( res );
        unsigned int pb_state;

        if ( current_info->xmms2.status == NULL )
            current_info->xmms2.status = malloc( TEXT_BUFFER_SIZE );

        xmmsc_result_get_uint( res, &pb_state );
        switch (pb_state) {
        case XMMS_PLAYBACK_STATUS_PLAY:
            strncpy( current_info->xmms2.status,
                     "Playing", TEXT_BUFFER_SIZE - 1 );
            break;
        case XMMS_PLAYBACK_STATUS_PAUSE:
            strncpy( current_info->xmms2.status,
                     "Paused", TEXT_BUFFER_SIZE - 1 );
            break;
        case XMMS_PLAYBACK_STATUS_STOP:
            strncpy( current_info->xmms2.status,
                     "Stopped", TEXT_BUFFER_SIZE - 1 );
            break;
        default:
            strncpy( current_info->xmms2.status,
                     "Unknown", TEXT_BUFFER_SIZE - 1 );
        }
        xmmsc_result_unref ( res );
	
        /* everything seems to be ok */
        current_info->xmms2_conn_state = CONN_OK;

        /*   fprintf(stderr,"Conky: xmms2 connected.\n");
              fflush(stderr);  */
    }


    /* handle callbacks */
    if ( current_info->xmms2_conn_state == CONN_OK ) {
        struct timeval tmout;
        tmout.tv_sec = 0;
        tmout.tv_usec = 100;

        select( current_info->xmms2_fd + 1, &current_info->xmms2_fdset, NULL, NULL, &tmout );

        xmmsc_io_in_handle(current_info->xmms2_conn);
        if (xmmsc_io_want_out(current_info->xmms2_conn)) {
            xmmsc_io_out_handle(current_info->xmms2_conn);
        }
    }
}
