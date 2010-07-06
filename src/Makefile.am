bin_PROGRAMS = eins
eins_SOURCES = eins.c log.c log.h measure.c mod_tcp.c mod_udp.c	\
		mod_malloc.c mod_accept.c util.c util_ip.c
eins_LDADD = -lm -lrt

if WITH_BMI
eins_SOURCES += mod_bmi.c
eins_LDADD += -lpthread -lpvfs2
endif

if WITH_NEON
eins_SOURCES += mod_neon.c
eins_LDADD += -lneon
endif

if WITH_SCTP
eins_SOURCES += mod_sctp.c
eins_LDADD += -lsctp
endif
