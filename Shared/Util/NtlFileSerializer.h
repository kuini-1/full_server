/*****************************************************************************
 *
 * File			: NtlFileSerializer.h
 * Author		: 
 * Copyright	: (??)NTL
 * Date			: 2007. 02. 06
 * Abstract		: 
 *****************************************************************************
 * Desc         : 
 *
 *****************************************************************************/

#pragma once

#include "NtlSerializer.h"
#if !defined(_WIN32)
#include "../NtlSharedCommon.h"
#endif

class CNtlFileSerializer : public CNtlSerializer
{
public:

	CNtlFileSerializer();
	CNtlFileSerializer(int iBufferSize, int iGlowSize);

	CNtlFileSerializer(const char* pszFullPath);
	CNtlFileSerializer(const WCHAR* pwszFullPath);

	virtual ~CNtlFileSerializer();

public:
	bool SaveFile(const char* pszFullPathFileName, bool bCrypt = FALSE, const char* szCryptPassword = NULL);
	bool SaveFile(const WCHAR* pwszFullPathFileName, bool bCrypt = FALSE, const WCHAR* szCryptPassword = NULL);

	bool LoadFile(const char* pszFullPathFileName, bool bCrypt = FALSE, const char* szCryptPassword = NULL);
	bool LoadFile(const char* pszBuffer, int nSize, bool bCrypt = FALSE, const char* szCryptPassword = NULL);

	bool LoadFile(const WCHAR* pwszFullPathFileName, bool bCrypt = FALSE, const WCHAR* szCryptPassword = NULL);
};