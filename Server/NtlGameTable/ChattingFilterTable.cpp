#include "stdafx.h"
#include "ChattingFilterTable.h"
#include "NtlDebug.h"
#include "NtlSerializer.h"

//- yoshiki : Let's consider of implementing NtlAssert series.
//#include "NtlAssert.h"

// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CChattingFilterTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CChattingFilterTable::CChattingFilterTable(void)
{
	Init();
}

CChattingFilterTable::~CChattingFilterTable(void)
{
	Destroy();
}

bool CChattingFilterTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CChattingFilterTable::Destroy()
{
	CTable::Destroy();
}

void CChattingFilterTable::Init()
{
}

void* CChattingFilterTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sCHAT_FILTER_TBLDAT* pFilter = new sCHAT_FILTER_TBLDAT;
		if (NULL == pFilter)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pFilter;
			return NULL;
		}
		
		m_dwCodePage = dwCodePage;
		return pFilter;
	}

	return NULL;
}

bool CChattingFilterTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sCHAT_FILTER_TBLDAT* pFilter = (sCHAT_FILTER_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pFilter, sizeof(*pFilter)))
			return false;

		delete pFilter;

		return true;
	}

	return false;
}

bool CChattingFilterTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);

	sCHAT_FILTER_TBLDAT* pTbldat = (sCHAT_FILTER_TBLDAT*) pvTable;

	if ( false == m_mapTableList.insert(std::pair<TBLIDX, sTBLDAT*>(pTbldat->tblidx, pTbldat)).second )
	{
		WCHAR wszFormatBuf[512];
		FormatStringToWCHAR(L"[File] : %s\r\n Table Tblidx[%u] is Duplicated ", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
		CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, pTbldat->tblidx );
		_ASSERTE( 0 );
		return false;
	}


	return true;
}

bool CChattingFilterTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sCHAT_FILTER_TBLDAT* pFilter = (sCHAT_FILTER_TBLDAT*)pvTable;

		if (0 == WStringCmpLiteral(*pstrDataName, L"Chatting_Filter_Tblidx"))
		{
			pFilter->tblidx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Slang_Text"))
		{
			READ_STRINGW(bstrData, pFilter->wszSlangText, _countof(pFilter->wszSlangText));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Filtering_Text_Index"))
		{
			pFilter->filteringTextIndex = READ_DWORD( bstrData );
		}
		else
		{
			WCHAR wszFormatBuf[512];
			FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
			return false;
		}
	}
	else
	{
		return false;
	}

	return true;
}


sTBLDAT* CChattingFilterTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;

	return (sTBLDAT*)(iter->second); 
}

bool CChattingFilterTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
{
	if( false == bReload )
	{
		Reset();
	}

	BYTE byMargin = 1;
	serializer >> byMargin;

	bool bLoop = true;
	do
	{
		sCHAT_FILTER_TBLDAT* pTableData = new sCHAT_FILTER_TBLDAT;
		if (NULL == pTableData)
		{
			//- yoshiki : To log system!
			Destroy();
			return false;
		}

		if (false == pTableData->LoadFromBinary(serializer))
		{
			delete pTableData;
			bLoop = false;
			break;
		}

		//  [4/26/2008 zeroera] : ???? : ????????? Load?? ??????? File Loading???? ???????
		if( false == AddTable(pTableData, bReload, bUpdate) )
		{
			delete pTableData;
		}

	} while (false != bLoop);

	return true;
}

bool CChattingFilterTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sCHAT_FILTER_TBLDAT* pTableData = (sCHAT_FILTER_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}