#ifndef LOG_H
#define LOG_H

#include <stdbool.h>

bool log_open(const char *path);
void l(const char *func, const char *format, ...);
void le(const char *func, const int err, const char *msg);
void log_close();

// Convience macros
#define L(...) l(__func__, __VA_ARGS__);
#define XL(...) do { L(__VA_ARGS__); exit(1); } while (false)
#define LE(MSG) le(__func__, errno, MSG);
#define XLE(MSG) do { LE(MSG); exit(1); } while (false)

#endif // LOG_H
