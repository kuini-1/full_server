//***********************************************************************************
//
//	File		:	NtlItem.cpp
//
//	Begin		:	2007-06-28
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	
//
//	Desc		:	
//
//***********************************************************************************

#include "stdafx.h"
#include "NtlItem.h"
#include <cmath>
#include "NtlBattle.h"
#include "NtlObject.h"
#include "NtlStringHandler.h"
#include "NtlHex.h"

WORD Dbo_GetFinalOffence(WORD wBaseOffence, BYTE byGrade)
{
	if (ITEM_GRADE_LEVEL_0 == byGrade)
	{
		return wBaseOffence;
	}
	else
	{
		WORD finaloffence = wBaseOffence;
		int gradestep = (int)byGrade;

		//calc first tier +10% 1-5 level
		if (gradestep < 1)
			return finaloffence;
		finaloffence += (WORD)(floor(wBaseOffence * 0.10) * (std::min)(5, gradestep));
		gradestep -= 5;

		//calc second tier +15% 6-9 level
		if (gradestep < 1)
			return finaloffence;
		finaloffence += (WORD)(floor(wBaseOffence * 0.15) * (std::min)(4, gradestep));
		gradestep -= 4;

		//calc third tier +20% 10-14 level
		if (gradestep < 1)
			return finaloffence;
		finaloffence += (WORD)(floor(wBaseOffence * 0.20) * (std::min)(5, gradestep));
		gradestep -= 5;

		//calc last tier +30% 15 level
		if (gradestep < 1)
			return finaloffence;
		finaloffence += (WORD)(floor(wBaseOffence * 0.30));

		return finaloffence;
	}
}

WORD Dbo_GetFinalDefence(WORD wBaseDefence, BYTE byGrade)
{
	if (ITEM_GRADE_LEVEL_0 == byGrade)
	{
		return wBaseDefence;
	}
	else
	{
		WORD finaldefence = wBaseDefence;
		int gradestep = (int)byGrade;

		//calc first tier +5% 1-4 level
		if (gradestep < 1)
			return finaldefence;
		finaldefence += (WORD)(floor(wBaseDefence * 0.05) * (std::min)(4, gradestep));
		gradestep -= 4;

		//calc second tier +10% 5-9 level
		if (gradestep < 1)
			return finaldefence;
		finaldefence += (WORD)(floor(wBaseDefence * 0.10) * (std::min)(5, gradestep));
		gradestep -= 5;

		//calc third tier +15% 10-13 level
		if (gradestep < 1)
			return finaldefence;
		finaldefence += (WORD)(floor(wBaseDefence * 0.15) * (std::min)(4, gradestep));
		gradestep -= 4;

		//calc last tier +20% 14-15 level
		if (gradestep < 1)
			return finaldefence;
		finaldefence += (WORD)(floor(wBaseDefence * 0.20) * (std::min)(2, gradestep));

		return finaldefence;
	}
}


BYTE Dbo_GetHoipoiStoneCount( const BYTE byStoneType, const BYTE byItemType, BYTE byGrade )
{
	// ??? ??????? ????
	const static BYTE byNeedCountBasicWeapon[NTL_ITEM_MAX_GRADE]	= {2,2,2,4,4,4,6,6,6,8,8,8,10,10,10};
	const static BYTE byNeedCountBasicArmor[NTL_ITEM_MAX_GRADE]		= {1,1,1,2,2,2,3,3,3,4,4,4,5, 5, 5};
	// ??? ??????? ????
	const static BYTE byNeedCountPureWeapon[NTL_ITEM_MAX_GRADE]		= {2,2,2,4,4,4,6,6,6,8,8,8,10,10,10};
	const static BYTE byNeedCountPureArmor[NTL_ITEM_MAX_GRADE]		= {1,1,1,2,2,2,3,3,3,4,4,4,5, 5, 5};
	// ???? ??????? ????
	const static BYTE byNeedCountBlackWeapon[NTL_ITEM_MAX_GRADE]	= {2,2,2,2,2,4,4,4,4,4,6,6,6, 6, 6};
	const static BYTE byNeedCountBlackArmor[NTL_ITEM_MAX_GRADE]		= {1,1,1,1,1,2,2,2,2,2,3,3,3, 3, 3};

	// ???? ??????? ???? ???? ???
	if ( ITEM_TYPE_BLACK_STONE == byStoneType )
	{
		--byGrade;
	}

	// ?????? ????? ??? ?????.
	if ( byGrade < 0 || byGrade >= NTL_ITEM_MAX_GRADE )
		return INVALID_BYTE;

	if ( ITEM_TYPE_WEAPON_FIRST <= byItemType && ITEM_TYPE_WEAPON_LAST >= byItemType )
	{
		if ( ITEM_TYPE_STONE == byStoneType )		return byNeedCountBasicWeapon[byGrade];
		if ( ITEM_TYPE_PURE_STONE == byStoneType )	return byNeedCountPureWeapon[byGrade];
		if ( ITEM_TYPE_BLACK_STONE == byStoneType ) return byNeedCountBlackWeapon[byGrade];
	}
	else if ( ITEM_TYPE_ARMOR_FIRST <= byItemType && ITEM_TYPE_ARMOR_LAST >= byItemType )
	{
		if ( ITEM_TYPE_STONE == byStoneType )		return byNeedCountBasicArmor[byGrade];
		if ( ITEM_TYPE_PURE_STONE == byStoneType )	return byNeedCountPureArmor[byGrade];
		if ( ITEM_TYPE_BLACK_STONE == byStoneType ) return byNeedCountBlackArmor[byGrade];
	}
	
	// ??????? ???? ?????? ????? ???????? ??????.
	return INVALID_BYTE;
}

