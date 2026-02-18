#include "stdafx.h"
#include "NtlDebug.h"
#include "NtlCharacter.h"
#include "NtlSkill.h"
#include "SkillTable.h"

#include "NtlSerializer.h"

// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CSkillTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CSkillTable::CSkillTable(void)
{
	Init();
}

CSkillTable::~CSkillTable(void)
{
	Destroy();
}

bool CSkillTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CSkillTable::Destroy()
{
	m_mapPreTableList.clear();
	m_mapBasicMascotSkills.clear();

	CTable::Destroy();
}

void CSkillTable::Init()
{
	m_mapPreTableList.clear();
}

void* CSkillTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sSKILL_TBLDAT* pNewSkill = new sSKILL_TBLDAT;
		if (NULL == pNewSkill)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pNewSkill;
			return NULL;
		}

		m_dwCodePage = dwCodePage;
		return pNewSkill;
	}

	return NULL;
}

bool CSkillTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sSKILL_TBLDAT* pSkill = (sSKILL_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pSkill, sizeof(*pSkill)))
			return false;

		delete pSkill;

		return true;
	}

	return false;
}

bool CSkillTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bUpdate);

	sSKILL_TBLDAT * pTbldat = (sSKILL_TBLDAT*)pvTable;
	sSKILL_TBLDAT * pExistTbldat = NULL;

	if (false == pTbldat->bValidity_Able)
	{
		return false;
	}

	if( bReload )
	{
		pExistTbldat = (sSKILL_TBLDAT*) FindData( pTbldat->tblidx );
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

	if (pTbldat->dwRequire_VP > 0 && pTbldat->bySkill_Grade == 1)
		m_mapBasicMascotSkills.push_back(pTbldat);

	if ( INVALID_TBLIDX != pTbldat->dwNextSkillTblidx )
	{
		if ( false == m_mapPreTableList.insert( std::map<TBLIDX, TBLIDX>::value_type(pTbldat->dwNextSkillTblidx, pTbldat->tblidx)).second )
		{
			WCHAR wszFormatBuf[512];
			FormatStringToWCHAR(L"Tblidx[%u]::dwNextSkillTblidx[%u] is Duplicated ", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
			CTable::CallErrorCallbackFunction(wszFormatBuf, pTbldat->tblidx, pTbldat->dwNextSkillTblidx);
			_ASSERTE( 0 );
			return false;
		}
	}
	
	return true;
}

bool CSkillTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sSKILL_TBLDAT* pSkill = (sSKILL_TBLDAT*)pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		static const WCHAR g_wszRPEffectPrefix[] = { 'R', 'P', '_', 'E', 'f', 'f', 'e', 'c', 't', '_', 0 };
		static const WCHAR g_wszRPEffectFormat[] = { 'R', 'P', '_', 'E', 'f', 'f', 'e', 'c', 't', '_', '%', 'd', 0 };
		static const WCHAR g_wszRPEffectValueFormat[] = { 'R', 'P', '_', 'E', 'f', 'f', 'e', 'c', 't', '_', 'V', 'a', 'l', 'u', 'e', '_', '%', 'd', 0 };

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pSkill->tblidx = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Skill_Name"))
		{
			pSkill->Skill_Name = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Name_Text"))
		{
			READ_STRINGW(bstrData, pSkill->wszNameText, _countof(pSkill->wszNameText));
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Validity_Able") )
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pSkill->bValidity_Able = READ_BOOL(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"PC_Class_Bit_Flag"))// PC_CLASS -> PC_Class_Bit_Flag
		{
			pSkill->dwPC_Class_Bit_Flag = (DWORD)READ_BITFLAG( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Skill_Class"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pSkill->bySkill_Class = READ_BYTE(bstrData, wszFieldNameBuf);

			if (NTL_SKILL_CLASS_FIRST > pSkill->bySkill_Class || NTL_SKILL_CLASS_LAST < pSkill->bySkill_Class)
			{
				_ASSERT(0);
				return false;
			}
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Skill_Type"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pSkill->bySkill_Type = READ_BYTE(bstrData, wszFieldNameBuf);

			if (NTL_SKILL_TYPE_FIRST > pSkill->bySkill_Type || NTL_SKILL_TYPE_LAST < pSkill->bySkill_Type)
			{
				_ASSERT(0);
				return false;
			}
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Skill_Active_Type"))
		{
			pSkill->bySkill_Active_Type = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Buff_Group"))
		{
			pSkill->byBuff_Group = READ_BYTE(bstrData, wszFieldNameBuf, INVALID_BUFF_GROUP);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Slot_Index"))
		{
			pSkill->bySlot_Index = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Skill_Grade"))
		{
			pSkill->bySkill_Grade = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Function_Bit_Flag"))
		{
			pSkill->dwFunction_Bit_Flag = (DWORD)(READ_BITFLAG(bstrData));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Appoint_Target"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pSkill->byAppoint_Target = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Apply_Target"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pSkill->byApply_Target = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Apply_Target_Max"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pSkill->byApply_Target_Max = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Apply_Range"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pSkill->byApply_Range = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Apply_Area_Size_1"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pSkill->byApply_Area_Size_1 = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Apply_Area_Size_2"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pSkill->byApply_Area_Size_2 = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Skill_Effect_1"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pSkill->skill_Effect[0] = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Skill_Effect_Type_1"))
		{
			pSkill->bySkill_Effect_Type[0] = READ_BYTE(bstrData, wszFieldNameBuf);
		}		
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Skill_Effect_Value_1"))
		{
			pSkill->aSkill_Effect_Value[0] = READ_DOUBLE(bstrData, wszFieldNameBuf, 0);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Skill_Effect_2"))
		{
			pSkill->skill_Effect[1] = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Skill_Effect_Type_2"))
		{
			pSkill->bySkill_Effect_Type[1] = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Skill_Effect_Value_2"))
		{
			pSkill->aSkill_Effect_Value[1] = READ_DOUBLE(bstrData, wszFieldNameBuf, 0);
		}
		else if( 0 == WCHARNCmp(wszFieldNameBuf, g_wszRPEffectPrefix, WCHARLen(g_wszRPEffectPrefix)) )
		{
			WCHAR wszFieldNameEffect[1024 + 1];
			WCHAR wszFieldNameEffectValue[1024 + 1];

			for (BYTE byRpEffectIndex = 0 ; byRpEffectIndex < DBO_MAX_RP_BONUS_COUNT_PER_SKILL ; byRpEffectIndex++)
			{
				NTL_SWPRINTF(wszFieldNameEffect, 1024, g_wszRPEffectFormat, byRpEffectIndex + 1);
				NTL_SWPRINTF(wszFieldNameEffectValue, 1024, g_wszRPEffectValueFormat, byRpEffectIndex + 1);

				if (0 == WCHARCmp(wszFieldNameBuf, wszFieldNameEffect))
				{
					(pSkill->abyRpEffect)[byRpEffectIndex] = READ_BYTE(bstrData, wszFieldNameBuf);
				}
				if (0 == WCHARCmp(wszFieldNameBuf, wszFieldNameEffectValue))
				{
					(pSkill->afRpEffectValue)[byRpEffectIndex] = READ_FLOAT(bstrData, wszFieldNameBuf);
				}
			}
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Require_Train_Level"))
		{
			pSkill->byRequire_Train_Level = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Require_Zenny"))
		{
			pSkill->dwRequire_Zenny = READ_DWORD(bstrData, 0);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Next_Skill_Train_Exp"))
		{
			pSkill->wNext_Skill_Train_Exp = READ_WORD(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Require_SP"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pSkill->wRequireSP = READ_WORD(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Self_Train"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pSkill->bSelfTrain = READ_BOOL(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Require_Skill_Tblidx_Min_1"))
		{
			pSkill->uiRequire_Skill_Tblidx_Min_1 = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Require_Skill_Tblidx_Max_1"))
		{
			pSkill->uiRequire_Skill_Tblidx_Max_1 = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Require_Skill_Tblidx_Min_2"))
		{
			pSkill->uiRequire_Skill_Tblidx_Min_2 = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Require_Skill_Tblidx_Max_2"))
		{
			pSkill->uiRequire_Skill_Tblidx_Max_2 = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Root_Skill"))
		{
			pSkill->Root_Skill = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Require_Epuip_Slot_Type"))
		{
			pSkill->byRequire_Epuip_Slot_Type = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Require_Item_Type"))
		{
			pSkill->byRequire_Item_Type = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Icon_Name"))
		{
			READ_STRING(bstrData, pSkill->szIcon_Name, _countof(pSkill->szIcon_Name));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Require_LP"))
		{
			pSkill->dwRequire_LP = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Require_EP"))
		{
			pSkill->wRequire_EP = READ_WORD(bstrData, wszFieldNameBuf, 0);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Require_RP_Ball"))
		{
			pSkill->byRequire_RP_Ball = READ_BYTE(bstrData, wszFieldNameBuf, 0);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Casting_Time"))
		{
			pSkill->fCasting_Time = READ_FLOAT(bstrData, wszFieldNameBuf, 0.0f);
			pSkill->dwCastingTimeInMilliSecs = (DWORD)(pSkill->fCasting_Time * 1000.0f);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Cool_Time"))
		{
			pSkill->wCool_Time = READ_WORD(bstrData, wszFieldNameBuf, 0);
			pSkill->dwCoolTimeInMilliSecs = pSkill->wCool_Time * 1000;
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Keep_Time"))
		{
			pSkill->wKeep_Time = READ_WORD(bstrData, wszFieldNameBuf, 0);
			pSkill->dwKeepTimeInMilliSecs = pSkill->wKeep_Time * 1000;
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Keep_Effect"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pSkill->bKeep_Effect = READ_BOOL(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Use_Range_Min"))
		{
			pSkill->byUse_Range_Min = READ_BYTE(bstrData, wszFieldNameBuf, 0);
			pSkill->fUse_Range_Min = (float)(pSkill->byUse_Range_Min);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Use_Range_Max"))
		{
			pSkill->byUse_Range_Max = READ_BYTE(bstrData, wszFieldNameBuf, 0);
			pSkill->fUse_Range_Max = (float)(pSkill->byUse_Range_Max);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Note"))
		{
			pSkill->Note = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Next_Skill_Tblidx"))
		{
			pSkill->dwNextSkillTblidx = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Default_Display_Off"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pSkill->bDefaultDisplayOff = READ_BOOL(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Animation_Time"))
		{
			pSkill->dwAnimation_Time = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Casting_Animation_Start"))
		{
			pSkill->wCasting_Animation_Start = READ_WORD(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Casting_Animation_Loop"))
		{
			pSkill->wCasting_Animation_Loop = READ_WORD(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Action_Animation_Index"))
		{
			pSkill->wAction_Animation_Index = READ_WORD(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Action_Loop_Animation_Index"))
		{
			pSkill->wAction_Loop_Animation_Index = READ_WORD(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Action_End_Animation_Index"))
		{
			pSkill->wAction_End_Animation_Index = READ_WORD(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Dash_Able"))
		{
			pSkill->bDash_Able = READ_BOOL(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Transform_Use_Info_Bit_Flag"))
		{
			pSkill->dwTransform_Use_Info_Bit_Flag = READ_BITFLAG(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Success_Rate"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pSkill->fSuccess_Rate = READ_FLOAT(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Additional_Aggro_Point"))
		{
			pSkill->dwAdditional_Aggro_Point = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"PC_Class_Change"))
		{
			pSkill->byPC_Class_Change = READ_BYTE(bstrData, wszFieldNameBuf, PC_CLASS_UNKNOWN);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Use_Type"))
		{
			pSkill->byUse_Type = READ_BYTE(bstrData, wszFieldNameBuf, 0);
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


sTBLDAT* CSkillTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;

	return (sTBLDAT*)(iter->second); 
}

bool CSkillTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sSKILL_TBLDAT* pTableData = new sSKILL_TBLDAT;
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

bool CSkillTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sSKILL_TBLDAT* pTableData = (sSKILL_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}

TBLIDX CSkillTable::FindPreSkillTblidx(TBLIDX tblidx)
{
	if (0 == tblidx || INVALID_TBLIDX == tblidx)
		return INVALID_TBLIDX;

	PRE_TABLE_IT iter;
	iter = m_mapPreTableList.find(tblidx);
	if ( m_mapPreTableList.end() == iter)
		return INVALID_TBLIDX;

	return iter->second; 
}

TBLIDX CSkillTable::FindBasicSkillTblidx(TBLIDX tblidx)
{
	if (0 == tblidx || INVALID_TBLIDX == tblidx)
		return INVALID_TBLIDX;

	TBLIDX basicTblidx = tblidx;
	while ( INVALID_TBLIDX != basicTblidx )
	{
		PRE_TABLE_IT iter;
		iter = m_mapPreTableList.find(basicTblidx);
		if ( m_mapPreTableList.end() == iter)
		{
			return basicTblidx;
		}
		else
		{
			if ( INVALID_TBLIDX != iter->second )
			{
				basicTblidx = iter->second;
			}
			else
			{
				return basicTblidx;
			}
		}
	}
	return INVALID_TBLIDX;
}