#pragma once
//created 5.5.2015
#include "Table.h"


#pragma pack(push, 4)
struct sSPECIALCHARACTER_TBLDAT : public sTBLDAT
{

public:

	BYTE	bySuperiorType;
	BYTE	byMinValue;
	BYTE	byMaxValue;
	TBLIDX	startTblidx;
	TBLIDX	endTblidx;
	TBLIDX	TextAllTblidx;
	BYTE	byRate[5];


protected:

	virtual int GetDataSize()
	{
		return sizeof(*this) - sizeof(void*);
	}
};
#pragma pack(pop)

class CSpecialCharacterTable : public CTable
{
public:

	CSpecialCharacterTable(void);
	virtual ~CSpecialCharacterTable(void);

	bool Create(DWORD dwCodePage);
	void Destroy();

protected:
	void Init();

public:
	sTBLDAT *			FindData(TBLIDX tblidx); 

protected:
	WCHAR** GetSheetListInWChar() { return const_cast<WCHAR**>(&(CSpecialCharacterTable::m_pwszSheetList[0])); }
	void* AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage);
	bool DeallocNewTable(void* pvTable, WCHAR* pwszSheetName);
	bool AddTable(void * pvTable, bool bReload, bool bUpdate);
	bool SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData);


public:

	virtual bool				LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate);

	virtual bool				SaveToBinary(CNtlSerializer& serializer);



private:
	static const WCHAR* m_pwszSheetList[];
};