all: TAGS eins

ifdef STATIC
CC = diet gcc
endif

LIBS = -lm
INCLUDES = 

ifndef DEBUG
CFLAGS = -std=gnu99 -D_GNU_SOURCE -Os -fomit-frame-pointer -pipe -Wall $(INCLUDES)
LDFLAGS = -s
else
CFLAGS = -std=gnu99 -D_GNU_SOURCE -O0 -pipe -Wall -g $(INCLUDES)
LDFLAGS =
endif


MOD_OBJS = util_ip.o mod_tcp.o mod_udp.o

ifdef WITH_BMI
MOD_OBJS += mod_bmi.o
CFLAGS += -DWITH_BMI
LIBS += -L$(HOME)/lib -lpvfs2 -lpthread
INCLUDES += -I$(HOME)/Diplom/pvfs2-1.3.2/src/io/bmi -I$(HOME)/include
endif

################################################################################
# Core

OBJS :=  eins.o measure.o util.o log.o $(MOD_OBJS)

eins: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o eins
ifndef DEBUG
	strip eins
endif

eins.o: eins.c eins.h measure.h util.h modules.h

################################################################################
# Mods
MOD_HDRS := eins.h util.h measure.h mods.h modules.h
util_ip.o: util_ip.c util_ip.h $(MOD_HDRS)
mod_tcp.o: mod_tcp.c mod_tcp.h util_ip.h $(MOD_HDRS)
mod_udp.o: mod_udp.c mod_udp.h util_ip.h $(MOD_HDRS)
mod_bmi.o: mod_bmi.c mod_bmi.h $(MOD_HDRS)

################################################################################
# Utils
measure.o: measure.c measure.h
util.o: util.c util.h
log.o: log.c log.h

################################################################################
# Tags
TAGS: *.c *.h
	- etags *.[hc]

################################################################################
.PHONY: clean sync
clean:
	rm -f eins *.o