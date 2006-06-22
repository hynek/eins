/* ISO */
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

/* POSIX */
#include <unistd.h>

#include "log.h"

static FILE *f;

bool
log_open(const char *path)
{
    if (path)
	return (f = fopen(path, "a"));
    else {
	f = stderr;
	return true;
    }
}

void
le(const char *func, const int err, const char *msg)
{
    l(func, "%s: %s", msg, strerror(err));
}

void
l(const char *func, const char *format, ...)
{
    // Timestamp
    struct tm tm;
    time_t t;
    time(&t);
    localtime_r(&t, &tm);
    
    char buf[20];
    strftime(buf, 20, "%Y-%m-%d %H:%M:%S", &tm);
//    fprintf(f, "%s [%d] <%s> ", buf, getpid(), func);
    fprintf(f, "%s <%s> ", buf, func);

    // Message
    va_list arg_ptr;
    va_start(arg_ptr, format);
    vfprintf(f, format, arg_ptr);
    va_end(arg_ptr);
    fputc('\n', f);
    fflush(f);
}

void
log_close()
{
    if (f)
	fclose(f);
}