DWORD Dbo_GetRepairPay( DWORD dwCost, BYTE byDur, BYTE byCurDur)
{
	float fRate = (float)byCurDur / (float)byDur;
	if ( fRate >= 1.0f )
		return 0;

	DWORD dwRepairCost = (DWORD)((dwCost - (dwCost *  fRate)) * 0.69999999f);

	if( dwRepairCost == 0 )
	{
		dwRepairCost = 1;
	}

	return dwRepairCost;
}

DWORD Dbo_GetHoipoiMixEXP(bool bIsSuccess, BYTE byMadeLevel, BYTE byNeedMixLevel)
{
	if (bIsSuccess == false)
		return 0;

	/*
		max receive exp = 25
		lose 5 exp per byMadeLevel and byNeedMixLevel difference. Ex: 1 - 1 = 25 exp, 1 - 2 = 20 exp, 1 - 3 = 15 exp
	*/

	BYTE byDif = byMadeLevel - byNeedMixLevel;
	if (byDif >= 5)
		return 0;

	return 25 - (byDif * 5);
}

// ?????? ??? ????
DWORD Dbo_GetHoipoiMixFare( DWORD dwCost, BYTE byDiscountRate /*=0*/ )
{
	return static_cast<DWORD>( dwCost * (1.0f- static_cast<float>(byDiscountRate)/100) );
}

//-----------------------------------------------------------------------------------
// ??? ????? 
// bool Dbo_SetItemData( sITEM_DATA* const pItemData_Output, sITEM_DATA* const pItemData_Input )
// ?? ???? ????: 
// sITEM_DATA?? ??????? ??????? ????? ???, ?????? ????? ?????? ???????? ??? ???????. 
// ????? ????????, ????? ????????~
// ?????, ??????? ???? ???????? ????.
//-----------------------------------------------------------------------------------
bool Dbo_SetItemData( sITEM_DATA* const pItemData
					 , ITEMID itemId
					 , TBLIDX itemNo
					 , CHARACTERID charId
					 , BYTE byPlace
					 , BYTE byPosition
					 , BYTE byStackcount
					 , BYTE byRank
					 , BYTE byCurrentDurability
					 , bool bNeedToIdentify
					 , BYTE byGrade
					 , BYTE byBattleAttribute
					 , BYTE byRestrictType
					 , const WCHAR* const awchMaker
					 , TBLIDX* const aOptionTblidx
					 , BYTE byDurationType
					 , DBOTIME nUseStartTime
					 , DBOTIME nUseEndTime )
{
	if( NULL == pItemData )
	{
		return false;
	}

	pItemData->itemId = itemId;
	pItemData->itemNo = itemNo; 
	pItemData->charId = charId;
	pItemData->byPlace = byPlace;
	pItemData->byPosition = byPosition;
	pItemData->byStackcount = byStackcount;
	pItemData->byRank = byRank;
	pItemData->byCurrentDurability = byCurrentDurability;
	pItemData->bNeedToIdentify = bNeedToIdentify;
	pItemData->byGrade = byGrade;
	pItemData->byBattleAttribute = byBattleAttribute;
	pItemData->byRestrictState = byRestrictType;
	pItemData->byDurationType = byDurationType;
	pItemData->nUseStartTime = nUseStartTime;
	pItemData->nUseEndTime = nUseEndTime;

	if( NULL == awchMaker )
	{
		memset( pItemData->awchMaker, 0x00, sizeof(pItemData->awchMaker) );
	}
	else
	{
		memcpy( pItemData->awchMaker, awchMaker, sizeof( pItemData->awchMaker ) );
		(pItemData->awchMaker)[NTL_MAX_SIZE_CHAR_NAME] = L'\0';
	}

	if( NULL == aOptionTblidx )
	{
		memset( pItemData->sOptionSet.aOptionTblidx, INVALID_TBLIDX, sizeof( pItemData->sOptionSet.aOptionTblidx ) );
		memset( pItemData->sOptionSet.aRandomOption, INVALID_TBLIDX, sizeof( pItemData->sOptionSet.aRandomOption ) );
	}
	else
	{
		memcpy( pItemData->sOptionSet.aOptionTblidx, aOptionTblidx, sizeof( pItemData->sOptionSet.aOptionTblidx ) );
		memcpy( pItemData->sOptionSet.aRandomOption, aOptionTblidx, sizeof( pItemData->sOptionSet.aRandomOption ) );
	}

	return true;
}

