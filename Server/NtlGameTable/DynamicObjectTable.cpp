//***********************************************************************************
//
//	File		:	DynamicObjectTable.cpp
//
//	Begin		:	2008-12-08
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Chung,DooSup ( john@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************


#include "stdafx.h"
#include "DynamicObjectTable.h"
#include "NtlDebug.h"

#include "NtlSerializer.h"

// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CDynamicObjectTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CDynamicObjectTable::CDynamicObjectTable( void )
{
	Init();
}

CDynamicObjectTable::~CDynamicObjectTable( void )
{
	Destroy();
}

bool CDynamicObjectTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CDynamicObjectTable::Destroy( void )
{
	CTable::Destroy();
}

void CDynamicObjectTable::Init( void )
{
	m_dwObjectSequence = 0;
}

void* CDynamicObjectTable::AllocNewTable( WCHAR* pwszSheetName, DWORD dwCodePage )
{
	if ( 0 == WCHARCmp( pwszSheetName, g_wszTableDataKOR ) )
	{
		sDYNAMIC_OBJECT_TBLDAT* pNewObj = new sDYNAMIC_OBJECT_TBLDAT;
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

bool CDynamicObjectTable::DeallocNewTable( void* pvTable, WCHAR* pwszSheetName )
{
	if ( 0 == WCHARCmp( pwszSheetName, g_wszTableDataKOR ) )
	{
		sDYNAMIC_OBJECT_TBLDAT* pObj = (sDYNAMIC_OBJECT_TBLDAT*)pvTable;
		if ( IsBadReadPtr( pObj, sizeof(*pObj) ) ) return false;

		delete pObj;

		return true;
	}

	return false;
}

bool CDynamicObjectTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bUpdate);

	sDYNAMIC_OBJECT_TBLDAT * pTbldat = (sDYNAMIC_OBJECT_TBLDAT*) pvTable;
	sDYNAMIC_OBJECT_TBLDAT * pExistTbldat = NULL;


	if( bReload )
	{
		pExistTbldat = (sDYNAMIC_OBJECT_TBLDAT*) FindData( pTbldat->tblidx );
		if( pExistTbldat )
		{
			CopyMemory( pTbldat, pExistTbldat, pTbldat->GetDataSize() );
			// return false for release reloaded table data
			return true; 
		}
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

bool CDynamicObjectTable::SetTableData( void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData )
{
#define READ_STATE( idx )															\
	if ( READ_STRING( bstrData, szTemp, 1024 ) )									\
	{																				\
		int nCnt = 0;																\
		char *pToken, *pNextToken;													\
																					\
		pToken = NTL_STRTOK( szTemp, ";", &pNextToken );								\
																					\
		while ( NULL != pToken )													\
		{																			\
			DWORD dwTemp = (DWORD)_atoi64( pToken );								\
																					\
			if ( dwTemp >= INVALID_BYTE )											\
			{																		\
				_ASSERTE( !"BYTE ????? ????? ???? ??? ?? ?????????." );		\
			}																		\
			else																	\
			{																		\
				pObj->abyState[idx][nCnt] = (BYTE)dwTemp;							\
			}																		\
																					\
			nCnt++;																	\
																					\
			pToken = NTL_STRTOK( NULL, ";", &pNextToken );							\
		}																			\
	}

	static char szTemp[1024];

	if ( 0 == WCHARCmp( pwszSheetName, g_wszTableDataKOR ) )
	{
		sDYNAMIC_OBJECT_TBLDAT* pObj = (sDYNAMIC_OBJECT_TBLDAT*)pvTable;

		if ( 0 == WStringCmpLiteral(*pstrDataName, L"Tblidx") )
		{
			pObj->tblidx = READ_DWORD( bstrData );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Validity_Able") )
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pObj->bValidityAble = READ_BOOL( bstrData, wszFieldNameBuf );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Type") )
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pObj->byType = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Model_Name"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );

			READ_STRING(bstrData, pObj->szModelName, _countof(pObj->szModelName));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"State_Type"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pObj->byStateType = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Spawn_Animation") )
		{
			pObj->spawnAnimation = READ_DWORD( bstrData );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Idle_Animation") )
		{
			pObj->idleAnimation = READ_DWORD( bstrData );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Despawn_Animation") )
		{
			pObj->despawnAnimation = READ_DWORD( bstrData );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"State1_Animation") )
		{
			pObj->state1Animation = READ_DWORD( bstrData );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"State2_Animation") )
		{
			pObj->state2Animation = READ_DWORD( bstrData );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Boundary_Distance") )
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pObj->byBoundaryDistance = READ_BYTE( bstrData, wszFieldNameBuf );
		}		
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Despawn_Distance") )
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pObj->byDespawnDistance = READ_BYTE( bstrData, wszFieldNameBuf );
		}	
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Note"))
		{
		}
		else
		{
			WCHAR wszFormatBuf[512];
			FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
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


sTBLDAT* CDynamicObjectTable::FindData( TBLIDX tblidx )
{
	TABLEIT iter = m_mapTableList.find( tblidx );
	if ( m_mapTableList.end() == iter ) return NULL;

	return (sTBLDAT*)(iter->second); 
}

bool CDynamicObjectTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sDYNAMIC_OBJECT_TBLDAT* pTableData = new sDYNAMIC_OBJECT_TBLDAT;
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

bool CDynamicObjectTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sDYNAMIC_OBJECT_TBLDAT* pTableData = (sDYNAMIC_OBJECT_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}