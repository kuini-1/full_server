#pragma once
//created 3.12.2014
#include "Table.h"


#pragma pack(push, 4)
struct sITEM_OPTION_VALUE_PROBABILITY_TBLDAT : public sTBLDAT
{

public:

	BYTE		byOptionValuePercent;
	DWORD		dwOptionProb_Superior;
	DWORD		dwOptionProb_Excellent;
	DWORD		dwOptionProb_Rare;
	DWORD		dwOptionProb_Legendary;

protected:
	virtual int GetDataSize()
	{
		return sizeof(*this) - sizeof(void*);
	}
};
#pragma pack(pop)

class CItemOptionValueProbabilityTable : public CTable
{
public:
	CItemOptionValueProbabilityTable(void);
	virtual ~CItemOptionValueProbabilityTable(void);

	bool Create(DWORD dwCodePage);
	void Destroy();

protected:
	void Init();

public:
	sTBLDAT* FindData(TBLIDX tblidx);

	BYTE		GetOptionValueInPercent(BYTE byItemRank);

protected:
	WCHAR** GetSheetListInWChar() { return &(CItemOptionValueProbabilityTable::m_pwszSheetList[0]); }
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