//-----------------------------------------------------------------------------------
// ??? ????? 
// bool Dbo_SetItemData_NeedToIdentify( sITEM_DATA* const pItemData_Output, sITEM_DATA* const pItemData_Input )
// ?? ???? ????: 
// sITEM_DATA?? ??????? ??????? ????? ???, ?????? ????? ?????? ???????? ??? ???????. 
// ????? ????????, ????? ????????~
// ?????, ??????? ???? ???????? ????.
//-----------------------------------------------------------------------------------
bool Dbo_SetItemData_CheckNeedToIdentify( sITEM_DATA* const pItemData
					 , ITEMID itemId
					 , TBLIDX itemNo
					 , CHARACTERID charId
					 , BYTE byPlace
					 , BYTE byPosition
					 , BYTE byStackcount
					 , BYTE byRank
					 , BYTE byCurrentDurability
					 , bool bNeedToIdentify
					 , BYTE byGrade
					 , BYTE byBattleAttribute
					 , BYTE byRestrictType
					 , WCHAR* const awchMaker
					 , TBLIDX* const aOptionTblidx
					 , BYTE byDurationType
					 , DBOTIME nUseStartTime
					 , DBOTIME nUseEndTime )
{
	if( NULL == pItemData )
	{
		return false;
	}

	pItemData->itemId = itemId;
	pItemData->charId = charId;
	pItemData->byPlace = byPlace;
	pItemData->byPosition = byPosition;
	pItemData->bNeedToIdentify = bNeedToIdentify;

	if ( true == pItemData->bNeedToIdentify )
	{
		//????? ???????? ???
		return true;
	}

	//??? ???????? ???
	pItemData->itemNo = itemNo; 
	pItemData->byStackcount = byStackcount;
	pItemData->byRank = byRank;
	pItemData->byCurrentDurability = byCurrentDurability;
	pItemData->byGrade = byGrade;
	pItemData->byBattleAttribute = byBattleAttribute;
	pItemData->byRestrictState = byRestrictType;
	pItemData->byDurationType = byDurationType;
	pItemData->nUseStartTime = nUseStartTime;
	pItemData->nUseEndTime = nUseEndTime;

	if( NULL == awchMaker )
	{
		memset( pItemData->awchMaker, 0x00, sizeof(pItemData->awchMaker) );
	}
	else
	{
		memcpy( pItemData->awchMaker, awchMaker, sizeof( pItemData->awchMaker ) );
		(pItemData->awchMaker)[NTL_MAX_SIZE_CHAR_NAME] = L'\0';
	}

	if( NULL == aOptionTblidx )
	{
		memset( pItemData->sOptionSet.aOptionTblidx, INVALID_TBLIDX, sizeof( pItemData->sOptionSet.aOptionTblidx ) );
		memset( pItemData->sOptionSet.aRandomOption, INVALID_TBLIDX, sizeof( pItemData->sOptionSet.aRandomOption ) );
	}
	else
	{
		memcpy( pItemData->sOptionSet.aOptionTblidx, aOptionTblidx, sizeof( pItemData->sOptionSet.aOptionTblidx ) );
		memcpy( pItemData->sOptionSet.aRandomOption, aOptionTblidx, sizeof( pItemData->sOptionSet.aRandomOption ) );
	}

	return true;
}


//-----------------------------------------------------------------------------------
// ??? ????? 
// bool Dbo_SetItemProfile( sITEM_PROFILE* const pItemData_Output, sITEM_PROFILE* const pItemData_Input )
// ?? ???? ????: 
// sITEM_PROFILE?? ??????? ??????? ????? ???, ?????? ????? ?????? ???????? ??? ???????. 
// ????? ????????, ????? ????????~
// ?????, ??????? ???? ???????? ????.
//-----------------------------------------------------------------------------------
bool Dbo_SetItemProfile( sITEM_PROFILE* const pItemProfile
						, HOBJECT handle
						, TBLIDX tblidx
						, BYTE byPlace
						, BYTE byPos
						, BYTE byStackcount
						, BYTE byRank
						, BYTE byCurDur
						, bool bNeedToIdentify
						, BYTE byGrade
						, BYTE byBattleAttribute
						, BYTE byRestrictState
						, WCHAR* const awchMaker
						, TBLIDX* const aOptionTblidx
						, BYTE byDurationType
						, DBOTIME nUseStartTime
						, DBOTIME nUseEndTime )
{

	if( NULL == pItemProfile )
	{
		return false;
	}

	pItemProfile->handle = handle;
	pItemProfile->byPlace = byPlace;
	pItemProfile->byPos = byPos;
	pItemProfile->bNeedToIdentify = bNeedToIdentify;

	if( true == bNeedToIdentify )
	{
		//????? ???????? ???
		return true;
	}

	//??? ???????? ???
	pItemProfile->tblidx = tblidx;
	pItemProfile->byStackcount = byStackcount;
	pItemProfile->byRank = byRank;
	pItemProfile->byCurDur = byCurDur;
	pItemProfile->byGrade = byGrade;
	pItemProfile->byBattleAttribute = byBattleAttribute;
	pItemProfile->byRestrictState = byRestrictState;
	pItemProfile->byDurationType = byDurationType; 
	pItemProfile->nUseStartTime = nUseStartTime;
	pItemProfile->nUseEndTime = nUseEndTime;

	if( NULL == awchMaker )
	{
		memset( pItemProfile->awchMaker, 0x00, sizeof(pItemProfile->awchMaker) );
	}
	else
	{
		memcpy( pItemProfile->awchMaker, awchMaker, sizeof( pItemProfile->awchMaker ) );
		(pItemProfile->awchMaker)[NTL_MAX_SIZE_CHAR_NAME] = L'\0';
	}

	if( NULL == aOptionTblidx )
	{
		memset( pItemProfile->sOptionSet.aOptionTblidx, INVALID_TBLIDX, sizeof( pItemProfile->sOptionSet.aOptionTblidx ) );
		memset( pItemProfile->sOptionSet.aRandomOption, INVALID_TBLIDX, sizeof( pItemProfile->sOptionSet.aRandomOption ) );
	}
	else
	{
		memcpy( pItemProfile->sOptionSet.aOptionTblidx, aOptionTblidx, sizeof( pItemProfile->sOptionSet.aOptionTblidx ) );
		memcpy( pItemProfile->sOptionSet.aRandomOption, aOptionTblidx, sizeof( pItemProfile->sOptionSet.aRandomOption ) );
	}

	return true;
}


