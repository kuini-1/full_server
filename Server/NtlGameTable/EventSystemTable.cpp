#include "stdafx.h"
#include "EventSystemTable.h"
#include "NtlDebug.h"
#include "NtlSerializer.h"

// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CEventSystemTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CEventSystemTable::CEventSystemTable(void)
{
	Init();
}

CEventSystemTable::~CEventSystemTable(void)
{
	Destroy();
}

bool CEventSystemTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CEventSystemTable::Destroy()
{
	CTable::Destroy();
}

void CEventSystemTable::Init()
{
	memset(m_abOnOff, false, sizeof(m_abOnOff));
	m_mapEventSystem.clear();
}

void* CEventSystemTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sEVENT_SYSTEM_TBLDAT* pNewHelp = new sEVENT_SYSTEM_TBLDAT;
		if (NULL == pNewHelp)
			return NULL;

		CPINFO cpInfo;
		if (false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pNewHelp;
			return NULL;
		}

		m_dwCodePage = dwCodePage;
		return pNewHelp;
	}

	return NULL;
}

bool CEventSystemTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sEVENT_SYSTEM_TBLDAT* pHelp = (sEVENT_SYSTEM_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pHelp, sizeof(*pHelp)))
			return false;

		delete pHelp;

		return true;
	}

	return false;
}

bool CEventSystemTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);

	sEVENT_SYSTEM_TBLDAT* pTbldat = (sEVENT_SYSTEM_TBLDAT*)pvTable;

	if (false == m_mapTableList.insert(std::pair<TBLIDX, sTBLDAT*>(pTbldat->tblidx, pTbldat)).second)
	{
		WCHAR wszFormatBuf[512];
		FormatStringToWCHAR(L"[File] : %s\r\n Table Tblidx[%u] is Duplicated ", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
		CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, pTbldat->tblidx);
		_ASSERTE(0);
		return false;
	}

	if (pTbldat->bOnOff == false)
		return true;

	m_abOnOff[pTbldat->byType] = true;
	m_mapEventSystem.insert(EVENT_SYSTEM_VAL(pTbldat->byType, pTbldat));

	return true;
}

bool CEventSystemTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sEVENT_SYSTEM_TBLDAT* pHelp = (sEVENT_SYSTEM_TBLDAT*)pvTable;

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			pHelp->tblidx = READ_DWORD(bstrData);
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


sTBLDAT* CEventSystemTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;

	return (sTBLDAT*)(iter->second);
}

bool CEventSystemTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sEVENT_SYSTEM_TBLDAT* pTableData = new sEVENT_SYSTEM_TBLDAT;
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

	//	wprintf(L"CEventSystemTable: Name:%s ", pTableData->wszName);
	//	printf("pTableData->tblidx %u, bOnOff %i, byType %u, tIndex %u, action %u, aIndex %u, byGroup %u\n", pTableData->tblidx, pTableData->bOnOff, pTableData->byType, pTableData->tIndex, pTableData->byAction, pTableData->aIndex, pTableData->byGroup);
		
		if (false == AddTable(pTableData, bReload, bUpdate))
		{
			delete pTableData;
		}

	} while (false != bLoop);

	return true;
}

bool CEventSystemTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin(); End() != iter; iter++)
	{
		sEVENT_SYSTEM_TBLDAT* pTableData = (sEVENT_SYSTEM_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}


bool CEventSystemTable::IsOn(eEVENT_SYSTEM_TYPE eEventSystemType)
{
	return m_abOnOff[eEventSystemType];
}


EVENT_SYSTEM_MAP::iterator CEventSystemTable::EventBegin()
{
	return m_mapEventSystem.begin();
}


EVENT_SYSTEM_MAP::iterator CEventSystemTable::EventEnd()
{
	return m_mapEventSystem.end();
}


bool CEventSystemTable::IsThisEventMob(TBLIDX tblidx)
{
	TABLE::iterator iter;
	for (iter = Begin(); End() != iter; iter++)
	{
		sEVENT_SYSTEM_TBLDAT* pTableData = (sEVENT_SYSTEM_TBLDAT*)(iter->second);

		if (pTableData->bOnOff == true && pTableData->tIndex == tblidx)
			return true;
	}

	return false;
}
