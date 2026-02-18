#include "stdafx.h"
#include "TextServerTable.h"
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

const WCHAR* CTextServerTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CTextServerTable::CTextServerTable(void)
{
	Init();
}

CTextServerTable::~CTextServerTable(void)
{
	Destroy();
}

bool CTextServerTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CTextServerTable::Destroy()
{
	CTable::Destroy();
}

void CTextServerTable::Init()
{
}

void* CTextServerTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sTEXT_SERVER_TBLDAT* pText = new sTEXT_SERVER_TBLDAT;
		if (NULL == pText)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pText;
			return NULL;
		}

		m_dwCodePage = dwCodePage;
		return pText;
	}

	return NULL;
}

bool CTextServerTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sTEXT_SERVER_TBLDAT* pText = (sTEXT_SERVER_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pText, sizeof(*pText)))
			return false;

		delete pText;

		return true;
	}

	return false;
}

bool CTextServerTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);

	sTEXT_SERVER_TBLDAT* pTbldat = (sTEXT_SERVER_TBLDAT*) pvTable;

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

bool CTextServerTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sTEXT_SERVER_TBLDAT* pText = (sTEXT_SERVER_TBLDAT*)pvTable;

		if (0 == WStringCmpLiteral(*pstrDataName, L"Server_Text_Index"))
		{
			pText->tblidx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Server_Text"))
		{
			READ_STR( pText->wstrText, bstrData);
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


sTBLDAT* CTextServerTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;

	return (sTBLDAT*)(iter->second); 
}

bool CTextServerTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
{
	if( false == bReload )
	{
		Reset();
	}

	BYTE byMargin = 1;
	serializer >> byMargin;

	while (0 < serializer.GetDataSize())
	{
		sTEXT_SERVER_TBLDAT* pTableData = new sTEXT_SERVER_TBLDAT;
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

bool CTextServerTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sTEXT_SERVER_TBLDAT* pTableData = (sTEXT_SERVER_TBLDAT*)(iter->second);

		serializer << pTableData->tblidx;
		serializer << (WORD)((pTableData->wstrText).size());
		serializer.In((pTableData->wstrText).c_str(), (int)((pTableData->wstrText).size() * sizeof(WCHAR)));
	}

	return true;
}