//-----------------------------------------------------------------------------------
// ????? ???? ?? ???????? ??????... ( ???????? ??????? ?????~ )
// ??? ????? 
// bool Dbo_SetItemBrief( sITEM_BRIEF* const pItemData_Output, sITEM_BRIEF* const pItemData_Input )
// ?? ???? ????: 
// sITEM_BRIEF?? ??????? ??????? ????? ???, ?????? ????? ?????? ???????? ??? ???????. 
// ????? ????????, ????? ????????~
// ?????, ??????? ???? ???????? ????.
//-----------------------------------------------------------------------------------
bool Dbo_SetItemBrief( sITEM_BRIEF* const pItemBrief
							, TBLIDX tblidx
							, BYTE byRank
							, BYTE byGrade
							, BYTE byBattleAttribute )
{
	if( NULL == pItemBrief )
	{
		return false;
	}

	pItemBrief->tblidx = tblidx;
	pItemBrief->byRank = byRank;
	pItemBrief->byGrade = byGrade;
	pItemBrief->byBattleAttribute = byBattleAttribute;

	return true;
}

//-----------------------------------------------------------------------------------
// ??? ????? 
// bool Dbo_SetItemBrief( sITEM_BRIEF* const pItemData_Output, sITEM_BRIEF* const pItemData_Input )
// ?? ???? ????: 
// sITEM_BRIEF?? ??????? ??????? ????? ???, ?????? ????? ?????? ???????? ??? ???????. 
// ????? ????????, ????? ????????~
// ?????, ??????? ???? ???????? ????.
//-----------------------------------------------------------------------------------
bool Dbo_SetShopBuyInven( sSHOP_BUY_INVEN* const pShopBuyInven
						 , BYTE byPlace
						 , BYTE byPos
						 , BYTE byStack
						 , TBLIDX tblItem
						 , BYTE byRank
						 , BYTE byCurrentDurability
						 , BYTE byGrade
						 , BYTE byRestrictState
						 , WCHAR* const awchMaker
						 , TBLIDX* const aOptionTblidx
						 , BYTE byDurationType
						 , DBOTIME nUseStartTime
						 , DBOTIME nUseEndTime )
{
	if( NULL == pShopBuyInven )
	{
		return false;
	}

	pShopBuyInven->byPlace = byPlace;
	pShopBuyInven->byPos = byPos;
	pShopBuyInven->byStack = byStack;
	pShopBuyInven->tblItem = tblItem;
	pShopBuyInven->byRank = byRank;
	pShopBuyInven->byCurrentDurability = byCurrentDurability;
	pShopBuyInven->byGrade = byGrade;
	pShopBuyInven->byRestrictState = byRestrictState;
	pShopBuyInven->byDurationType = byDurationType;
	pShopBuyInven->nUseStartTime = nUseStartTime;
	pShopBuyInven->nUseEndTime = nUseEndTime;

	if( NULL == awchMaker )
	{
		memset( pShopBuyInven->awchMaker, 0x00, sizeof(pShopBuyInven->awchMaker) );
	}
	else
	{
		memcpy( pShopBuyInven->awchMaker, awchMaker, sizeof( pShopBuyInven->awchMaker ) );
		(pShopBuyInven->awchMaker)[NTL_MAX_SIZE_CHAR_NAME] = L'\0';
	}

	if( NULL == aOptionTblidx )
	{
		memset( pShopBuyInven->sOptionSet.aOptionTblidx, 0, sizeof( pShopBuyInven->sOptionSet.aOptionTblidx ) );
		memset( pShopBuyInven->sOptionSet.aRandomOption, 0, sizeof( pShopBuyInven->sOptionSet.aRandomOption ) );
	}
	else
	{
		memcpy( pShopBuyInven->sOptionSet.aOptionTblidx, aOptionTblidx, sizeof( pShopBuyInven->sOptionSet.aOptionTblidx ) );
		memcpy( pShopBuyInven->sOptionSet.aRandomOption, aOptionTblidx, sizeof( pShopBuyInven->sOptionSet.aRandomOption ) );
	}

	return true;
}


bool Dbo_CheckClass( const BYTE byClass, const DWORD dwNeedClassBitFlag )
{
	if(dwNeedClassBitFlag == 127) // 127 = for all
		return true;

	bool bIsSecClass = false;

	if(byClass >= PC_CLASS_2_FIRST)
		bIsSecClass = true;

	BYTE byFirstClass = 0;

	//get first class
	if(bIsSecClass == true)
	{
		if(byClass == PC_CLASS_STREET_FIGHTER || byClass == PC_CLASS_SWORD_MASTER)
			byFirstClass = PC_CLASS_HUMAN_FIGHTER;

		else if(byClass == PC_CLASS_CRANE_ROSHI || byClass == PC_CLASS_TURTLE_ROSHI)
			byFirstClass = PC_CLASS_HUMAN_MYSTIC;

		else if(byClass == PC_CLASS_DARK_WARRIOR || byClass == PC_CLASS_SHADOW_KNIGHT)
			byFirstClass = PC_CLASS_NAMEK_FIGHTER;

		else if(byClass == PC_CLASS_DENDEN_HEALER || byClass == PC_CLASS_POCO_SUMMONER)
			byFirstClass = PC_CLASS_NAMEK_MYSTIC;

		else if(byClass == PC_CLASS_ULTI_MA || byClass == PC_CLASS_GRAND_MA)
			byFirstClass = PC_CLASS_MIGHTY_MAJIN;
		
		else if(byClass == PC_CLASS_PLAS_MA || byClass == PC_CLASS_KAR_MA)
			byFirstClass = PC_CLASS_WONDER_MAJIN;
	}


	if(dwNeedClassBitFlag & MAKE_BIT_FLAG(byClass))
		return true;

	//check first class if char is second class
	if(bIsSecClass == true)
	{
		if(dwNeedClassBitFlag & MAKE_BIT_FLAG(byFirstClass))
			return true;
	}

	return false;
}

DWORD Dbo_GetAuctionHouseSellFee(DWORD dwCost)
{
	DWORD fee = 1;

	if (dwCost == 0)
		return fee;

	fee = (dwCost / 100);

	if (fee == 0) fee = 1;

	return fee;
}


