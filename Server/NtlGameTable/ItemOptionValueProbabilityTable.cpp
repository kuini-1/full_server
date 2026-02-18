#include "stdafx.h"
#include "ItemOptionValueProbabilityTable.h"
#include "NtlDebug.h"
#include "NtlSerializer.h"

// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CItemOptionValueProbabilityTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CItemOptionValueProbabilityTable::CItemOptionValueProbabilityTable(void)
{
	Init();
}

CItemOptionValueProbabilityTable::~CItemOptionValueProbabilityTable(void)
{
	Destroy();
}

bool CItemOptionValueProbabilityTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CItemOptionValueProbabilityTable::Destroy()
{
	CTable::Destroy();
}

void CItemOptionValueProbabilityTable::Init()
{
}

void* CItemOptionValueProbabilityTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sITEM_OPTION_VALUE_PROBABILITY_TBLDAT* pNewHelp = new sITEM_OPTION_VALUE_PROBABILITY_TBLDAT;
		if (NULL == pNewHelp)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pNewHelp;
			return NULL;
		}
		
		m_dwCodePage = dwCodePage;
		return pNewHelp;
	}

	return NULL;
}

bool CItemOptionValueProbabilityTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sITEM_OPTION_VALUE_PROBABILITY_TBLDAT* pHelp = (sITEM_OPTION_VALUE_PROBABILITY_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pHelp, sizeof(*pHelp)))
			return false;

		delete pHelp;

		return true;
	}

	return false;
}

bool CItemOptionValueProbabilityTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bUpdate);
	UNREFERENCED_PARAMETER(bReload);

	sITEM_OPTION_VALUE_PROBABILITY_TBLDAT* pTbldat = (sITEM_OPTION_VALUE_PROBABILITY_TBLDAT*) pvTable;

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

bool CItemOptionValueProbabilityTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sITEM_OPTION_VALUE_PROBABILITY_TBLDAT* pHelp = (sITEM_OPTION_VALUE_PROBABILITY_TBLDAT*)pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			pHelp->tblidx = READ_DWORD( bstrData );
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


sTBLDAT* CItemOptionValueProbabilityTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;

	return (sTBLDAT*)(iter->second); 
}

bool CItemOptionValueProbabilityTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sITEM_OPTION_VALUE_PROBABILITY_TBLDAT* pTableData = new sITEM_OPTION_VALUE_PROBABILITY_TBLDAT;
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
		if( false == AddTable(pTableData, bReload, bUpdate) )
		{
			delete pTableData;
		}

	} while (false != bLoop);

	return true;
}

bool CItemOptionValueProbabilityTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sITEM_OPTION_VALUE_PROBABILITY_TBLDAT* pTableData = (sITEM_OPTION_VALUE_PROBABILITY_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}