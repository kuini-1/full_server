#include "stdafx.h"
#include "GuideHintTable.h"
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

const WCHAR* CGuideHintTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CGuideHintTable::CGuideHintTable(void)
{
	Init();
}

CGuideHintTable::~CGuideHintTable(void)
{
	Destroy();
}

bool CGuideHintTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CGuideHintTable::Destroy()
{
	CTable::Destroy();
}

void CGuideHintTable::Init()
{
}

void* CGuideHintTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sGUIDE_HINT_TBLDAT* pGuide = new sGUIDE_HINT_TBLDAT;
		if (NULL == pGuide)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pGuide;
			return NULL;
		}
		
		m_dwCodePage = dwCodePage;
		return pGuide;
	}

	return NULL;
}

bool CGuideHintTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sGUIDE_HINT_TBLDAT* pGuide = (sGUIDE_HINT_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pGuide, sizeof(*pGuide)))
			return false;

		delete pGuide;

		return true;
	}

	return false;
}

bool CGuideHintTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);

	sGUIDE_HINT_TBLDAT* pTbldat = (sGUIDE_HINT_TBLDAT*) pvTable;

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

bool CGuideHintTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sGUIDE_HINT_TBLDAT* pGuide = (sGUIDE_HINT_TBLDAT*)pvTable;

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			pGuide->tblidx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Type"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pGuide->byType = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"WidthPosition"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pGuide->byWidthPosition = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"HeightPosition"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pGuide->byHeightPosition = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"WidthSize"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pGuide->wWidthSize = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"HeightSize"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pGuide->wHeightSize = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Resource"))
		{
			READ_STRING(bstrData, pGuide->szResource, _countof(pGuide->szResource));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Note"))
		{
			READ_STRING(bstrData, pGuide->szNote, _countof(pGuide->szNote));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Auto_Show"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pGuide->bAutoShow = READ_BOOL( bstrData, wszFieldNameBuf );
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


sTBLDAT* CGuideHintTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;

	return (sTBLDAT*)(iter->second); 
}

bool CGuideHintTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sGUIDE_HINT_TBLDAT* pTableData = new sGUIDE_HINT_TBLDAT;
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

bool CGuideHintTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sGUIDE_HINT_TBLDAT* pTableData = (sGUIDE_HINT_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}