DWORD Dbo_GetItemRestrictFlagType(BYTE byItemRestrictType, BYTE byItemRestrictState)
{
	const int dwArrRestrictFlag[ITEM_RESTRICT_TYPE_MAX][ITEM_RESTRICT_STATE_TYPE_MAX] =
	{
		{ 0, 123 },
		{ 123, 123 },
		{ 115, 115 },
		{ 0, 123 },
		{ 0, 123 },
		{ 127, 127 },
		{ 510, 510 },
		{ 127, 127 },
		{ 0, 0 },
		{ 0, 0 }
	};

	if (byItemRestrictState <= ITEM_RESTRICT_STATE_TYPE_LIMIT || byItemRestrictState >= ITEM_RESTRICT_STATE_TYPE_SEAL)
	{
		if (byItemRestrictState < ITEM_RESTRICT_STATE_TYPE_SEAL || byItemRestrictState >= ITEM_RESTRICT_STATE_TYPE_INSERT_BEAD)
		{
			if (byItemRestrictState >= ITEM_RESTRICT_STATE_TYPE_INSERT_BEAD)
			{
				byItemRestrictType = byItemRestrictState - ITEM_RESTRICT_STATE_TYPE_INSERT_BEAD;
				byItemRestrictState = 1;
			}
		}
		else
		{
			byItemRestrictType = 0;
			byItemRestrictState = 0;
		}
	}
	else
	{
		byItemRestrictType = byItemRestrictState - 1;
		byItemRestrictState = 1;
	}

	if (byItemRestrictType < 10)
	{
		if (byItemRestrictState < 2)
			return dwArrRestrictFlag[byItemRestrictType][byItemRestrictState];
	}
	

	return INVALID_DWORD;
}

float Dbo_GetRandomOptionSystemEffectRate(BYTE byRvType, WORD wMaxValue, BYTE byValue, float fAttack_Physical_Revision, float fAttack_Energy_Revision, float fDefence_Physical_Revision, float fDefence_Energy_Revision)
{
	float fAdjRate = 1.0f;
	float fRetRate = 0.0f;

	switch (byRvType)
	{
	case 1: fAdjRate = fAttack_Physical_Revision;
	case 2: fAdjRate = fAttack_Energy_Revision;
	case 3: fAdjRate = fDefence_Physical_Revision;
	case 4: fAdjRate = fDefence_Energy_Revision;
	default: printf("Dbo_GetRandomOptionSystemEffectRate: byRvType %u not found \n", byRvType); break;
	}

	if (fAdjRate == INVALID_FLOAT)
		fAdjRate = 1.0f;

	fRetRate = ((float)(byValue * wMaxValue) * fAdjRate) / 100.0f;
	if (fRetRate < 1.0f)
		fRetRate = 1.0f;

	return fRetRate;
}

bool IsBagContainer(BYTE byContainr)
{
	return byContainr == CONTAINER_TYPE_BAGSLOT;
}

bool IsInvenContainer(BYTE byContainr)
{
	return byContainr >= CONTAINER_TYPE_BAG1 && byContainr <= CONTAINER_TYPE_BAG5;
}

bool IsEquipContainer(BYTE byContainr)
{
	return byContainr == CONTAINER_TYPE_EQUIP;
}

bool IsInfoContainer(BYTE byContainr)
{
	return byContainr && (byContainr <= CONTAINER_TYPE_NETPYSTORE || byContainr == CONTAINER_TYPE_MASCOT_BAG);
}

bool IsBankContainer(BYTE byContainr)
{
	return byContainr >= CONTAINER_TYPE_BANKSLOT && byContainr <= CONTAINER_TYPE_BANK4;
}

bool IsGuildContainer(BYTE byContainr)
{
	return byContainr >= CONTAINER_TYPE_GUILD1 && byContainr <= CONTAINER_TYPE_GUILD3;
}

BYTE GetBagSlotPositionByContainerType(eCONTAINER_TYPE container)
{
	switch (container)
	{
		case CONTAINER_TYPE_BAG1: return BAGSLOT_POSITION_BAGSLOT_POSITION_0; break;
		case CONTAINER_TYPE_BAG2: return BAGSLOT_POSITION_BAGSLOT_POSITION_1; break;
		case CONTAINER_TYPE_BAG3: return BAGSLOT_POSITION_BAGSLOT_POSITION_2; break;
		case CONTAINER_TYPE_BAG4: return BAGSLOT_POSITION_BAGSLOT_POSITION_3; break;
		case CONTAINER_TYPE_BAG5: return BAGSLOT_POSITION_BAGSLOT_POSITION_4; break;
		case CONTAINER_TYPE_NETPYSTORE: return BAGSLOT_POSITION_BAGSLOT_POSITION_NETPYSTORE; break;
		default: return BAGSLOT_POSITION_NONE; break;
	}

	//return BAGSLOT_POSITION_NONE; // Unreachable ~Mateo
}

BYTE GetContainerTypeByBagSlotPosition(eBAGSLOT_POSITION eBagSlotPosition)
{
	switch (eBagSlotPosition)
	{
	case BAGSLOT_POSITION_BAGSLOT_POSITION_0: return CONTAINER_TYPE_BAG1; break;
	case BAGSLOT_POSITION_BAGSLOT_POSITION_1: return CONTAINER_TYPE_BAG2; break;
	case BAGSLOT_POSITION_BAGSLOT_POSITION_2: return CONTAINER_TYPE_BAG3; break;
	case BAGSLOT_POSITION_BAGSLOT_POSITION_3: return CONTAINER_TYPE_BAG4; break;
	case BAGSLOT_POSITION_BAGSLOT_POSITION_4: return CONTAINER_TYPE_BAG5; break;
	case BAGSLOT_POSITION_BAGSLOT_POSITION_NETPYSTORE: return CONTAINER_TYPE_NETPYSTORE; break;
	default: return CONTAINER_TYPE_NONE; break;
	}

	//return CONTAINER_TYPE_NONE; // Unreachable ~Mateo
}

