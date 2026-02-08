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

// Wide char and 64-bit types (MSVC uses __int64; use standard types on Linux)
typedef wchar_t WCHAR;
#ifndef __int64
typedef long long __int64;
#endif
typedef unsigned long long ntl_uint64;

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
#ifndef TEXT
#define TEXT(x) x
#endif
#ifndef _T
#define _T(x) x
#endif

#endif // _WIN32
