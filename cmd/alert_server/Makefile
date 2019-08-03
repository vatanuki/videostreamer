PROG = nvr
OBJS = nvr.o tools.o str.o sock.o conf.o clients.o db_mysql.o jsmn.o \
	streams.o ../../videostreamer.o

CC = gcc
CFLAGS = -g -Wall -Wno-unused-function -I../../
LDFLAGS = -lavformat -lavdevice -lavcodec -lavutil -lpthread -lmysqlclient_r

HEADERS = log.h tools.h str.h sock.h conf.h defs.h clients.h structs.h \
	db_mysql.h jsmn.h streams.h ../../videostreamer.h

install: all
	install -m 755 $(PROG) /usr/sbin/$(PROG)

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
	@echo ==================== DONE ====================

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) $(PROG)

%.o: %.c
	$(CC) $(CFLAGS) -c $*.c -o $*.o

$(OBJS): $(HEADERS)
