//***********************************************************************************
//
//	File		:	ObjectTable.cpp
//
//	Begin		:	2006-09-20
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Jeong Ho, Rho ( lleo52@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************


#include "stdafx.h"
#include "ObjectTable.h"
#include "NtlDebug.h"

#include "NtlSerializer.h"

static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) { WCharTLiteralToWCHAR(fmt, dest, destSize); }

const WCHAR* CObjectTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CObjectTable::CObjectTable( void )
{
	Init();
}

CObjectTable::~CObjectTable( void )
{
	Destroy();
}

bool CObjectTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CObjectTable::Destroy( void )
{
	CTable::Destroy();
}

void CObjectTable::Init( void )
{
	m_dwObjectSequence = 0;
}

void* CObjectTable::AllocNewTable( WCHAR* pwszSheetName, DWORD dwCodePage )
{
	if ( 0 == WCHARCmp( pwszSheetName, g_wszTableDataKOR ) )
	{
		sOBJECT_TBLDAT* pNewObj = new sOBJECT_TBLDAT;
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

bool CObjectTable::DeallocNewTable( void* pvTable, WCHAR* pwszSheetName )
{
	if ( 0 == WCHARCmp( pwszSheetName, g_wszTableDataKOR ) )
	{
		sOBJECT_TBLDAT* pObj = (sOBJECT_TBLDAT*)pvTable;
		if ( IsBadReadPtr( pObj, sizeof(*pObj) ) ) return false;

		delete pObj;

		return true;
	}

	return false;
}

bool CObjectTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bUpdate);

	sOBJECT_TBLDAT * pTbldat = (sOBJECT_TBLDAT*) pvTable;
	sOBJECT_TBLDAT * pExistTbldat = NULL;

	if (CNtlVector::ZERO == pTbldat->vDir)
	{
		pTbldat->vDir = CNtlVector::UNIT_X;
	}
	else
	{
		if ( false == pTbldat->vDir.SafeNormalize() )
		{
			_ASSERTE( 0 );
			return false;
		}
	}

	pTbldat->fRadius = ( ( pTbldat->vMax - pTbldat->vMin ) * 0.5).Length();


	if( bReload )
	{
		pExistTbldat = (sOBJECT_TBLDAT*) FindData( pTbldat->tblidx );
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


	pTbldat->dwSequence = m_dwObjectSequence++;


	return true;
}

bool CObjectTable::SetTableData( void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData )
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
				_ASSERTE( !"BYTE " );		\
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
		sOBJECT_TBLDAT* pObj = (sOBJECT_TBLDAT*)pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		static const WCHAR g_wszStatePrefix[] = { 'S', 't', 'a', 't', 'e', 0 };
		static const WCHAR g_wszStateFormat[] = { 'S', 't', 'a', 't', 'e', '%', 'd', 0 };
		static const WCHAR g_wszClickSoundPrefix[] = { 'C', 'l', 'i', 'c', 'k', '_', 'S', 'o', 'u', 'n', 'd', 0 };
		static const WCHAR g_wszClickSoundFormat[] = { 'C', 'l', 'i', 'c', 'k', '_', 'S', 'o', 'u', 'n', 'd', '%', 'd', 0 };

		if ( 0 == WStringCmpLiteral( *pstrDataName, L"Tblidx" ) )
		{
			pObj->tblidx = READ_DWORD( bstrData );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Name" ) )
		{
			pObj->dwName = READ_DWORD( bstrData );
		}
//		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Note" ) )
//		{
//			pObj->dwNote = READ_DWORD( bstrData );
//		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Loc_X" ) )
		{
			pObj->vLoc.x = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Loc_Y" ) )
		{
			pObj->vLoc.y = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Loc_Z" ) )
		{
			pObj->vLoc.z = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Dir_X" ) )
		{
			pObj->vDir.x = READ_FLOAT( bstrData, wszFieldNameBuf, 0.0f );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Dir_Y" ) )
		{
			pObj->vDir.y = READ_FLOAT( bstrData, wszFieldNameBuf, 0.0f );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Dir_Z" ) )
		{
			pObj->vDir.z = READ_FLOAT( bstrData, wszFieldNameBuf, 0.0f );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Func" ) )
		{
			pObj->dwFunction = (DWORD)READ_BITFLAG( bstrData, 0 );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Min_X" ) )
		{
			pObj->vMin.x = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Min_Y" ) )
		{
			pObj->vMin.y = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Min_Z" ) )
		{
			pObj->vMin.z = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Max_X" ) )
		{
			pObj->vMax.x = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Max_Y" ) )
		{
			pObj->vMax.y = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Max_Z" ) )
		{
			pObj->vMax.z = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"StateType"))
		{
			pObj->byStateType = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"DefMainState"))
		{
			pObj->byDefMainState = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"DefSubState"))
		{
			pObj->byDefSubState = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszStatePrefix, WCHARLen(g_wszStatePrefix)) )
		{
			bool bFound = false;
			WCHAR szBuffer[1024] = { 0x00, };

			for( int i = 0; i < DBO_MAX_OBJECT_STATE; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszStateFormat, i );

				if( 0 == WCHARCmp( wszFieldNameBuf, szBuffer ) )
				{
					READ_STATE( i );
					bFound = true;
					break;
				}
			}

			if( false == bFound )
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
		}
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszClickSoundPrefix, WCHARLen(g_wszClickSoundPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < DBO_MAX_OBJECT_STATE; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszClickSoundFormat, i );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					READ_STRING(bstrData, pObj->achClickSound[ i ], _countof(pObj->achClickSound[ i ]));
					bFound = true;
					break;
				}
			}

			if( false == bFound )
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Boundary_Distance" ) )
		{
			pObj->byBoundaryDistance = READ_BYTE( bstrData, wszFieldNameBuf );
		}		
		else if (0 == WStringCmpLiteral(*pstrDataName, L"ModelName"))
		{
			CheckNegativeInvalid( wszFieldNameBuf, bstrData );

			READ_STRING(bstrData, pObj->szModelName, _countof(pObj->szModelName));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Note"))
		{
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Contents_Tblidx" ) )
		{
			pObj->contentsTblidx = READ_DWORD( bstrData );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Object_Direction_Index" ) )
		{
			pObj->objectDirectionIndex = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"MinQuestId"))
		{
			pObj->minQuestId = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"MaxQuestId"))
		{
			pObj->maxQuestId = READ_WORD( bstrData, wszFieldNameBuf );
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


sTBLDAT* CObjectTable::FindData( TBLIDX tblidx )
{
	TABLEIT iter = m_mapTableList.find( tblidx );
	if ( m_mapTableList.end() == iter ) return NULL;

	return (sTBLDAT*)(iter->second); 
}



bool CObjectTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sOBJECT_TBLDAT* pTableData = new sOBJECT_TBLDAT;
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

	//	printf("pTableData->tblidx %d %d %d %d %d\n", pTableData->tblidx,pTableData->dwFunction ,pTableData->dwSequence, pTableData->objectDirectionIndex, pTableData->dwUnknown3);
		if( false == AddTable(pTableData, bReload, bUpdate) )
		{
			delete pTableData;
		}

	} while (false != bLoop);

	return true;
}

bool CObjectTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sOBJECT_TBLDAT* pTableData = (sOBJECT_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}