#pragma once

#if !defined(_WIN32)

#include <cstdio>
#include <cstring>
#include <cwchar>
#include <strings.h>
#include <iconv.h>
#include <cstdarg>
#include <string>

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

/* wcscpy_s(dest, size, src) -> manual WCHAR copy (WCHAR is unsigned short, not wchar_t on Linux) */
#define NTL_WCSCPY_S(dest, size, src) do { \
    if (src) { \
        size_t _max = (size_t)(size) - 1; \
        size_t _i = 0; \
        while (_i < _max && (src)[_i] != 0) { \
            (dest)[_i] = (src)[_i]; \
            _i++; \
        } \
        (dest)[_i] = 0; \
    } else { \
        (dest)[0] = 0; \
    } \
} while(0)

/* wcsncpy_s(dest, size, src, count) -> manual WCHAR copy */
#define NTL_WCSNCPY_S(dest, size, src, count) do { \
    size_t _n = (size_t)(count); \
    if (_n >= (size_t)(size)) _n = (size_t)(size) - 1; \
    if (src) { \
        size_t _i = 0; \
        while (_i < _n && (src)[_i] != 0) { \
            (dest)[_i] = (src)[_i]; \
            _i++; \
        } \
        (dest)[_i] = 0; \
    } else { \
        (dest)[0] = 0; \
    } \
} while(0)

/* strcpy_s(dest, size, src) */
#define NTL_STRCPY_S(dest, size, src) NTL_STRNCPY_S((dest), (size), (src), (size))

/* strtok_s(str, delim, ctx) -> strtok_r */
#define NTL_STRTOK(str, delim, ctx)  strtok_r((str), (delim), (ctx))

/* WCHARLen: Calculate length of WCHAR* string (WCHAR is unsigned short, not wchar_t on Linux) */
static inline size_t WCHARLen(const WCHAR* str) {
    if (!str) return 0;
    size_t len = 0;
    while (str[len] != 0)
        len++;
    return len;
}

/* wcscmp for WCHAR*: Compare two WCHAR strings */
static inline int WCHARCmp(const WCHAR* s1, const WCHAR* s2) {
    if (!s1) return s2 ? -1 : 0;
    if (!s2) return 1;
    while (*s1 && *s2) {
        if (*s1 != *s2) return (*s1 < *s2) ? -1 : 1;
        s1++;
        s2++;
    }
    if (*s1) return 1;
    if (*s2) return -1;
    return 0;
}

/* wcsncmp for WCHAR*: Compare first n characters of two WCHAR strings */
static inline int WCHARNCmp(const WCHAR* s1, const WCHAR* s2, size_t n) {
    if (!s1) return s2 ? -1 : 0;
    if (!s2) return 1;
    if (n == 0) return 0;
    size_t i = 0;
    while (i < n && s1[i] && s2[i]) {
        if (s1[i] != s2[i]) return (s1[i] < s2[i]) ? -1 : 1;
        i++;
    }
    if (i == n) return 0;
    if (!s1[i]) return s2[i] ? -1 : 0;
    if (!s2[i]) return 1;
    return 0;
}

/* _wtoi64 for WCHAR*: Convert WCHAR string to long long (__int64 equivalent) */
static inline long long _wtoi64_WCHAR(const WCHAR* s) {
    if (!s) return 0;
#if defined(_WIN32)
    // On Windows, WCHAR == wchar_t, so direct cast works
    return (long long)wcstoll((const wchar_t*)s, NULL, 10);
#else
    // On Linux, convert WCHAR* (UTF-16LE) to wchar_t* (UTF-32) for wcstoll
    // For simple ASCII numbers, direct character conversion works
    wchar_t buf[64];
    size_t i = 0;
    while (s[i] != 0 && i < 63 && s[i] < 128) {
        buf[i] = (wchar_t)s[i];
        i++;
    }
    buf[i] = L'\0';
    return (long long)wcstoll(buf, NULL, 10);
#endif
}

