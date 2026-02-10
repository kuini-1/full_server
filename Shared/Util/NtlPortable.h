#pragma once

#if !defined(_WIN32)

#include <cstdio>
#include <cstring>
#include <cwchar>
#include <strings.h>

/* fopen_s: Windows returns 0 on success; we need same semantics */
#define NTL_FOPEN(pFile, path, mode)  (((*(pFile)) = fopen((path), (mode))) != NULL)

/* sprintf_s(buf, size, fmt, ...) -> snprintf */
#define NTL_SNPRINTF(buf, size, fmt, ...)  snprintf((buf), (size_t)(size), (fmt), ##__VA_ARGS__)

/* vsprintf_s(buf, size, fmt, args) -> vsnprintf */
#define NTL_VSNPRINTF(buf, size, fmt, args)  vsnprintf((buf), (size_t)(size), (fmt), (args))

/* _stprintf_s / _vstprintf_s: TCHAR = char on Linux */
#define NTL_STPRINTF(buf, size, fmt, ...)   snprintf((buf), (size_t)(size), (fmt), ##__VA_ARGS__)
#define NTL_VSTPRINTF(buf, size, fmt, args) vsnprintf((buf), (size_t)(size), (fmt), (args))

/* strncpy_s(dest, destSize, src, count) - count may be smaller than destSize */
#define NTL_STRNCPY_S(dest, destSize, src, count) do { \
    size_t _n = (size_t)(count); \
    if (_n >= (size_t)(destSize)) _n = (size_t)(destSize) - 1; \
    if (src) { strncpy((dest), (src), _n); (dest)[_n] = '\0'; } \
    else (dest)[0] = '\0'; \
} while(0)

/* strncpy_s(dest, destSize, src) - copy up to destSize-1 */
#define NTL_STRNCPY_S_FULL(dest, destSize, src) NTL_STRNCPY_S((dest), (destSize), (src), (destSize))

/* wcscpy_s(dest, size, src) -> wcsncpy + null */
#define NTL_WCSCPY_S(dest, size, src) do { \
    if (src) { wcsncpy((dest), (src), (size_t)(size)-1); (dest)[(size_t)(size)-1] = L'\0'; } \
    else (dest)[0] = L'\0'; \
} while(0)

/* wcsncpy_s(dest, size, src, count) */
#define NTL_WCSNCPY_S(dest, size, src, count) do { \
    size_t _n = (size_t)(count); \
    if (_n >= (size_t)(size)) _n = (size_t)(size) - 1; \
    if (src) { wcsncpy((dest), (src), _n); (dest)[_n] = L'\0'; } \
    else (dest)[0] = L'\0'; \
} while(0)

/* strcpy_s(dest, size, src) */
#define NTL_STRCPY_S(dest, size, src) NTL_STRNCPY_S((dest), (size), (src), (size))

/* strtok_s(str, delim, ctx) -> strtok_r */
#define NTL_STRTOK(str, delim, ctx)  strtok_r((str), (delim), (ctx))

/* swprintf_s(buf, size, fmt, ...) -> swprintf */
#define NTL_SWPRINTF(buf, size, fmt, ...)  swprintf((buf), (size_t)(size), (fmt), ##__VA_ARGS__)

/* vswprintf_s(buf, size, fmt, args) -> vswprintf */
#define NTL_VSWPRINTF(buf, size, fmt, args)  vswprintf((buf), (size_t)(size), (fmt), (args))

/* strncpy_s with _TRUNCATE (4th arg) - same as NTL_STRNCPY_S_FULL */
#ifndef _TRUNCATE
#define _TRUNCATE ((size_t)-1)
#endif

/* _stricmp / NTL_STRICMP: case-insensitive string compare -> strcasecmp (POSIX) */
#define _stricmp strcasecmp
#define NTL_STRICMP(s1, s2) strcasecmp((s1), (s2))

/* _wcsicmp / NTL_WCSICMP: case-insensitive wide string compare -> wcscasecmp (POSIX) */
#ifndef _wcsicmp
#define _wcsicmp wcscasecmp
#endif
#define NTL_WCSICMP(w1, w2) wcscasecmp((w1), (w2))

/* _strnicmp: case-insensitive string compare with length limit -> strncasecmp (POSIX) */
#ifndef _strnicmp
#define _strnicmp(s1, s2, n) strncasecmp((s1), (s2), (n))
#endif

/* fprintf_s: same signature as fprintf on POSIX */
#define fprintf_s fprintf

/* Raw CRT _s compatibility so code using sprintf_s/strcpy_s etc. compiles on Linux */
#ifndef sprintf_s
#define sprintf_s(buf, size, fmt, ...)       snprintf((buf), (size_t)(size), (fmt), ##__VA_ARGS__)
#endif
#ifndef vsprintf_s
#define vsprintf_s(buf, size, fmt, args)    vsnprintf((buf), (size_t)(size), (fmt), (args))
#endif
#ifndef strcpy_s
#define strcpy_s(dest, size, src)            NTL_STRCPY_S((dest), (size), (src))
#endif
#ifndef strtok_s
#define strtok_s(str, delim, ctx)            strtok_r((str), (delim), (ctx))
#endif
/* strncpy_s(dest, destSize, src, count); for 3-arg use NTL_STRNCPY_S_FULL */
#ifndef strncpy_s
#define strncpy_s(dest, destSize, src, count)  NTL_STRNCPY_S((dest), (destSize), (src), (count))
#endif

#else

#include <string.h>

/* Windows: use native functions - define as pass-through */
#define NTL_FOPEN(pFile, path, mode)           (fopen_s((pFile), (path), (mode)) == 0)
#define NTL_SNPRINTF(buf, size, fmt, ...)      sprintf_s((buf), (size), (fmt), ##__VA_ARGS__)
#define NTL_VSNPRINTF(buf, size, fmt, args)    vsprintf_s((buf), (size), (fmt), (args))
#define NTL_STPRINTF(buf, size, fmt, ...)      _stprintf_s((buf), (size), (fmt), ##__VA_ARGS__)
#define NTL_VSTPRINTF(buf, size, fmt, args)    _vstprintf_s((buf), (size), (fmt), (args))
#define NTL_STRNCPY_S(dest, destSize, src, count)  strncpy_s((dest), (destSize), (src), (count))
#define NTL_STRNCPY_S_FULL(dest, destSize, src)    strncpy_s((dest), (destSize), (src), _TRUNCATE)
#define NTL_WCSCPY_S(dest, size, src)          wcscpy_s((dest), (size), (src))
#define NTL_WCSNCPY_S(dest, size, src, count)  wcsncpy_s((dest), (size), (src), (count))
#define NTL_STRCPY_S(dest, size, src)          strcpy_s((dest), (size), (src))
#define NTL_STRTOK(str, delim, ctx)            strtok_s((str), (delim), (ctx))
#define NTL_SWPRINTF(buf, size, fmt, ...)      swprintf_s((buf), (size), (fmt), ##__VA_ARGS__)
#define NTL_VSWPRINTF(buf, size, fmt, args)    vswprintf_s((buf), (size), (fmt), (args))

/* _stricmp / NTL_STRICMP: case-insensitive string compare (CRT) */
#define NTL_STRICMP(s1, s2) _stricmp((s1), (s2))

/* _wcsicmp / NTL_WCSICMP: case-insensitive wide string compare (CRT) */
#define NTL_WCSICMP(w1, w2) _wcsicmp((w1), (w2))

#endif
