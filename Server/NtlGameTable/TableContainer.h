//***********************************************************************************
//
//	File		:	TableContainer.h
//
//	Begin		:	2007-01-31
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	
//
//	Desc		:	
//
//***********************************************************************************

#pragma once

#include "DropTableEx.h"


class CNtlFileSerializer;
class CNtlBitFlagManager;

class CExpTable;
class CHelpTable;
class CMerchantTable;
class CMobTable;
class CNewbieTable;
class CNPCTable;
class CPCTable;
class CStatusTransformTable;
class CGameManiaTimeTable;

class CBasicDropTable;
class CItemOptionTable;
class CItemTable;
class CNormalDropTable;
class CLegendaryDropTable;
class CExcellentDropTable;
class CSuperiorDropTable;
class CEachDropTable;
class CTypeDropTable;
class CUseItemTable;
class CDragonBallTable;
class CDragonBallRewardTable;
class CDragonBallReturnPointTable;

class CActionTable;
class CChatCommandTable;
class CDirectionLinkTable;
class CFormulaTable;
class CServerConfigTable;

class CCharmTable;

class CQuestDropTable;
class CQuestItemTable;
class CQuestProbabilityTable;
class CQuestTextDataTable;
class CQuestRewardTable;

class CHTBSetTable;
class CSkillTable;
class CSystemEffectTable;

class CTextAllTable;
class CChattingFilterTable;
class CTextServerTable;

class CObjectTable;
class CSpawnTable;
class CWayPointTable;
class CWorldTable;
class CWorldZoneTable;
class CWorldPlayTable;
class CWorldMapTable;
class CLandMarkTable;
class CModelToolCharDataTable;
class CModelToolObjDataTable;
class CWorldPathTable;

class CTimeQuestTable;
class CRankBattleTable;
class CBudokaiTable;
class CDungeonTable;

class CTableFileNameList;

class CGuideHintTable;
class CPortalTable;
class CNpcSpeechTable;
class CSetItemTable;
class CScriptLinkTable;
class CQuestNarrationTable;

class CVehicleTable;
class CItemRecipeTable;


class CDynamicObjectTable;
class CMobMovePatternTable;
class CDojoTable;
class CItemUpgradeTable;
class CItemMixMachineTable;

class CHLSItemTable;
class CHLSMerchantTable;

//new
class CCharTitleTable;
class CDwcTable;
class CDwcMissionTable;
class CItemBagListTable;
class CItemEnchantTable;
class CItemEnchantRateTable;
class CItemGroupListTable;
class CItemOptionValueProbabilityTable;
class CItemUpgradeRateTable;
class CMascotTable;
class CQuestRewardSelectTable;
class CRaceTable;
class CClassTable;
class CMascotGradeTable;
class CMascotStatusTable;
class CItemMixExpTable;
class CItemUpgradeRateNewTable;
class CAirCostumeTable;
class CCommonConfigTable;
class CSlotMachineTable;
class CHlsSlotMachineItemTable;
class CItemDisassembleTable;
class CContentsOnOffTable;
class CNpcServerTable;
class CMobServerTable;
class CSpecialCharacterTable;
class CContentsConditionTable;
class CEventSystemTable;
class CDynamicFieldSystemTable;



class CTableContainer
{
public:
	enum eTABLE
	{
		// Character
		TABLE_EXP = 0,
		TABLE_HELP,
		TABLE_MERCHANT,
		TABLE_MOB,
		TABLE_NEWBIE,
		TABLE_NPC,
		TABLE_PC,
		TABLE_STATUS_TRANSFORM,
		TABLE_GAME_MANIA_TIME,
		TABLE_GUIDE_HINT,

		// Item
		TABLE_BASIC_DROP,
		TABLE_ITEM_OPTION,
		TABLE_ITEM,
		TABLE_LEGENDARY_DROP,
		TABLE_NORMAL_DROP,
		TABLE_SUPERIOR_DROP,
		TABLE_USE_ITEM,
		TABLE_SET_ITEM,
		TABLE_DRAGONBALL,
		TABLE_DRAGONBALL_REWARD,
		TABLE_DRAGONBALL_RETURN_POINT, //new


		// Misc
		TABLE_ACTION,
		TABLE_CHAT_COMMAND,
		TABLE_DIRECTION_LINK,
		TABLE_FORMULA,
		TABLE_SERVERCONFIG,
		TABLE_RACE, //new
		TABLE_CLASS, //new

