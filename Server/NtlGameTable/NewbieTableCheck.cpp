
//***********************************************************************************
//
//	File		:	NewbieTableCheck.cpp
//
//	Begin		:	2006-03-27
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Doo  Sup, Chung   ( john@ntl-inc.com )
//
//	Desc		:	
//
//************************************************************************************

#include "stdafx.h"
#include "NewbieTableCheck.h"
#include "NtlDebug.h"
#include "NtlCharacter.h"

static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) { WCharTLiteralToWCHAR(fmt, dest, destSize); }

const WCHAR* CNewbieTableCheck::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};
CNewbieTableCheck::CNewbieTableCheck(void)
{
	Init();
}

CNewbieTableCheck::~CNewbieTableCheck(void)
{
	Destroy();
}

bool CNewbieTableCheck::Create(WCHAR* pwszFileName, DWORD dwCodePage)
{
	return CCheckTable::Create(pwszFileName, dwCodePage);
}

bool CNewbieTableCheck::Create(char* pszFileName, DWORD dwCodePage)
{
	return CCheckTable::Create(pszFileName, dwCodePage);
}

void CNewbieTableCheck::Destroy()
{
}

void CNewbieTableCheck::Init()
{
}

void* CNewbieTableCheck::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sNEWBIE_CHECK_TBLDAT* pNewSpawn = new sNEWBIE_CHECK_TBLDAT;
		if (NULL == pNewSpawn)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pNewSpawn;
			return NULL;
		}

		m_dwCodePage = dwCodePage;
		return pNewSpawn;
	}

	return NULL;
}

bool CNewbieTableCheck::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sNEWBIE_CHECK_TBLDAT* pNewbie = (sNEWBIE_CHECK_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pNewbie, sizeof(*pNewbie)))
			return false;

		delete pNewbie;

		return true;
	}

	return false;
}

bool CNewbieTableCheck::AddTable(void* pvTable, WCHAR* pwszSheetName)
{
	if ( 0 != WCHARCmp(pwszSheetName, g_wszTableDataKOR) )
	{
		_ASSERTE( 0 );
		return false;
	}


	sNEWBIE_CHECK_TBLDAT* pNewbie = (sNEWBIE_CHECK_TBLDAT*)pvTable;

	if ( false == m_mapTableList.insert(std::pair<std::wstring , sTBLDATCHECK*>((pNewbie->wstrField_Name), pNewbie)).second )
	{
		_ASSERTE( 0 );
		return false;
	}
	

	return true;
}

bool CNewbieTableCheck::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sNEWBIE_CHECK_TBLDAT* pNewbie = (sNEWBIE_CHECK_TBLDAT*)pvTable;

		if (0 == WStringCmpLiteral(*pstrDataName, L"Field_Name"))
		{
			pNewbie->wstrField_Name = (bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Only_N"))
		{
			pNewbie->byOnly_N = (BYTE)(_wtoi(bstrData));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Invalid_Check"))
		{
			pNewbie->byInvalid_Check = (BYTE)(_wtoi(bstrData));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"N_From_N"))
		{
			pNewbie->byN_From_N = (BYTE)(_wtoi(bstrData));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Min"))
		{
			pNewbie->nMin = (_wtoi(bstrData));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Max"))
		{
			pNewbie->nMax = (BYTE)(_wtoi(bstrData));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"N_Above"))
		{
			pNewbie->byN_Above = (BYTE)(_wtoi(bstrData));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Above"))
		{
			pNewbie->nAbove = (_wtoi(bstrData));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"N_Below"))
		{
			pNewbie->byN_Below = (BYTE)(_wtoi(bstrData));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Below"))
		{
			pNewbie->nBelow = (_wtoi(bstrData));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"String_Below"))
		{
			pNewbie->byString_Below = (BYTE)(_wtoi(bstrData));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"String"))
		{
			pNewbie->nString = (_wtoi(bstrData));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"In_Tblidx"))
		{
			pNewbie->byIn_Tblidx = (BYTE)(_wtoi(bstrData));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Table"))
		{
			pNewbie->wstrTable = (bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Look_Up_Level"))
		{
			pNewbie->byLook_Up_Level = (BYTE)(_wtoi(bstrData));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Table_Level"))
		{
			pNewbie->wstrTable_Level = (bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Look_Up_Field"))
		{
			pNewbie->byLook_Up_Field = (BYTE)(_wtoi(bstrData));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Field"))
		{
			pNewbie->wstrField = (bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Table_Field"))
		{
			pNewbie->wstrTable_Field = (bstrData);
		}
		else
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			WCHAR wszFormatBuf[512];
			FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
			CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
			return false;
		}
	}
	else
	{
		return false;
	}

	return true;
}


sTBLDATCHECK* CNewbieTableCheck::FindData(std::wstring wstrField)
{
	CHTABLEIT iter;
	iter = m_mapTableList.find(wstrField);
	if (End() == iter)
		return NULL;

	return (sTBLDATCHECK*)(iter->second);
}

