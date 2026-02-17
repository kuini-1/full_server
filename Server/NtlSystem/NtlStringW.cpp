//***********************************************************************************
//
//	File		:	NtlStringW.cpp
//
//	Begin		:	2006-10-16
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Hyun Woo, Koo   ( zeroera@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************


#include "stdafx.h"
#include "NtlStringW.h"
#include <cstdarg>
#if !defined(_WIN32)
#include <iconv.h>
#include <cstring>
#endif

const unsigned int	MAX_FORMAT_STR_BUFF = 2048;

// Helper function to convert WCHAR* (UTF-16LE) to std::wstring (UTF-32 on Linux)
static std::wstring WCHARToWString(const WCHAR* src)
{
	if (!src)
		return std::wstring();
	
#if defined(_WIN32)
	// On Windows, WCHAR == wchar_t (both 2 bytes), so direct conversion works
	return std::wstring((const wchar_t*)src);
#else
	// On Linux, WCHAR is UTF-16LE (2 bytes), wchar_t is UTF-32 (4 bytes)
	// Convert using iconv
	size_t srcLen = 0;
	const WCHAR* p = src;
	while (*p != 0) { p++; srcLen++; }
	if (srcLen == 0)
		return std::wstring();
	
	iconv_t cd = iconv_open("UTF-32", "UTF-16LE");
	if (cd == (iconv_t)-1)
	{
		cd = iconv_open("UTF-32LE", "UTF-16LE");
	}
	if (cd == (iconv_t)-1)
	{
		// Fallback: simple ASCII conversion
		std::wstring result;
		result.reserve(srcLen);
		for (size_t i = 0; i < srcLen; i++)
		{
			if (src[i] < 128)
				result += (wchar_t)src[i];
			else
				result += L'?';
		}
		return result;
	}
	
	size_t inbytesleft = (srcLen + 1) * sizeof(WCHAR); // Include null terminator
	size_t outbytesleft = (srcLen + 1) * sizeof(wchar_t);
	wchar_t* outbuf = new wchar_t[srcLen + 1];
	if (!outbuf)
	{
		iconv_close(cd);
		return std::wstring();
	}
	
	char* inbuf = (char*)src;
	char* outbuf_char = (char*)outbuf;
	
	size_t result = iconv(cd, &inbuf, &inbytesleft, &outbuf_char, &outbytesleft);
	iconv_close(cd);
	
	if (result == (size_t)-1)
	{
		delete[] outbuf;
		// Fallback: simple ASCII conversion
		std::wstring wstr;
		wstr.reserve(srcLen);
		for (size_t i = 0; i < srcLen; i++)
		{
			if (src[i] < 128)
				wstr += (wchar_t)src[i];
			else
				wstr += L'?';
		}
		return wstr;
	}
	
	size_t actualLen = (srcLen * sizeof(wchar_t) - outbytesleft) / sizeof(wchar_t);
	outbuf[actualLen] = L'\0';
	
	std::wstring wstr(outbuf, actualLen);
	delete[] outbuf;
	return wstr;
#endif
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlStringW::CNtlStringW(void)
{
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlStringW::~CNtlStringW(void)
{
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlStringW::CNtlStringW(const WCHAR* pwszString)//:std::wstring(pwszString)
{
	m_str = WCHARToWString(pwszString);
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlStringW::CNtlStringW(const char * pszString)
{
	*this = pszString;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlStringW & CNtlStringW::operator=(const char * pszString)
{
	int nWStrLen = MultiByteToWideChar( ::GetACP(), 0, pszString, -1, NULL, 0 );
	WCHAR * pwString = new WCHAR[ nWStrLen ];
	if( pwString )
	{
		MultiByteToWideChar( ::GetACP(), 0, pszString, -1, pwString, nWStrLen );

		m_str = WCHARToWString(pwString);

		delete[] pwString;
	}

	return *this;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlStringW& CNtlStringW::operator=(const WCHAR* pwszString)
{
	m_str = WCHARToWString(pwszString);

	return *this;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CNtlStringW::operator==(CNtlStringW& string)
{
	if (0 == wcscmp(c_str(), string.c_str()))
		return true;

	return false;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
int CNtlStringW::Format(const WCHAR *format, ...)
{
	int nRV = 0;
#if defined(_WIN32)
	// On Windows, WCHAR == wchar_t (both 2 bytes), so direct usage works
	WCHAR szBuf[MAX_FORMAT_STR_BUFF] = { 0x00, };

	va_list valist;

	memset(szBuf, 0x00, sizeof(szBuf));

	va_start(valist, format);

	nRV = NTL_VSWPRINTF((wchar_t*)szBuf, MAX_FORMAT_STR_BUFF, (wchar_t*)format, valist);

	va_end(valist);

	if( nRV > 0 )
		m_str.assign((wchar_t*)szBuf);
	else
		m_str.clear();
#else
	// On Linux, WCHAR is UTF-16LE (2 bytes), wchar_t is UTF-32 (4 bytes)
	// Convert format string and use wchar_t buffer
	wchar_t* wFormat = NULL;
	size_t formatLen = 0;
	const WCHAR* p = format;
	while (*p != 0) { p++; formatLen++; }
	
	// Convert format string from WCHAR* to wchar_t*
	iconv_t cd = iconv_open("UTF-32", "UTF-16LE");
	if (cd == (iconv_t)-1)
	{
		cd = iconv_open("UTF-32LE", "UTF-16LE");
	}
	if (cd != (iconv_t)-1)
	{
		size_t inbytesleft = (formatLen + 1) * sizeof(WCHAR);
		size_t outbytesleft = (formatLen + 1) * sizeof(wchar_t);
		wFormat = new wchar_t[formatLen + 1];
		if (wFormat)
		{
			char* inbuf = (char*)format;
			char* outbuf = (char*)wFormat;
			iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
			wFormat[formatLen] = L'\0';
		}
		iconv_close(cd);
	}
	
	if (!wFormat)
	{
		// Fallback: simple ASCII conversion
		wFormat = new wchar_t[formatLen + 1];
		for (size_t i = 0; i < formatLen; i++)
		{
			if (format[i] < 128)
				wFormat[i] = (wchar_t)format[i];
			else
				wFormat[i] = L'?';
		}
		wFormat[formatLen] = L'\0';
	}
	
	wchar_t szBuf[MAX_FORMAT_STR_BUFF] = { 0x00, };

	va_list valist;

	memset(szBuf, 0x00, sizeof(szBuf));

	va_start(valist, format);

	nRV = vswprintf(szBuf, MAX_FORMAT_STR_BUFF, wFormat, valist);

	va_end(valist);
	
	delete[] wFormat;

	if( nRV > 0 )
		m_str.assign(szBuf);
	else
		m_str.clear();
#endif

	return nRV;
}