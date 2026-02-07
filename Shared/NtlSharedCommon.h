#pragma once
// WARNING :
// This header file is assumed to be used only for Win32 or Win64.
// It has no responsibility for any problems which occurs on another platforms.
// - YOSHIKI

#ifdef _WIN32
#include <ws2tcpip.h> //includes winsock2.h
#include <windows.h>

#if defined(_MSC_VER)
#pragma warning(disable:4819) // vs2005 codepage bug disable
#endif

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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>

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

#endif // _WIN32
