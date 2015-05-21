# Autoconf script for eins

AC_PREREQ(2.59)
AC_INIT([eins], [0.1], [hs+eins@ox.cx])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR([src/eins.c])
AC_CONFIG_HEADER([src/config.h])
AM_INIT_AUTOMAKE([foreign])

# We need gcc for now
AC_PROG_CC(gcc)
CFLAGS="-std=gnu99 -Wall $CFLAGS"

# Checks for libraries.
AC_ARG_WITH(pvfs2, [AC_HELP_STRING([--with-pvfs2=DIR], [compile also the BMI module])], [ac_pvfs2_path="$withval"], [ac_pvfs2_path="no"])
if test "x$ac_pvfs2_path" != "xno"; then
   CFLAGS="-I$ac_pvfs2_path/include -I$ac_pvfs2_path/src/io/bmi -L$ac_pvfs2_path/lib -DWITH_BMI $CFLAGS"
fi
AM_CONDITIONAL(WITH_BMI, test "x$ac_pvfs2_path" != "xno")

AC_ARG_WITH(neon, [AC_HELP_STRING([--with-neon=DIR], [compile also the NEON-related modules])], [ac_neon_path="$withval"], [ac_neon_path="no"])
if test "x$ac_neon_path" != "xno"; then
   CFLAGS="-L$ac_neon_path/src -I$ac_neon_path/src/neon -DWITH_NEON $CFLAGS"
fi
AM_CONDITIONAL(WITH_NEON, test "x$ac_neon_path" != "xno")

AC_ARG_WITH(sctp, [AC_HELP_STRING([--with-sctp=DIR], [compile also the SCTP module])], [ac_sctp_path="$withval"], [ac_sctp_path="no"])
if test "x$ac_sctp_path" != "xno"; then
   CFLAGS="-I$ac_sctp_path/include -DWITH_SCTP $CFLAGS"
fi
AM_CONDITIONAL(WITH_SCTP, test "x$ac_sctp_path" != "xno")

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([limits.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([gettimeofday localtime_r memset socket strdup strerror])

# check for clock_gettime (sc)
AC_SEARCH_LIBS([clock_gettime], [rt])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
