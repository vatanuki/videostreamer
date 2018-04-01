PROG = nvr
OBJS = nvr.o tools.o str.o sock.o conf.o clients.o db_mysql.o jsmn.o \
	streams.o ../../videostreamer.o

CC = gcc -g -I/usr/include/ffmpeg -I../../
CFLAGS = -Wall -O2 -g -I/usr/include/ffmpeg -Wno-unused-function -I../../
LDFLAGS = $(CFLAGS) -lpthread -L/usr/lib64/mysql -L/usr/lib/mysql -lmysqlclient_r \
	-lavformat -lavcodec -lavdevice -lavutil

HEADERS = log.h tools.h str.h sock.h conf.h defs.h clients.h structs.h \
	db_mysql.h jsmn.h streams.h ../../videostreamer.h

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)
	@echo ==================== DONE ====================

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) $(PROG)

%.o: %.c
	$(CC) $(CFLAGS) -c $*.c -o $*.o

$(OBJS): $(HEADERS)