		// Party
		TABLE_CHARM,

		// Quest
		TABLE_QUEST_DROP,
		TABLE_QUEST_ITEM,
		TABLE_QUEST_PROBABILITY,
		TABLE_QUEST_TEXT_DATA,
		TABLE_QUEST_REWARD,
		TABLE_QUEST_REWARD_SELECT, //new

		// Rule
		TABLE_RANKBATTLE,
		TABLE_TIMEQUEST,
		TABLE_BUDOKAI,

		// Skill
		TABLE_HTB_SET,
		TABLE_SKILL,
		TABLE_SYSTEM_EFFECT,

		// Text
		TABLE_TEXT_ALL,
		TABLE_CHATTING_FILTER,
		TABLE_TEXT_SERVER,

		// World
		TABLE_LAND_MARK,
		TABLE_OBJECT,
		TABLE_NPC_SPAWN,
		TABLE_MOB_SPAWN,
		TABLE_WORLD_MAP,
		TABLE_WORLD,
		TABLE_WORLD_ZONE,
		TABLE_WORLD_PLAY,

		// GraphicData
		TABLE_MODEL_TOOL_CHAR,
		TABLE_MODEL_TOOL_OBJ,

		// World Data
		TABLE_PASS_POINT,

		// Portal Data
		TABLE_PORTAL,

		TABLE_SPEECH,

		TABLE_EACH_DROP,
		TABLE_TYPE_DROP,

		TABLE_SCRIPT_LINK,
		TABLE_EXCELLENT_DROP,
		TABLE_DUNGEON,
		
		TABLE_QUEST_NARRATION,

		TABLE_VEHICLE,
		TABLE_ITEM_RECIPE,
		TABLE_ITEM_RECIPE_NEW, //new
		TABLE_DYNAMIC_OBJECT,
		TABLE_MOB_MOVE_PATTERN,
		TABLE_DOJO,
		TABLE_ITEM_UPGRADE,
		TABLE_ITEM_UPGRADE_RATE, //new
		TABLE_ITEM_UPGRADE_RATE_NEW, //new
		TABLE_MIX_MACHINE,
		TABLE_ITEM_MIX_EXP, //new
		
		TABLE_HLS_MERCHANT,
		TABLE_HLS_ITEM,
		TABLE_HLS_SLOT_MACHINE,
		TABLE_HLS_SLOT_MACHINE_ITEM,

		TABLE_SPECIAL_CHARACTER, //new
		TABLE_NPC_SERVER, //new
		TABLE_MOB_SERVER, //new
		TABLE_DROP, //new
		TABLE_DROP_NORBAG, //new
		TABLE_DROP_GRPBAG, //new
		TABLE_ITEM_ENCHANT, //new
		TABLE_ITEM_ENCHANT_RATE, //new
		TABLE_ITEM_DISASSEMBLE,

		//mascot
		TABLE_MASCOT, //new
		TABLE_MASCOT_GRADE, //new
		TABLE_MASCOT_STATUS, //new

		TABLE_COMMON_CONFIG, //new
		TABLE_CONTENTSONOFF,
		TABLE_DWC, //new
		TABLE_DWCMISSION, //new
		TABLE_CHARTITLE, //new
		TABLE_EVENT_SYSTEM, //new
		TABLE_DYNAMIC_FIELD_SYSTEM, //new
		TABLE_ITEM_OPTION_VALUE_PROBABILITY, //new
		TABLE_CONTENTS_CONDITION, //new
		TABLE_ITEM_BAG_LIST, //new
		TABLE_ITEM_GROUP_LIST, //new
		TABLE_LOAD_TYPE_REWRITE, //new
		TABLE_AIR_COSTUME,//new

		TABLE_COUNT
	};

public:
	CTableContainer(void);

	virtual ~CTableContainer(void);

public:

	typedef std::map<TBLIDX, CSpawnTable*> SPAWNTABLEMAP;
	typedef SPAWNTABLEMAP::iterator SPAWNTABLEIT;
	typedef SPAWNTABLEMAP::value_type SPAWNTABLEVAL;

	typedef std::map<TBLIDX, CObjectTable*> OBJTABLEMAP;
	typedef OBJTABLEMAP::iterator OBJTABLEIT;
	typedef OBJTABLEMAP::value_type OBJTABLEVAL;


public:

	class ICallBack
	{
	public:
		virtual bool Call( const char* pfilename, CNtlFileSerializer* pSeralize, const char* pszCryptPassword ) = 0;
	};

	// Create ??? ?? pCall ?? NULL ?? ??? ?????? ???? ??????? ???????. pCall -> Client ???? 
	// Create pCall used in this way, the behavior is the same as the previous case NULL. pCall -> Client for use
	bool							Create(CNtlBitFlagManager& rTableFlag, char* pszPath, CTableFileNameList* pFileNameList, CTable::eLOADING_METHOD eLoadingMethod, DWORD dwCodePage, ICallBack* pCall );	

	bool							Create(CNtlBitFlagManager& rTableFlag, WCHAR* pwszPath, CTableFileNameList* pFileNameList, CTable::eLOADING_METHOD eLoadingMethod, DWORD dwCodePage, ICallBack* pCall );

	void							Destroy();


protected:

	void							Init();


public:

	bool							Reload(CNtlBitFlagManager& rTableFlag, CTableFileNameList* pFileNameList);
									//update current tables with the ones from localize folder
	bool							Update(CNtlBitFlagManager& rTableFlag, CTableFileNameList* pFileNameList);

	bool							SaveToFile(CNtlBitFlagManager& rTableFlag, CTableFileNameList* pFileNameList, bool bNeedToEncrypt);

	void							SetPath(char* pszPath);

	void							SetPath(WCHAR* pwszPath);

protected:

	bool							InitializeTable(CTable* pTable, CNtlFileSerializer& serializer, char* pszFileNameWithoutExtension, CTableContainer::ICallBack* pCall);

	bool							InitializeTable(CTable* pTable, CNtlFileSerializer& serializer, const WCHAR* pwszFileNameWithoutExtension, CTableContainer::ICallBack* pCall);

	bool							InitializeTable(CTextAllTable* pTextAllTable, CNtlFileSerializer& serializer, char* pszFileNameWithoutExtension, CTableContainer::ICallBack* pCall);

	bool							InitializeTable(CTextAllTable* pTextAllTable, CNtlFileSerializer& serializer, const WCHAR* pwszFileNameWithoutExtension, CTableContainer::ICallBack* pCall);

	bool							InitializePackTable(CTable* pTable, CNtlFileSerializer& serializer, const WCHAR* pszFileNameWithoutExtension, CTableContainer::ICallBack* pCall);

	bool							InitializePackTable(CTable* pTable, CNtlFileSerializer& serializer, char* pszFileNameWithoutExtension, CTableContainer::ICallBack* pCall);

	bool							InitializePackTable(CTextAllTable* pTextAllTable, CNtlFileSerializer& serializer, const WCHAR* pszFileNameWithoutExtension, CTableContainer::ICallBack* pCall);

	bool							InitializePackTable(CTextAllTable* pTextAllTable, CNtlFileSerializer& serializer, char* pszFileNameWithoutExtension, CTableContainer::ICallBack* pCall);

	bool							ReloadTable(CTable* pTable, CNtlFileSerializer& serializer, char* pwszFileNameWithoutExtension);
	bool							ReloadTable(CTable* pTable, CNtlFileSerializer& serializer, const WCHAR* pwszFileNameWithoutExtension);

	//Update tables with data from localize folder
	bool							UpdateTable(CTable* pTable, CNtlFileSerializer& serializer, char* pwszFileNameWithoutExtension);
	bool							UpdateTable(CTable* pTable, CNtlFileSerializer& serializer, const WCHAR* pwszFileNameWithoutExtension);

	bool							UpdateTextAllTable(CTextAllTable* pTextAllTable, CNtlFileSerializer& serializer, char* pwszFileNameWithoutExtension);
	bool							UpdateTextAllTable(CTextAllTable* pTextAllTable, CNtlFileSerializer& serializer, const WCHAR* pwszFileNameWithoutExtension);

private:

	template<class T>
	void DboCreateTable(const char *szClassName, CNtlFileSerializer & serializer, const WCHAR * pwszFileName, T **pTable, CTableContainer::ICallBack *pCall)
	{
		if (!*pTable)
		{
			T* pNewTable = new T;

			if (false == pNewTable->Create(m_dwCodePage))
			{
				CTable::CallErrorCallbackFunction("%s::Create() failed.(Table : %s)", szClassName);
				delete pNewTable;
			}

			*pTable = pNewTable;
		}

		if (NULL == pCall)
		{
			if (false == InitializeTable(*pTable, serializer, pwszFileName, NULL))
			{
				CTable::CallErrorCallbackFunction("InitializeTable() failed.(Table : %s)", szClassName);
				delete pTable;
			}
		}
		else
		{
			if (false == InitializePackTable(*pTable, serializer, pwszFileName, pCall))
			{
				CTable::CallErrorCallbackFunction("InitializeTable() failed.(Table : %s)", szClassName);
				delete pTable;
			}
		}
	}

public:

// Character
	CExpTable*						GetExpTable() { return m_pExpTable; }

	CGuideHintTable*				GetGuideHintTable() { return m_pGuideHintTable; }

	CHelpTable*						GetHelpTable() { return m_pHelpTable; }

	CMerchantTable*					GetMerchantTable() { return m_pMerchantTable; }

	CMobTable*						GetMobTable() { return m_pMobTable; }

	CNewbieTable*					GetNewbieTable() { return m_pNewbieTable; }

	CNPCTable*						GetNpcTable() { return m_pNpcTable; }

	CPCTable*						GetPcTable() { return m_pPcTable; }

	CStatusTransformTable*			GetStatusTransformTable() { return m_pStatusTransformTable; }

	CGameManiaTimeTable*			GetGameManiaTimeTable() { return m_pGameManiaTimeTable; }

	//new
	CCharTitleTable*				GetCharTitleTable() { return m_pCharTitleTable; }
	CDwcTable*						GetDwcTable()		{ return m_pDwcTable;	}
	CDwcMissionTable*				GetDwcMissionTable()		{ return m_pDwcMissionTable;	}
	CItemBagListTable*				GetItemBagListTable()	{ return m_pItemBagListTable; }
	CItemEnchantTable*				GetItemEnchantTable()	{ return m_pItemEnchantTable; }
	CItemEnchantRateTable*			GetItemEnchantRateTable()	{ return m_pItemEnchantRateTable; }
	CItemGroupListTable*			GetItemGroupListTable()	{ return m_pItemGroupListTable;	}
	CItemOptionValueProbabilityTable*	GetItemOptionValueProbabilityTable()	{ return m_pItemOptionValueProbabilityTable; }
	CItemUpgradeRateTable*			GetItemUpgradeRateTable()	{ return m_pItemUpgradeRateTable;	}
	CMascotTable*					GetMascotTable()	{ return m_pMascotTable;	}
	CQuestRewardSelectTable*		GetQuestRewardSelectTable()	{ return m_pQuestRewardSelectTable; }
	CRaceTable*						GetRaceTable()	{ return m_pRaceTable; }
	CClassTable*					GetClassTable()	{ return m_pClassTable; }
	CMascotGradeTable*				GetMascotGradeTable()	{ return m_pMascotGradeTable; }
	CMascotStatusTable*				GetMascotStatusTable()	{ return m_pMascotStatusTable; }
	CItemMixExpTable*				GetItemMixExpTable()	{ return m_pItemMixExpTable; }
	CItemUpgradeRateNewTable*		GetItemUpgradeRateNewTable()	{ return m_pItemUpgradeRateNewTable;	}
	CAirCostumeTable*				GetAirCostumeTable()	{ return m_pAirCostumeTable;	}
	CCommonConfigTable*				GetCommonConfigTable()	{ return m_pCommonConfigTable;	}
	CContentsOnOffTable*			GetContentsOnOffTable()	{ return m_pContentsOnOffTable; }
	CNpcServerTable*				GetNpcServerTable()	{ return m_pNpcServerTable; }
	CMobServerTable*				GetMobServerTable()	{ return m_pMobServerTable; }
	CSpecialCharacterTable*			GetSpecialCharacterTable()	{ return m_pSpecialCharacterTable; }
	CContentsConditionTable*		GetContentsConditionTable()	{ return m_pContentsConditionTable; }

// Item
	CBasicDropTable*				GetBasicDropTable() { return m_pBasicDropTable; }

	CDragonBallRewardTable*			GetDragonBallRewardTable() { return m_pDragonBallRewardTable; }
	CDragonBallTable*				GetDragonBallTable() { return m_pDragonBallTable; }
	CDragonBallReturnPointTable*	GetDragonBallReturnPointTable() { return m_pDragonBallReturnPointTable; }

