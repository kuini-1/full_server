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
/* MSVC __unaligned pointer modifier: no-op on Linux (pointers are naturally unaligned-safe in this use) */
#ifndef __unaligned
#define __unaligned
#endif
/* Windows SAL annotations: no-op on Linux */
#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif
#ifndef INOUT
#define INOUT
#endif
#include "Util/NtlPortable.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
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
#include <cstdlib>
#include <cwchar>
#include <sys/time.h>

/* Time/CRT compatibility: __time32_t, errno_t, _localtime32_s */
typedef time_t __time32_t;
#ifndef _ERRNO_T_DEFINED
#define _ERRNO_T_DEFINED
typedef int errno_t;
#endif
static inline int _localtime32_s(struct tm* _tm, const __time32_t* _t) {
	return localtime_r(_t, _tm) ? 0 : (errno ? errno : -1);
}
#define localtime_s(_tm, _t) _localtime32_s((_tm), (const __time32_t*)(_t))

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
typedef const void* LPCVOID;
typedef void* PVOID;
typedef unsigned char* LPBYTE;

typedef unsigned int UINT;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;
#ifndef INT64
typedef long long INT64;
#endif
#ifndef WPARAM
typedef unsigned long WPARAM;
#endif
#ifndef LPARAM
typedef long LPARAM;
#endif

typedef char CHAR;
typedef short SHORT;
typedef int INT;
typedef void VOID;
typedef long LONG;
typedef float FLOAT;
typedef double DOUBLE;
#ifndef DWORD64
typedef unsigned long long DWORD64;
#endif
#ifndef ULONG
typedef unsigned long ULONG;
#endif
/* Windows PtrToUlong: convert pointer to ULONG (e.g. for offsetof-style member offset) */
#ifndef PtrToUlong
#define PtrToUlong(ptr) ((ULONG)(uintptr_t)(ptr))
#endif

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

#ifndef _wtoi
static inline int _wtoi(const wchar_t* s) { return (int)wcstol(s, NULL, 10); }
#endif
#ifndef _wtoi64
static inline __int64 _wtoi64(const wchar_t* s) { return (__int64)wcstoll(s, NULL, 10); }
#endif
#ifndef _wtof
static inline double _wtof(const wchar_t* s) { return wcstod(s, NULL); }
#endif
#ifndef _atoi64
static inline __int64 _atoi64(const char* s) { return (__int64)strtoll(s, NULL, 10); }
#endif

/* CPINFO / GetCPInfo: Windows codepage API; stub for Linux (XML table load is Windows-only) */
typedef struct _cpinfo_linux {
	UINT MaxCharSize;
	BYTE DefaultChar[2];
	BYTE LeadByte[12];
} CPINFO;
static inline BOOL GetCPInfo(DWORD CodePage, CPINFO* lpCPInfo) { (void)CodePage; (void)lpCPInfo; return FALSE; }

/* IsBadReadPtr: Windows API; deprecated. On Linux, assume pointer is valid. */
#define IsBadReadPtr(ptr, size) (FALSE)

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
#ifndef _ASSERT
#define _ASSERT(x) assert(x)
#endif

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
typedef OVERLAPPED* LPOVERLAPPED;

#ifndef LPTSTR
#define LPTSTR char*
#endif

#define SOCKADDR_IN struct sockaddr_in

#include <stdint.h>

static inline LONG InterlockedIncrement(volatile LONG* p) {
	return __sync_add_and_fetch(p, 1);
}
static inline LONG InterlockedDecrement(volatile LONG* p) {
	return __sync_sub_and_fetch(p, 1);
}
static inline LONG InterlockedExchangeAdd(volatile LONG* p, LONG val) {
	return __sync_fetch_and_add(p, val);
}
static inline LONG InterlockedCompareExchange(volatile LONG* p, LONG exchange, LONG comparand) {
	return __sync_val_compare_and_swap(p, comparand, exchange);
}
static inline LONG InterlockedExchange(volatile LONG* p, LONG val) {
	return __sync_lock_test_and_set(p, val);
}
#ifndef ULONG_PTR
typedef uintptr_t ULONG_PTR;
#endif
#ifndef LPDWORD
typedef DWORD* LPDWORD;
#endif
#ifndef PULONG_PTR
typedef ULONG_PTR* PULONG_PTR;
#endif
#ifndef SD_BOTH
#define SD_BOTH SHUT_RDWR
#endif
#ifndef SD_SEND
#define SD_SEND SHUT_WR
#endif

