//***********************************************************************************
//
//	File		:	TimeQuestTable.cpp
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
#include "TimeQuestTable.h"

#include "NtlDebug.h"
#include "NtlSerializer.h"

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
const WCHAR* CTimeQuestTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CTimeQuestTable::CTimeQuestTable( void )
{
	Init();
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CTimeQuestTable::~CTimeQuestTable( void )
{
	Destroy();
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CTimeQuestTable::Init( void )
{
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CTimeQuestTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CTimeQuestTable::Destroy( void )
{
	CTable::Destroy();
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void* CTimeQuestTable::AllocNewTable( WCHAR* pwszSheetName, DWORD dwCodePage )
{
	if ( 0 == WCHARCmp( pwszSheetName, g_wszTableDataKOR ) )
	{
		sTIMEQUEST_TBLDAT* pNewObj = new sTIMEQUEST_TBLDAT;
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
bool CTimeQuestTable::DeallocNewTable( void* pvTable, WCHAR* pwszSheetName )
{
	if ( 0 == WCHARCmp( pwszSheetName, g_wszTableDataKOR ) )
	{
		sTIMEQUEST_TBLDAT* pObj = (sTIMEQUEST_TBLDAT*)pvTable;
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
bool CTimeQuestTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);

	sTIMEQUEST_TBLDAT * pTbldat = (sTIMEQUEST_TBLDAT*)pvTable;

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
bool CTimeQuestTable::SetTableData( void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData )
{
	static char szTemp[1024] = { 0x00, };

	if ( 0 == WCHARCmp( pwszSheetName, g_wszTableDataKOR ) )
	{
		sTIMEQUEST_TBLDAT * pTbldat = (sTIMEQUEST_TBLDAT*) pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		static const WCHAR g_wszNamePrefix[] = { 'N', 'a', 'm', 'e', '_', 0 };
		static const WCHAR g_wszQuestStringTblidxPrefix[] = { 'Q', 'u', 'e', 's', 't', 'S', 't', 'r', 'i', 'n', 'g', 'T', 'b', 'l', 'i', 'd', 'x', '_', 0 };
		static const WCHAR g_wszWorldTblidxPrefix[] = { 'W', 'o', 'r', 'l', 'd', 'T', 'b', 'l', 'i', 'd', 'x', '_', 0 };
		static const WCHAR g_wszScriptTblidxPrefix[] = { 'S', 'c', 'r', 'i', 'p', 't', 'T', 'b', 'l', 'i', 'd', 'x', '_', 0 };
		static const WCHAR g_wszLimitTimePrefix[] = { 'L', 'i', 'm', 'i', 't', 'T', 'i', 'm', 'e', '_', 0 };
		static const WCHAR g_wszMinMemberCountPrefix[] = { 'M', 'i', 'n', 'M', 'e', 'm', 'b', 'e', 'r', 'C', 'o', 'u', 'n', 't', '_', 0 };
		static const WCHAR g_wszMaxMemberCountPrefix[] = { 'M', 'a', 'x', 'M', 'e', 'm', 'b', 'e', 'r', 'C', 'o', 'u', 'n', 't', '_', 0 };
		static const WCHAR g_wszMinMemberLevelPrefix[] = { 'M', 'i', 'n', 'M', 'e', 'm', 'b', 'e', 'r', 'L', 'e', 'v', 'e', 'l', '_', 0 };
		static const WCHAR g_wszMaxMemberLevelPrefix[] = { 'M', 'a', 'x', 'M', 'e', 'm', 'b', 'e', 'r', 'L', 'e', 'v', 'e', 'l', '_', 0 };
		static const WCHAR g_wszNeedZennyPrefix[] = { 'N', 'e', 'e', 'd', 'Z', 'e', 'n', 'n', 'y', '_', 0 };
		static const WCHAR g_wszNeedItemPrefix[] = { 'N', 'e', 'e', 'd', 'I', 't', 'e', 'm', '_', 0 };
		static const WCHAR g_wszStartTriggerDirectionStatePrefix[] = { 'S', 't', 'a', 'r', 't', '_', 'T', 'r', 'i', 'g', 'g', 'e', 'r', '_', 'D', 'i', 'r', 'e', 'c', 't', 'i', 'o', 'n', '_', 'S', 't', 'a', 't', 'e', 0 };
		static const WCHAR g_wszStartTriggerDirectionStateFormat[] = { 'S', 't', 'a', 'r', 't', '_', 'T', 'r', 'i', 'g', 'g', 'e', 'r', '_', 'D', 'i', 'r', 'e', 'c', 't', 'i', 'o', 'n', '_', 'S', 't', 'a', 't', 'e', '%', 'd', 0 };
		static const WCHAR g_wszNeedLimitCountPrefix[] = { 'N', 'e', 'e', 'd', 'L', 'i', 'm', 'i', 't', 'C', 'o', 'u', 'n', 't', '_', 0 };
		static const WCHAR g_wszWorldCountPrefix[] = { 'W', 'o', 'r', 'l', 'd', '_', 'C', 'o', 'u', 'n', 't', '_', 0 };
		static const WCHAR g_wszDayRecordRewardPrefix[] = { 'D', 'a', 'y', 'R', 'e', 'c', 'o', 'r', 'd', '_', 'R', 'e', 'w', 'a', 'r', 'd', '_', 0 };
		static const WCHAR g_wszBestRecordRewardPrefix[] = { 'B', 'e', 's', 't', 'R', 'e', 'c', 'o', 'r', 'd', '_', 'R', 'e', 'w', 'a', 'r', 'd', '_', 0 };

		if ( 0 == WStringCmpLiteral(*pstrDataName, L"Tblidx") )
		{
			pTbldat->tblidx = READ_TBLIDX( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Type"))
		{
			pTbldat->byTimeQuestType = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Difficultyflag") )
		{
			pTbldat->byDifficultyFlag = (BYTE) READ_BITFLAG( bstrData );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"StartTime") )
		{
			pTbldat->dwStartTime = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Note"))
		{
			// ??? ???? ???
		}
		else if( 0 == WCHARNCmp(wszFieldNameBuf, g_wszNamePrefix, WCHARLen(g_wszNamePrefix)) )
		{
			if (0 == WStringCmpLiteral(*pstrDataName, L"Name_Easy"))
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_EASY ];
				pTimeQuestDataSet->nameTblidx = READ_TBLIDX( bstrData );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"Name_Normal") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_NORMAL ];
				pTimeQuestDataSet->nameTblidx = READ_TBLIDX( bstrData );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"Name_Hard") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_HARD ];
				pTimeQuestDataSet->nameTblidx = READ_TBLIDX( bstrData );
			}
			else
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
		}
		else if( 0 == WCHARNCmp(wszFieldNameBuf, g_wszQuestStringTblidxPrefix, WCHARLen(g_wszQuestStringTblidxPrefix)) )
		{
			if (0 == WStringCmpLiteral(*pstrDataName, L"QuestStringTblidx_Easy"))
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_EASY ];
				pTimeQuestDataSet->questStringTblidx = READ_TBLIDX( bstrData );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"QuestStringTblidx_Normal") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_NORMAL ];
				pTimeQuestDataSet->questStringTblidx = READ_TBLIDX( bstrData );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"QuestStringTblidx_Hard") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_HARD ];
				pTimeQuestDataSet->questStringTblidx = READ_TBLIDX( bstrData );
			}
			else
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
		}
		else if( 0 == WCHARNCmp(wszFieldNameBuf, g_wszWorldTblidxPrefix, WCHARLen(g_wszWorldTblidxPrefix)) )
		{
			if (0 == WStringCmpLiteral(*pstrDataName, L"WorldTblidx_Easy"))
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_EASY ];
				pTimeQuestDataSet->worldTblidx = READ_TBLIDX( bstrData );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"WorldTblidx_Normal") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_NORMAL ];
				pTimeQuestDataSet->worldTblidx = READ_TBLIDX( bstrData );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"WorldTblidx_Hard") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_HARD ];
				pTimeQuestDataSet->worldTblidx = READ_TBLIDX( bstrData );
			}
			else
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
		}
		else if( 0 == WCHARNCmp(wszFieldNameBuf, g_wszScriptTblidxPrefix, WCHARLen(g_wszScriptTblidxPrefix)) )
		{
			if (0 == WStringCmpLiteral(*pstrDataName, L"ScriptTblidx_Easy"))
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_EASY ];
				pTimeQuestDataSet->scriptTblidx = READ_TBLIDX( bstrData );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"ScriptTblidx_Normal") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_NORMAL ];
				pTimeQuestDataSet->scriptTblidx = READ_TBLIDX( bstrData );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"ScriptTblidx_Hard") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_HARD ];
				pTimeQuestDataSet->scriptTblidx = READ_TBLIDX( bstrData );
			}
			else
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
		}
		else if( 0 == WCHARNCmp(wszFieldNameBuf, g_wszLimitTimePrefix, WCHARLen(g_wszLimitTimePrefix)) )
		{
			if (0 == WStringCmpLiteral(*pstrDataName, L"LimitTime_Easy"))
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_EASY ];
				pTimeQuestDataSet->dwLimitTime = READ_DWORD( bstrData );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"LimitTime_Normal") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_NORMAL ];
				pTimeQuestDataSet->dwLimitTime = READ_DWORD( bstrData );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"LimitTime_Hard") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_HARD ];
				pTimeQuestDataSet->dwLimitTime = READ_DWORD( bstrData );
			}
			else
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
		}
		else if( 0 == WCHARNCmp(wszFieldNameBuf, g_wszMinMemberCountPrefix, WCHARLen(g_wszMinMemberCountPrefix)) )
		{
			if (0 == WStringCmpLiteral(*pstrDataName, L"MinMemberCount_Easy"))
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_EASY ];
				pTimeQuestDataSet->byMinMemberCount = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"MinMemberCount_Normal") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_NORMAL ];
				pTimeQuestDataSet->byMinMemberCount = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"MinMemberCount_Hard") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_HARD ];
				pTimeQuestDataSet->byMinMemberCount = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
		}
		else if( 0 == WCHARNCmp(wszFieldNameBuf, g_wszMaxMemberCountPrefix, WCHARLen(g_wszMaxMemberCountPrefix)) )
		{
			if (0 == WStringCmpLiteral(*pstrDataName, L"MaxMemberCount_Easy"))
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_EASY ];
				pTimeQuestDataSet->byMaxMemberCount = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"MaxMemberCount_Normal") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_NORMAL ];
				pTimeQuestDataSet->byMaxMemberCount = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"MaxMemberCount_Hard") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_HARD ];
				pTimeQuestDataSet->byMaxMemberCount = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
		}
		else if( 0 == WCHARNCmp(wszFieldNameBuf, g_wszMinMemberLevelPrefix, WCHARLen(g_wszMinMemberLevelPrefix)) )
		{
			if (0 == WStringCmpLiteral(*pstrDataName, L"MinMemberLevel_Easy"))
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_EASY ];
				pTimeQuestDataSet->byMinMemberLevel = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"MinMemberLevel_Normal") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_NORMAL ];
				pTimeQuestDataSet->byMinMemberLevel = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"MinMemberLevel_Hard") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_HARD ];
				pTimeQuestDataSet->byMinMemberLevel = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
		}
		else if( 0 == WCHARNCmp(wszFieldNameBuf, g_wszMaxMemberLevelPrefix, WCHARLen(g_wszMaxMemberLevelPrefix)) )
		{
			if (0 == WStringCmpLiteral(*pstrDataName, L"MaxMemberLevel_Easy"))
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_EASY ];
				pTimeQuestDataSet->byMaxMemberLevel = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"MaxMemberLevel_Normal") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_NORMAL ];
				pTimeQuestDataSet->byMaxMemberLevel = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"MaxMemberLevel_Hard") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_HARD ];
				pTimeQuestDataSet->byMaxMemberLevel = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
		}
		else if( 0 == WCHARNCmp(wszFieldNameBuf, g_wszNeedZennyPrefix, WCHARLen(g_wszNeedZennyPrefix)) )
		{
			if (0 == WStringCmpLiteral(*pstrDataName, L"NeedZenny_Easy"))
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_EASY ];
				pTimeQuestDataSet->dwNeedZenny = READ_DWORD( bstrData );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"NeedZenny_Normal") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_NORMAL ];
				pTimeQuestDataSet->dwNeedZenny = READ_DWORD( bstrData );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"NeedZenny_Hard") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_HARD ];
				pTimeQuestDataSet->dwNeedZenny = READ_DWORD( bstrData );
			}
			else
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
		}
		else if( 0 == WCHARNCmp(wszFieldNameBuf, g_wszNeedItemPrefix, WCHARLen(g_wszNeedItemPrefix)) )
		{
			if (0 == WStringCmpLiteral(*pstrDataName, L"NeedItem_Easy"))
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_EASY ];
				pTimeQuestDataSet->needItemTblidx = READ_TBLIDX( bstrData );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"NeedItem_Normal") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_NORMAL ];
				pTimeQuestDataSet->needItemTblidx = READ_TBLIDX( bstrData );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"NeedItem_Hard") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_HARD ];
				pTimeQuestDataSet->needItemTblidx = READ_TBLIDX( bstrData );
			}
			else
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Start_Character_Direction" ) )
		{
			pTbldat->startCharacterDirection = READ_DWORD( bstrData );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Start_Object_Index" ) )
		{
			pTbldat->startObjectIndex = READ_DWORD( bstrData );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Leave_Object_Index" ) )
		{
			pTbldat->leaveObjectIndex = READ_DWORD( bstrData );
		}		
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Start_Trigger_Id" ) )
		{
			pTbldat->startTriggerId = READ_DWORD( bstrData );
		}


		else if( 0 == WCHARNCmp(wszFieldNameBuf, g_wszStartTriggerDirectionStatePrefix, WCHARLen(g_wszStartTriggerDirectionStatePrefix)) )
		{
			WCHAR wszFieldName[1024 + 1];

			for (BYTE i = 0 ; i < 10 ; i++)
			{
				NTL_SWPRINTF(wszFieldName, 1024, g_wszStartTriggerDirectionStateFormat, i + 1);

				if (0 == WCHARCmp(wszFieldNameBuf, wszFieldName))
				{
					pTbldat->abyStartTriggerDirectionState[i] = READ_BYTE(bstrData, wszFieldNameBuf);
				}
			}
		}


		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Leave_Trigger_Id" ) )
		{
			pTbldat->leaveTriggerId = READ_DWORD( bstrData );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Arrive_Character_Direction" ) )
		{
			pTbldat->arriveCharacterDirection = READ_DWORD( bstrData );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Leave_Character_Direction" ) )
		{
			pTbldat->leaveCharacterDirection = READ_DWORD( bstrData );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Arrive_Object_Index" ) )
		{
			pTbldat->arriveObjectIndex = READ_DWORD( bstrData );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Arrive_Trigger_Id" ) )
		{
			pTbldat->arriveTriggerId = READ_DWORD( bstrData );
		}

		else if( 0 == WCHARNCmp(wszFieldNameBuf, g_wszNeedLimitCountPrefix, WCHARLen(g_wszNeedLimitCountPrefix)) )
		{
			if (0 == WStringCmpLiteral(*pstrDataName, L"NeedLimitCount_Easy"))
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_EASY ];
				pTimeQuestDataSet->byNeedLimitCount = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"NeedLimitCount_Normal") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_NORMAL ];
				pTimeQuestDataSet->byNeedLimitCount = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"NeedLimitCount_Hard") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_HARD ];
				pTimeQuestDataSet->byNeedLimitCount = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
		}

		else if( 0 == WCHARNCmp(wszFieldNameBuf, g_wszWorldCountPrefix, WCHARLen(g_wszWorldCountPrefix)) )
		{
			if (0 == WStringCmpLiteral(*pstrDataName, L"World_Count_Easy"))
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_EASY ];
				pTimeQuestDataSet->byWorldCount = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"World_Count_Normal") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_NORMAL ];
				pTimeQuestDataSet->byWorldCount = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"World_Count_Hard") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_HARD ];
				pTimeQuestDataSet->byWorldCount = READ_BYTE( bstrData, wszFieldNameBuf );
			}
			else
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
		}
		else if( 0 == WCHARNCmp(wszFieldNameBuf, g_wszDayRecordRewardPrefix, WCHARLen(g_wszDayRecordRewardPrefix)) )
		{
			if (0 == WStringCmpLiteral(*pstrDataName, L"DayRecord_Reward_Easy"))
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_EASY ];
				pTimeQuestDataSet->dayRecordRewardTblidx = READ_TBLIDX( bstrData );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"DayRecord_Reward_Normal") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_NORMAL ];
				pTimeQuestDataSet->dayRecordRewardTblidx = READ_TBLIDX( bstrData );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"DayRecord_Reward_Hard") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_HARD ];
				pTimeQuestDataSet->dayRecordRewardTblidx = READ_TBLIDX( bstrData );
			}
			else
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
		}
		else if( 0 == WCHARNCmp(wszFieldNameBuf, g_wszBestRecordRewardPrefix, WCHARLen(g_wszBestRecordRewardPrefix)) )
		{
			if (0 == WStringCmpLiteral(*pstrDataName, L"BestRecord_Reward_Easy"))
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_EASY ];
				pTimeQuestDataSet->bestRecordRewardTblidx = READ_TBLIDX( bstrData );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"BestRecord_Reward_Normal") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_NORMAL ];
				pTimeQuestDataSet->bestRecordRewardTblidx = READ_TBLIDX( bstrData );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"BestRecord_Reward_Hard") )
			{
				sTIMEQUEST_DATASET * pTimeQuestDataSet = &pTbldat->sTimeQuestDataset[ TIMEQUEST_DIFFICULTY_HARD ];
				pTimeQuestDataSet->bestRecordRewardTblidx = READ_TBLIDX( bstrData );
			}
			else
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"DayRecord_MailIndex" ) )
		{
			pTbldat->dayRecordMailTblidx = READ_TBLIDX( bstrData );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"BestRecord_MailIndex" ) )
		{
			pTbldat->bestRecordMailTblidx = READ_TBLIDX( bstrData );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Reset_Time" ) )
		{
			pTbldat->byResetTime = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Prologue_Direction" ) )
		{
			READ_STRINGW(bstrData, pTbldat->wszPrologueDirection, _countof(pTbldat->wszPrologueDirection));			
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Open_Cine" ) )
		{
			pTbldat->openCine = READ_TBLIDX( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Note"))
		{
			pTbldat->Note = READ_DWORD(bstrData);
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Stage_BGM1" ) )
		{
			READ_STRINGW(bstrData, pTbldat->wszStageBgm1, _countof(pTbldat->wszStageBgm1));			
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Stage_BGM2" ) )
		{
			READ_STRINGW(bstrData, pTbldat->wszStageBgm2, _countof(pTbldat->wszStageBgm2));			
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Last_BGM" ) )
		{
			READ_STRINGW(bstrData, pTbldat->wszLastBgm, _countof(pTbldat->wszLastBgm));			
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
		_ASSERT( 0 );
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
sTBLDAT* CTimeQuestTable::FindData( TBLIDX tblidx )
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
bool CTimeQuestTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sTIMEQUEST_TBLDAT* pTableData = new sTIMEQUEST_TBLDAT;
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
bool CTimeQuestTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sTIMEQUEST_TBLDAT* pTableData = (sTIMEQUEST_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}