	CItemOptionTable*				GetItemOptionTable() { return m_pItemOptionTable; }

	CItemTable*						GetItemTable() { return m_pItemTable; }

	CLegendaryDropTable*			GetLegendaryDropTable() { return m_pLegendaryDropTable; }

	CNormalDropTable*				GetNormalDropTable() { return m_pNormalDropTable; }

	CSuperiorDropTable*				GetSuperiorDropTable() { return m_pSuperiorDropTable; }

	CUseItemTable*					GetUseItemTable() { return m_pUseItemTable; }

	CSetItemTable*					GetSetItemTable() { return m_pSetItemTable; }

	CEachDropTable*					GetEachDropTable() { return m_pEachDropTable; }

	CTypeDropTable*					GetTypeDropTable() { return m_pTypeDropTable; }
	
	CExcellentDropTable*			GetExcellentDropTable() { return m_pExcellentDropTable; }

	CTableTmp<CDropTableEx::sTBLDAT_DropItem>*			GetDropBagTable() { return m_pDropBag; }
	CTableTmp<CDropTableEx::sTBLDAT_DropBag>*			GetDropBigBagTable() { return m_pDropBigBag; }
	CDropTableEx*										GetDropTableEx() { return &m_DropTableEx; }

	CItemDisassembleTable*			GetItemDisassembleTable() { return m_pItemDisassembleTable; }
	
// Misc
	CActionTable*					GetActionTable() { return m_pActionTable; }

	CChatCommandTable*				GetChatCommandTable() { return m_pChatCommandTable; }

	CDirectionLinkTable *			GetDirectionLinkTable() { return m_pDirectionLinkTable; }

	CFormulaTable*					GetFormulaTable() { return m_pFormulaTable; }

	CServerConfigTable*				GetServerConfigTable() { return m_pServerConfigTable; }

// Party
	CCharmTable*					GetCharmTable() { return m_pCharmTable; }

// Quest
	CQuestDropTable*				GetQuestDropTable() { return m_pQuestDropTable; }

	CQuestItemTable*				GetQuestItemTable() { return m_pQuestItemTable; }

	CQuestProbabilityTable*			GetQuestProbabilityTable() { return m_pQuestProbalityTable; }

	CQuestTextDataTable*			GetQuestTextDataTable() { return m_pQuestTextDataTable; }

	CQuestRewardTable*				GetQuestRewardTable() { return m_pQuestRewardTable; }

// Rule
	CRankBattleTable *				GetRankBattleTable() { return m_pRankBattleTable; }

	CTimeQuestTable *				GetTimeQuestTable() { return m_pTimeQuestTable; }

	CBudokaiTable *					GetBudokaiTable() { return m_pBudokaiTable;}

	CDungeonTable *					GetDungeonTable() { return m_pDungeonTable; }
// Skill
	CHTBSetTable*					GetHTBSetTable() { return m_pHTBSetTable; }

	CSkillTable*					GetSkillTable() { return m_pSkillTable; }

	CSystemEffectTable*				GetSystemEffectTable() { return m_pSystemEffectTable; }

// Text
	CTextAllTable*					GetTextAllTable() { return m_pTextAllTable; }
	CChattingFilterTable*			GetChattingFilterTable() { return m_pChattingFilterTable; }
	CTextServerTable*				GetTextServerTable() { return m_pTextServerTable; }

// World
	CLandMarkTable*					GetLandMarkTable() { return m_pLandMarkTable; } 

	CObjectTable*					GetObjectTable(TBLIDX worldTblidx);

	CSpawnTable*					GetNpcSpawnTable(TBLIDX worldTblidx);

	CSpawnTable*					GetMobSpawnTable(TBLIDX worldTblidx);

	CWorldMapTable*					GetWorldMapTable() { return m_pWorldMapTable; }

	CWorldTable*					GetWorldTable() { return m_pWorldTable; }

	CWorldZoneTable*				GetWorldZoneTable() { return m_pWorldZoneTable; }

// Graphic Data
	CModelToolCharDataTable *		GetModelToolCharDataTable() { return m_pCharDataAnimTable; }

	CModelToolObjDataTable *		GetModelToolObjDataTable() { return m_pObjDataAnimTable; }

// World Data
	CWorldPathTable*				GetWorldPathTable() { return m_pWorldPathTable; }

