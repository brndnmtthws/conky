/* Substitute for and wrapper around <unistd.h>.
   Copyright (C) 2004-2007 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _GL_UNISTD_H

/* The include_next requires a split double-inclusion guard.  */
#if @HAVE_UNISTD_H@
# @INCLUDE_NEXT@ @NEXT_UNISTD_H@
#endif

#ifndef _GL_UNISTD_H
#define _GL_UNISTD_H

/* mingw doesn't define the SEEK_* macros in <unistd.h>.  */
#if !(defined SEEK_CUR && defined SEEK_END && defined SEEK_SET)
# include <stdio.h>
#endif

/* mingw fails to declare _exit in <unistd.h>.  */
#include <stdlib.h>

/* The definition of GL_LINK_WARNING is copied here.  */


/* Declare overridden functions.  */

#ifdef __cplusplus
extern "C" {
#endif


#if @GNULIB_CHOWN@
# if @REPLACE_CHOWN@
#  ifndef REPLACE_CHOWN
#   define REPLACE_CHOWN 1
#  endif
#  if REPLACE_CHOWN
/* Change the owner of FILE to UID (if UID is not -1) and the group of FILE
   to GID (if GID is not -1).  Follow symbolic links.
   Return 0 if successful, otherwise -1 and errno set.
   See the POSIX:2001 specification
   <http://www.opengroup.org/susv3xsh/chown.html>.  */
#   define chown rpl_chown
extern int chown (const char *file, uid_t uid, gid_t gid);
#  endif
# endif
#elif defined GNULIB_POSIXCHECK
# undef chown
# define chown(f,u,g) \
    (GL_LINK_WARNING ("chown fails to follow symlinks on some systems and " \
                      "doesn't treat a uid or gid of -1 on some systems - " \
                      "use gnulib module chown for portability"), \
     chown (f, u, g))
#endif


#if @GNULIB_DUP2@
# if !@HAVE_DUP2@
/* Copy the file descriptor OLDFD into file descriptor NEWFD.  Do nothing if
   NEWFD = OLDFD, otherwise close NEWFD first if it is open.
   Return 0 if successful, otherwise -1 and errno set.
   See the POSIX:2001 specification
   <http://www.opengroup.org/susv3xsh/dup2.html>.  */
extern int dup2 (int oldfd, int newfd);
# endif
#elif defined GNULIB_POSIXCHECK
# undef dup2
# define dup2(o,n) \
    (GL_LINK_WARNING ("dup2 is unportable - " \
                      "use gnulib module dup2 for portability"), \
     dup2 (o, n))
#endif


#if @GNULIB_FCHDIR@
# if @REPLACE_FCHDIR@

/* Change the process' current working directory to the directory on which
   the given file descriptor is open.
   Return 0 if successful, otherwise -1 and errno set.
   See the POSIX:2001 specification
   <http://www.opengroup.org/susv3xsh/fchdir.html>.  */
extern int fchdir (int /*fd*/);

#  define close rpl_close
extern int close (int);
#  define dup rpl_dup
extern int dup (int);
#  define dup2 rpl_dup2
extern int dup2 (int, int);

# endif
#elif defined GNULIB_POSIXCHECK
# undef fchdir
# define fchdir(f) \
    (GL_LINK_WARNING ("fchdir is unportable - " \
                      "use gnulib module fchdir for portability"), \
     fchdir (f))
#endif


#if @GNULIB_FTRUNCATE@
# if !@HAVE_FTRUNCATE@
/* Change the size of the file to which FD is opened to become equal to LENGTH.
   Return 0 if successful, otherwise -1 and errno set.
   See the POSIX:2001 specification
   <http://www.opengroup.org/susv3xsh/ftruncate.html>.  */
extern int ftruncate (int fd, off_t length);
# endif
#elif defined GNULIB_POSIXCHECK
# undef ftruncate
# define ftruncate(f,l) \
    (GL_LINK_WARNING ("ftruncate is unportable - " \
                      "use gnulib module ftruncate for portability"), \
     ftruncate (f, l))
#endif


#if @GNULIB_GETCWD@
/* Include the headers that might declare getcwd so that they will not
   cause confusion if included after this file.  */
# include <stdlib.h>
# if @REPLACE_GETCWD@
/* Get the name of the current working directory, and put it in SIZE bytes
   of BUF.
   Return BUF if successful, or NULL if the directory couldn't be determined
   or SIZE was too small.
   See the POSIX:2001 specification
   <http://www.opengroup.org/susv3xsh/getcwd.html>.
   Additionally, the gnulib module 'getcwd' guarantees the following GNU
   extension: If BUF is NULL, an array is allocated with 'malloc'; the array
   is SIZE bytes long, unless SIZE == 0, in which case it is as big as
   necessary.  */
#  define getcwd rpl_getcwd
extern char * getcwd (char *buf, size_t size);
# endif
#elif defined GNULIB_POSIXCHECK
# undef getcwd
# define getcwd(b,s) \
    (GL_LINK_WARNING ("getcwd is unportable - " \
                      "use gnulib module getcwd for portability"), \
     getcwd (b, s))
#endif


#if @GNULIB_GETLOGIN_R@
/* Copies the user's login name to NAME.
   The array pointed to by NAME has room for SIZE bytes.

   Returns 0 if successful.  Upon error, an error number is returned, or -1 in
   the case that the login name cannot be found but no specific error is
   provided (this case is hopefully rare but is left open by the POSIX spec).

   See <http://www.opengroup.org/susv3xsh/getlogin.html>.
 */
# if !@HAVE_DECL_GETLOGIN_R@
#  include <stddef.h>
extern int getlogin_r (char *name, size_t size);
# endif
#elif defined GNULIB_POSIXCHECK
# undef getlogin_r
# define getlogin_r(n,s) \
    (GL_LINK_WARNING ("getlogin_r is unportable - " \
                      "use gnulib module getlogin_r for portability"), \
     getlogin_r (n, s))
#endif


#if @GNULIB_LCHOWN@
# if @REPLACE_LCHOWN@
/* Change the owner of FILE to UID (if UID is not -1) and the group of FILE
   to GID (if GID is not -1).  Do not follow symbolic links.
   Return 0 if successful, otherwise -1 and errno set.
   See the POSIX:2001 specification
   <http://www.opengroup.org/susv3xsh/lchown.html>.  */
#  define lchown rpl_lchown
extern int lchown (char const *file, uid_t owner, gid_t group);
# endif
#elif defined GNULIB_POSIXCHECK
# undef lchown
# define lchown(f,u,g) \
    (GL_LINK_WARNING ("lchown is unportable to pre-POSIX.1-2001 " \
                      "systems - use gnulib module lchown for portability"), \
     lchown (f, u, g))
#endif


#if @GNULIB_LSEEK@
# if @REPLACE_LSEEK@
/* Set the offset of FD relative to SEEK_SET, SEEK_CUR, or SEEK_END.
   Return the new offset if successful, otherwise -1 and errno set.
   See the POSIX:2001 specification
   <http://www.opengroup.org/susv3xsh/lseek.html>.  */
#  define lseek rpl_lseek
   extern off_t lseek (int fd, off_t offset, int whence);
# endif
#elif defined GNULIB_POSIXCHECK
# undef lseek
# define lseek(f,o,w) \
    (GL_LINK_WARNING ("lseek does not fail with ESPIPE on pipes on some " \
                      "systems - use gnulib module lseek for portability"), \
     lseek (f, o, w))
#endif


#if @GNULIB_READLINK@
/* Read the contents of the symbolic link FILE and place the first BUFSIZE
   bytes of it into BUF.  Return the number of bytes placed into BUF if
   successful, otherwise -1 and errno set.
   See the POSIX:2001 specification
   <http://www.opengroup.org/susv3xsh/readlink.html>.  */
# if !@HAVE_READLINK@
#  include <stddef.h>
extern int readlink (const char *file, char *buf, size_t bufsize);
# endif
#elif defined GNULIB_POSIXCHECK
# undef readlink
# define readlink(f,b,s) \
    (GL_LINK_WARNING ("readlink is unportable - " \
                      "use gnulib module readlink for portability"), \
     readlink (f, b, s))
#endif


#if @GNULIB_SLEEP@
/* Pause the execution of the current thread for N seconds.
   Returns the number of seconds left to sleep.
   See the POSIX:2001 specification
   <http://www.opengroup.org/susv3xsh/sleep.html>.  */
# if !@HAVE_SLEEP@
extern unsigned int sleep (unsigned int n);
# endif
#elif defined GNULIB_POSIXCHECK
# undef sleep
# define sleep(n) \
    (GL_LINK_WARNING ("sleep is unportable - " \
                      "use gnulib module sleep for portability"), \
     sleep (n))
#endif


#ifdef __cplusplus
}
#endif


#endif /* _GL_UNISTD_H */
#endif /* _GL_UNISTD_H */
