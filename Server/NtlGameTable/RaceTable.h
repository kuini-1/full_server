#pragma once
//created 3.12.2014
#include "Table.h"


#pragma pack(push, 4)
struct sRACE_TBLDAT : public sTBLDAT
{

public:

	WORD			wRaceID;
	WORD			charStr;
	WORD			charCon;
	WORD			charFoc;
	WORD			charDex;
	WORD			charSol;
	WORD			charEng;
	float			fRewardRate;
	float			fRate_Grade_Defence;


protected:

	virtual int GetDataSize()
	{
		return sizeof(*this) - sizeof(void*);
	}
};
#pragma pack(pop)

class CRaceTable : public CTable
{
public:

	CRaceTable(void);
	virtual ~CRaceTable(void);

	bool Create(DWORD dwCodePage);
	void Destroy();

protected:
	void Init();

public:
	sTBLDAT *			FindData(TBLIDX tblidx); 

protected:
	WCHAR** GetSheetListInWChar() { return &(CRaceTable::m_pwszSheetList[0]); }
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