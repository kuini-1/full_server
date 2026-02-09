//***********************************************************************************
//
//	File		:	CheckTable.h
//
//	Begin		:	2006-08-02
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Desc		:	Chung Doo sup  ( john@ntl-inc.com )
//
//************************************************************************************

#pragma once

#if defined(_WIN32)
#include <comutil.h>
#else
#ifndef BSTR
typedef WCHAR* BSTR;
#endif
#endif
#include <string>
#include <map>
#include "NtlSharedType.h"
#include "Table.h"

struct sTBLDATCHECK
{
	sTBLDATCHECK()
	{
		wstrField_Name = L"";
	}
	virtual ~sTBLDATCHECK() {}
	std::wstring	wstrField_Name;
};

class CCheckTable
{
public:

	typedef std::map<std::wstring , sTBLDATCHECK *> CHTABLE;
	typedef CHTABLE::iterator CHTABLEIT;
	typedef CHTABLE::value_type CHTABLEVAL;
	
public:

	CCheckTable(void);
	virtual ~CCheckTable(void);
	
	bool Create(WCHAR* pwszFileName, DWORD dwCodePage);
	bool Create(char* pszFileName, DWORD dwCodePage);
	void Destroy();

protected:

	void Init();

public:

	CHTABLEIT	Begin() { return m_mapTableList.begin(); }
	CHTABLEIT	End() { return m_mapTableList.end(); }
	virtual sTBLDATCHECK *			FindData(std::wstring wstrField) { (void) wstrField; return NULL; }
	size_t GetNumberOfTables() { return m_mapTableList.size(); }

protected:

	virtual WCHAR** GetSheetListInWChar() = 0;
	virtual void* AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage) = 0;
	virtual bool DeallocNewTable(void* pvTable, WCHAR* pwszSheetName) = 0;
	virtual bool AddTable(void* pvTable, WCHAR* pwszSheetName) = 0;
	virtual bool SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData) = 0;

	static DWORD HexToDec(WCHAR* pwszHexString);

protected:
	CHTABLE						m_mapTableList;

protected:
	DWORD						m_dwCodePage;

	// This is only for displaying a message, so it can't be used for the other purposes.
	// ??????? ?????? ???? ????? ??? ????????, ?????? ?? ?????? ?????? ?? ???.
	// by YOSHIKI(2007-08-07)
	WCHAR						m_wszXmlFileName[CTable::TABLE_XML_FILE_NAME_MAX_LENGTH + 1];
};