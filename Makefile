all: TAGS eins

LIBS = -lm
INCLUDES = 

ifndef DEBUG
CFLAGS = -std=gnu99 -D_GNU_SOURCE -Os -fomit-frame-pointer -pipe -Wall $(INCLUDES)
LDFLAGS = -s
else
CFLAGS = -std=gnu99 -D_GNU_SOURCE -O0 -pipe -Wall -g $(INCLUDES)
LDFLAGS =
endif


MOD_OBJS =  ip.o tcp.o udp.o

ifndef NO_BMI
MOD_OBJS += bmi.o
LIBS += -L$(HOME)/lib -lpvfs2 -lpthread
INCLUDES += -I$(HOME)/Diplom/pvfs2-1.3.2/src/io/bmi -I$(HOME)/include
else
CFLAGS += -DNO_BMI
endif

################################################################################
# Core

CLIENT_OBJS :=  eins.o measure.o util.o $(MOD_OBJS)

eins: $(CLIENT_OBJS)
	gcc $(LDFLAGS) $(CLIENT_OBJS) $(LIBS) -o eins

eins.o: eins.c eins.h measure.h util.h

################################################################################
# Mods
MOD_HDRS := eins.h util.h measure.h
ip.o: ip.c ip.h $(MOD_HDRS)
tcp.o: tcp.c tcp.h ip.h $(MOD_HDRS)
udp.o: udp.c ip.h $(MOD_HDRS)
bmi.o: bmi.c bmi.h $(MOD_HDRS)

################################################################################
# Utils
measure.o: measure.c measure.h
util.o: util.c util.h

################################################################################
# Tags
TAGS: *.c *.h
	- etags *

################################################################################
.PHONY: clean sync
clean:
	rm -f eins *.o