typedef struct _WSABUF {
	unsigned long len;
	char* buf;
} WSABUF;
typedef WSABUF* LPWSABUF;

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

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#endif

/* INFINITE for Wait* / GetQueuedCompletionStatus timeout (wait forever) */
#ifndef INFINITE
#define INFINITE 0xFFFFFFFFUL
#endif

/* I/O Completion Port emulation for Linux: queue + mutex + cond (used by NtlNetworkProcessor) */
typedef struct _ntl_iocp_item {
	DWORD dwNumberOfBytesTransferred;
	ULONG_PTR dwCompletionKey;
	LPOVERLAPPED lpOverlapped;
	struct _ntl_iocp_item* next;
} ntl_iocp_item;

#define NTL_IOCP_MAGIC 0x494F4350u  /* 'IOCP' */

typedef struct _ntl_iocp_linux {
	unsigned int magic;
	pthread_mutex_t mtx;
	pthread_cond_t cond;
	pthread_cond_t refcond;
	ntl_iocp_item* head;
	ntl_iocp_item* tail;
	int closed;
	int refcount;
} ntl_iocp_linux;

static inline HANDLE CreateIoCompletionPort(HANDLE FileHandle, HANDLE ExistingCompletionPort, ULONG_PTR CompletionKey, DWORD NumberOfConcurrentThreads)
{
	(void)NumberOfConcurrentThreads;
	if (FileHandle != INVALID_HANDLE_VALUE && ExistingCompletionPort != NULL) {
		/* Associate: not supported on Linux; return existing port so callers don't fail */
		return ExistingCompletionPort;
	}
	ntl_iocp_linux* p = (ntl_iocp_linux*)malloc(sizeof(ntl_iocp_linux));
	if (!p) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); return NULL; }
	p->magic = NTL_IOCP_MAGIC;
	pthread_mutex_init(&p->mtx, NULL);
	pthread_cond_init(&p->cond, NULL);
	pthread_cond_init(&p->refcond, NULL);
	p->head = NULL;
	p->tail = NULL;
	p->closed = 0;
	p->refcount = 0;
	return (HANDLE)p;
}

static inline BOOL GetQueuedCompletionStatus(HANDLE CompletionPort, LPDWORD lpNumberOfBytesTransferred, PULONG_PTR lpCompletionKey, LPOVERLAPPED* lpOverlapped, DWORD dwMilliseconds)
{
	(void)dwMilliseconds; /* INFINITE only */
	ntl_iocp_linux* p = (ntl_iocp_linux*)CompletionPort;
	if (!p || p->magic != NTL_IOCP_MAGIC) { SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }
	pthread_mutex_lock(&p->mtx);
	p->refcount++;
	while (!p->closed && !p->head)
		pthread_cond_wait(&p->cond, &p->mtx);
	if (p->closed && !p->head) {
		p->refcount--;
		if (p->refcount == 0) pthread_cond_signal(&p->refcond);
		pthread_mutex_unlock(&p->mtx);
		return FALSE;
	}
	ntl_iocp_item* it = p->head;
	p->head = it->next;
	if (!p->head) p->tail = NULL;
	if (lpNumberOfBytesTransferred) *lpNumberOfBytesTransferred = it->dwNumberOfBytesTransferred;
	if (lpCompletionKey) *lpCompletionKey = it->dwCompletionKey;
	if (lpOverlapped) *lpOverlapped = it->lpOverlapped;
	free(it);
	p->refcount--;
	if (p->refcount == 0) pthread_cond_signal(&p->refcond);
	pthread_mutex_unlock(&p->mtx);
	return TRUE;
}

static inline BOOL PostQueuedCompletionStatus(HANDLE CompletionPort, DWORD dwNumberOfBytesTransferred, ULONG_PTR dwCompletionKey, LPOVERLAPPED lpOverlapped)
{
	ntl_iocp_linux* p = (ntl_iocp_linux*)CompletionPort;
	if (!p || p->magic != NTL_IOCP_MAGIC) { SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }
	ntl_iocp_item* it = (ntl_iocp_item*)malloc(sizeof(ntl_iocp_item));
	if (!it) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); return FALSE; }
	it->dwNumberOfBytesTransferred = dwNumberOfBytesTransferred;
	it->dwCompletionKey = dwCompletionKey;
	it->lpOverlapped = lpOverlapped;
	it->next = NULL;
	pthread_mutex_lock(&p->mtx);
	if (p->closed) { pthread_mutex_unlock(&p->mtx); free(it); return FALSE; }
	if (p->tail) p->tail->next = it; else p->head = it;
	p->tail = it;
	pthread_cond_signal(&p->cond);
	pthread_mutex_unlock(&p->mtx);
	return TRUE;
}

