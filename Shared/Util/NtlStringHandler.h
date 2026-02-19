#pragma once

#include <string>
#if !defined(_WIN32)
#include "../NtlSharedCommon.h"
#else
#ifndef _countof
#define _countof(a) (sizeof(a) / sizeof((a)[0]))
#endif
#include "NtlPortable.h"
#endif

#define NTL_SAFE_STRCPY(buffer, original_string) \
	if ( NULL==(char*)original_string ) buffer[0] = '\0'; \
	else NTL_STRNCPY_S((buffer), _countof(buffer), (original_string), _countof(buffer) - 1); \
	buffer[_countof(buffer) - 1] = '\0';

#define NTL_SAFE_STRCPY_SIZEINPUT(buffer, buffer_size, original_string) \
	if ( NULL==(char*)original_string ) buffer[0] = '\0'; \
	else NTL_STRNCPY_S((buffer), (buffer_size), (original_string), _countof(original_string)); \
	buffer[buffer_size - 1] = '\0';

#define NTL_SAFE_WCSCPY(buffer, original_string) \
	if ( NULL==(WCHAR*)original_string ) buffer[0] = (WCHAR)0; \
	else NTL_WCSNCPY_S((WCHAR*)(buffer), _countof(buffer), (original_string), _countof(buffer) - 1); \
	buffer[_countof(buffer) - 1] = (WCHAR)0;

#define NTL_SAFE_STRNCPY(buffer, original_string, character_count) \
	if ( NULL==(char*)original_string ) buffer[0] = '\0'; \
	else NTL_STRNCPY_S((buffer), _countof(buffer), (original_string), (character_count)); \
	buffer[character_count] = '\0';

#define NTL_SAFE_WCSNCPY(buffer, original_string, character_count) \
	if ( NULL==(WCHAR*)original_string ) buffer[0] = (WCHAR)0; \
	else NTL_WCSNCPY_S((WCHAR*)(buffer), _countof(buffer), (original_string), (character_count)); \
	buffer[character_count] = (WCHAR)0;

#define NTL_SAFE_WCSNCPY_SIZEINPUT(buffer, buffer_size, original_string, character_count) \
	if ( NULL==(WCHAR*)original_string ) buffer[0] = (WCHAR)0; \
	else NTL_WCSNCPY_S((WCHAR*)(buffer), (buffer_size), (original_string), (character_count)); \
	buffer[character_count] = (WCHAR)0;


WCHAR* Ntl_MB2WC(char* pszOriginalString);
char* Ntl_WC2MB(WCHAR* pwszOriginalString);

std::wstring s2ws(const std::string& s);
std::string ws2s(const std::wstring& wstr);

void Ntl_CleanUpHeapString(char* pwszString);
void Ntl_CleanUpHeapStringW(WCHAR* pwszString);

int Ntl_GenerateFormattedString(std::string& rstrResult, char* pszFormat, ...);
int Ntl_GenerateFormattedStringW(std::wstring& rwstrResult, WCHAR* pwszFormat, ...);