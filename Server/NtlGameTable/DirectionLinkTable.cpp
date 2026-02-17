//***********************************************************************************
//
//	File		:	DirectionLinkTable.cpp
//
//	Begin		:	2007-06-01
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Hyun Woo, Koo   ( zeroera@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************

#include "stdafx.h"
#include "DirectionLinkTable.h"

#include "NtlDebug.h"
#include "NtlSerializer.h"
#include "NtlBitFlag.h"


// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
const WCHAR* CDirectionLinkTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CDirectionLinkTable::CDirectionLinkTable( void )
{
	Init();
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CDirectionLinkTable::~CDirectionLinkTable( void )
{
	Destroy();
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CDirectionLinkTable::Init( void )
{
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CDirectionLinkTable::Create(DWORD dwCodePage)
{
	if( false == CTable::Create(dwCodePage) )
	{
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CDirectionLinkTable::Destroy( void )
{
	CTable::Destroy();
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void* CDirectionLinkTable::AllocNewTable( WCHAR* pwszSheetName, DWORD dwCodePage )
{
	if ( 0 == WCHARCmp( pwszSheetName, g_wszTableDataKOR ) )
	{
		sDIRECTION_LINK_TBLDAT* pNewObj = new sDIRECTION_LINK_TBLDAT;
		if ( NULL == pNewObj )
		{
			return NULL;
		}

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


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CDirectionLinkTable::DeallocNewTable( void* pvTable, WCHAR* pwszSheetName )
{
	if ( 0 == WCHARCmp( pwszSheetName, g_wszTableDataKOR ) )
	{
		sDIRECTION_LINK_TBLDAT* pObj = (sDIRECTION_LINK_TBLDAT*)pvTable;
		if ( IsBadReadPtr( pObj, sizeof(*pObj) ) ) return false;

		delete pObj;

		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CDirectionLinkTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bUpdate);

	sDIRECTION_LINK_TBLDAT * pTbldat = (sDIRECTION_LINK_TBLDAT*)pvTable;
	sDIRECTION_LINK_TBLDAT * pExistTbldat = NULL;

	if( bReload )
	{
		pExistTbldat = (sDIRECTION_LINK_TBLDAT*) FindData( pTbldat->tblidx );
		if( pExistTbldat )
		{
			CopyMemory( pTbldat, pExistTbldat, pTbldat->GetDataSize() );
			// return false for release reloaded table data
			return true; 
		}
	}

	if( false == CheckData( pTbldat) )
	{
		_ASSERTE( 0 );
		return false;
	}

	if ( false == m_mapTableList.insert( std::map<TBLIDX, sTBLDAT*>::value_type(pTbldat->tblidx, pTbldat)).second )
	{
		WCHAR wszFormatBuf[512];
		FormatStringToWCHAR(L"[File] : %s\r\n Table Tblidx[%u] is Duplicated ", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
		CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, pTbldat->tblidx );
		_ASSERTE( 0 );
		return false;
	}


	return true;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CDirectionLinkTable::SetTableData( void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData )
{
	static char szTemp[1024] = { 0x00, };

	if ( 0 == WCHARCmp( pwszSheetName, g_wszTableDataKOR ) )
	{
		sDIRECTION_LINK_TBLDAT * pTbldat = (sDIRECTION_LINK_TBLDAT*) pvTable;

		if ( 0 == WStringCmpLiteral(*pstrDataName, L"Tblidx") )
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pTbldat->tblidx = READ_TBLIDX( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Function_Name"))
		{
			READ_STRING( bstrData, pTbldat->szFunctionName, _countof(pTbldat->szFunctionName) );			
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Note"))
		{
			READ_STRING( bstrData, pTbldat->szNote, _countof(pTbldat->szNote) );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Type") )
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pTbldat->byType = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Animation_ID") )
		{
			pTbldat->dwAnimationID = READ_DWORD( bstrData );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Direction_Func_Flag") )
		{
			pTbldat->byFuncFlag = (BYTE) READ_BITFLAG( bstrData );
		}
		else
		{
			_ASSERT( 0 );
			return false;
		}
	}
	else
	{
		_ASSERT( 0 );
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
sTBLDAT* CDirectionLinkTable::FindData( TBLIDX tblidx )
{
	TABLEIT iter = m_mapTableList.find( tblidx );
	if ( m_mapTableList.end() == iter )
	{
		return NULL;
	}

	return (sTBLDAT*)(iter->second); 
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CDirectionLinkTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sDIRECTION_LINK_TBLDAT* pTableData = new sDIRECTION_LINK_TBLDAT;
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


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CDirectionLinkTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sDIRECTION_LINK_TBLDAT* pTableData = (sDIRECTION_LINK_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CDirectionLinkTable::CheckData(sDIRECTION_LINK_TBLDAT * pTbldat)
{
	if( BIT_FLAG_TEST( pTbldat->byFuncFlag, DIRECTION_FUNC_FLAG_TIMEOUT ) &&
		BIT_FLAG_TEST( pTbldat->byFuncFlag, DIRECTION_FUNC_FLAG_KEEPUP ) )
	{
		_ASSERT( 0 );
		WCHAR wszFormatBuf[512];
		FormatStringToWCHAR(L"file[%s] index[%u] : The value error.( byFuncFlag can't set DIRECTION_FUNC_FLAG_TIMEOUT and DIRECTION_FUNC_FLAG_KEEPUP simultaneously)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
		CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, pTbldat->tblidx );
		return false;
	}

	return true;
}