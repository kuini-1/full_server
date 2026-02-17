//***********************************************************************************
//
//	File		:	DojoTable.cpp
//
//	Begin		:	2008-12-29
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Chung,DooSup   ( mailto:john@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************

#include "stdafx.h"
#include "DojoTable.h"

#include "NtlDebug.h"
#include "NtlSerializer.h"


// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };
static const WCHAR g_wszObjectTblidx[] = { 'O', 'b', 'j', 'e', 'c', 't', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 0 };
static const WCHAR g_wszGetPoint[] = { 'G', 'e', 't', '_', 'P', 'o', 'i', 'n', 't', 0 };
static const WCHAR g_wszGetRock[] = { 'G', 'e', 't', '_', 'R', 'o', 'c', 'k', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
const WCHAR* CDojoTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CDojoTable::CDojoTable( void )
{
	Init();
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CDojoTable::~CDojoTable( void )
{
	Destroy();
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CDojoTable::Init( void )
{
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CDojoTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CDojoTable::Destroy( void )
{
	CTable::Destroy();
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void* CDojoTable::AllocNewTable( WCHAR* pwszSheetName, DWORD dwCodePage )
{
	if ( 0 == WCHARCmp( pwszSheetName, g_wszTableDataKOR ) )
	{
		sDOJO_TBLDAT* pNewObj = new sDOJO_TBLDAT;
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
bool CDojoTable::DeallocNewTable( void* pvTable, WCHAR* pwszSheetName )
{
	if ( 0 == WCHARCmp( pwszSheetName, g_wszTableDataKOR ) )
	{
		sDOJO_TBLDAT* pObj = (sDOJO_TBLDAT*)pvTable;
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
bool CDojoTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);

	sDOJO_TBLDAT * pTbldat = (sDOJO_TBLDAT*)pvTable;

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


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CDojoTable::SetTableData( void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData )
{
	static char szTemp[1024] = { 0x00, };

	if ( 0 == WCHARCmp( pwszSheetName, g_wszTableDataKOR ) )
	{
		sDOJO_TBLDAT * pTbldat = (sDOJO_TBLDAT*) pvTable;

		if ( 0 == WStringCmpLiteral(*pstrDataName, L"Tblidx") )
		{
			pTbldat->tblidx = READ_TBLIDX( bstrData );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Zone_Tblidx") )
		{
			pTbldat->zoneTblidx = READ_TBLIDX( bstrData );
		}
		else
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszObjectTblidx, WCHARLen(g_wszObjectTblidx)) )
			{
				bool bFound = false;

				WCHAR szBuffer[1024] = { 0x00, };
				for( int i = 0; i < DOJO_MAX_UPGRADE_OBJECT_COUNT; i++ )
				{
					NTL_SWPRINTF( szBuffer, 1024, L"Object_Tblidx_%d", i + 1 );

					if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
					{
						pTbldat->objectTblidx[ i ] = READ_DWORD( bstrData );

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
			else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Map_Name") )
			{
				pTbldat->mapName = READ_TBLIDX( bstrData );
			}
			else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Receive_Hour") )
			{
				CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
				pTbldat->byReceiveHour = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Receive_Minute") )
			{
				CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
				pTbldat->byReceiveMinute = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Repeat_Type") )
			{
				CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
				pTbldat->byRepeatType = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Repeat_Time") )
			{
				pTbldat->byRepeatTime = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Week_Bit_Flag") )
			{
				CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
				pTbldat->wWeekBitFlag = (WORD)READ_BITFLAG( bstrData );
			}
			else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Receive_Duration") )
			{
				CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
				pTbldat->byReceiveDuration = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Reject_Duration") )
			{
				CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
				pTbldat->byRejectDuration = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Standby_Duration") )
			{
				CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
				pTbldat->byStandbyDuration = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Initial_Duration") )
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pTbldat->byInitialDuration = READ_BYTE( bstrData, pstrDataName->c_str() );
		}
		else if ( 0 == wcscmp( pstrDataName->c_str(), L"Ready_Duration" ) )
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pTbldat->byReadyDuration = READ_BYTE( bstrData, pstrDataName->c_str() );
		}
		else if ( 0 == wcscmp( pstrDataName->c_str(), L"Battle_Duration" ) )
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pTbldat->byBattleDuration = READ_BYTE( bstrData, pstrDataName->c_str() );
		}
		else if (0 == wcscmp(pstrDataName->c_str(), L"Receive_Point"))
		{
			pTbldat->dwReceivePoint = READ_DWORD( bstrData );
		}
		else if (0 == wcscmp(pstrDataName->c_str(), L"Receive_Zeny"))
		{
			pTbldat->dwReceiveZenny = READ_DWORD( bstrData );
		}
		else if (0 == wcscmp(pstrDataName->c_str(), L"Controller_Tblidx"))
		{
			pTbldat->controllerTblidx = READ_DWORD( bstrData );
		}		
		else if (0 == wcscmp(pstrDataName->c_str(), L"Battle_Point_Goal"))
		{
			pTbldat->dwBattlePointGoal = READ_DWORD( bstrData );
		}
		else if (0 == wcscmp(pstrDataName->c_str(), L"Battle_Point_Get"))
		{
			pTbldat->dwBattlePointGet = READ_DWORD( bstrData );
		}
		else if (0 == wcscmp(pstrDataName->c_str(), L"Battle_Point_Charge"))
		{
			pTbldat->dwBattlePointCharge = READ_DWORD( bstrData );
		}
		else if (0 == wcscmp(pstrDataName->c_str(), L"Charge_Point_Goal"))
		{
			pTbldat->dwChargePointGoal = READ_DWORD( bstrData );
		}
		else if (0 == wcscmp(pstrDataName->c_str(), L"Charge_Time"))
		{
			pTbldat->dwChargeTime = READ_DWORD( bstrData );
		}
		else if (0 == wcscmp(pstrDataName->c_str(), L"Charge_Time_Point"))
		{
			pTbldat->dwChageTimePoint = READ_DWORD( bstrData );
		}
		else if (0 == wcscmp(pstrDataName->c_str(), L"Rock_Tblidx"))
		{
			pTbldat->rockTblidx = READ_DWORD( bstrData );
		}		
				if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszGetPoint, WCHARLen(g_wszGetPoint)) )
				{
					bool bFound = false;

					WCHAR szBuffer[1024] = { 0x00, };
					for( int i = 0; i < DOJO_MAX_REWARD_TYPE_COUNT; i++ )
					{
						NTL_SWPRINTF( szBuffer, 1024, L"Get_Point%d", i + 1 );

						if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
						{
							pTbldat->asRawrd[ i ].dwGetPoint = READ_DWORD( bstrData );

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
				else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszGetRock, WCHARLen(g_wszGetRock)) )
				{
					bool bFound = false;

					WCHAR szBuffer[1024] = { 0x00, };
					for( int i = 0; i < DOJO_MAX_REWARD_TYPE_COUNT; i++ )
					{
						NTL_SWPRINTF( szBuffer, 1024, L"Get_Rock%d", i + 1 );

						if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
						{
							pTbldat->asRawrd[ i ].byGetRock = READ_BYTE( bstrData, wszFieldNameBuf );

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
				else
				{
					WCHAR wszFormatBuf[512];
					FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
					CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
					return false;
				}
			}
		}
	}
	else
	{
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
sTBLDAT* CDojoTable::FindData( TBLIDX tblidx )
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
sTBLDAT* CDojoTable::GetDojoData()
{
	TABLEIT iter = m_mapTableList.begin();
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
bool CDojoTable::IsWorldDojo( TBLIDX worldIdx )
{
	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sDOJO_TBLDAT* pTableData = (sDOJO_TBLDAT*)(iter->second);

		if ( worldIdx == pTableData->zoneTblidx )
		{
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
TBLIDX CDojoTable::GetTblidxByWorldIdx(TBLIDX worldIdx )
{
	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sDOJO_TBLDAT* pTableData = (sDOJO_TBLDAT*)(iter->second);

		if ( worldIdx == pTableData->zoneTblidx )
		{
			return pTableData->tblidx;
		}
	}
	return INVALID_TBLIDX;
}

//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
sDOJO_TBLDAT* CDojoTable::GetDojoByMapName(TBLIDX mapname)
{
	TABLE::iterator iter;
	for (iter = Begin(); End() != iter; iter++)
	{
		sDOJO_TBLDAT* pTableData = (sDOJO_TBLDAT*)(iter->second);

		if (mapname == pTableData->mapName)
		{
			return pTableData;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CDojoTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sDOJO_TBLDAT* pTableData = new sDOJO_TBLDAT;
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
bool CDojoTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sDOJO_TBLDAT* pTableData = (sDOJO_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}
