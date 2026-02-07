#pragma once
//created 2.12.2014
#include "Table.h"


#pragma pack(push, 4)
struct sITEM_ENCHANT_RATE_TBLDAT : public sTBLDAT
{

public:

	BYTE		bySuperiorRate;
	BYTE		byExcellentRate;
	BYTE		byRareRate;
	BYTE		byLegendaryRate;
	

protected:
	virtual int GetDataSize()
	{
		return sizeof(*this) - sizeof(void*);
	}
};
#pragma pack(pop)

class CItemEnchantRateTable : public CTable
{
public:
	CItemEnchantRateTable(void);
	virtual ~CItemEnchantRateTable(void);

	bool Create(DWORD dwCodePage);
	void Destroy();

protected:
	void Init();

public:
	sTBLDAT* FindData(TBLIDX tblidx);

protected:
	WCHAR** GetSheetListInWChar() { return &(CItemEnchantRateTable::m_pwszSheetList[0]); }
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