	CWorldPlayTable*				GetWorldPlayTable() { return m_pWorldPlayTable; }

// Portal Data
	CPortalTable*					GetPortalTable() { return m_pPortalTable; }

	CNpcSpeechTable*				GetNpcSpeechTable() { return m_pNpcSpeechTable; } 
	
	CScriptLinkTable*				GetScriptLinkTable() { return m_pScriptLinkTable; }

	CQuestNarrationTable*			GetQuestNarrationTable() { return m_pQuestNarrationTable; }

	CVehicleTable*					GetVehicleTable() { return m_pVehicleTable; }

	CItemRecipeTable*				GetItemRecipeTable() { return m_pItemRecipeTable; }

	CDynamicObjectTable*			GetDynamicObjectTable() { return m_pDynamicObjectTable; }

	CMobMovePatternTable*			GetMobMovePatternTable() { return m_pMobMovePatternTable; }

	CDojoTable*						GetDojoTable() { return m_pDojoTable; }

	CItemUpgradeTable*				GetItemUpgradeTable() { return m_pItemUpgradeTable; }

	CItemMixMachineTable*			GetItemMixMachineTable() { return m_pItemMixMachineTable; }

	CHLSItemTable*					GetHLSItemTable() { return m_pHLSItemTable; }
	
	CHLSMerchantTable*				GetHLSMerchantTable() { return m_pHLSMerchantTable; }
	CSlotMachineTable*				GetSlotMachineTable() { return m_pSlotMachineTable; }

	CHlsSlotMachineItemTable*		GetSlotMachineItemTable() { return m_pHlsSlotMachineItemTable; }

	CEventSystemTable*				GetEventSystemTable() { return m_pEventSystemTable; }
	CDynamicFieldSystemTable*		GetDynamicFieldSystemTable() { return m_pDynamicFieldSystemTable; }


public:

	OBJTABLEIT						BeginObjectTable();

	OBJTABLEIT						EndObjectTable();

	SPAWNTABLEIT					BeginNpcSpawnTable();

	SPAWNTABLEIT					EndNpcSpawnTable();

	SPAWNTABLEIT					BeginMobSpawnTable();

	SPAWNTABLEIT					EndMobSpawnTable();


protected:

	std::wstring					m_wstrPath;

	CTable::eLOADING_METHOD			m_eLoadingMethod;

	DWORD							m_dwCodePage;

// Character
	CExpTable*						m_pExpTable;

	CGuideHintTable*				m_pGuideHintTable;

	CHelpTable *					m_pHelpTable;

	CMerchantTable*					m_pMerchantTable;

	CMobTable*						m_pMobTable;

	CNewbieTable*					m_pNewbieTable;

	CNPCTable*						m_pNpcTable;

	CPCTable*						m_pPcTable;

	CStatusTransformTable*			m_pStatusTransformTable;

	CGameManiaTimeTable*			m_pGameManiaTimeTable;

	//new
	CCharTitleTable*				m_pCharTitleTable;
	CDwcTable*						m_pDwcTable;
	CDwcMissionTable*				m_pDwcMissionTable;
	CItemBagListTable*				m_pItemBagListTable;
	CItemEnchantTable*				m_pItemEnchantTable;
	CItemEnchantRateTable*			m_pItemEnchantRateTable;
	CItemGroupListTable*			m_pItemGroupListTable;
	CItemOptionValueProbabilityTable*	m_pItemOptionValueProbabilityTable;
	CItemUpgradeRateTable*			m_pItemUpgradeRateTable;
	CMascotTable*					m_pMascotTable;
	CQuestRewardSelectTable*		m_pQuestRewardSelectTable;
	CRaceTable*						m_pRaceTable;
	CClassTable*					m_pClassTable;
	CMascotGradeTable*				m_pMascotGradeTable;
	CMascotStatusTable*				m_pMascotStatusTable;
	CItemMixExpTable*				m_pItemMixExpTable;
	CItemUpgradeRateNewTable*		m_pItemUpgradeRateNewTable;
	CAirCostumeTable*				m_pAirCostumeTable;
	CCommonConfigTable*				m_pCommonConfigTable;
	CContentsOnOffTable*			m_pContentsOnOffTable;
	CNpcServerTable*				m_pNpcServerTable;
	CMobServerTable*				m_pMobServerTable;
	CSpecialCharacterTable*			m_pSpecialCharacterTable;
	CContentsConditionTable*		m_pContentsConditionTable;
	CEventSystemTable*				m_pEventSystemTable;
	CDynamicFieldSystemTable*		m_pDynamicFieldSystemTable;

// Item
	CBasicDropTable*				m_pBasicDropTable;

