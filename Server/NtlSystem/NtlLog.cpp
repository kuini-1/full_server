//***********************************************************************************
//
//	File		:	NtlLog.cpp
//
//	Begin		:	2006-01-05
//
//	Copyright	:	�� NTL-Inc Co., Ltd
//
//	Author		:	Hyun Woo, Koo   ( zeroera@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************

#include "stdafx.h"
#include "NtlLog.h"

#include <stdio.h>
#include <stdarg.h>
#if defined(_WIN32)
#include <tchar.h>
#endif
#if !defined(_WIN32)
#include <time.h>
#endif
#include <cstdarg>


//-----------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------
LPCTSTR s_log_channel_string[ LOG_LAST ] = 
{
	TEXT( "GENERAL" ),
	TEXT( "SYSTEM" ),
	TEXT( "WARNING" ),
	TEXT( "ASSERT" ),
	TEXT( "NETWORK" ),
	TEXT( "TRAFFIC" ),
	TEXT( "LOG_USER" ),
	TEXT( "LOG_QUEST" ),
	TEXT("LOG_SCRIPT"),
	TEXT("LOG_BOTAI"),
	TEXT("LOG_HACK"),
	TEXT("LOG_RANKBATTLE"),
};
//-----------------------------------------------------------------------------------
FILE * s_log_stream					= stderr;
//-----------------------------------------------------------------------------------
const unsigned int BUFSIZE_LOG = 1024;
//-----------------------------------------------------------------------------------


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
LPCTSTR GetLogChannelString(BYTE byLogChannel)
{
	if( byLogChannel >= LOG_LAST )
	{
		return "NOT DEFINED TYPE";
	}

	return s_log_channel_string[ byLogChannel ];
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlLog::SetDefaultLogSource(DWORD dwDefaultLogSource)
{
	m_dwDefaultLogSource = dwDefaultLogSource;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
int CNtlLog::AttachLogStream(FILE * fp)
{
	s_log_stream = fp;

	return NTL_SUCCESS;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlLog::DetachLogStream()
{
	s_log_stream = NULL;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
int CNtlLog::RegisterSource(DWORD dwSource, LPCTSTR lpszSourceName)
{
	UNREFERENCED_PARAMETER( dwSource );
	UNREFERENCED_PARAMETER( lpszSourceName );

	return NTL_SUCCESS;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
int CNtlLog::RegisterChannel(DWORD dwSource, BYTE byChannel, LPCTSTR lpszChannelName, LPCTSTR lpszFileNamePrefix, LPCTSTR lpszFileNameSuffix)
{
	UNREFERENCED_PARAMETER( dwSource );
	UNREFERENCED_PARAMETER( byChannel );
	UNREFERENCED_PARAMETER( lpszChannelName );
	UNREFERENCED_PARAMETER( lpszFileNamePrefix );
	UNREFERENCED_PARAMETER( lpszFileNameSuffix );

	return NTL_SUCCESS;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
int CNtlLog::RegisterBaseChannel(DWORD dwSource, LPCTSTR lpszSourceName)
{
	RegisterChannel( dwSource,	LOG_GENERAL,	"LOG_GENERAL",	lpszSourceName,	"GENERAL" );
	RegisterChannel( dwSource,	LOG_SYSTEM,		"LOG_SYSTEM",	lpszSourceName,	"SYSTEM" );
	RegisterChannel( dwSource,	LOG_WARNING,	"LOG_WARNING",	lpszSourceName,	"WARNING" );
	RegisterChannel( dwSource,	LOG_ASSERT,		"LOG_ASSERT",	lpszSourceName,	"ASSERT" );
	RegisterChannel( dwSource,	LOG_NETWORK,	"LOG_NETWORK",	lpszSourceName,	"NETWORK" );
	RegisterChannel( dwSource,	LOG_TRAFFIC,	"LOG_TRAFFIC",	lpszSourceName,	"TRAFFIC" );
	RegisterChannel( dwSource,	LOG_USER,		"LOG_USER",		lpszSourceName,	"USER" );
	RegisterChannel( dwSource,	LOG_QUEST,		"LOG_QUEST",	lpszSourceName, "QUEST");
	RegisterChannel(dwSource, LOG_SCRIPT, "LOG_SCRIPT", lpszSourceName, "SCRIPT");
	RegisterChannel(dwSource, LOG_BOTAI, "LOG_BOTAI", lpszSourceName, "BOTAI");
	RegisterChannel(dwSource, LOG_HACK, "LOG_HACK", lpszSourceName, "HACK");
	RegisterChannel(dwSource, LOG_RANKBATTLE, "LOG_RANKBATTLE", lpszSourceName, "RANKBATTLE");

	NTL_LOG( LOG_TRAFFIC, "SessionType,Address,Port,ConnectTime,TotalSize,BytesRecvSize,BytesRecvSizeMax,BytesSendSize,BytesSendSizeMax,"
							"PacketTotalCount,PacketRecvCount,PacketSendCount,RecvQueueMaxUseSize,SendQueueMaxUseSize,RecvBufferLoopCount,SendBufferLoopCount");

	return NTL_SUCCESS;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlLog::Log(BYTE byLogChannel, bool bDate, LPCTSTR lpszFile, int nLine, LPCTSTR lpszFunc, LPCTSTR lpszText, ...)
{
	char szLogBuffer[BUFSIZE_LOG + 1] = { 0x00, };
	int nBuffSize = (int)sizeof( szLogBuffer );
	int nWriteSize = 0;

#if defined(_WIN32)
	nWriteSize += NTL_STPRINTF( szLogBuffer, nBuffSize, TEXT("[%s]\t"), GetLogChannelString(byLogChannel) );

	if( bDate )
	{
		SYSTEMTIME	systemTime;
		GetLocalTime( &systemTime );
		nWriteSize += NTL_STPRINTF( szLogBuffer + nWriteSize, nBuffSize - nWriteSize, TEXT("[%d-%02d-%02d %d:%d:%d:%d]\t"), systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds );
	}


	va_list args;
	va_start( args, lpszText );
	nWriteSize += NTL_VSTPRINTF( szLogBuffer + nWriteSize, nBuffSize - nWriteSize, lpszText, args );
	va_end( args );


	if( lpszFile )
	{
		nWriteSize += NTL_STPRINTF( szLogBuffer + nWriteSize, nBuffSize - nWriteSize, TEXT(" file[%s]\tline[%d]\t"), lpszFile, nLine );
	}


	if( lpszFunc )
	{
		nWriteSize += NTL_STPRINTF( szLogBuffer + nWriteSize, nBuffSize - nWriteSize, TEXT(" function[%s]\t"), lpszFunc );
	}

	if( s_log_stream)
	{
		_ftprintf( s_log_stream, "%s\n", szLogBuffer );
		fflush( s_log_stream );
	}
#else
	nWriteSize += snprintf( szLogBuffer, (size_t)nBuffSize, "[%s]\t", GetLogChannelString(byLogChannel) );

	if( bDate )
	{
		time_t now = time(NULL);
		struct tm* t = localtime(&now);
		if ( t )
			nWriteSize += snprintf( szLogBuffer + nWriteSize, (size_t)(nBuffSize - nWriteSize), "[%d-%02d-%02d %d:%d:%d]\t", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec );
	}

	va_list args;
	va_start( args, lpszText );
	nWriteSize += vsnprintf( szLogBuffer + nWriteSize, (size_t)(nBuffSize - nWriteSize), lpszText, args );
	va_end( args );

	if( lpszFile )
		nWriteSize += snprintf( szLogBuffer + nWriteSize, (size_t)(nBuffSize - nWriteSize), " file[%s]\tline[%d]\t", lpszFile, nLine );
	if( lpszFunc )
		nWriteSize += snprintf( szLogBuffer + nWriteSize, (size_t)(nBuffSize - nWriteSize), " function[%s]\t", lpszFunc );

	if(byLogChannel == LOG_ASSERT)
		printf( "LOG_ASSERT: %s\n", szLogBuffer);

	if( s_log_stream)
	{
		fprintf( s_log_stream, "%s\n", szLogBuffer );
		fflush( s_log_stream );
	}
#endif
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlLog::Log(LPCTSTR lpszText, ...)
{
	char szLogBuffer[BUFSIZE_LOG + 1] = { 0x00, };
	int nBuffSize = (int)sizeof(szLogBuffer);
	int nWriteSize = 0;

#if defined(_WIN32)
	nWriteSize += NTL_STPRINTF(szLogBuffer, nBuffSize, TEXT("[%s]\t"), GetLogChannelString(LOG_HACK));

	SYSTEMTIME	systemTime;
	GetLocalTime(&systemTime);
	nWriteSize += NTL_STPRINTF(szLogBuffer + nWriteSize, nBuffSize - nWriteSize, TEXT("[%d-%02d-%02d %d:%d:%d:%d]\t"), systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);


	va_list args;
	va_start(args, lpszText);
	nWriteSize += NTL_VSTPRINTF(szLogBuffer + nWriteSize, nBuffSize - nWriteSize, lpszText, args);
	va_end(args);

	if (s_log_stream)
	{
		_ftprintf(s_log_stream, "%s\n", szLogBuffer);
		fflush(s_log_stream);
	}
#else
	nWriteSize += snprintf(szLogBuffer, (size_t)nBuffSize, "[%s]\t", GetLogChannelString(LOG_HACK));
	time_t now = time(NULL);
	struct tm* t = localtime(&now);
	if ( t )
		nWriteSize += snprintf(szLogBuffer + nWriteSize, (size_t)(nBuffSize - nWriteSize), "[%d-%02d-%02d %d:%d:%d]\t", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

	va_list args;
	va_start(args, lpszText);
	nWriteSize += vsnprintf(szLogBuffer + nWriteSize, (size_t)(nBuffSize - nWriteSize), lpszText, args);
	va_end(args);

	if (s_log_stream)
	{
		fprintf(s_log_stream, "%s\n", szLogBuffer);
		fflush(s_log_stream);
	}
#endif
}



//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
int CNtlLog::UnitTest()
{
	int nTemp = 1;
	LPCTSTR lpszTemp = "Hello World!!";

	NTL_LOG( LOG_GENERAL, TEXT( "test : [%d] [%s]" ) , nTemp, lpszTemp );
	NTL_LOGD( LOG_SYSTEM, TEXT( "test : [%d] [%s]" ), nTemp, lpszTemp );
	NTL_LOGL( LOG_ASSERT, TEXT( "test : [%d] [%s]" ), nTemp, lpszTemp );
	NTL_LOGDL( LOG_GENERAL, TEXT( "test : [%d] [%s]" ), nTemp, lpszTemp );
	NTL_LOG_ASSERT( TEXT( "test : [%d] [%s]" ), nTemp, lpszTemp );

	return 0;
}

//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlLog::ResetList()
{
	::EnterCriticalSection(&m_loglock);
	for (LOGLISTIT iter = m_listLog.begin() ; m_listLog.end() != iter ; iter++)
	{
		SAFE_DELETE(*iter);
	}
	m_listLog.clear();

	::LeaveCriticalSection(&m_loglock);
}

//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CNtlLog::IsNew(LPCTSTR lpszFile, int nLine, LPCTSTR lpszFunc)
{
	::EnterCriticalSection(&m_loglock);
	for (LOGLISTIT iter = m_listLog.begin() ; m_listLog.end() != iter ; iter++)
	{
		sLOG_BRIEF* psData = *iter;
		if ( NULL != psData )
		{
			if ( (0 == strcmp(psData->szFile.c_str(), lpszFile ) ) && (nLine == psData->nLine) && (0 == strcmp(psData->szFuntion.c_str(), lpszFunc ) ) )
			{
				::LeaveCriticalSection(&m_loglock);
				return false;
			}
		}
	}

	sLOG_BRIEF* psData = new sLOG_BRIEF;
	if (NULL == psData)
	{
		::LeaveCriticalSection(&m_loglock);
		return false;
	}

	psData->szFile = lpszFile;
	psData->nLine = nLine;
	psData->szFuntion = lpszFunc;

	m_listLog.push_back(psData);

	::LeaveCriticalSection(&m_loglock);
	return true;
}
