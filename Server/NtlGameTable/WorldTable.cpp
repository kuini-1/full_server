//***********************************************************************************
//
//	File		:	WorldTable.cpp
//
//	Begin		:	2006-03-09
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Hyun Woo, Koo   ( zeroera@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************

#include "stdafx.h"
#include "WorldTable.h"

#include "NtlSerializer.h"

// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CWorldTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CWorldTable::CWorldTable(void)
{
	Init();
}

CWorldTable::~CWorldTable(void)
{
	Destroy();
}

bool CWorldTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CWorldTable::Destroy()
{
	CTable::Destroy();
}

void CWorldTable::Init()
{
}

void* CWorldTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sWORLD_TBLDAT* pNewWorld = new sWORLD_TBLDAT;
		if (NULL == pNewWorld)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pNewWorld;
			return NULL;
		}
		
		m_dwCodePage = dwCodePage;
		return pNewWorld;
	}
	
	return NULL;
}

bool CWorldTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sWORLD_TBLDAT* pWorld = (sWORLD_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pWorld, sizeof(*pWorld)))
			return false;

		delete pWorld;

		return true;
	}
	
	return false;
}

bool CWorldTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);

	sWORLD_TBLDAT* pTbldat = (sWORLD_TBLDAT*)pvTable;

	if (CNtlVector::ZERO == pTbldat->vDefaultDir)
	{
		pTbldat->vDefaultDir = CNtlVector::UNIT_X;
	}
	else
	{
		if ( false == (pTbldat->vDefaultDir).SafeNormalize())
		{
			_ASSERTE( 0 );
			return false;
		}
	}

	if (CNtlVector::ZERO == pTbldat->vStart1Dir)
	{
		pTbldat->vStart1Dir = CNtlVector::UNIT_X;
	}
	else
	{
		if ( false == (pTbldat->vStart1Dir).SafeNormalize())
		{
			_ASSERTE( 0 );
			return false;
		}
	}

	if (CNtlVector::ZERO == pTbldat->vStart2Dir)
	{
		pTbldat->vStart2Dir = CNtlVector::UNIT_X;
	}
	else
	{
		if ( false == (pTbldat->vStart2Dir).SafeNormalize())
		{
			_ASSERTE( 0 );
			return false;
		}
	}

	if (CNtlVector::ZERO == pTbldat->outWorldDir)
	{
		pTbldat->outWorldDir = CNtlVector::UNIT_X;
	}
	else
	{
		if ( false == (pTbldat->outWorldDir).SafeNormalize())
		{
			_ASSERTE( 0 );
			return false;
		}
	}

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

