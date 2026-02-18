#include "stdafx.h"
#include "ScriptLinkTable.h"
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

const WCHAR* CScriptLinkTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CScriptLinkTable::CScriptLinkTable(void)
{
	Init();
}

CScriptLinkTable::~CScriptLinkTable(void)
{
	Destroy();
}

bool CScriptLinkTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CScriptLinkTable::Destroy()
{
	CTable::Destroy();
}

void CScriptLinkTable::Init()
{
}

void* CScriptLinkTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sSCRIPT_LINK_TBLDAT* pLink = new sSCRIPT_LINK_TBLDAT;
		if (NULL == pLink)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pLink;
			return NULL;
		}
		
		m_dwCodePage = dwCodePage;
		return pLink;
	}

	return NULL;
}

bool CScriptLinkTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sSCRIPT_LINK_TBLDAT* pLink = (sSCRIPT_LINK_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pLink, sizeof(*pLink)))
			return false;

		delete pLink;

		return true;
	}

	return false;
}

bool CScriptLinkTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);

	sSCRIPT_LINK_TBLDAT* pTbldat = (sSCRIPT_LINK_TBLDAT*) pvTable;

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

bool CScriptLinkTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sSCRIPT_LINK_TBLDAT* pLink = (sSCRIPT_LINK_TBLDAT*)pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			pLink->tblidx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Value"))
		{
			READ_STRINGW(bstrData, pLink->wszValue, _countof(pLink->wszValue));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Type"))
		{
			pLink->byType = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Action"))
		{
			pLink->byAction = READ_BYTE( bstrData, wszFieldNameBuf );
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


sTBLDAT* CScriptLinkTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;

	return (sTBLDAT*)(iter->second); 
}

bool CScriptLinkTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sSCRIPT_LINK_TBLDAT* pTableData = new sSCRIPT_LINK_TBLDAT;
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

	//	printf("pTableData->tblidx %d \n", pTableData->tblidx);
		if( false == AddTable(pTableData, bReload, bUpdate) )
		{
			delete pTableData;
		}

	} while (false != bLoop);

	return true;
}

bool CScriptLinkTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sSCRIPT_LINK_TBLDAT* pTableData = (sSCRIPT_LINK_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}