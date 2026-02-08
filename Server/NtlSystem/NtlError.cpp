//***********************************************************************************
//
//	File		:	NtlError.cpp
//
//	Begin		:	2005-11-30
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Hyun Woo, Koo   ( zeroera@ntl-inc.com )
//
//	Desc		:	Error ??? ????
//
//***********************************************************************************

#include "stdafx.h"
#include "NtlError.h"
#if !defined(_WIN32)
#include <cstring>
#include <cerrno>
#endif

#ifdef NTL_DEFINE_ERROR
	#undef NTL_DEFINE_ERROR
	#define NTL_DEFINE_ERROR(x)		#x,
#endif

const char * ntl_error_string[ MAX_NTL_ERROR - NTL_ERR_BEGIN ] =
{
	"NTL_ERR_BEGIN",

	#include "NtlErrorcodes.h"
};

const char * GetNtlErrorString(int nErrorCode )
{
	if( nErrorCode >= MAX_NTL_ERROR )
		return "Ntl Error message not found";

	return ntl_error_string[ nErrorCode - NTL_ERR_BEGIN ];
}



const unsigned int	MAX_ERR_STR_BUFF = 256;


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void NtlGetErrorString( CNtlString & rStrError, int iErrorCode )
{
	if( iErrorCode >= NTL_ERR_BEGIN )
	{
		rStrError = GetNtlErrorString( iErrorCode );
	}
	else
	{
		char szBuf[MAX_ERR_STR_BUFF] = { 0x00, };

#if defined(_WIN32)
		FormatMessage(	FORMAT_MESSAGE_FROM_SYSTEM,
						0,
						iErrorCode,
						MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
						szBuf,
						MAX_ERR_STR_BUFF,
						NULL);
#else
#if defined(__GLIBC__) && defined(_GNU_SOURCE)
		{
			char *p = strerror_r(iErrorCode, szBuf, sizeof(szBuf));
			if (p != szBuf && p)
				strncpy(szBuf, p, sizeof(szBuf) - 1);
		}
#else
		(void)strerror_r(iErrorCode, szBuf, sizeof(szBuf));
#endif
		szBuf[sizeof(szBuf) - 1] = '\0';
#endif
		rStrError = szBuf;
	}
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
const char * NtlGetErrorMessage( int iErrorCode )
{
	if( iErrorCode >= NTL_ERR_BEGIN )
	{
		return GetNtlErrorString( iErrorCode );
	}
	else
	{
		static char szBuf[MAX_ERR_STR_BUFF] = { 0x00, };

#if defined(_WIN32)
		FormatMessage(	FORMAT_MESSAGE_FROM_SYSTEM,
						0,
						iErrorCode,
						MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
						szBuf,
						MAX_ERR_STR_BUFF,
						NULL);
#else
#if defined(__GLIBC__) && defined(_GNU_SOURCE)
		{
			char *p = strerror_r(iErrorCode, szBuf, sizeof(szBuf));
			if (p != szBuf && p)
				strncpy(szBuf, p, sizeof(szBuf) - 1);
		}
#else
		(void)strerror_r(iErrorCode, szBuf, sizeof(szBuf));
#endif
		szBuf[sizeof(szBuf) - 1] = '\0';
#endif
		return szBuf;
	}
}