/* Helper function to convert wchar_t* literal (L"") to WCHAR* at runtime */
static inline WCHAR* WCharTLiteralToWCHAR(const wchar_t* src, WCHAR* dest, size_t destSize) {
    if (!src || !dest || destSize == 0) {
        if (dest && destSize > 0) dest[0] = 0;
        return dest;
    }
#if defined(_WIN32)
    // On Windows, wchar_t == WCHAR, so direct copy works
    size_t len = 0;
    while (src[len] != 0 && len < destSize - 1) {
        dest[len] = (WCHAR)src[len];
        len++;
    }
    dest[len] = 0;
    return dest;
#else
    // On Linux, convert UTF-32 to UTF-16LE using iconv (already included at top)
    iconv_t cd = iconv_open("UTF-16LE", "UTF-32");
    if (cd == (iconv_t)-1) {
        // Fallback: simple ASCII conversion
        size_t len = 0;
        while (src[len] != 0 && len < destSize - 1 && src[len] < 128) {
            dest[len] = (WCHAR)src[len];
            len++;
        }
        dest[len] = 0;
        return dest;
    }
    
    size_t srcLen = 0;
    const wchar_t* p = src;
    while (*p != 0) { p++; srcLen++; }
    
    char* inbuf = (char*)src;
    char* outbuf = (char*)dest;
    size_t inbytesleft = (srcLen + 1) * sizeof(wchar_t);
    size_t outbytesleft = (destSize - 1) * sizeof(WCHAR);
    
    size_t result = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    iconv_close(cd);
    
    if (result == (size_t)-1) {
        // Fallback: simple ASCII conversion
        size_t len = 0;
        while (src[len] != 0 && len < destSize - 1 && src[len] < 128) {
            dest[len] = (WCHAR)src[len];
            len++;
        }
        dest[len] = 0;
        return dest;
    }
    
    // Ensure null termination
    if (outbytesleft >= sizeof(WCHAR)) {
        *((WCHAR*)outbuf) = 0;
    } else {
        dest[destSize - 1] = 0;
    }
    return dest;
#endif
}

/* Helper function to convert std::wstring::c_str() to WCHAR* buffer */
static inline WCHAR* WStringCStrToWCHAR(const std::wstring& wstr, WCHAR* dest, size_t destSize) {
    if (!dest || destSize == 0) return dest;
    const wchar_t* src = wstr.c_str();
    return WCharTLiteralToWCHAR(src, dest, destSize);
}

/* Helper function to compare std::wstring with WCHAR* */
static inline int WStringCmpWCHAR(const std::wstring& wstr, const WCHAR* wcharStr) {
    if (!wcharStr) return wstr.empty() ? 0 : 1;
    const wchar_t* wstr_cstr = wstr.c_str();
    WCHAR tempBuf[512];
    WCharTLiteralToWCHAR(wstr_cstr, tempBuf, sizeof(tempBuf)/sizeof(WCHAR));
    return WCHARCmp(tempBuf, wcharStr);
}

/* Helper function to compare first n chars of std::wstring with WCHAR* */
static inline int WStringNCmpWCHAR(const std::wstring& wstr, const WCHAR* wcharStr, size_t n) {
    if (!wcharStr) return wstr.empty() ? 0 : 1;
    const wchar_t* wstr_cstr = wstr.c_str();
    WCHAR tempBuf[512];
    WCharTLiteralToWCHAR(wstr_cstr, tempBuf, sizeof(tempBuf)/sizeof(WCHAR));
    return WCHARNCmp(tempBuf, wcharStr, n);
}

/* Helper function to convert WCHAR* format string to wchar_t* for swprintf/vswprintf */
static inline wchar_t* WCHARFormatToWCharT(const WCHAR* fmt) {
    if (!fmt) return NULL;
#if defined(_WIN32)
    return (wchar_t*)fmt; // On Windows, WCHAR == wchar_t
#else
    // On Linux, convert UTF-16LE to UTF-32
    static thread_local wchar_t* cached_result = NULL;
    static thread_local size_t cached_size = 0;
    
    size_t fmtLen = 0;
    const WCHAR* p = fmt;
    while (*p != 0) { p++; fmtLen++; }
    
    size_t needed_size = (fmtLen + 1) * sizeof(wchar_t);
    if (cached_size < needed_size) {
        if (cached_result) delete[] cached_result;
        cached_result = new wchar_t[fmtLen + 1];
        cached_size = needed_size;
    }
    
    iconv_t cd = iconv_open("UTF-32", "UTF-16LE");
    if (cd == (iconv_t)-1) cd = iconv_open("UTF-32LE", "UTF-16LE");
    if (cd != (iconv_t)-1) {
        size_t inbytesleft = (fmtLen + 1) * sizeof(WCHAR);
        size_t outbytesleft = (fmtLen + 1) * sizeof(wchar_t);
        char* inbuf = (char*)fmt;
        char* outbuf = (char*)cached_result;
        if (iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft) != (size_t)-1) {
            cached_result[fmtLen] = L'\0';
            iconv_close(cd);
            return cached_result;
        }
        iconv_close(cd);
    }
    
    // Fallback: simple ASCII conversion
    for (size_t i = 0; i < fmtLen; i++) {
        if (fmt[i] < 128)
            cached_result[i] = (wchar_t)fmt[i];
        else
            cached_result[i] = L'?';
    }
    cached_result[fmtLen] = L'\0';
    return cached_result;
