//***********************************************************************************
//
//	File		:	RankBattleTable.cpp
//
//	Begin		:	2007-06-07
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Ju-hyoung   ( niam@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************

#include "stdafx.h"
#include "RankBattleTable.h"

#include "NtlDebug.h"
#include "NtlSerializer.h"

#include "NtlWorld.h"

static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) { WCharTLiteralToWCHAR(fmt, dest, destSize); }

//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
const WCHAR* CRankBattleTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CRankBattleTable::CRankBattleTable( void )
{
	Init();
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CRankBattleTable::~CRankBattleTable( void )
{
	Destroy();
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CRankBattleTable::Init( void )
{
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CRankBattleTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CRankBattleTable::Destroy( void )
{
	CTable::Destroy();
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void* CRankBattleTable::AllocNewTable( WCHAR* pwszSheetName, DWORD dwCodePage )
{
	if ( 0 == WCHARCmp( pwszSheetName, g_wszTableDataKOR ) )
	{
		sRANKBATTLE_TBLDAT* pNewObj = new sRANKBATTLE_TBLDAT;
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
bool CRankBattleTable::DeallocNewTable( void* pvTable, WCHAR* pwszSheetName )
{
	if ( 0 == WCHARCmp( pwszSheetName, g_wszTableDataKOR ) )
	{
		sRANKBATTLE_TBLDAT* pObj = (sRANKBATTLE_TBLDAT*)pvTable;
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
bool CRankBattleTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);

	sRANKBATTLE_TBLDAT * pTbldat = (sRANKBATTLE_TBLDAT*)pvTable;

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
bool CRankBattleTable::SetTableData( void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData )
{
	static char szTemp[1024] = { 0x00, };

	if ( 0 == WCHARCmp( pwszSheetName, g_wszTableDataKOR ) )
	{
		sRANKBATTLE_TBLDAT * pTbldat = (sRANKBATTLE_TBLDAT*) pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		if ( 0 == WStringCmpLiteral( *pstrDataName, L"Tblidx" ) )
		{
			pTbldat->tblidx = READ_TBLIDX( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Name"))
		{
			CheckNegativeInvalid( wszFieldNameBuf, bstrData );

			READ_STRINGW( bstrData, pTbldat->wszName, _countof(pTbldat->wszName) );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Rule_Type" ) )
		{
			BYTE byMatchRule = READ_BYTE( bstrData, wszFieldNameBuf );
			if( 1 == byMatchRule )
			{
				pTbldat->byRuleType = GAMERULE_RANKBATTLE;				
			}
			else if( 100 == byMatchRule )
			{
				pTbldat->byRuleType = GAMERULE_MINORMATCH;
			}
			else if( 101 == byMatchRule )
			{
				pTbldat->byRuleType = GAMERULE_MAJORMATCH;
			}
			else if( 102 == byMatchRule )
			{
				pTbldat->byRuleType = GAMERULE_FINALMATCH;
			}
			else
			{
				WCHAR wszRuleFmt[512];
			FormatStringToWCHAR(L"[File] : %s\n[Error] : invalid \"Rule_Type\"[%u] (Field Name = %s)", wszRuleFmt, sizeof(wszRuleFmt)/sizeof(WCHAR));
			CTable::CallErrorCallbackFunction(wszRuleFmt, m_wszXmlFileName, byMatchRule, wszFieldNameBuf);
				return false;
			}
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Battle_Mode" ) )
		{
			pTbldat->byBattleMode = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Map_Index" ) )
		{
			pTbldat->worldTblidx = READ_TBLIDX( bstrData );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Need_Item" ) )
		{
			pTbldat->needItemTblidx = READ_TBLIDX( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Need_Zenny"))
		{
			pTbldat->dwZenny = READ_DWORD( bstrData );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Min_Level" ) )
		{
			pTbldat->byMinLevel = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Max_Level" ) )
		{
			pTbldat->byMaxLevel = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Battle_Count" ) )
		{
			pTbldat->byBattleCount = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"WaitTime"))
		{
			pTbldat->dwWaitTime = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"DirectionTime"))
		{
			pTbldat->dwDirectionTime = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"MatchReadyTime"))
		{
			pTbldat->dwMatchReadyTime = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"StageReadyTime"))
		{
			pTbldat->dwStageReadyTime = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"StageRunTime"))
		{
			pTbldat->dwStageRunTime = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"StageFinishTime"))
		{
			pTbldat->dwStageFinishTime = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"MatchFinishTime"))
		{
			pTbldat->dwMatchFinishTime = READ_DWORD( bstrData );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"BossDirection_Time" ) )
		{
			pTbldat->dwBossDirectionTime = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"BossKill_Time"))
		{
			pTbldat->dwBossKillTime = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"BossEndingTime"))
		{
			pTbldat->dwBossEndingTime = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"EndTime"))
		{
			pTbldat->dwEndTime = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"KO_Score"))
		{
			pTbldat->chScoreKO = READ_CHAR( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"OutofArea_Score"))
		{
			pTbldat->chScoreOutOfArea= READ_CHAR( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Pointwin_Score"))
		{
			pTbldat->chScorePointWin = READ_CHAR( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Draw_Score"))
		{
			pTbldat->chScoreDraw = READ_CHAR( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Lost_Score"))
		{
			pTbldat->chScoreLose = READ_CHAR( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Excellent_Result"))
		{
			pTbldat->chResultExcellent = READ_CHAR( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Greate_Result"))
		{
			pTbldat->chResultGreate = READ_CHAR( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Good_Result"))
		{
			pTbldat->chResultGood = READ_CHAR( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Draw_Result"))
		{
			pTbldat->chResultDraw = READ_CHAR( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Lost_Result"))
		{
			pTbldat->chResultLose = READ_CHAR( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"PerfectWinner_Score"))
		{
			pTbldat->chBonusPerfectWinner = READ_CHAR( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"NormalWinner_Score"))
		{
			pTbldat->chBonusNormalWinner = READ_CHAR( bstrData, wszFieldNameBuf );
		}

		//new
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Day_Entry_Num"))
		{
			pTbldat->byDayEntryNum = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"OutSide_Able"))
		{
			pTbldat->bOutSizeAble = READ_BOOL( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Info_Index"))
		{
			pTbldat->dwInfoIndex = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"StageMinClearTime"))
		{
			pTbldat->dwStateMinClearTime = READ_DWORD( bstrData );
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


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
sTBLDAT* CRankBattleTable::FindData( TBLIDX tblidx )
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
bool CRankBattleTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sRANKBATTLE_TBLDAT* pTableData = new sRANKBATTLE_TBLDAT;
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

	//	printf("CRankBattleTable: pTableData->tblidx %d\n", pTableData->tblidx);
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
bool CRankBattleTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sRANKBATTLE_TBLDAT* pTableData = (sRANKBATTLE_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}
