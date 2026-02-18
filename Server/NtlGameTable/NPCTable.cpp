//***********************************************************************************
//
//	File		:	NPCTable.cpp
//
//	Begin		:	2006-03-15
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Doo Sup, Chung ( john@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************

#include "stdafx.h"
#include "NPCTable.h"
#include "NtlDebug.h"
#include "NtlSerializer.h"

//- yoshiki : Let's consider of implementing NtlAssert series.
//#include "NtlAssert.h"

// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CNPCTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CNPCTable::CNPCTable(void)
{
	Init();
}

CNPCTable::~CNPCTable(void)
{
	Destroy();
}

bool CNPCTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CNPCTable::Destroy()
{
	CTable::Destroy();
}

void CNPCTable::Init()
{
}

void* CNPCTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sNPC_TBLDAT* pNewNPC = new sNPC_TBLDAT;
		if (NULL == pNewNPC)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pNewNPC;
			return NULL;
		}
		
		m_dwCodePage = dwCodePage;
		return pNewNPC;
	}
	
	return NULL;
}

bool CNPCTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sNPC_TBLDAT* pNPC = (sNPC_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pNPC, sizeof(*pNPC)))
			return false;

		delete pNPC;

		return true;
	}
	
	return false;
}

bool CNPCTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	sNPC_TBLDAT * pTbldat = (sNPC_TBLDAT*) pvTable;
	sNPC_TBLDAT * pExistTbldat = NULL;

	if ( false == pTbldat->bValidity_Able)
	{
		return false;
	}

	pTbldat->fRadius = (float) ( ( pTbldat->fRadius_X + pTbldat->fRadius_Z  ) * 0.5 );

	if( bReload )
	{
		pExistTbldat = (sNPC_TBLDAT*) FindData( pTbldat->tblidx );
		if( pExistTbldat )
		{
			CopyMemory( pTbldat, pExistTbldat, pTbldat->GetDataSize() );
			return true; 
		}
	}

	if( bUpdate) //for tables inside localize folder
	{
		pExistTbldat = (sNPC_TBLDAT*) FindData( pTbldat->tblidx );
		if( pExistTbldat )
		{ //if exist then remove
			m_mapTableList.erase( pTbldat->tblidx );
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


bool CNPCTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sNPC_TBLDAT* pNPC = (sNPC_TBLDAT*)pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		static const WCHAR g_wszUseSkillTimePrefix[] = { 'U', 's', 'e', '_', 'S', 'k', 'i', 'l', 'l', '_', 'T', 'i', 'm', 'e', '_', 0 };
		static const WCHAR g_wszUseSkillTimeFormat[] = { 'U', 's', 'e', '_', 'S', 'k', 'i', 'l', 'l', '_', 'T', 'i', 'm', 'e', '_', '%', 'd', 0 };
		static const WCHAR g_wszUseSkillTblidxPrefix[] = { 'U', 's', 'e', '_', 'S', 'k', 'i', 'l', 'l', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 0 };
		static const WCHAR g_wszUseSkillTblidxFormat[] = { 'U', 's', 'e', '_', 'S', 'k', 'i', 'l', 'l', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', '%', 'd', 0 };
		static const WCHAR g_wszUseSkillBasisPrefix[] = { 'U', 's', 'e', '_', 'S', 'k', 'i', 'l', 'l', '_', 'B', 'a', 's', 'i', 's', '_', 0 };
		static const WCHAR g_wszUseSkillBasisFormat[] = { 'U', 's', 'e', '_', 'S', 'k', 'i', 'l', 'l', '_', 'B', 'a', 's', 'i', 's', '_', '%', 'd', 0 };
		static const WCHAR g_wszUseSkillLPPrefix[] = { 'U', 's', 'e', '_', 'S', 'k', 'i', 'l', 'l', '_', 'L', 'P', '_', 0 };
		static const WCHAR g_wszUseSkillLPFormat[] = { 'U', 's', 'e', '_', 'S', 'k', 'i', 'l', 'l', '_', 'L', 'P', '_', '%', 'd', 0 };
		static const WCHAR g_wszMerchantTblidxPrefix[] = { 'M', 'e', 'r', 'c', 'h', 'a', 'n', 't', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 0 };
		static const WCHAR g_wszMerchantTblidxFormat[] = { 'M', 'e', 'r', 'c', 'h', 'a', 'n', 't', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', '%', 'd', 0 };

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pNPC->tblidx = READ_DWORD( bstrData );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Validity_Able") )
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pNPC->bValidity_Able = READ_BOOL( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Name"))
		{
			pNPC->Name = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Name_Text"))
		{
			READ_STRING(bstrData, pNPC->szNameText, _countof(pNPC->szNameText));
			READ_STRINGW(bstrData, pNPC->wszNameText, _countof(pNPC->wszNameText));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Model"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );

			READ_STRING(bstrData, pNPC->szModel, _countof(pNPC->szModel));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Level"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pNPC->byLevel = READ_BYTE( bstrData, wszFieldNameBuf, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Job"))
		{
			pNPC->byJob = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Function_Bit_Flag"))
		{
			pNPC->dwFunc_Bit_Flag = READ_BITFLAG( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Ai_Bit_Flag"))
		{
			pNPC->dwAi_Bit_Flag = READ_BITFLAG( bstrData, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Dialog_Script_Index"))
		{
			pNPC->Dialog_Script_Index = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Battle_Attribute"))
		{
			pNPC->byBattle_Attribute = READ_BYTE( bstrData, wszFieldNameBuf, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"NPC_type"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pNPC->byNpcType = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_LP"))
		{
			pNPC->dwBasic_LP = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"LP_Regeneration"))
		{
			pNPC->wLP_Regeneration = READ_WORD( bstrData, wszFieldNameBuf, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_EP"))
		{
			pNPC->wBasic_EP = READ_WORD( bstrData, wszFieldNameBuf, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"EP_Regeneration"))
		{
			pNPC->wEP_Regeneration = READ_WORD( bstrData, wszFieldNameBuf, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Attack_Type"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pNPC->byAttack_Type = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Physical_Offence"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pNPC->wBasic_Physical_Offence = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Energy_Offence"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pNPC->wBasic_Energy_Offence = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Physical_Defence"))
		{
			pNPC->wBasic_Physical_Defence = READ_WORD( bstrData, wszFieldNameBuf, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Energy_Defence"))
		{
			pNPC->wBasic_Energy_Defence = READ_WORD( bstrData, wszFieldNameBuf, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Str"))
		{
			pNPC->wBasicStr = READ_WORD( bstrData, wszFieldNameBuf, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Con"))
		{
			pNPC->wBasicCon = READ_WORD( bstrData, wszFieldNameBuf, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Foc"))
		{
			pNPC->wBasicFoc = READ_WORD( bstrData, wszFieldNameBuf, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Dex"))
		{
			pNPC->wBasicDex = READ_WORD( bstrData, wszFieldNameBuf, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Sol"))
		{
			pNPC->wBasicSol = READ_WORD( bstrData, wszFieldNameBuf, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Eng"))
		{
			pNPC->wBasicEng = READ_WORD( bstrData, wszFieldNameBuf, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Scale"))
		{
			pNPC->fScale = READ_FLOAT( bstrData, wszFieldNameBuf, 1.0f );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Walk_Speed_Origin"))
		{
			pNPC->fWalk_Speed_Origin = READ_FLOAT( bstrData, wszFieldNameBuf, 0.0f );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Walk_Speed"))
		{
			pNPC->fWalk_Speed = READ_FLOAT( bstrData, wszFieldNameBuf, 0.0f );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Run_Speed_Origin"))
		{
			pNPC->fRun_Speed_Origin = READ_FLOAT( bstrData, wszFieldNameBuf, 0.0f );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Run_Speed"))
		{
			pNPC->fRun_Speed = READ_FLOAT( bstrData, wszFieldNameBuf, 0.0f );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Radius_X"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pNPC->fRadius_X = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Radius_Z"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pNPC->fRadius_Z = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Attack_Speed_Rate"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pNPC->wAttack_Speed_Rate = READ_WORD( bstrData, wszFieldNameBuf, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Attack_Cool_Time"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pNPC->wAttackCoolTime = READ_WORD( bstrData, wszFieldNameBuf, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Attack_Range"))
		{
			pNPC->fAttack_Range = READ_FLOAT( bstrData, wszFieldNameBuf, 0.0f );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Attack_Rate"))
		{
			pNPC->wAttack_Rate = READ_WORD( bstrData, wszFieldNameBuf, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Dodge_Rate"))
		{
			pNPC->wDodge_Rate = READ_WORD( bstrData, wszFieldNameBuf, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Block_Rate"))
		{
			pNPC->wBlock_Rate = READ_WORD( bstrData, wszFieldNameBuf, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Curse_Success_Rate"))
		{
			pNPC->wCurse_Success_Rate = READ_WORD( bstrData, wszFieldNameBuf, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Curse_Tolerance_Rate"))
		{
			pNPC->wCurse_Tolerance_Rate = READ_WORD( bstrData, wszFieldNameBuf, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Sight_Range"))
		{
			pNPC->wSight_Range = READ_BYTE( bstrData, wszFieldNameBuf, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Scan_Range"))
		{
			pNPC->wScan_Range = READ_BYTE( bstrData, wszFieldNameBuf, 0 );
		}
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszUseSkillTimePrefix, WCHARLen(g_wszUseSkillTimePrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_NPC_HAVE_SKILL; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszUseSkillTimeFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pNPC->wUse_Skill_Time[ i ] = READ_WORD( bstrData, wszFieldNameBuf );

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
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszUseSkillTblidxPrefix, WCHARLen(g_wszUseSkillTblidxPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_NPC_HAVE_SKILL; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszUseSkillTblidxFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pNPC->use_Skill_Tblidx[ i ] = READ_DWORD( bstrData );

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
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszUseSkillBasisPrefix, WCHARLen(g_wszUseSkillBasisPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_NPC_HAVE_SKILL; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszUseSkillBasisFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pNPC->byUse_Skill_Basis[ i ] = READ_BYTE( bstrData, wszFieldNameBuf );

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
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszUseSkillLPPrefix, WCHARLen(g_wszUseSkillLPPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_NPC_HAVE_SKILL; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszUseSkillLPFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pNPC->wUse_Skill_LP[ i ] = READ_WORD( bstrData, wszFieldNameBuf );

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
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Visible_Sight_Range"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pNPC->byVisible_Sight_Range = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszMerchantTblidxPrefix, WCHARLen(g_wszMerchantTblidxPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_MERCHANT_TAB_COUNT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszMerchantTblidxFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pNPC->amerchant_Tblidx[ i ] = READ_DWORD( bstrData );

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
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Camera_Bone_Name"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );

			READ_STRING(bstrData, pNPC->szCamera_Bone_Name, _countof(pNPC->szCamera_Bone_Name));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Status_Transform_Tblidx"))
		{
			pNPC->statusTransformTblidx = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Fly_Height"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pNPC->fFly_Height = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Spawn_Animation"))
		{
			pNPC->bSpawn_Animation = READ_BOOL( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"ILLust"))
		{
			READ_STRING(bstrData, pNPC->szILLust, _countof(pNPC->szILLust));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Contents_Tblidx"))
		{
			pNPC->contentsTblidx = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Dialog_Group"))
		{
			pNPC->dwDialogGroup = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Alliance_Idx"))
		{
			pNPC->dwAllianceIdx = READ_DWORD( bstrData );
		}	
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Aggro_Max_Count"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );// [3/25/2008 SGpro]
			pNPC->wAggroMaxCount = READ_WORD( bstrData, wszFieldNameBuf, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Aggro_Point"))
		{
			pNPC->wBasic_Aggro_Point = READ_WORD( bstrData, wszFieldNameBuf, 0 );
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


sTBLDAT* CNPCTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;
	
	return (sTBLDAT*)(iter->second); 
}

TBLIDX CNPCTable::FindMerchantItem(sNPC_TBLDAT* psTbldat, BYTE byIndex)
{
	if ( NTL_MAX_MERCHANT_TAB_COUNT <= byIndex || 0 > byIndex )
		return INVALID_TBLIDX;

	return psTbldat->amerchant_Tblidx[byIndex];	
}

bool CNPCTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
{
	if( false == bReload && bUpdate == false )
	{
		Reset();
	}

	BYTE byMargin = 1;
	serializer >> byMargin;

	bool bLoop = true;
	do
	{
		sNPC_TBLDAT* pTableData = new sNPC_TBLDAT;
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

//		printf("pTableData->tblidx %d Name %d JOB %d  contentsTblidx %d\n", pTableData->tblidx, pTableData->Name, pTableData->byJob, pTableData->contentsTblidx);

		if( false == AddTable(pTableData, bReload, bUpdate) )
		{
			delete pTableData;
		}

	} while (false != bLoop);

	return true;
}

bool CNPCTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sNPC_TBLDAT* pTableData = (sNPC_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}