#endif
}

/* Helper function for swprintf/vswprintf: convert wchar_t* result back to WCHAR* */
static inline void WCharTResultToWCHAR(WCHAR* dest, const wchar_t* src, size_t maxSize) {
    if (!src || !dest) return;
    size_t len = 0;
    while (len < maxSize - 1 && src[len] != L'\0') {
        if (src[len] < 0x10000) {
            dest[len] = (WCHAR)src[len];
        } else {
            dest[len] = L'?'; // Surrogate pair or out of range
        }
        len++;
    }
    dest[len] = 0;
}

/* swprintf_s(buf, size, fmt, ...) -> convert format and call swprintf */
#define NTL_SWPRINTF(buf, size, fmt, ...) do { \
    wchar_t* _wfmt = WCHARFormatToWCharT(fmt); \
    if (_wfmt) { \
        wchar_t* _wbuf = new wchar_t[(size_t)(size)]; \
        if (_wbuf) { \
            swprintf(_wbuf, (size_t)(size), _wfmt, ##__VA_ARGS__); \
            WCharTResultToWCHAR((buf), _wbuf, (size_t)(size)); \
            delete[] _wbuf; \
        } \
    } \
} while(0)

/* vswprintf_s(buf, size, fmt, args) -> convert format and call vswprintf */
#define NTL_VSWPRINTF(buf, size, fmt, args) do { \
    wchar_t* _wfmt = WCHARFormatToWCharT(fmt); \
    if (_wfmt) { \
        wchar_t* _wbuf = new wchar_t[(size_t)(size)]; \
        if (_wbuf) { \
            vswprintf(_wbuf, (size_t)(size), _wfmt, args); \
            WCharTResultToWCHAR((buf), _wbuf, (size_t)(size)); \
            delete[] _wbuf; \
        } \
    } \
} while(0)

/* strncpy_s with _TRUNCATE (4th arg) - same as NTL_STRNCPY_S_FULL */
#ifndef _TRUNCATE
#define _TRUNCATE ((size_t)-1)
#endif

/* _stricmp / NTL_STRICMP: case-insensitive string compare -> strcasecmp (POSIX) */
#define _stricmp strcasecmp
#define NTL_STRICMP(s1, s2) strcasecmp((s1), (s2))

/* _wcsicmp / NTL_WCSICMP: case-insensitive wide string compare -> manual WCHAR comparison (WCHAR is unsigned short, not wchar_t on Linux) */
#ifndef _wcsicmp
static inline int _wcsicmp_impl(const WCHAR* s1, const WCHAR* s2) {
    if (!s1) return s2 ? -1 : 0;
    if (!s2) return 1;
    while (*s1 && *s2) {
        WCHAR c1 = *s1;
        WCHAR c2 = *s2;
        // Convert to uppercase for comparison (simple ASCII case conversion)
        if (c1 >= 'a' && c1 <= 'z') c1 = c1 - 'a' + 'A';
        if (c2 >= 'a' && c2 <= 'z') c2 = c2 - 'a' + 'A';
        if (c1 != c2) return (c1 < c2) ? -1 : 1;
        s1++;
        s2++;
    }
    if (*s1) return 1;
    if (*s2) return -1;
    return 0;
}
#define _wcsicmp _wcsicmp_impl
#endif
#define NTL_WCSICMP(w1, w2) _wcsicmp((w1), (w2))

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
