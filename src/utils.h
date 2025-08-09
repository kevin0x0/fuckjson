#ifndef _UTILS_H
#define _UTILS_H

#if defined(__GNUC__) || defined(__clang__)
#define likely(expr)  __builtin_expect(!!(expr), 1)
#define unlikely(expr)  __builtin_expect(!!(expr), 0)
#else
#define likely(expr)
#define unlikely(expr)
#endif

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define max(a, b) ((a) < (b) ? (b) : (a))
#define min(a, b) ((a) > (b) ? (b) : (a))

#endif

