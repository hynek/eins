bin_PROGRAMS = eins
eins_SOURCES = eins.c log.c log.h measure.c mod_tcp.c mod_udp.c util.c	\
		util_ip.c
eins_LDADD = -lm

if WITH_BMI
eins_SOURCES += mod_bmi.c
eins_LDADD += -lpthread -lpvfs2
endif