#pragma once
//created 5.5.2015
#include "Table.h"

const DWORD		DBO_MAX_LENGTH_CONTENTS_CONDITION_NOTE = 128;

#pragma pack(push, 4)
struct sCONTENTS_CONDITION_TBLDAT : public sTBLDAT
{

public:

	DWORD	dwName;
	WCHAR	wszNote[DBO_MAX_LENGTH_CONTENTS_CONDITION_NOTE + 1];
	DWORD	dwConditionType;
	QWORD	nValue;
	BYTE	byOutputType;
	DWORD	adwTargetInfo[20];


protected:

	virtual int GetDataSize()
	{
		return sizeof(*this) - sizeof(void*);
	}
};
#pragma pack(pop)

class CContentsConditionTable : public CTable
{
public:

	CContentsConditionTable(void);
	virtual ~CContentsConditionTable(void);

	bool Create(DWORD dwCodePage);
	void Destroy();

protected:
	void Init();

public:
	sTBLDAT *			FindData(TBLIDX tblidx); 

protected:
	WCHAR** GetSheetListInWChar() { return &(CContentsConditionTable::m_pwszSheetList[0]); }
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