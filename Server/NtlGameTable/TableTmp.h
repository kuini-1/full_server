#pragma once


#include "Table.h"


template < typename T1 >
class CTableTmp : public CTable
{
public:

	CTableTmp() {}
	virtual ~CTableTmp() {}


protected:

protected:
	WCHAR**						GetSheetListInWChar();
	void*						AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage);
	bool						DeallocNewTable(void* pvTable, WCHAR* pwszSheetName);
	bool						AddTable(void * pvTable, bool bReload, bool bUpdate);
	bool						SetTableData(void *pvTable, WCHAR *pwszSheetName, std::wstring *pstrDataName, BSTR bstrData);


public:

	virtual bool				LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate);

	virtual bool				SaveToBinary(CNtlSerializer& serializer);


public:

	virtual sTBLDAT *			FindData(TBLIDX tblidx);



private:
	static WCHAR*				m_pwszSheetList[];

};

template<typename T1>
WCHAR* CTableTmp<T1>::m_pwszSheetList[] =
{
	L"Table_Data_KOR",
	NULL
};

template<typename T1>
inline WCHAR** CTableTmp<T1>::GetSheetListInWChar()
{
	return &(CTableTmp<T1>::m_pwszSheetList[0]);
}


template<typename T1>
inline void * CTableTmp<T1>::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	UNREFERENCED_PARAMETER(pwszSheetName);

	T1* pNewTable = new T1;
	if (NULL == pNewTable)
		return NULL;

	CPINFO cpInfo;
	if (false == GetCPInfo(dwCodePage, &cpInfo))
	{
		delete pNewTable;
		return NULL;
	}

	m_dwCodePage = dwCodePage;
	return pNewTable;
}

template<typename T1>
inline bool CTableTmp<T1>::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	UNREFERENCED_PARAMETER(pwszSheetName);

	T1* pTable = (T1*)pvTable;
	if (FALSE != IsBadReadPtr(pTable, sizeof(*pTable)))
		return false;

	delete pTable;

	return true;
}

template<typename T1>
inline bool CTableTmp<T1>::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);

	T1* pTbldat = (T1*)pvTable;

	if (false == m_mapTableList.insert(std::pair<TBLIDX, sTBLDAT*>(pTbldat->tblidx, pTbldat)).second)
	{
		CTable::CallErrorCallbackFunction(L"[File] : %s\r\n Table Tblidx[%u] is Duplicated ", m_wszXmlFileName, pTbldat->tblidx);
		_ASSERTE(0);
		return false;
	}

	return true;
}

template <typename T1>
bool CTableTmp<T1>::SetTableData(void *pvTable, WCHAR *pwszSheetName, std::wstring *pstrDataName, BSTR bstrData) {
	UNREFERENCED_PARAMETER(pvTable);
	UNREFERENCED_PARAMETER(pwszSheetName);
	UNREFERENCED_PARAMETER(pstrDataName);
	UNREFERENCED_PARAMETER(bstrData);
	return true;
}

template<typename T1>
inline bool CTableTmp<T1>::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
{
	if (false == bReload)
	{
		Reset();
	}

	BYTE byMargin = 1;
	serializer >> byMargin;

	bool bLoop = true;
	do
	{
		T1* pTableData = new T1;
		if (NULL == pTableData)
		{
			Destroy();
			return false;
		}

		if (false == pTableData->LoadFromBinary(serializer))
		{
			delete pTableData;
			bLoop = false;
			break;
		}

	//	printf("pTableData->tblidx %d \n", pTableData->tblidx);
		if (false == AddTable(pTableData, bReload, bUpdate))
		{
			delete pTableData;
			return false;
		}

	} while (false != bLoop);

	return true;
}

template<typename T1>
inline bool CTableTmp<T1>::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin(); End() != iter; iter++)
	{
		T1* pTableData = (T1*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}



template<typename T1>
inline sTBLDAT * CTableTmp<T1>::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;

	return (sTBLDAT*)(iter->second);
}