float GetDisassembleHigherRankRate(BYTE byItemRank, BYTE byItemNeedMinLevel, BYTE byHigherRank, BYTE byItemNeedMaxLevel)
{
	return ((float)((float)byItemRank * (float)byItemNeedMinLevel / (float)byHigherRank * (float)byItemNeedMaxLevel) * 100.0f);
}

bool Dbo_NeedToSaveItemCoolTime(BYTE byItemCoolTimeGroup)
{
	bool bResult;

	switch (byItemCoolTimeGroup)
	{
	case ITEM_COOL_TIME_GROUP_DH_LP:
	case ITEM_COOL_TIME_GROUP_DH_EP:
	case ITEM_COOL_TIME_GROUP_HOT_LP:
	case ITEM_COOL_TIME_GROUP_HOT_EP:
	case ITEM_COOL_TIME_GROUP_DH_RESCUE:
	case ITEM_COOL_TIME_GROUP_DH_POISON:
	case ITEM_COOL_TIME_GROUP_DH_STOMACHACHE:
	case ITEM_COOL_TIME_GROUP_TELEPORT:
	case ITEM_COOL_TIME_GROUP_CB_OFFENCE:
	case ITEM_COOL_TIME_GROUP_CB_DEFENCE:
	case ITEM_COOL_TIME_GROUP_HOT_FOOD:
	case ITEM_COOL_TIME_GROUP_HOT_DRINK:
	case ITEM_COOL_TIME_GROUR_MASCOT_FOOT:
		bResult = false;
		break;

	case ITEM_COOL_TIME_GROUP_GIFT_BOX: bResult = true; break;

	case ITEM_COOL_TIME_GROUP_TMQ_TIME_PLUS:
	case ITEM_COOL_TIME_GROUP_HLS_SKILL:
	case ITEM_COOL_TIME_GROUP_EVENT_COIN_PLUS:
		bResult = false;
		break;

	case ITEM_COOL_TIME_GROUP_LINGER_LP:
	case ITEM_COOL_TIME_GROUP_LINGER_EP:
		bResult = true;
		break;
		
		default: bResult = false; break;
	}

	return bResult;
}

BYTE Dbo_GetItemCategoryByItemType(BYTE byItemType)
{
	switch (byItemType)
	{
		case ITEM_TYPE_GLOVE:
		case ITEM_TYPE_STAFF:
		case ITEM_TYPE_GUN:
		case ITEM_TYPE_DUAL_GUN:
			return DBO_ITEM_CATEGORY_MAIN_WEAPON; break;

		case ITEM_TYPE_CLAW:
		case ITEM_TYPE_AXE:
		case ITEM_TYPE_SCROLL:
		case ITEM_TYPE_GEM:
		case ITEM_TYPE_STICK:
		case ITEM_TYPE_SWORD:
		case ITEM_TYPE_FAN:
		case ITEM_TYPE_WAND:
		case ITEM_TYPE_BAZOOKA:
		case ITEM_TYPE_BACK_PACK:
		case ITEM_TYPE_INSTRUMENT:
		case ITEM_TYPE_CLUB:
		case ITEM_TYPE_DRUM:
		case ITEM_TYPE_MASK:
			return DBO_ITEM_CATEGORY_SUB_WEAPON; break;

		case ITEM_TYPE_JACKET: return DBO_ITEM_CATEGORY_JACKET; break;
		case ITEM_TYPE_PANTS: return DBO_ITEM_CATEGORY_PANTS; break;
		case ITEM_TYPE_BOOTS: return DBO_ITEM_CATEGORY_BOOTS; break;
		case ITEM_TYPE_NECKLACE: return DBO_ITEM_CATEGORY_NECKLACE; break;
		case ITEM_TYPE_EARRING: return DBO_ITEM_CATEGORY_EARRING; break;
		case ITEM_TYPE_RING: return DBO_ITEM_CATEGORY_RING; break;

		case ITEM_TYPE_COSTUME_SET: return DBO_ITEM_CATEGORY_DOGI; break;
		case ITEM_TYPE_COSTUME_HAIR_STYLE: return DBO_ITEM_CATEGORY_HAIR; break;
		case ITEM_TYPE_COSTUME_MASK: return DBO_ITEM_CATEGORY_MASK; break;
		case ITEM_TYPE_COSTUME_HAIR_ACCESSORY: return DBO_ITEM_CATEGORY_RICHNESS; break;
		case ITEM_TYPE_COSTUME_BACK_ACCESSORY: return DBO_ITEM_CATEGORY_BACKPACK; break;

		default: return DBO_ITEM_CATEGORY_ETC; break;
	}
}

float Dbo_GetItemWorth(BYTE byItemRank, BYTE byItemLevel)
{
	float fItemWorthLevel = (float)byItemLevel;

	fItemWorthLevel = (float)byItemLevel + 100.0f;

	switch (byItemRank)
	{
	case ITEM_RANK_NOTHING: return 0.0f; break;
	case ITEM_RANK_NORMAL:
	case ITEM_RANK_SUPERIOR:
	case ITEM_RANK_EXCELLENT:
	case ITEM_RANK_RARE:
	case ITEM_RANK_LEGENDARY:
	{
		return std::pow(pow((fItemWorthLevel * 1.0f) * 100.0f, 1.4f), 0.66666669f);
	}
	break;

	default: printf("Dbo_GetItemWorth: \"byItemRank\" has a wrong value., byItemRank = %u \n", byItemRank); break;
	}

	return 0.0f;
}