bool CWorldTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sWORLD_TBLDAT* pWorld = (sWORLD_TBLDAT*)pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		static const WCHAR g_wszDragonballHaveRatePrefix[] = { 'd', 'r', 'a', 'g', 'o', 'n', 'b', 'a', 'l', 'l', '_', 'h', 'a', 'v', 'e', '_', 'r', 'a', 't', 'e', '_', 0 };
		static const WCHAR g_wszDragonballHaveRateFormat[] = { 'd', 'r', 'a', 'g', 'o', 'n', 'b', 'a', 'l', 'l', '_', 'h', 'a', 'v', 'e', '_', 'r', 'a', 't', 'e', '_', '%', 'd', 0 };
		static const WCHAR g_wszDragonballDropRatePrefix[] = { 'd', 'r', 'a', 'g', 'o', 'n', 'b', 'a', 'l', 'l', '_', 'd', 'r', 'o', 'p', '_', 'r', 'a', 't', 'e', '_', 0 };
		static const WCHAR g_wszDragonballDropRateFormat[] = { 'd', 'r', 'a', 'g', 'o', 'n', 'b', 'a', 'l', 'l', '_', 'd', 'r', 'o', 'p', '_', 'r', 'a', 't', 'e', '_', '%', 'd', 0 };

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->tblidx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Name"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );

			READ_STRING(bstrData, pWorld->szName, _countof(pWorld->szName));
			READ_STRINGW(bstrData, pWorld->wszName, _countof(pWorld->wszName));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Dynamic_Able"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->bDynamic = READ_BOOL( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Dynamic_Create_Count"))
		{
			pWorld->nCreateCount = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Field_Door_Type"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->byDoorType = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Mob_Spawn_Table_Name"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );

			READ_STRINGW(bstrData, pWorld->wszMobSpawn_Table_Name, _countof(pWorld->wszMobSpawn_Table_Name));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"NPC_Spawn_Table_Name"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );

			READ_STRINGW(bstrData, pWorld->wszNpcSpawn_Table_Name, _countof(pWorld->wszNpcSpawn_Table_Name));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Object_Spawn_Table_Name"))
		{
			READ_STRINGW(bstrData, pWorld->wszObjSpawn_Table_Name, _countof(pWorld->wszObjSpawn_Table_Name));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Field_Start_Loc_X"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->vStart.x = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Field_Start_Loc_Z"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->vStart.z = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Field_End_Loc_X"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->vEnd.x = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Field_End_Loc_Z"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->vEnd.z = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Battle_Start_X"))
		{
			pWorld->vBattleStartLoc.x = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Battle_Start_Z"))
		{
			pWorld->vBattleStartLoc.z = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Battle_End_X"))
		{
			pWorld->vBattleEndLoc.x = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Battle_End_Z"))
		{
			pWorld->vBattleEndLoc.z = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"OutSide_Battle_Start_X"))
		{
			pWorld->vOutSideBattleStartLoc.x = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"OutSide_Battle_Start_Z"))
		{
			pWorld->vOutSideBattleStartLoc.z = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"OutSide_Battle_End_X"))
		{
			pWorld->vOutSideBattleEndLoc.x = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"OutSide_Battle_End_Z"))
		{
			pWorld->vOutSideBattleEndLoc.z = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Spectator_Start_X"))
		{
			pWorld->vSpectatorStartLoc.x = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Spectator_Start_Z"))
		{
			pWorld->vSpectatorStartLoc.z = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Spectator_End_X"))
		{
			pWorld->vSpectatorEndLoc.x = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Spectator_End_Z"))
		{
			pWorld->vSpectatorEndLoc.z = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Default_Loc_X"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->vDefaultLoc.x = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Default_Loc_Y"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->vDefaultLoc.y = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Default_Loc_Z"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->vDefaultLoc.z = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Default_Dir_X"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->vDefaultDir.x = READ_FLOAT( bstrData, wszFieldNameBuf, 0.0f );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Default_Dri_Y"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->vDefaultDir.y = READ_FLOAT( bstrData, wszFieldNameBuf, 0.0f );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Default_Dir_Z"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->vDefaultDir.z = READ_FLOAT( bstrData, wszFieldNameBuf, 0.0f );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Start1_Point_Loc_X"))
		{
			pWorld->vStart1Loc.x = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Start1_Point_Loc_Y"))
		{
			pWorld->vStart1Loc.y = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Start1_Point_Loc_Z"))
		{
			pWorld->vStart1Loc.z = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Start1_Point_Dir_X"))
		{
			pWorld->vStart1Dir.x = READ_FLOAT( bstrData, wszFieldNameBuf, 0.0f );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Start1_Point_Dir_Z"))
		{
			pWorld->vStart1Dir.z = READ_FLOAT( bstrData, wszFieldNameBuf, 0.0f );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Start2_Point_Loc_X"))
		{
			pWorld->vStart2Loc.x = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Start2_Point_Loc_Y"))
		{
			pWorld->vStart2Loc.y = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Start2_Point_Loc_Z"))
		{
			pWorld->vStart2Loc.z = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Start2_Point_Dir_X"))
		{
			pWorld->vStart2Dir.x = READ_FLOAT( bstrData, wszFieldNameBuf, 0.0f );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Start2_Point_Dir_Z"))
		{
			pWorld->vStart2Dir.z = READ_FLOAT( bstrData, wszFieldNameBuf, 0.0f );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Standard_Loc_X"))
		{
			pWorld->vStandardLoc.x = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Standard_Loc_Z"))
		{
			pWorld->vStandardLoc.z = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Field_Destory_Time"))
		{
			pWorld->dwDestroyTimeInMilliSec = READ_DWORD( bstrData ) * 1000;
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Split_Size"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->fSplitSize = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Night_Able"))
		{
			pWorld->bNight_Able = READ_BOOL( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Static_Time"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->byStatic_Time = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"funcflag"))
		{
			pWorld->wFuncFlag = (WORD) READ_BITFLAG( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"World_Rule_Type"))
		{
			pWorld->byWorldRuleType = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"World_Rule_Index"))
		{
			pWorld->worldRuleTbldx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"OutField_Tblidx"))
		{
			pWorld->outWorldTblidx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Out_Field_Loc_X"))
		{
			pWorld->outWorldLoc.x = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Out_Field_Loc_Z"))
		{
			pWorld->outWorldLoc.z = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Out_Field_Dir_X"))
		{
			pWorld->outWorldDir.x = READ_FLOAT( bstrData, wszFieldNameBuf, 0.0f );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Out_Field_Dir_Z"))
		{
			pWorld->outWorldDir.x = READ_FLOAT( bstrData, wszFieldNameBuf, 0.0f );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"ResourceFolder"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );

			READ_STRINGW(bstrData, pWorld->wszResourceFolder, _countof(pWorld->wszResourceFolder));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"BGM_Rest_Time"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->fBGMRestTime = READ_FLOAT( bstrData, wszFieldNameBuf, 0.0f );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"World_Resource_ID"))
		{
			pWorld->dwWorldResourceID = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"FreeCamera_Height"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->fFreeCamera_Height = READ_FLOAT( bstrData, wszFieldNameBuf, 0.0f );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Enter_Resource_Flash"))
		{
			READ_STRINGW(bstrData, pWorld->wszEnterResourceFlash, _countof(pWorld->wszEnterResourceFlash));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Leave_Resource_Flash"))
		{
			READ_STRINGW(bstrData, pWorld->wszLeaveResourceFlash, _countof(pWorld->wszLeaveResourceFlash));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"WPS_Link_Index"))
		{
			pWorld->wpsLinkIndex = READ_TBLIDX(bstrData);
		}

		//new
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Start_Point_Range"))
		{
			pWorld->byStartPointRange = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Can_Fly"))
		{
//			pWorld->bCanFly = READ_BOOL( bstrData, pstrDataName->c_str() );
		}
		
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszDragonballHaveRatePrefix, WCHARLen(g_wszDragonballHaveRatePrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < DBO_MAX_WORLD_DRAGONBALLDROP; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszDragonballHaveRateFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pWorld->abyDragonBallHaveRate[ i ] = READ_BYTE( bstrData, wszFieldNameBuf );

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
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszDragonballDropRatePrefix, WCHARLen(g_wszDragonballDropRatePrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < DBO_MAX_WORLD_DRAGONBALLDROP; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszDragonballDropRateFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pWorld->abyDragonBallDropRate[ i ] = READ_BYTE( bstrData, wszFieldNameBuf );

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
	else
	{
		return false;
	}

	return true;
}


sTBLDAT* CWorldTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;
	
	return (sTBLDAT*)(iter->second); 
}

bool CWorldTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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

		sWORLD_TBLDAT* pTableData = new sWORLD_TBLDAT;
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

		if(pTableData->tblidx == 1)
		{
		//	printf("pTableData->tblidx %d  %f %f\n", pTableData->tblidx, pTableData->vStart.x, pTableData->vStart.z);
		//	printf("pTableData->tblidx %d  %f %f\n", pTableData->tblidx, pTableData->vEnd.x, pTableData->vEnd.z);
		}

		if( false == AddTable(pTableData, bReload, bUpdate) )
		{
			delete pTableData;
		}

	} while (false != bLoop);

	return true;
}

bool CWorldTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sWORLD_TBLDAT* pTableData = (sWORLD_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}