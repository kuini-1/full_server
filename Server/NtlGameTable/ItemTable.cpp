#include "stdafx.h"
#include "ItemTable.h"
#include "NtlDebug.h"
#include "NtlBattle.h"
#include "NtlSerializer.h"
#include "NtlRandom.h"

static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) { WCharTLiteralToWCHAR(fmt, dest, destSize); }

const WCHAR* CItemTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CItemTable::CItemTable(void)
{
	Init();
}

CItemTable::~CItemTable(void)
{
	Destroy();
}

bool CItemTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CItemTable::Destroy()
{
	CTable::Destroy();
}

void CItemTable::Init()
{
}

void* CItemTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sITEM_TBLDAT* pNewItem = new sITEM_TBLDAT;
		if (NULL == pNewItem)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pNewItem;
			return NULL;
		}
		
		m_dwCodePage = dwCodePage;
		return pNewItem;
	}

	return NULL;
}

bool CItemTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sITEM_TBLDAT* pItem = (sITEM_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pItem, sizeof(*pItem)))
			return false;

		delete pItem;

		return true;
	}

	return false;
}

bool CItemTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	sITEM_TBLDAT * pTbldat = (sITEM_TBLDAT*) pvTable;
	sITEM_TBLDAT * pExistTbldat = NULL;
	
	if ( false == pTbldat->bValidity_Able )
	{
		return false;
	}

	if( bReload )
	{
		pExistTbldat = (sITEM_TBLDAT*) FindData( pTbldat->tblidx );
		if( pExistTbldat )
		{
			CopyMemory( pTbldat, pExistTbldat, pTbldat->GetDataSize() );
			return true;  
		}
	}

	if( bUpdate) //for tables inside localize folder
	{
		//if exist then remove
		pExistTbldat = (sITEM_TBLDAT*) FindData( pTbldat->tblidx );
		if( pExistTbldat )
		{
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

bool CItemTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sITEM_TBLDAT* pItem = (sITEM_TBLDAT*)pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->tblidx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Name_Text"))
		{
			READ_STRINGW(bstrData, pItem->wszNameText, _countof(pItem->wszNameText));
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Validity_Able") )
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->bValidity_Able = READ_BOOL( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Name"))
		{
			pItem->Name = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Icon_Name"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );

			READ_STRING(bstrData, pItem->szIcon_Name, _countof(pItem->szIcon_Name));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Model_Type"))
		{
			pItem->byModel_Type = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Model"))
		{
			READ_STRING(bstrData, pItem->szModel, _countof(pItem->szModel));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Sub_Weapon_Act_Model"))
		{
			READ_STRING(bstrData, pItem->szSub_Weapon_Act_Model, _countof(pItem->szSub_Weapon_Act_Model));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Item_Type"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->byItem_Type = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Equip_Type"))
		{
			pItem->byEquip_Type = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Equip_Slot_Type_Bit_Flag"))
		{
			pItem->dwEquip_Slot_Type_Bit_Flag = (DWORD)READ_BITFLAG( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Function_Bit_Flag"))
		{
			pItem->wFunction_Bit_Flag = (WORD)READ_BITFLAG( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Max_Stack"))
		{
			pItem->byMax_Stack = READ_BYTE( bstrData, wszFieldNameBuf, NTL_UNSTACKABLE_ITEM_COUNT );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Rank"))
		{
			pItem->byRank = READ_BYTE( bstrData, wszFieldNameBuf, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Weight"))
		{
			pItem->dwWeight = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Cost"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->dwCost = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Sell_Price"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->dwSell_Price = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Durability"))
		{
			pItem->byDurability = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Durability_Count"))
		{
			pItem->byDurability_Count = READ_BYTE(bstrData, wszFieldNameBuf, 0);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Battle_Attribute"))
		{
			pItem->byBattle_Attribute = READ_BYTE(bstrData, wszFieldNameBuf, BATTLE_ATTRIBUTE_NONE);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Physical_Offence"))
		{
			pItem->wPhysical_Offence = READ_WORD(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Energy_Offence"))
		{
			pItem->wEnergy_Offence = READ_WORD(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Physical_Piercing_Offence"))
		{
//			pItem->wPhysical_Piercing_Offence = READ_WORD(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Energy_Piercing_Offence"))
		{
//			pItem->wEnergy_Piercing_Offence = READ_WORD(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Physical_Defence"))
		{
			pItem->wPhysical_Defence = READ_WORD(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Energy_Defence"))
		{
			pItem->wEnergy_Defence = READ_WORD(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Attack_Range_Bonus"))
		{
			pItem->fAttack_Range_Bonus = READ_FLOAT(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Attack_Speed_Rate"))
		{
			pItem->wAttack_Speed_Rate = READ_WORD(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Need_Level"))
		{
			pItem->byNeed_Min_Level = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Need_Max_Level"))
		{
			pItem->byNeed_Max_Level = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Need_Class_Bit_Flag"))
		{			
			pItem->dwNeed_Class_Bit_Flag = READ_BITFLAG( bstrData, NTL_ITEM_ALL_USE_FLAG);
		}

		else if (0 == WStringCmpLiteral(*pstrDataName, L"Need_Gender_Bit_Flag"))
		{			
			pItem->dwNeed_Gender_Bit_Flag = READ_BITFLAG( bstrData, NTL_ITEM_ALL_USE_FLAG);
		}

		else if (0 == WStringCmpLiteral(*pstrDataName, L"Class_Special"))
		{
			pItem->byClass_Special = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Race_Special"))
		{
			pItem->byRace_Special = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Need_Str"))
		{
			pItem->wNeed_Str = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Need_Con"))
		{
			pItem->wNeed_Con = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Need_Foc"))
		{
			pItem->wNeed_Foc = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Need_Dex"))
		{
			pItem->wNeed_Dex = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Need_Sol"))
		{
			pItem->wNeed_Sol = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Need_Eng"))
		{
			pItem->wNeed_Eng = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Set_Item_Tblidx"))
		{
			pItem->set_Item_Tblidx = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Note"))
		{
			pItem->Note = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Bag_Size"))
		{
			pItem->byBag_Size = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Scouter_Watt"))
		{
			pItem->wScouter_Watt = READ_WORD(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Scouter_MaxPower"))
		{
			pItem->dwScouter_MaxPower = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Scouter_Parts_Type_1"))
		{
			pItem->byScouter_Parts_Type1 = READ_BYTE(bstrData, wszFieldNameBuf, SCOUTER_PARTS_NONE);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Scouter_Parts_Type_2"))
		{
			pItem->byScouter_Parts_Type2 = READ_BYTE(bstrData, wszFieldNameBuf, SCOUTER_PARTS_NONE);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Scouter_Parts_Type_3"))
		{
			pItem->byScouter_Parts_Type3 = READ_BYTE(bstrData, wszFieldNameBuf, SCOUTER_PARTS_NONE);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Scouter_Parts_Type_4"))
		{
			pItem->byScouter_Parts_Type4 = READ_BYTE(bstrData, wszFieldNameBuf, SCOUTER_PARTS_NONE);
		}

		else if (0 == WStringCmpLiteral(*pstrDataName, L"Use_Item_Tblidx"))
		{
			pItem->Use_Item_Tblidx = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"bCan_Have_Option"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->bIsCanHaveOption = READ_BOOL( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Item_Option_Tblidx"))
		{
			pItem->Item_Option_Tblidx = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Item_Group"))
		{
			pItem->byItemGroup = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Charm_Tblidx"))
		{
			pItem->Charm_Tblidx = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Costume_Hide_Bit_Flag"))
		{
			pItem->wCostumeHideBitFlag = (WORD)READ_BITFLAG(bstrData);
		}	
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Need_Item_Tblidx"))
		{
			pItem->NeedItemTblidx = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Common_Point"))
		{
			pItem->CommonPoint = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Common_Point_Type"))
		{
			pItem->byCommonPointType = READ_BYTE(bstrData, wszFieldNameBuf);
		}		
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Need_Function"))
		{
			pItem->byNeedFunction = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if(0 == WStringCmpLiteral(*pstrDataName, L"Use_Duration_Max" ))
		{
			pItem->dwUseDurationMax = READ_DWORD( bstrData );
		}
		else if(0 == WStringCmpLiteral(*pstrDataName, L"Duration_Type" ))
		{
			pItem->byDurationType = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Contents_Tblidx"))
		{
			pItem->contentsTblidx = READ_TBLIDX(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Duration_Group"))
		{
			pItem->dwDurationGroup = READ_DWORD(bstrData);
		}

		else if(0 == WStringCmpLiteral(*pstrDataName, L"Drop_Level" ))
		{
			pItem->byDropLevel = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Create_Enchant_Rate_Tblidx"))
		{
			pItem->enchantRateTblidx = READ_TBLIDX(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Excellent_Tblidx"))
		{
			pItem->excellentTblidx = READ_TBLIDX(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Rare_Tblidx"))
		{
			pItem->rareTblidx = READ_TBLIDX(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Legendary_Tblidx"))
		{
			pItem->legendaryTblidx = READ_TBLIDX(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Create_Superior_Able"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->bCreateSuperiorAble = READ_BOOL( bstrData, wszFieldNameBuf, false );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Create_Excellent_Able"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->bCreateExcellentAble = READ_BOOL( bstrData, wszFieldNameBuf, false );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Create_Rare_Able"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->bCreateRareAble = READ_BOOL( bstrData, wszFieldNameBuf, false );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Create_Legendary_Able"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->bCreateLegendaryAble = READ_BOOL( bstrData, wszFieldNameBuf, false );
		}
		else if(0 == WStringCmpLiteral(*pstrDataName, L"Restrict_Type" ))
		{
			pItem->byRestrictType = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"fAtk_Phy"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->fAttack_Physical_Revision = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"fAtk_Eng"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->fAttack_Energy_Revision = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"fDef_Phy"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->fDefence_Physical_Revision = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"fDef_Eng"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->fDefence_Energy_Revision = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if(0 == WStringCmpLiteral(*pstrDataName, L"TMP_Category_Type" ))
		{
			pItem->byTmpTabType = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Can_Renewal"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->bIsCanRenewal = READ_BOOL( bstrData, wszFieldNameBuf, false );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Disassamble_Bit_Flag"))
		{
			pItem->wDisassemble_Bit_Flag = (WORD)READ_BITFLAG( bstrData );
		}
		else if(0 == WStringCmpLiteral(*pstrDataName, L"Normal_Min" ))
		{
			pItem->byDisassembleNormalMin = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if(0 == WStringCmpLiteral(*pstrDataName, L"Normal_Max" ))
		{
			pItem->byDisassembleNormalMax = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if(0 == WStringCmpLiteral(*pstrDataName, L"Rank_Up_Min" ))
		{
			pItem->byDisassembleUpperMin = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if(0 == WStringCmpLiteral(*pstrDataName, L"Rank_Up_Max" ))
		{
			pItem->byDisassembleUpperMax = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if(0 == WStringCmpLiteral(*pstrDataName, L"Drop_Visual" ))
		{
			pItem->byDropVisual = READ_BYTE( bstrData, wszFieldNameBuf );
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


sTBLDAT* CItemTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;
	
	return (sTBLDAT*)(iter->second); 
}

TBLIDX CItemTable::GetRandomTblidx(TBLIDX minItemTblidx, TBLIDX maxItemTblidx, BYTE byMinLevel, BYTE byMaxLevel)
{
	std::vector<TBLIDX> vecTempItem;
	//int nCount = 0;

	for (TBLIDX j = minItemTblidx; j <= maxItemTblidx; j++)
	{
		sITEM_TBLDAT* psItemTblDat = (sITEM_TBLDAT*)FindData(j);
		if (psItemTblDat)
		{
			if (psItemTblDat->byNeed_Min_Level >= byMinLevel && psItemTblDat->byNeed_Max_Level <= byMaxLevel)
			{
				vecTempItem.push_back(psItemTblDat->tblidx);
			}
		}
	}

	if (vecTempItem.size() == 0)
		return INVALID_TBLIDX;

	return vecTempItem[RandomRange(0, (int)(vecTempItem.size() - 1))];
}

TBLIDX CItemTable::FindDisassembleData(int nMaterialType, BYTE byRank, BYTE byStep)
{
	UNREFERENCED_PARAMETER(byRank);
	UNREFERENCED_PARAMETER(byStep);
	if (nMaterialType >= 6)
		return INVALID_TBLIDX;

	return INVALID_TBLIDX;
}

bool CItemTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
{
	if( false == bReload && bUpdate == false )
	{
		Reset(); //delete table if not release
	}

	BYTE byMargin = 1;
	serializer >> byMargin;

	bool bLoop = true;
	do
	{

		sITEM_TBLDAT* pTableData = new sITEM_TBLDAT;
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

		if( false == AddTable(pTableData, bReload, bUpdate) )
		{
			delete pTableData;
		}

	} while (false != bLoop);

	return true;
}

bool CItemTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sITEM_TBLDAT* pTableData = (sITEM_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}
