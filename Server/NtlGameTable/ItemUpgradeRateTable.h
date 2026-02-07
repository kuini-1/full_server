#pragma once
//created 3.12.2014
#include "Table.h"




#pragma pack(push, 4)
struct sITEM_UPGRADE_RATE_TBLDAT : public sTBLDAT
{

public:

	BYTE		byItem_Type;
	BYTE		byGrade;
	BYTE		byHigh_Upgrade_Success_1;
	BYTE		byHigh_Upgrade_Success_2;
	BYTE		byHigh_Upgrade_Success_3;
	float		fAdditional_Ability;
	

protected:
	virtual int GetDataSize()
	{
		return sizeof(*this) - sizeof(void*);
	}
};
#pragma pack(pop)

class CItemUpgradeRateTable : public CTable
{
public:
	CItemUpgradeRateTable(void);
	virtual ~CItemUpgradeRateTable(void);

	bool Create(DWORD dwCodePage);
	void Destroy();

protected:
	void Init();

public:
	sTBLDAT* FindData(TBLIDX tblidx);

protected:
	WCHAR** GetSheetListInWChar() { return &(CItemUpgradeRateTable::m_pwszSheetList[0]); }
	void* AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage);
	bool DeallocNewTable(void* pvTable, WCHAR* pwszSheetName);
	bool AddTable(void * pvTable, bool bReload, bool bUpdate);
	bool SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData);

public:

	virtual bool				LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate);

	virtual bool				SaveToBinary(CNtlSerializer& serializer);


private:
	static WCHAR* m_pwszSheetList[];
};