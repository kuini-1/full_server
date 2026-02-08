#pragma once
// WARNING :
// This header file is assumed to be used only for Win32 or Win64.
// It has no responsibility for any problems which occurs on another platforms.
// - YOSHIKI

#ifdef _WIN32
#include "Util/NtlPortable.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#if defined(_MSC_VER)
#pragma warning(disable:4819) // vs2005 codepage bug disable
#endif

typedef unsigned __int64 ntl_uint64;

/* Path separator for format strings (e.g. "%s" NTL_PATH_SEP "%04d") */
#define NTL_PATH_SEP "\\"

//typedef char Char;
//typedef unsigned char Byte;

//typedef wchar_t WChar;

//typedef signed __int8 Int8;
//typedef unsigned __int8 UInt8;

//typedef signed __int16 Int16;
//typedef unsigned __int16 UInt16;

//typedef signed __int32 Int32;
//typedef unsigned __int32 UInt32;

//typedef signed __int64 Int64;
//typedef unsigned __int64 UInt64;

#else
// Linux / POSIX: do not include Windows headers.
#include "Util/NtlPortable.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <pthread.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <cstdio>

#ifndef _MAX_DIR
#define _MAX_DIR 256
#endif
#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#ifndef HFILE
typedef int HFILE;
#endif
#ifndef HFILE_ERROR
#define HFILE_ERROR (-1)
#endif
#ifndef _O_CREAT
#define _O_CREAT O_CREAT
#endif
#ifndef _O_APPEND
#define _O_APPEND O_APPEND
#endif
#ifndef _O_RDWR
#define _O_RDWR O_RDWR
#endif
#ifndef _O_RDONLY
#define _O_RDONLY O_RDONLY
#endif
#ifndef _O_WRONLY
#define _O_WRONLY O_WRONLY
#endif
#ifndef _SH_DENYNO
#define _SH_DENYNO 0
#endif
#ifndef _S_IREAD
#define _S_IREAD S_IRUSR
#endif
#ifndef _S_IWRITE
#define _S_IWRITE S_IWUSR
#endif

// Socket type mapping (Winsock -> POSIX)
typedef int SOCKET;
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif

// Minimal types required by NtlSocket.h and other includers
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
#ifndef BOOL
#define BOOL int
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
typedef void* HANDLE;
typedef void* LPVOID;
typedef unsigned char* LPBYTE;

typedef char CHAR;
typedef int INT;
typedef void VOID;
typedef long LONG;

// Wide char and fixed-width types (MSVC __int8/16/32/64; use standard types on Linux)
typedef wchar_t WCHAR;
#ifndef __int8
typedef signed char __int8;
#endif
#ifndef __int16
typedef short __int16;
#endif
#ifndef __int32
typedef int __int32;
#endif
#ifndef __int64
typedef long long __int64;
#endif
typedef unsigned long long ntl_uint64;
typedef unsigned long long QWORD;
typedef QWORD DWORDLONG;

/* LARGE_INTEGER / QueryPerformanceCounter / QueryPerformanceFrequency: Win32 API; on Linux use clock_gettime */
typedef union _LARGE_INTEGER {
	long long QuadPart;
} LARGE_INTEGER;
static inline void QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	lpPerformanceCount->QuadPart = (long long)ts.tv_sec * 1000000000LL + (long long)ts.tv_nsec;
}
static inline void QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency)
{
	lpFrequency->QuadPart = 1000000000LL; /* nanoseconds per second, matches QueryPerformanceCounter */
}

/* SYSTEMTIME / GetLocalTime: Win32 API; on Linux use time + localtime_r */
typedef struct _SYSTEMTIME {
	WORD wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds;
} SYSTEMTIME;
static inline void GetLocalTime(SYSTEMTIME* pst)
{
	time_t t = time(NULL);
	struct tm tm_buf;
	struct tm* pt = localtime_r(&t, &tm_buf);
	if (pt) {
		pst->wYear = (WORD)(pt->tm_year + 1900);
		pst->wMonth = (WORD)(pt->tm_mon + 1);
		pst->wDayOfWeek = (WORD)pt->tm_wday;
		pst->wDay = (WORD)pt->tm_mday;
		pst->wHour = (WORD)pt->tm_hour;
		pst->wMinute = (WORD)pt->tm_min;
		pst->wSecond = (WORD)pt->tm_sec;
		pst->wMilliseconds = 0;
	} else {
		memset(pst, 0, sizeof(SYSTEMTIME));
	}
}

/* _mkdir: Win32; on Linux use mkdir(path, 0755) */
#define _mkdir(path) mkdir((path), 0755)

/* Path separator for format strings (e.g. "%s" NTL_PATH_SEP "%04d") */
#define NTL_PATH_SEP "/"

#include <cassert>
#define _ASSERTE(x) assert(x)

// Windows error code equivalents for compatibility
#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS 0
#endif
#ifndef ERROR_INVALID_PARAMETER
#define ERROR_INVALID_PARAMETER 87
#endif
#ifndef ERROR_NOT_READY
#define ERROR_NOT_READY 21
#endif
#ifndef ERROR_EMPTY
#define ERROR_EMPTY 0x100
#endif
#ifndef ERROR_NOT_ENOUGH_MEMORY
#define ERROR_NOT_ENOUGH_MEMORY 8
#endif
#ifndef ERROR_OUTOFMEMORY
#define ERROR_OUTOFMEMORY 14
#endif

#define ZeroMemory(ptr, size) memset((ptr), 0, (size))
#define CopyMemory(dest, src, size) memcpy((dest), (src), (size))

