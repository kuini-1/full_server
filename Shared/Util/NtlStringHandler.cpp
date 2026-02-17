#include "stdafx.h"
#include "NtlStringHandler.h"
#include <cstdarg>
#if defined(_WIN32)
#include <codecvt>
#else
#include <cstdlib>
#include <cwchar>
#include <clocale>
#include <iconv.h>
#include <errno.h>
#include <string.h>
#endif


const DWORD NTL_MAX_LENGTH_OF_FORMAT_STRING_RESULT = 1024;

std::wstring s2ws(const std::string& s)
{
#if defined(_WIN32)
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;
	return converterX.from_bytes(s);
#else
	if (s.empty())
		return std::wstring();
	size_t len = mbstowcs(NULL, s.c_str(), 0);
	if (len == (size_t)-1)
		return std::wstring();
	std::wstring result(len + 1, 0);
	mbstowcs(&result[0], s.c_str(), len + 1);
	result.resize(len);
	return result;
#endif
}

std::string ws2s(const std::wstring& wstr)
{
#if defined(_WIN32)
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;
	return converterX.to_bytes(wstr);
#else
	if (wstr.empty())
		return std::string();
	size_t len = wcstombs(NULL, wstr.c_str(), 0);
	if (len == (size_t)-1)
		return std::string();
	std::string result(len + 1, 0);
	wcstombs(&result[0], wstr.c_str(), len + 1);
	result.resize(len);
	return result;
#endif
}

WCHAR* Ntl_MB2WC(char* pszOriginalString)
{
	if (NULL == pszOriginalString)
	{
		return NULL;
	}

#if defined(_WIN32)
	int iRequiredChars = ::MultiByteToWideChar(::GetACP(), 0, pszOriginalString, -1, NULL, 0);
	WCHAR* pwszResultString = new WCHAR[iRequiredChars];
	if (NULL == pwszResultString)
	{
		return NULL;
	}
	::MultiByteToWideChar(::GetACP(), 0, pszOriginalString, -1, pwszResultString, iRequiredChars);
	return pwszResultString;
#else
	// On Linux, WCHAR is unsigned short (UTF-16LE), not wchar_t (UTF-32)
	// Use iconv to convert UTF-8 to UTF-16LE
	iconv_t cd = iconv_open("UTF-16LE", "UTF-8");
	if (cd == (iconv_t)-1)
	{
		// Fallback: simple ASCII conversion (not ideal but better than nothing)
		size_t len = strlen(pszOriginalString);
		WCHAR* pwszResultString = new WCHAR[len + 1];
		if (NULL == pwszResultString)
			return NULL;
		for (size_t i = 0; i < len; i++)
		{
			if ((unsigned char)pszOriginalString[i] < 128)
				pwszResultString[i] = (unsigned short)(unsigned char)pszOriginalString[i];
			else
				pwszResultString[i] = '?'; // Invalid character
		}
		pwszResultString[len] = 0;
		return pwszResultString;
	}
	
	size_t inbytesleft = strlen(pszOriginalString);
	size_t outbytesleft = (inbytesleft + 1) * sizeof(WCHAR); // UTF-16 can be up to 2 bytes per UTF-8 byte
	WCHAR* pwszResultString = new WCHAR[inbytesleft + 1];
	if (NULL == pwszResultString)
	{
		iconv_close(cd);
		return NULL;
	}
	
	char* inbuf = pszOriginalString;
	char* outbuf = (char*)pwszResultString;
	
	size_t result = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
	iconv_close(cd);
	
	if (result == (size_t)-1)
	{
		delete[] pwszResultString;
		return NULL;
	}
	
	// Null terminate
	*((WCHAR*)outbuf) = 0;
	return pwszResultString;
#endif
}

char* Ntl_WC2MB(WCHAR* pwszOriginalString)
{
	if (NULL == pwszOriginalString)
	{
		return NULL;
	}

#if defined(_WIN32)
	int iRequiredChars = ::WideCharToMultiByte(::GetACP(), 0, pwszOriginalString, -1, NULL, 0, NULL, NULL);
	char* pszResultString = new char[iRequiredChars];
	if (NULL == pszResultString)
	{
		return NULL;
	}
	::WideCharToMultiByte(::GetACP(), 0, pwszOriginalString, -1, pszResultString, iRequiredChars, NULL, NULL);
	return pszResultString;
#else
	// On Linux, WCHAR is unsigned short (UTF-16LE), not wchar_t (UTF-32)
	// Use iconv to convert UTF-16LE to UTF-8
	iconv_t cd = iconv_open("UTF-8", "UTF-16LE");
	if (cd == (iconv_t)-1)
	{
		// Fallback: simple ASCII conversion (not ideal but better than nothing)
		size_t len = 0;
		while (pwszOriginalString[len] != 0)
			len++;
		char* pszResultString = new char[len + 1];
		if (NULL == pszResultString)
			return NULL;
		for (size_t i = 0; i < len; i++)
		{
			if (pwszOriginalString[i] < 128)
				pszResultString[i] = (char)pwszOriginalString[i];
			else
				pszResultString[i] = '?'; // Invalid character
		}
		pszResultString[len] = '\0';
		return pszResultString;
	}
	
	// Calculate input length (UTF-16LE string length in bytes)
	size_t wlen = 0;
	while (pwszOriginalString[wlen] != 0)
		wlen++;
	size_t inbytesleft = (wlen + 1) * sizeof(WCHAR); // Include null terminator
	
	// Estimate output size (UTF-8 can be up to 4 bytes per UTF-16 char, but usually 1-3)
	size_t outbytesleft = (wlen + 1) * 4;
	char* pszResultString = new char[outbytesleft];
	if (NULL == pszResultString)
	{
		iconv_close(cd);
		return NULL;
	}
	
	char* inbuf = (char*)pwszOriginalString;
	char* outbuf = pszResultString;
	
	size_t result = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
	iconv_close(cd);
	
	if (result == (size_t)-1)
	{
		delete[] pszResultString;
		return NULL;
	}
	
	// Null terminate
	*outbuf = '\0';
	return pszResultString;
#endif
}

