#include "precomp_trigger.h"
#include "NtlTSLog.h"

#include <cstdarg>
#if !defined(_WIN32)
#include <cstdio>
#endif

/**
	Log
*/


CNtlTSLog* CNtlTSLog::s_pLog = 0;
char CNtlTSLog::s_szStrBuf[LOG_MAX_BUFFER];


void CNtlTSLog::Register( CNtlTSLog* pLog )
{
	s_pLog = pLog;
}

void CNtlTSLog::Unregister( void )
{
	s_pLog = 0;
}

void CNtlTSLog::Log( const char* pFormat, ... )
{
	va_list vaList;

	va_start( vaList, pFormat );
	NTL_VSNPRINTF( s_szStrBuf, LOG_MAX_BUFFER, pFormat, vaList );
	va_end( vaList );

	if ( s_pLog ) s_pLog->OnLogEvent( s_szStrBuf );
}

void CNtlTSLog::OnLogEvent( const char* pLog )
{

#ifndef _DEBUG
#if defined(_WIN32)
	UNREFERENCED_PARAMETER( pLog );
#else
	(void)pLog;
#endif
#endif

#if defined(_WIN32)
	_RPT0( _CRT_ERROR, pLog );
#else
	fprintf( stderr, "%s\n", pLog ? pLog : "(null)" );
#endif
}
