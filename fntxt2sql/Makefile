# Makefile for 'Finance System'
#
# Copyright (C) 1999-2018 by Stefan Schallenberg
#


# Standard Configuration (may be overwritten by command-line args to make
# e.g. for Release use: make DEBUG=0
DEBUG=1
CONF_MYSQL=1
CONF_HBCIPX=1

#
# End of user serviceable parts.  From here on anything is my fault
#

#
# Where executables/manpage should be installed
#
BINDIR  = /home/finance/bin

#
# Standard compiler settings
#
CC=gcc
CFLAGS=-pipe -Wall

ifneq ($(CONF_MYSQL),0)
CFLAGS+=-DCONF_MYSQL
LIBS+=-lmysqlclient
endif

ifneq ($(CONF_HBCIPX),0)
CFLAGS+=-DCONF_HBCIPX
LIBS+=-lpx
#LIBS+=/usr/local/lib/libpx.so
endif

# Debugging/profiling version?
ifneq ($(DEBUG), 0)
  # Flags for developer's version
  CFLAGS+=-g -pg -DDEBUG
  LDFLAGS+=-pg
else
  # Flags for production version
  CFLAGS+=-O
  LDFLAGS+=-s
endif

##### Use Math library for floor function!
LIBS+=-lm

export CC
export CPPFLAGS
export LDFLAGS
export CFLAGS

all: fntxt2sql

fntxt2sql: fntxt2sql.o \
	       fntxt2sql-btx.o fntxt2sql-csv.o fntxt2sql-aqm.o \
	       fntxt2sql-aqb-tran.o fntxt2sql-aqb-bal.o \
	       fntxt2sql-hbci.o \
	       fntxt2sql-util.o fntxt2sql-mysql.o fnSql.o
	$(LINK.o) -o $@ $(LIBS) $^  

fntxt2sql.o: fntxt2sql.c fntxt2sql.h
	$(CC) $(CFLAGS) -DVERSION="\"$(VERSION)\"" -o $@ -c fntxt2sql.c

fntxt2sql-btx.o: fntxt2sql-btx.c fntxt2sql.h
	$(CC) $(CFLAGS) -DVERSION="\"$(VERSION)\"" -o $@ -c fntxt2sql-btx.c

fntxt2sql-csv.o: fntxt2sql-csv.c fntxt2sql.h
	$(CC) $(CFLAGS) -DVERSION="\"$(VERSION)\"" -o $@ -c fntxt2sql-csv.c

fntxt2sql-aqm.o: fntxt2sql-aqm.c fntxt2sql.h
	$(CC) $(CFLAGS) -DVERSION="\"$(VERSION)\"" -o $@ -c fntxt2sql-aqm.c

fntxt2sql-aqb-tran.o: fntxt2sql-aqb-tran.c fntxt2sql.h
	$(CC) $(CFLAGS) -DVERSION="\"$(VERSION)\"" -o $@ -c fntxt2sql-aqb-tran.c

fntxt2sql-aqb-bal.o: fntxt2sql-aqb-bal.c fntxt2sql.h
	$(CC) $(CFLAGS) -DVERSION="\"$(VERSION)\"" -o $@ -c fntxt2sql-aqb-bal.c

fntxt2sql-hbci.o: fntxt2sql-hbci.c fntxt2sql.h
	$(CC) $(CFLAGS) -DVERSION="\"$(VERSION)\"" -o $@ -c fntxt2sql-hbci.c

fntxt2sql-mysql.o: fntxt2sql-mysql.c fntxt2sql.h
	$(CC) $(CFLAGS) -DVERSION="\"$(VERSION)\"" -o $@ -c fntxt2sql-mysql.c

fntxt2sql-util.o: fntxt2sql-util.c fntxt2sql.h
	$(CC) $(CFLAGS) -DVERSION="\"$(VERSION)\"" -o $@ -c fntxt2sql-util.c

fnSql.o: fnSql.c fntxt2sql.h
	$(CC) $(CFLAGS) -DVERSION="\"$(VERSION)\"" -o $@ -c fnSql.c

.c.o:
	$(CC) $(CFLAGS) -DVERSION="\"$(VERSION)\"" -o $@ -c $^

install: fntxt2sql finance 
	install -o finance -m 755 -d $(BINDIR)
	install -o finance -m 755 cron.finance   $(BINDIR)
	install -o finance -m 755 mailSilvi.sh   $(BINDIR)
	install -o finance -m 755 finance        $(BINDIR)
	install -o finance -m 755 fntxt2sql      $(BINDIR)

clean:
	-rm -f *.o fntxt2sql 