#ifndef _countof
#define _countof(a) (sizeof(a) / sizeof((a)[0]))
#endif

typedef struct _OVERLAPPED {
	void* Internal;
	void* InternalHigh;
	union { struct { DWORD Offset; DWORD OffsetHigh; }; void* Pointer; };
	HANDLE hEvent;
} OVERLAPPED;

typedef struct _WSABUF {
	unsigned long len;
	char* buf;
} WSABUF;

// Map Win32 error APIs to errno for compatibility
#define GetLastError() (errno)
#define SetLastError(x) (void)(errno = (x))

// closesocket on Windows; on POSIX use close()
#define closesocket close

// CRITICAL_SECTION as pthread_mutex_t for code that uses Win32 mutex API
typedef pthread_mutex_t CRITICAL_SECTION;
static inline void InitializeCriticalSection(CRITICAL_SECTION* p) { pthread_mutex_init(p, NULL); }
static inline void InitializeCriticalSectionAndSpinCount(CRITICAL_SECTION* p, DWORD dwSpinCount) { (void)dwSpinCount; pthread_mutex_init(p, NULL); }
static inline void DeleteCriticalSection(CRITICAL_SECTION* p) { pthread_mutex_destroy(p); }
static inline void EnterCriticalSection(CRITICAL_SECTION* p) { pthread_mutex_lock(p); }
static inline void LeaveCriticalSection(CRITICAL_SECTION* p) { pthread_mutex_unlock(p); }

#ifndef LPCTSTR
#define LPCTSTR const char*
#endif
#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(x) ((void)(x))
#endif
#ifndef TEXT
#define TEXT(x) x
#endif
#ifndef _T
#define _T(x) x
#endif

/* GetTickCount: milliseconds since an epoch (monotonic on Linux) */
static inline DWORD GetTickCount(void)
{
	struct timespec ts;
#ifdef CLOCK_MONOTONIC
	clock_gettime(CLOCK_MONOTONIC, &ts);
#else
	clock_gettime(CLOCK_REALTIME, &ts);
#endif
	return (DWORD)((unsigned long)ts.tv_sec * 1000UL + (unsigned long)ts.tv_nsec / 1000000UL);
}

/* FindFirstFile / FindNextFile / FindClose compatibility (same API as Win32) */
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#endif
#ifndef FILE_ATTRIBUTE_DIRECTORY
#define FILE_ATTRIBUTE_DIRECTORY 0x10
#endif

typedef struct _WIN32_FIND_DATAA_LINUX {
	DWORD dwFileAttributes;
	char  cFileName[260];
	/* padding for minimal compatibility; Win32 struct has more fields */
} WIN32_FIND_DATAA_LINUX;
#define WIN32_FIND_DATA WIN32_FIND_DATAA_LINUX

static inline HANDLE FindFirstFile(const char* lpFileName, WIN32_FIND_DATAA_LINUX* lpFindFileData)
{
	char dirpath[260];
	size_t len = strlen(lpFileName);
	if (len >= 3 && strcmp(lpFileName + len - 3, "*.*") == 0)
		len -= 3;
	else if (len >= 1 && lpFileName[len - 1] == '*')
		while (len > 0 && lpFileName[len - 1] != '/' && lpFileName[len - 1] != '\\') len--;
	if (len >= sizeof(dirpath)) return INVALID_HANDLE_VALUE;
	memcpy(dirpath, lpFileName, len);
	dirpath[len] = '\0';

	DIR* d = opendir(dirpath);
	if (!d) return INVALID_HANDLE_VALUE;

	struct dirent* ent = readdir(d);
	if (!ent) { closedir(d); return INVALID_HANDLE_VALUE; }

	lpFindFileData->dwFileAttributes = 0;
	if (ent->d_type == DT_DIR) lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
	else if (ent->d_type == DT_UNKNOWN) {
		/* Buffer sized so dirpath + "/" + d_name (up to 255) + NUL fits; avoids -Wformat-truncation */
		char full[MAX_PATH + 260 + 4];
		snprintf(full, sizeof(full), "%.*s/%.*s", (int)(sizeof(dirpath) - 1), dirpath, 255, ent->d_name);
		struct stat st;
		if (stat(full, &st) == 0 && S_ISDIR(st.st_mode))
			lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
	}
	strncpy(lpFindFileData->cFileName, ent->d_name, 259);
	lpFindFileData->cFileName[259] = '\0';

	return (HANDLE)d;
}

static inline int FindNextFile(HANDLE hFindFile, WIN32_FIND_DATAA_LINUX* lpFindFileData)
{
	DIR* d = (DIR*)hFindFile;
	struct dirent* ent = readdir(d);
	if (!ent) return 0;

	lpFindFileData->dwFileAttributes = 0;
	if (ent->d_type == DT_DIR) lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
	else if (ent->d_type == DT_UNKNOWN) {
		/* optional: could get dir path from somewhere; skip for simplicity */
	}
	strncpy(lpFindFileData->cFileName, ent->d_name, 259);
	lpFindFileData->cFileName[259] = '\0';

	return 1;
}

static inline void FindClose(HANDLE hFindFile)
{
	if (hFindFile != INVALID_HANDLE_VALUE)
		closedir((DIR*)hFindFile);
}

/* CreateDirectory: Win32 API; on Linux use mkdir (single directory, no parents) */
static inline BOOL CreateDirectory(const char* path, void* unused)
{
	(void)unused;
	return mkdir(path, 0755) == 0 ? TRUE : FALSE;
}

#endif // _WIN32
