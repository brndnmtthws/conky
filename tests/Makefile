#   --------------------------------------------------------------------
#   Philip Kovacs (kovacsp3@comcast.net) 2005
#   Make programs for testing libtcp-portmon and hash stuff up in ../src
#   Sorry no autoconf/automake for this stuff as of now.
#   --------------------------------------------------------------------
   
#CFLAGS = -g -Wall
CFLAGS = -g -Wall -DHASH_DEBUG
CC = gcc

all:			test-hash test-portmon

test-hash:		test-hash.o hash.o
			$(CC) $(CFLAGS) -o $@ test-hash.o hash.o

test-portmon:		test-portmon.o libtcp-portmon.o hash.o
			$(CC) $(CFLAGS) -o $@ test-portmon.o libtcp-portmon.o hash.o

test-hash.o:		test-hash.c hash.h
test-portmon.o:		test-portmon.c libtcp-portmon.h hash.h

hash.h:			../src/hash.h
			cp ../src/hash.h .

hash.c:			../src/hash.c
			cp ../src/hash.c .

libtcp-portmon.h:       ../src/libtcp-portmon.h
			cp ../src/libtcp-portmon.h .

libtcp-portmon.c:	../src/libtcp-portmon.c
			cp ../src/libtcp-portmon.c .
		
libtcp-portmon.o:	libtcp-portmon.c libtcp-portmon.h
hash.o:			hash.c hash.h

.c.o:		
			${CC} ${CFLAGS} -c $<

clean:		
			/bin/rm -f *.o test-portmon test-hash hash.? libtcp-portmon.?