//float Dbo_GetItemWorth(BYTE byItemRank, BYTE byItemLevel)
//{
//	float fItemWorthLevel = (float)byItemLevel;
//	
//	if (byItemLevel < 56)
//		fItemWorthLevel = (float)byItemLevel + 5.0f;
//	else
//		fItemWorthLevel = (float)byItemLevel + 10.0f;
//	
//
//	switch (byItemRank)
//	{
//		case ITEM_RANK_NOTHING: return 0.0f; break;
//		case ITEM_RANK_NORMAL:
//		{
//			return std::pow(pow((fItemWorthLevel * 1.0f) * 100.0f, 1.5f), 0.66666669f);
//		}
//		break;
//		case ITEM_RANK_SUPERIOR:
//		{
//			return std::pow(pow((fItemWorthLevel * 1.1f) * 100.0f, 1.5f), 0.66666669f);
//		}
//		break;
//		case ITEM_RANK_EXCELLENT:
//		{ 
//			return std::pow(pow((fItemWorthLevel * 2.3f) * 100.0f, 1.5f), 0.66666669f);
//		}
//		break;
//		case ITEM_RANK_RARE:
//		{ 
//			return std::pow(pow((fItemWorthLevel * 2.8f) * 100.0f, 1.5f), 0.66666669f);
//		}
//		break;
//		case ITEM_RANK_LEGENDARY:
//		{ 
//			return std::pow(pow((fItemWorthLevel * 3.4f) * 100.0f, 1.5f), 0.66666669f);
//		}
//		break;
//
//		default: printf("Dbo_GetItemWorth: \"byItemRank\" has a wrong value., byItemRank = %u \n", byItemRank); break;
//	}
//
//	return 0.0f;
//}

BYTE GetRequiredSealItemNum(BYTE byRank, BYTE byGrade)
{
	BYTE byItemNum = byGrade;

	if (byRank > ITEM_RANK_SUPERIOR)
	{
		if (byRank == ITEM_RANK_EXCELLENT)
			byItemNum += 1;
		else if (byRank == ITEM_RANK_RARE)
			byItemNum += 2;
		else if (byRank == ITEM_RANK_LEGENDARY)
			byItemNum += 3;
	}

	return byItemNum;
}

DWORD Dbo_GetChargeItemBattleAttributeChange(BYTE byRank, BYTE byItemLv)
{
	DWORD dwFinalCharge = 15000;
	int nStepCount = (byItemLv - 1) / 10;

	if (nStepCount)
	{
		for (int nStep = 1; nStep < nStepCount; nStep++)
			dwFinalCharge *= 2;
		if (byRank > 3)
			dwFinalCharge *= 2;
	}

	return dwFinalCharge;
}

float Dbo_GetChangeItemBattleAttributeSuccessRate(BYTE byAttributePosNum)
{
	if (byAttributePosNum == INVALID_BYTE)
		return 20.0f;

	return 50.0f;
}

void Dbo_GetChangeItemBattleAttributeEachRate(BYTE byAdditionalAttribute, int *panEachRate)
{
	*panEachRate = 0;

	if (byAdditionalAttribute == BATTLE_ATTRIBUTE_UNKNOWN)
	{
		for (int j = BATTLE_ATTRIBUTE_HONEST; j < BATTLE_ATTRIBUTE_COUNT; j++)
			panEachRate[j] = 20;
	}
	else
	{
		for (int k = BATTLE_ATTRIBUTE_HONEST; k < BATTLE_ATTRIBUTE_COUNT; k++)
		{
			if (byAdditionalAttribute == k)
				panEachRate[k] = 24;
			else
				panEachRate[k] = 19;
		}
	}
}

bool IsSealItem(BYTE byRestrictState)
{
	return byRestrictState == ITEM_RESTRICT_STATE_TYPE_SEAL;
}

bool IsSealing(BYTE byRestrictType, BYTE byRestrictState, BYTE byEquipType)
{
	//printf("byRestrictType %u byRestrictState %u, byEquipType %u\n", byRestrictType, byRestrictState, byEquipType);
	if (byEquipType != EQUIP_TYPE_QUEST && byEquipType != EQUIP_TYPE_UNKNOWN && byEquipType <= EQUIP_TYPE_COSTUME)
	{
		if (byRestrictType == ITEM_RESTRICT_TYPE_EQUIP)
			return byRestrictState == ITEM_RESTRICT_STATE_TYPE_LIMIT || byRestrictState == ITEM_RESTRICT_STATE_TYPE_EQUIP;
	}

	return false;
}


bool IsChangeOption(BYTE byItemType, BYTE byItemRank, BYTE byRestrictType)
{
	if (IsSealItem(byRestrictType))
	{
		return false;
	}
	else if (byItemRank > ITEM_RANK_NORMAL && byItemRank < ITEM_RANK_COUNT)
	{
		if (byItemType > ITEM_TYPE_MASK)
		{
			if (byItemType < ITEM_TYPE_JACKET || byItemType > ITEM_TYPE_BOOTS)
				return byItemType == ITEM_TYPE_NECKLACE || byItemType == ITEM_TYPE_EARRING;
			else
				return true;
		}
		else
			return true;
	}

	return false;
}

bool GetInsertBeadByDurationType(BYTE byItem_Type)
{
	return byItem_Type == ITEM_TYPE_BEAD;
}

