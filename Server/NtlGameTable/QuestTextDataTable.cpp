#include "stdafx.h"
#include "NtlDebug.h"

#include "QuestTextDataTable.h"
#include "NtlSerializer.h"

// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CQuestTextDataTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CQuestTextDataTable::CQuestTextDataTable(void)
{
	Init();
}

CQuestTextDataTable::~CQuestTextDataTable(void)
{
	Destroy();
}

bool CQuestTextDataTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CQuestTextDataTable::Destroy()
{
	CTable::Destroy();
}

void CQuestTextDataTable::Init()
{
}

void* CQuestTextDataTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sQUEST_TEXT_DATA_TBLDAT* pQuestTextData = new sQUEST_TEXT_DATA_TBLDAT;
		if (NULL == pQuestTextData)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pQuestTextData;
			return NULL;
		}
		
		m_dwCodePage = dwCodePage;
		return pQuestTextData;
	}
	
	return NULL;
}

bool CQuestTextDataTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sQUEST_TEXT_DATA_TBLDAT* pQuestTextData = (sQUEST_TEXT_DATA_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pQuestTextData, sizeof(*pQuestTextData)))
			return false;

		delete pQuestTextData;

		return true;
	}
	
	return false;
}

bool CQuestTextDataTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);

	sQUEST_TEXT_DATA_TBLDAT* pTbldat = (sQUEST_TEXT_DATA_TBLDAT*)pvTable;
	if( false == m_mapTableList.insert(std::pair<TBLIDX, sTBLDAT*>(pTbldat->tblidx, pTbldat)).second )
	{
		WCHAR wszFormatBuf[512];
		FormatStringToWCHAR(L"[File] : %s\r\n Table Tblidx[%u] is Duplicated ", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
		CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, pTbldat->tblidx );
		_ASSERTE( 0 );
		return false;
	}


	return true;
}

bool CQuestTextDataTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sQUEST_TEXT_DATA_TBLDAT* pQuestTextData = (sQUEST_TEXT_DATA_TBLDAT*)pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		if (0 == WStringCmpLiteral(*pstrDataName, L"Quest_Text_Index"))
		{
			pQuestTextData->tblidx = READ_TBLIDX(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Quest_Text"))
		{
			if ( false == READ_STR( pQuestTextData->wstrText, bstrData) )
			{
				return false;
			}
		}
		else
		{
			WCHAR wszFormatBuf[512];
			FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
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


sTBLDAT* CQuestTextDataTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;
	
	return (sTBLDAT*)(iter->second); 
}

void CQuestTextDataTable::FindDataRange( TBLIDX tblBegin, TBLIDX tblEnd, std::vector<sQUEST_TEXT_DATA_TBLDAT*>& vecRangeList )
{
	TABLEIT iter = m_mapTableList.begin();
	for ( ; iter != m_mapTableList.end(); ++iter )
	{
		TBLIDX tblIdx = iter->first;

		if ( tblIdx < tblBegin )
			continue;

		if ( tblIdx > tblEnd )
			break;

		vecRangeList.push_back( (sQUEST_TEXT_DATA_TBLDAT*)iter->second );
	}
}

bool CQuestTextDataTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
{
	if( false == bReload )
	{
		Reset();
	}

	BYTE byMargin = 1;
	serializer >> byMargin;

	while (0 < serializer.GetDataSize())
	{
		sQUEST_TEXT_DATA_TBLDAT* pTableData = new sQUEST_TEXT_DATA_TBLDAT;
		if (NULL == pTableData)
		{
			//- yoshiki : To log system!
			Destroy();
			return false;
		}

		WORD wTextLength = 0;
		if (serializer.GetDataSize() < sizeof(pTableData->tblidx) + sizeof(wTextLength))
		{
			//- yoshiki : To log system!

			delete pTableData;

			Destroy();
			return false;
		}
		serializer >> pTableData->tblidx;
		serializer >> wTextLength;

		if (serializer.GetDataSize() < (int)(wTextLength * sizeof(WCHAR)))
		{
			//- yoshiki : To log system!

			delete pTableData;

			Destroy();
			return false;
		}
		
		WCHAR* pwszText = new WCHAR[wTextLength + 1];
		if (NULL == pwszText)
		{
			//- yoshiki : To log system!

			delete pTableData;

			Destroy();
			return false;
		}
		serializer.Out(pwszText, wTextLength * sizeof(WCHAR));
		pwszText[wTextLength] = L'\0';

		/* WCHAR* to std::wstring: on Linux WCHAR is unsigned short, std::wstring is wchar_t (UTF-32) */
		{
			size_t len = WCHARLen(pwszText);
			wchar_t* wbuf = new wchar_t[len + 1];
			for (size_t i = 0; i <= len; i++)
				wbuf[i] = (wchar_t)pwszText[i];
			pTableData->wstrText = std::wstring(wbuf);
			delete[] wbuf;
		}
		delete [] pwszText;

		//  [4/26/2008 zeroera] : ???? : ????????? Load?? ??????? File Loading???? ???????
		if( false == AddTable(pTableData, bReload, bUpdate) )
		{
			delete pTableData;
		}
	};

	return true;
}

bool CQuestTextDataTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sQUEST_TEXT_DATA_TBLDAT* pTableData = (sQUEST_TEXT_DATA_TBLDAT*)(iter->second);

		serializer << pTableData->tblidx;
		serializer << (WORD)((pTableData->wstrText).size());
		serializer.In((pTableData->wstrText).c_str(), (int)((pTableData->wstrText).size() * sizeof(WCHAR)));
	}

	return true;
}