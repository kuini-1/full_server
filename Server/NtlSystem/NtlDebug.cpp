//***********************************************************************************
//
//	File		:	NtlDebug.cpp
//
//	Begin		:	2005-12-06
//
//	Copyright	:	NTL-Inc Co., Ltd
//
//	Author		:	Hyun Woo, Koo   ( zeroera@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************

#include "Stdafx.h"
#include "NtlDebug.h"
#include "NtlMutex.h"
#include <cstdarg>


#ifdef __NTL_DEBUG_PRINT__

#include <stdarg.h>
#include <string.h>
#if defined(_WIN32)
#include <tchar.h>
#else
#include <time.h>
#endif


//-----------------------------------------------------------------------------------
// static variable
//-----------------------------------------------------------------------------------
const unsigned int PRINT_BUF_SIZE	= 2048;
unsigned int s_dwCurFlag				= 0xFFFFFFFF;
FILE * s_curStream					= stderr;
//-----------------------------------------------------------------------------------


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void NtlSetPrintStream(FILE * fp)
{
	s_curStream = fp;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void NtlSetPrintFlag(unsigned int dwFlag)
{
	s_dwCurFlag = dwFlag;
}

//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void NtlDebugPrint(unsigned int dwFlag, LPCTSTR lpszText, ...)
{
	if( dwFlag & s_dwCurFlag )
	{
		char szLogBuffer[PRINT_BUF_SIZE + 1] = { 0x00, };
		int nBuffSize = (int)sizeof( szLogBuffer );
		int nWriteSize = 0;

#if defined(_WIN32)
		SYSTEMTIME	systemTime;
		GetLocalTime( &systemTime );
		nWriteSize += NTL_STPRINTF( szLogBuffer + nWriteSize, nBuffSize - nWriteSize, TEXT("[%d-%02d-%02d %d:%d:%d:%d] "), systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds );

		va_list args;
		va_start( args, lpszText );
		nWriteSize += NTL_VSTPRINTF( szLogBuffer + nWriteSize, nBuffSize - nWriteSize, lpszText, args );
		va_end( args );
#else
		time_t now = time(NULL);
		struct tm* t = localtime(&now);
		if ( t )
			nWriteSize += snprintf( szLogBuffer + nWriteSize, (size_t)(nBuffSize - nWriteSize), "[%d-%02d-%02d %d:%d:%d] ", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec );

		va_list args;
		va_start( args, lpszText );
		nWriteSize += vsnprintf( szLogBuffer + nWriteSize, (size_t)(nBuffSize - nWriteSize), lpszText, args );
		va_end( args );
#endif

		fprintf( stderr, "%s\n", szLogBuffer );
		fflush( stderr );

		if( s_curStream && s_curStream != stderr)
		{
			fprintf( s_curStream, "%s\n", szLogBuffer );
			fflush( s_curStream );
		}
	}
}


#endif // __NTL_DEBUG_PRINT__