static inline BOOL CloseHandle(HANDLE hObject)
{
	if (hObject == NULL || hObject == INVALID_HANDLE_VALUE) return TRUE;
	if ((uintptr_t)hObject < 4096) return TRUE; /* avoid treating small fd as pointer */
	ntl_iocp_linux* p = (ntl_iocp_linux*)hObject;
	if (p->magic != NTL_IOCP_MAGIC) return TRUE; /* not our IOCP */
	pthread_mutex_lock(&p->mtx);
	p->closed = 1;
	pthread_cond_broadcast(&p->cond);
	while (p->refcount > 0)
		pthread_cond_wait(&p->refcond, &p->mtx);
	ntl_iocp_item* it = p->head;
	while (it) { ntl_iocp_item* next = it->next; free(it); it = next; }
	p->head = NULL;
	p->tail = NULL;
	pthread_mutex_unlock(&p->mtx);
	pthread_mutex_destroy(&p->mtx);
	pthread_cond_destroy(&p->cond);
	pthread_cond_destroy(&p->refcond);
	p->magic = 0;
	free(p);
	return TRUE;
}

#ifndef TCHAR
typedef char TCHAR;
#endif
#ifndef LPCTSTR
#define LPCTSTR const char*
#endif
#ifndef LPCSTR
#define LPCSTR const char*
#endif
#ifndef LPSTR
#define LPSTR char*
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

/* FILETIME / GetSystemTimeAsFileTime: for code that needs time-based seeds */
typedef struct _FILETIME_LINUX {
	DWORD dwLowDateTime;
	DWORD dwHighDateTime;
} FILETIME;
static inline void GetSystemTimeAsFileTime(FILETIME* lpSystemTimeAsFileTime)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	/* Convert to 100-ns units since Unix epoch for compatibility; callers typically use dwLowDateTime for seeding */
	unsigned long long ns = (unsigned long long)tv.tv_sec * 10000000ULL + (unsigned long long)tv.tv_usec * 10ULL;
	lpSystemTimeAsFileTime->dwLowDateTime = (DWORD)(ns & 0xFFFFFFFFUL);
	lpSystemTimeAsFileTime->dwHighDateTime = (DWORD)(ns >> 32);
}

/* IsBadWritePtr: deprecated on Windows; on Linux use NULL check only */
#define IsBadWritePtr(ptr, size) ((ptr) == NULL ? 1 : 0)

/* GetACP: active code page; on Linux return 0 (use locale) */
static inline int GetACP(void) { return 0; }

/* WideCharToMultiByte / MultiByteToWideChar: minimal wrappers for wchar_t <-> char conversion */
#include <cwchar>
#include <stdlib.h>
static inline int WideCharToMultiByte_linux(int, unsigned long, const WCHAR* src, int srcLen, char* dst, int dstSize, const char*, void*)
{
	if (!src) return 0;
	size_t wlen = (srcLen < 0) ? wcslen(src) + 1 : (size_t)(srcLen + 1);
	if (dst && dstSize > 0) {
		size_t r = wcstombs(dst, src, (size_t)dstSize);
		if (r == (size_t)-1) return 0;
		return (int)r + (r > 0 && dst[r - 1] != '\0' ? 1 : 0);
	}
	/* Get required size: use upper bound wcslen*MB_CUR_MAX+1 */
	return (int)(wcslen(src) * (size_t)MB_CUR_MAX + 1);
}
static inline int MultiByteToWideChar_linux(int, unsigned long, const char* src, int srcLen, WCHAR* dst, int dstSize)
{
	if (!src) return 0;
	if (dst && dstSize > 0) {
		size_t r = mbstowcs(dst, src, (size_t)dstSize);
		if (r == (size_t)-1) return 0;
		return (int)r + (r > 0 && dst[r - 1] != L'\0' ? 1 : 0);
	}
	/* Get required size: use upper bound strlen+1 */
	return (int)(strlen(src) + 1);
}
#define WideCharToMultiByte(cp, flags, src, srcLen, dst, dstSize, def, used) WideCharToMultiByte_linux(cp, flags, src, srcLen, dst, dstSize, def, used)
#define MultiByteToWideChar(cp, flags, src, srcLen, dst, dstSize) MultiByteToWideChar_linux(cp, flags, src, srcLen, dst, dstSize)

#endif // _WIN32