void Ntl_CleanUpHeapString(char* pszString)
{
	if (NULL != pszString)
	{
		delete [] pszString;
	}
}

void Ntl_CleanUpHeapStringW(WCHAR* pwszString)
{
	if (NULL != pwszString)
	{
		delete [] pwszString;
	}
}

int Ntl_GenerateFormattedString(std::string& rstrResult, char* pszFormat, ...)
{
	char szResult[NTL_MAX_LENGTH_OF_FORMAT_STRING_RESULT + 1];
	::ZeroMemory(szResult, _countof(szResult));

	va_list valist;
	va_start(valist, pszFormat);

	int nWrittenBytes = vsnprintf(szResult, _countof(szResult), pszFormat, valist);

	va_end(valist);

	if (0 < nWrittenBytes)
	{
		rstrResult.assign(szResult);
	}
	else
	{
		rstrResult.clear();
	}

	return nWrittenBytes;
}

// Helper function to convert WCHAR* (UTF-16LE) to wchar_t* (UTF-32) for format strings
static wchar_t* WCHARToWCharT(const WCHAR* src)
{
	if (!src)
		return NULL;
	
#if defined(_WIN32)
	// On Windows, WCHAR == wchar_t (both 2 bytes), so direct cast works
	return (wchar_t*)src;
#else
	// On Linux, WCHAR is UTF-16LE (2 bytes), wchar_t is UTF-32 (4 bytes)
	// Convert using iconv
	static thread_local wchar_t* cached_result = NULL;
	static thread_local size_t cached_size = 0;
	
	size_t srcLen = 0;
	const WCHAR* p = src;
	while (*p != 0) { p++; srcLen++; }
	
	// Allocate buffer (UTF-32: 4 bytes per UTF-16 char, but usually 1:1 for ASCII)
	size_t needed_size = (srcLen + 1) * sizeof(wchar_t);
	if (cached_size < needed_size)
	{
		if (cached_result)
			delete[] cached_result;
		cached_result = new wchar_t[srcLen + 1];
		cached_size = needed_size;
	}
	
	iconv_t cd = iconv_open("UTF-32", "UTF-16LE");
	if (cd == (iconv_t)-1)
	{
		cd = iconv_open("UTF-32LE", "UTF-16LE");
	}
	if (cd == (iconv_t)-1)
	{
		// Fallback: simple ASCII conversion
		for (size_t i = 0; i < srcLen; i++)
		{
			if (src[i] < 128)
				cached_result[i] = (wchar_t)src[i];
			else
				cached_result[i] = L'?';
		}
		cached_result[srcLen] = L'\0';
		return cached_result;
	}
	
	size_t inbytesleft = (srcLen + 1) * sizeof(WCHAR); // Include null terminator
	size_t outbytesleft = (srcLen + 1) * sizeof(wchar_t);
	char* inbuf = (char*)src;
	char* outbuf = (char*)cached_result;
	
	size_t result = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
	iconv_close(cd);
	
	if (result == (size_t)-1)
	{
		// Fallback: simple ASCII conversion
		for (size_t i = 0; i < srcLen; i++)
		{
			if (src[i] < 128)
				cached_result[i] = (wchar_t)src[i];
			else
				cached_result[i] = L'?';
		}
		cached_result[srcLen] = L'\0';
		return cached_result;
	}
	
	cached_result[srcLen] = L'\0';
	return cached_result;
#endif
}

int Ntl_GenerateFormattedStringW(std::wstring& rwstrResult, WCHAR* pwszFormat, ...)
{
#if defined(_WIN32)
	// On Windows, WCHAR == wchar_t (both 2 bytes), so direct usage works
	WCHAR wszResult[NTL_MAX_LENGTH_OF_FORMAT_STRING_RESULT + 1];
	::ZeroMemory(wszResult, _countof(wszResult));

	va_list valist;
	va_start(valist, pwszFormat);

	int nWrittenBytes = vswprintf((wchar_t*)wszResult, _countof(wszResult), (wchar_t*)pwszFormat, valist);

	va_end(valist);

	if (0 < nWrittenBytes)
	{
		rwstrResult.assign((wchar_t*)wszResult);
	}
	else
	{
		rwstrResult.clear();
	}

	return nWrittenBytes;
#else
	// On Linux, WCHAR is UTF-16LE (2 bytes), wchar_t is UTF-32 (4 bytes)
	// Convert format string and use wchar_t buffer
	wchar_t* wFormat = WCHARToWCharT(pwszFormat);
	if (!wFormat)
	{
		rwstrResult.clear();
		return 0;
	}
	
	wchar_t wszResult[NTL_MAX_LENGTH_OF_FORMAT_STRING_RESULT + 1];
	memset(wszResult, 0, sizeof(wszResult));

	va_list valist;
	va_start(valist, pwszFormat);

	int nWrittenBytes = vswprintf(wszResult, _countof(wszResult), wFormat, valist);

	va_end(valist);

	if (0 < nWrittenBytes)
	{
		rwstrResult.assign(wszResult);
	}
	else
	{
		rwstrResult.clear();
	}

	return nWrittenBytes;
#endif
}