BYTE GetDefaultRestrictState(BYTE byRestrictType, BYTE byItemType, bool bActivate)
{
	//printf("byItemType %u, byRestrictType %u\n", byItemType, byRestrictType);

	if (bActivate == false)
		ITEM_RESTRICT_STATE_TYPE_NONE;
	
	if (byItemType == ITEM_TYPE_DRAGONBALL)				//bind dragon balls (also blank) to character
		return ITEM_RESTRICT_STATE_TYPE_CHARACTER_GET;

	switch (byRestrictType)
	{
		case ITEM_RESTRICT_TYPE_CHARACTER_GET:
		{
			return ITEM_RESTRICT_STATE_TYPE_CHARACTER_GET;
		}
		break;
		case ITEM_RESTRICT_TYPE_ACCOUNT_GET: return ITEM_RESTRICT_STATE_TYPE_ACCOUNT_GET;
		case ITEM_RESTRICT_TYPE_EQUIP: return ITEM_RESTRICT_STATE_TYPE_NONE;
		case ITEM_RESTRICT_TYPE_USE: return ITEM_RESTRICT_STATE_TYPE_NONE;
		case ITEM_RESTRICT_TYPE_DRAGONBALL: return ITEM_RESTRICT_STATE_TYPE_DRAGONBALL;
		case ITEM_RESTRICT_TYPE_BATTLE_DRAGONBALL: return ITEM_RESTRICT_STATE_TYPE_BATTLE_DRAGONBALL;
		case ITEM_RESTRICT_TYPE_QUEST: return ITEM_RESTRICT_STATE_TYPE_QUEST;
		case ITEM_RESTRICT_TYPE_FOREVER_EQUIP: return ITEM_RESTRICT_STATE_TYPE_EQUIP;
		case ITEM_RESTRICT_TYPE_FOREVER_CHARACTER_GET: return ITEM_RESTRICT_STATE_TYPE_CHARACTER_GET;

		default: return ITEM_RESTRICT_STATE_TYPE_NONE;
	}
}

BYTE GetItemTypeGroup(BYTE byItemType)
{
	BYTE byItemTypeGroup = ITEM_TYPE_GROUP_ETC;

	if (byItemType >= ITEM_TYPE_WEAPON_FIRST && byItemType <= ITEM_TYPE_WEAPON_LAST)
		byItemTypeGroup = ITEM_TYPE_GROUP_WEAPON;
	else if (byItemType >= ITEM_TYPE_ARMOR_FIRST && byItemType <= ITEM_TYPE_ARMOR_LAST)
		byItemTypeGroup = ITEM_TYPE_GROUP_ARMOR;

	return byItemTypeGroup;
}

bool IsValidStateToUseItem(TBLIDX itemIdx, WORD wNeed_State_Bit_Flag, BYTE byCurStateId, BYTE byCurAspectStateId, QWORD qwCurCharCondition, bool bIsSwimmingState, bool bIsJumpingState, BYTE byAirState)
{
	if (wNeed_State_Bit_Flag == INVALID_WORD)
		return true;

	static bool bInitItemStateCheck = false;
	static QWORD qwInvalidItemStateCondition = 0;

	if (!bInitItemStateCheck)
	{
		//cant use item when char has one of these conditions
		BIT_FLAG_SET(qwInvalidItemStateCondition, MAKE_BIT_FLAG64(CHARCOND_DIRECT_PLAY));
		BIT_FLAG_SET(qwInvalidItemStateCondition, MAKE_BIT_FLAG64(CHARCOND_CONFUSED));
		BIT_FLAG_SET(qwInvalidItemStateCondition, MAKE_BIT_FLAG64(CHARCOND_TERROR));
		BIT_FLAG_SET(qwInvalidItemStateCondition, MAKE_BIT_FLAG64(CHARCOND_FAKE_DEATH));
		BIT_FLAG_SET(qwInvalidItemStateCondition, MAKE_BIT_FLAG64(CHARCOND_RABIES));
		BIT_FLAG_SET(qwInvalidItemStateCondition, MAKE_BIT_FLAG64(CHARCOND_DRUNK));

		bInitItemStateCheck = true;
	}

	switch (wNeed_State_Bit_Flag)
	{
		case 1:  //only use-able when NOT transformed (items like super saiyan pot, vehicle have this)
		{
			if (BIT_FLAG_TEST(qwInvalidItemStateCondition, qwCurCharCondition) == true) //check if forbidden char conditions are set
				return false;

			if (byCurAspectStateId != ASPECTSTATE_INVALID) //check if transformed ( If we dont have this check, then we have issues using vehicle while transformed )
				return false;

			if (byAirState == AIR_STATE_ON)
				return false;

			if (bIsSwimmingState || bIsJumpingState)
				return false;
		}
		break;

		case 3: //only use-able while standing (items like food have this)
		{
			if (byCurStateId != CHARSTATE_STANDING && byCurStateId != CHARSTATE_SITTING)
				return false;

			if (byAirState == AIR_STATE_ON)
				return false;

			if (bIsSwimmingState || bIsJumpingState)
				return false;
		}
		break;

		case 15: //used by items like pet feed, bitter tea(dodge pot) exp bonus, lp/ep recover
		{
			if (itemIdx == 200309) //bitter tea
			{
				if (BIT_FLAG_TEST(qwInvalidItemStateCondition, qwCurCharCondition) == true) //check if forbidden char conditions are set
					return false;

				if (byCurStateId == CHARSTATE_STUNNED || byCurStateId == CHARSTATE_SLEEPING || byCurStateId == CHARSTATE_PARALYZED)
					return false;
			}
		}
		break;

		case 31: //used by flying scroll
		{

		}
		break;

		default: printf("IsValidStateToUseItem: %i wNeed_State_Bit_Flag not used \n", wNeed_State_Bit_Flag); break;
	}
	
	return true;
}
