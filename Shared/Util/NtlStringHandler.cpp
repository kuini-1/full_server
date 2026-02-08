#include "stdafx.h"
#include "NtlStringHandler.h"
#include <cstdarg>
#if defined(_WIN32)
#include <codecvt>
#else
#include <cstdlib>
#include <cwchar>
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
	size_t len = mbstowcs(NULL, pszOriginalString, 0);
	if (len == (size_t)-1)
		return NULL;
	WCHAR* pwszResultString = new WCHAR[len + 1];
	if (NULL == pwszResultString)
		return NULL;
	mbstowcs(pwszResultString, pszOriginalString, len + 1);
	pwszResultString[len] = 0;
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
	size_t len = wcstombs(NULL, pwszOriginalString, 0);
	if (len == (size_t)-1)
		return NULL;
	char* pszResultString = new char[len + 1];
	if (NULL == pszResultString)
		return NULL;
	wcstombs(pszResultString, pwszOriginalString, len + 1);
	pszResultString[len] = '\0';
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

int Ntl_GenerateFormattedStringW(std::wstring& rwstrResult, WCHAR* pwszFormat, ...)
{
	WCHAR wszResult[NTL_MAX_LENGTH_OF_FORMAT_STRING_RESULT + 1];
	::ZeroMemory(wszResult, _countof(wszResult));

	va_list valist;
	va_start(valist, pwszFormat);

	int nWrittenBytes = vswprintf(wszResult, _countof(wszResult), pwszFormat, valist);

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
}