//***********************************************************************************
//
//	File		:	ItemUpgradeTable.h
//
//	Begin		:	2009-04-10
//
//	Copyright	:	¨Ï NTL-Inc Co., Ltd
//
//	Author		:	Jace Jung ( sooshia@ntl-inc.com )
//
//***********************************************************************************

#pragma once

#include "Table.h"

const DWORD	DBO_MAX_LENGTH_ITEM_UPGRADE_NAME_TEXT = 32;

#pragma pack(push, 4)
struct sITEM_UPGRADE_TBLDAT : public sTBLDAT
{

public:
	WCHAR			wszNameText[ DBO_MAX_LENGTH_ITEM_UPGRADE_NAME_TEXT + 1 ];
	WORD			wUp1;
	WORD			wUp2;
	WORD			wUp3;
	WORD			wUp4;
	WORD			wUp5;
	WORD			wUp6;
	WORD			wUp7;
	WORD			wUp8;
	WORD			wUp9;
	WORD			wUp10;
	WORD			wUp11;
	WORD			wUp12;
	WORD			wUp13;
	WORD			wUp14;
	WORD			wUp15;

public:

	virtual int GetDataSize()
	{
		return sizeof(*this) - sizeof(void*);
	}
};
#pragma pack(pop)

class CItemUpgradeTable : public CTable
{
public:
	CItemUpgradeTable(void);
	virtual	~CItemUpgradeTable(void);

	bool			Create(DWORD dwCodePage);
	void			Destroy();

protected:
	void			Init();

public:
	sTBLDAT*		FindData(TBLIDX tblidx); 

protected:
	WCHAR** GetSheetListInWChar() { return const_cast<WCHAR**>(&(CItemUpgradeTable::m_pwszSheetList[0])); }
	void*			AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage);
	bool			DeallocNewTable(void* pvTable, WCHAR* pwszSheetName);
	bool			AddTable(void * pvTable, bool bReload, bool bUpdate);
	bool			SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData);

public:
	virtual bool	LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate);
	virtual bool	SaveToBinary(CNtlSerializer& serializer);

public:
	WORD			GetItemUpgradeValue(BYTE byGrade, sITEM_UPGRADE_TBLDAT* tbldat);

private:
	static const WCHAR*	m_pwszSheetList[];
};