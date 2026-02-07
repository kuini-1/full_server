#pragma once
//created 3.12.2014
#include "Table.h"


#pragma pack(push, 4)
struct sCLASS_TBLDAT : public sTBLDAT
{

public:

	WORD			wClassID;
	WORD			charStr;
	WORD			charCon;
	WORD			charFoc;
	WORD			charDex;
	WORD			charSol;
	WORD			charEng;
	float			fRate_PhyDefence;
	float			fRate_EngDefence;
	float			fRate_Stomachache_Defence;
	float			fRate_Poison_Defence;
	float			fRate_Bleed_Defence;
	float			fRate_Burn_Defence;


protected:

	virtual int GetDataSize()
	{
		return sizeof(*this) - sizeof(void*);
	}
};
#pragma pack(pop)

class CClassTable : public CTable
{
public:

	CClassTable(void);
	virtual ~CClassTable(void);

	bool Create(DWORD dwCodePage);
	void Destroy();

protected:
	void Init();

public:
	sTBLDAT *			FindData(TBLIDX tblidx); 

protected:
	WCHAR** GetSheetListInWChar() { return &(CClassTable::m_pwszSheetList[0]); }
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