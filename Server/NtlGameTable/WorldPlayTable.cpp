//***********************************************************************************
//
//	File		:	WorldPlayTable.cpp
//
//	Begin		:	2008-03-03
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Lee Ju-hyung
//
//	Desc		:	
//
//***********************************************************************************

#include "stdafx.h"
#include "WorldPlayTable.h"
#include "NtlDebug.h"

#include "NtlSerializer.h"

// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CWorldPlayTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};


CWorldPlayTable::CWorldPlayTable(void)
{
	Init();
}

CWorldPlayTable::~CWorldPlayTable(void)
{
	Destroy();
}

bool CWorldPlayTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CWorldPlayTable::Destroy()
{
	CTable::Destroy();
}

void CWorldPlayTable::Init()
{
}

void* CWorldPlayTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sWORLDPLAY_TBLDAT * pNewWorldPlay = new sWORLDPLAY_TBLDAT;
		if (NULL == pNewWorldPlay)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pNewWorldPlay;
			return NULL;
		}

		m_dwCodePage = dwCodePage;
		return pNewWorldPlay;
	}

	return NULL;
}

bool CWorldPlayTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sWORLDPLAY_TBLDAT * pWorldPlay = (sWORLDPLAY_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pWorldPlay, sizeof(*pWorldPlay)))
			return false;

		delete pWorldPlay;

		return true;
	}

	return false;
}


bool CWorldPlayTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);

	sWORLDPLAY_TBLDAT* pTbldat = (sWORLDPLAY_TBLDAT*)pvTable;

	if ( false == m_mapTableList.insert( std::pair<TBLIDX, sTBLDAT*>(pTbldat->tblidx, pTbldat) ).second )
	{
		WCHAR wszFormatBuf[512];
		FormatStringToWCHAR(L"[File] : %s\r\n Table Tblidx[%u] is Duplicated ", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
		CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, pTbldat->tblidx );
		_ASSERTE( 0 );
		return false;
	}

	return true;
}


bool CWorldPlayTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sWORLDPLAY_TBLDAT* pTbldat = (sWORLDPLAY_TBLDAT*)pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pTbldat->tblidx = READ_DWORD(bstrData);
		}
//		else if (0 == WStringCmpLiteral(*pstrDataName, L"WpsId"))
//		{
//			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
//			pTbldat->dwWpsId = READ_DWORD(bstrData);
//		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Group"))
		{
			pTbldat->dwGroup = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"ExecuterType"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pTbldat->byExecuterType = READ_BYTE( bstrData, wszFieldNameBuf);

			if( WORLDPLAY_EXECUTER_TYPE_COUNT <= pTbldat->byExecuterType)
			{
				_ASSERTE(0);
				return false;
			}
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"ShareType"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pTbldat->byShareType = READ_BYTE( bstrData, wszFieldNameBuf);

			if( WORLDPLAY_SHARE_TYPE_COUNT <= pTbldat->byShareType)
			{
				_ASSERTE(0);
				return false;
			}
		}
		else if( 0 == WStringCmpLiteral(*pstrDataName, L"ShareLimitTime"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pTbldat->dwShareLimitTime = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Desc"))
		{
//			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );

//			READ_STRINGW( bstrData, pTbldat->wszDesc, _countof(pTbldat->wszName) );
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


sTBLDAT* CWorldPlayTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;

	return (sTBLDAT*)(iter->second); 
}


bool CWorldPlayTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sWORLDPLAY_TBLDAT* pTableData = new sWORLDPLAY_TBLDAT;
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

bool CWorldPlayTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sWORLDPLAY_TBLDAT* pTableData = (sWORLDPLAY_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}