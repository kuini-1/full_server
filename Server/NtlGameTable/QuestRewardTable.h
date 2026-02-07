#pragma once
//last update 1.12.2014
#include "Table.h"
#include "NtlQuest.h"


#pragma pack(push, 4)

enum eTABLE_REWARD_TYPE
{
	TABLE_REWARD_TYPE_NORMAL_ITEM = 0x0,
	TABLE_REWARD_TYPE_QUEST_ITEM = 0x1,
	TABLE_REWARD_TYPE_EXP = 0x2,
	TABLE_REWARD_TYPE_SKILL = 0x3,
	TABLE_REWARD_TYPE_ZENY = 0x4,
	TABLE_REWARD_TYPE_CHANGE_CLASS = 0x5,
	TABLE_REWARD_TYPE_BUFF = 0x6,
	TABLE_REWARD_TYPE_PROBABILITY = 0x7,

	TABLE_REWARD_TYPE_PASSIVE_SKILL = 11,

	TABLE_REWARD_TYPE_SUB_REWARD_INDEX = 0x64,
	INVALID_TEMP_REWARD_TYPE = 0xFF
};

struct sQUEST_REWARD_TBLDAT : public sTBLDAT
{
	sQUEST_REWARD_TBLDAT()
	{
		memset( arsDefRwd, 0xff, sizeof( arsDefRwd ) );
		memset( arsSelRwd, 0xff, sizeof( arsSelRwd ) );
	}

public:
	DWORD						dwDef_Reward_EXP;
	DWORD						dwDef_Reward_Zeny;
	sQUEST_REWARD_DATASET		arsDefRwd[QUEST_REWARD_DEF_MAX_CNT];
	sQUEST_REWARD_DATASET		arsSelRwd[QUEST_REWARD_SEL_MAX_CNT];

protected:

	virtual int GetDataSize()
	{
		return sizeof(*this) - sizeof(void*);
	}
};
#pragma pack(pop)



class CQuestRewardTable : public CTable
{
public:
	CQuestRewardTable(void);
	virtual ~CQuestRewardTable(void);

public:
	bool 						Create(DWORD dwCodePage);
	void 						Destroy();

protected:
	void 						Init();

public:
	sTBLDAT *					FindData(TBLIDX tblidx); 
	

protected:
	WCHAR**						GetSheetListInWChar() { return &(CQuestRewardTable::m_pwszSheetList[0]); }
	void*						AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage);
	bool 						DeallocNewTable(void* pvTable, WCHAR* pwszSheetName);
	bool 						AddTable(void * pvTable, bool bReload, bool bUpdate);
	bool 						SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData);


public:
	virtual bool				LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate);
	virtual bool				SaveToBinary(CNtlSerializer& serializer);


private:
	static WCHAR* m_pwszSheetList[];
};

