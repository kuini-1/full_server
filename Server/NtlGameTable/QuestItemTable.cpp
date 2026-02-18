//***********************************************************************************
//
//	File		:	QuestItemTable.cpp
//
//	Begin		:	2006-09-26
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Jeong Ho, Rho ( lleo52@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************


#include "stdafx.h"
#include "QuestItemTable.h"
#include "NtlDebug.h"
#include "NtlSerializer.h"

static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) { WCharTLiteralToWCHAR(fmt, dest, destSize); }

const WCHAR* CQuestItemTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CQuestItemTable::CQuestItemTable( void )
{
	Init();
}

CQuestItemTable::~CQuestItemTable( void )
{
	Destroy();
}

bool CQuestItemTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CQuestItemTable::Destroy( void )
{
	CTable::Destroy();
}

void CQuestItemTable::Init( void )
{
}

void* CQuestItemTable::AllocNewTable( WCHAR* pwszSheetName, DWORD dwCodePage )
{
	if ( 0 == WCHARCmp( pwszSheetName, g_wszTableDataKOR ) )
	{
		sQUESTITEM_TBLDAT* pNewObj = new sQUESTITEM_TBLDAT;
		if ( NULL == pNewObj ) return NULL;

		CPINFO cpInfo;
		if ( !GetCPInfo( dwCodePage, &cpInfo ) )
		{
			delete pNewObj;
			return NULL;
		}

		m_dwCodePage = dwCodePage;
		return pNewObj;
	}

	return NULL;
}

bool CQuestItemTable::DeallocNewTable( void* pvTable, WCHAR* pwszSheetName )
{
	if ( 0 == WCHARCmp( pwszSheetName, g_wszTableDataKOR ) )
	{
		sQUESTITEM_TBLDAT* pObj = (sQUESTITEM_TBLDAT*)pvTable;
		if ( IsBadReadPtr( pObj, sizeof(*pObj) ) ) return false;

		delete pObj;

		return true;
	}

	return false;
}

bool CQuestItemTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);

	sQUESTITEM_TBLDAT* pTbldat = (sQUESTITEM_TBLDAT*)pvTable;
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

bool CQuestItemTable::SetTableData( void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData )
{
	if ( 0 == WCHARCmp( pwszSheetName, g_wszTableDataKOR ) )
	{
		sQUESTITEM_TBLDAT* pObj = (sQUESTITEM_TBLDAT*)pvTable;

		if ( 0 == WStringCmpLiteral( *pstrDataName, L"Item_Tblidx" ) )
		{
			pObj->tblidx = READ_TBLIDX( bstrData );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Item_Name" ) )
		{
			pObj->ItemName = READ_DWORD(bstrData);
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Icon_Name" ) )
		{
			READ_STRING(bstrData, pObj->szIconName, _countof(pObj->szIconName));
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Note" ) )
		{
			pObj->Note = READ_DWORD(bstrData);
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Function_Bit_Flag" ) )
		{
			pObj->byFunctionBitFlag = (BYTE)READ_BITFLAG(bstrData);
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


sTBLDAT* CQuestItemTable::FindData( TBLIDX tblidx )
{
	if ( 0 == tblidx ) return NULL;

	TABLEIT iter = m_mapTableList.find( tblidx );
	if ( m_mapTableList.end() == iter ) return NULL;

	return (sTBLDAT*)(iter->second); 
}

bool CQuestItemTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sQUESTITEM_TBLDAT* pTableData = new sQUESTITEM_TBLDAT;
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

bool CQuestItemTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sQUESTITEM_TBLDAT* pTableData = (sQUESTITEM_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}