	CDragonBallRewardTable*			m_pDragonBallRewardTable;
	CDragonBallTable*				m_pDragonBallTable;
	CDragonBallReturnPointTable*	m_pDragonBallReturnPointTable;

	CItemOptionTable*				m_pItemOptionTable;

	CItemTable*						m_pItemTable;

	CLegendaryDropTable*			m_pLegendaryDropTable;

	CNormalDropTable*				m_pNormalDropTable;

	CSuperiorDropTable*				m_pSuperiorDropTable;

	CUseItemTable*					m_pUseItemTable;

	CSetItemTable*					m_pSetItemTable;

	CEachDropTable*					m_pEachDropTable;

	CTypeDropTable*					m_pTypeDropTable;

	CExcellentDropTable*			m_pExcellentDropTable;

	CTableTmp<CDropTableEx::sTBLDAT_DropItem>*		m_pDropBag;
	CTableTmp<CDropTableEx::sTBLDAT_DropBag>*		m_pDropBigBag;

	CDropTableEx					m_DropTableEx;

	CItemDisassembleTable*			m_pItemDisassembleTable;
	
// Misc
	CActionTable*					m_pActionTable;

	CChatCommandTable*				m_pChatCommandTable;

	CDirectionLinkTable *			m_pDirectionLinkTable;

	CFormulaTable*					m_pFormulaTable;

	CServerConfigTable*				m_pServerConfigTable;

// Party
	CCharmTable*					m_pCharmTable;

// Quest
	CQuestDropTable*				m_pQuestDropTable;		// ????? ??? ?????? ?????

	CQuestItemTable*				m_pQuestItemTable;		// ????? ?????? ?????

	CQuestProbabilityTable*			m_pQuestProbalityTable;

	CQuestTextDataTable*			m_pQuestTextDataTable;

	CQuestRewardTable*				m_pQuestRewardTable;

// Rule
	CRankBattleTable *				m_pRankBattleTable;

	CTimeQuestTable *				m_pTimeQuestTable;

	CBudokaiTable *					m_pBudokaiTable;

	CDungeonTable *					m_pDungeonTable;
// Skill
	CHTBSetTable*					m_pHTBSetTable;

	CSkillTable*					m_pSkillTable;

	CSystemEffectTable*				m_pSystemEffectTable;

// Text
	CTextAllTable*					m_pTextAllTable;
	CChattingFilterTable*			m_pChattingFilterTable;
	CTextServerTable*				m_pTextServerTable;

// World
	CLandMarkTable*					m_pLandMarkTable;

	OBJTABLEMAP						m_mapObjectTable;

	SPAWNTABLEMAP					m_mapNpcSpawnTable;

	SPAWNTABLEMAP					m_mapMobSpawnTable;

	CWorldMapTable*					m_pWorldMapTable;

	CWorldTable*					m_pWorldTable;

	CWorldZoneTable*				m_pWorldZoneTable;

	CWorldPlayTable*				m_pWorldPlayTable;

// Graphic Data
	CModelToolCharDataTable	*		m_pCharDataAnimTable;

	CModelToolObjDataTable *		m_pObjDataAnimTable;

// World Data
	CWorldPathTable*				m_pWorldPathTable;

// Portal Data
	CPortalTable*					m_pPortalTable;

	CNpcSpeechTable*				m_pNpcSpeechTable;	

	CScriptLinkTable*				m_pScriptLinkTable;

	CQuestNarrationTable*			m_pQuestNarrationTable;

	CVehicleTable*					m_pVehicleTable; 

	CItemRecipeTable*				m_pItemRecipeTable;

	CDynamicObjectTable*			m_pDynamicObjectTable;

	CMobMovePatternTable*			m_pMobMovePatternTable;

	CDojoTable*						m_pDojoTable;

	CItemUpgradeTable*				m_pItemUpgradeTable;

	CItemMixMachineTable*			m_pItemMixMachineTable;

	CHLSItemTable*					m_pHLSItemTable; 
	
	CHLSMerchantTable*				m_pHLSMerchantTable; 

	CSlotMachineTable*				m_pSlotMachineTable;

	CHlsSlotMachineItemTable*		m_pHlsSlotMachineItemTable;
};