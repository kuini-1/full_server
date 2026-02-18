//***********************************************************************************
//
//	File		:	WorldZoneTable.cpp
//
//	Begin		:	2007-08-14
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	
//
//	Desc		:	
//
//***********************************************************************************

#include "stdafx.h"
#include "WorldZoneTable.h"

#include "NtlSerializer.h"

// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CWorldZoneTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CWorldZoneTable::CWorldZoneTable(void)
{
	Init();
}

CWorldZoneTable::~CWorldZoneTable(void)
{
	Destroy();
}

bool CWorldZoneTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CWorldZoneTable::Destroy()
{
	CTable::Destroy();
}

void CWorldZoneTable::Init()
{
}

void* CWorldZoneTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sWORLD_ZONE_TBLDAT* pNewWorldZone = new sWORLD_ZONE_TBLDAT;
		if (NULL == pNewWorldZone)
		{
			return NULL;
		}

		CPINFO cpInfo;
		if (false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pNewWorldZone;
			return NULL;
		}
		
		m_dwCodePage = dwCodePage;
		return pNewWorldZone;
	}
	
	return NULL;
}

bool CWorldZoneTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sWORLD_ZONE_TBLDAT* pWorldZone = (sWORLD_ZONE_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pWorldZone, sizeof(*pWorldZone)))
		{
			return false;
		}

		delete pWorldZone;

		return true;
	}
	
	return false;
}

bool CWorldZoneTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);

	sWORLD_ZONE_TBLDAT* pTbldat = (sWORLD_ZONE_TBLDAT*)pvTable;

	if ( false == m_mapTableList.insert(std::pair<TBLIDX, sTBLDAT*>(pTbldat->tblidx, pTbldat)).second )
	{
		WCHAR wszFormatBuf[512];
		FormatStringToWCHAR(L"[File] : %s\r\n Table Tblidx[%u] is Duplicated ", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
		CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, pTbldat->tblidx );
		_ASSERTE( 0 );
		return false;
	}

	return true;
}

bool CWorldZoneTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sWORLD_ZONE_TBLDAT* pWorldZone = (sWORLD_ZONE_TBLDAT*)pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorldZone->tblidx = READ_DWORD( bstrData );
		}		
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Function_Bit_Flag") )
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorldZone->wFunctionBitFlag = (WORD)READ_BITFLAG( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"World"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorldZone->worldTblidx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Name"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorldZone->nameTblidx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Name_Text"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );

			READ_STRINGW(bstrData, pWorldZone->wszName_Text, _countof(pWorldZone->wszName_Text));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Forbidden_Vehicle"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorldZone->bForbidden_Vehicle = READ_BOOL( bstrData, wszFieldNameBuf );
		}
		else
		{
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


sTBLDAT* CWorldZoneTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
	{
		return NULL;
	}

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;
	
	return (sTBLDAT*)(iter->second); 
}

bool CWorldZoneTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
{
	if( false == bReload )
	{
		Reset();
	}

	BYTE byMargin = 1;
	serializer >> byMargin;

	bool bLoop = true;
	do
	{
		sWORLD_ZONE_TBLDAT* pTableData = new sWORLD_ZONE_TBLDAT;
		if (NULL == pTableData)
		{
			//- yoshiki : To log system!
			Destroy();
			return false;
		}

		if (false == pTableData->LoadFromBinary(serializer))
		{
			delete pTableData;
			bLoop = false;
			break;
		}

	//	printf("pTableData->tblidx %d \n", pTableData->tblidx);
		if( false == AddTable(pTableData, bReload, bUpdate) )
		{
			delete pTableData;
		}

	} while (false != bLoop);

	return true;
}

bool CWorldZoneTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sWORLD_ZONE_TBLDAT* pTableData = (sWORLD_ZONE_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}