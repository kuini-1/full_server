#include "stdafx.h"
#include "HlsSlotMachineItemTable.h"
#include "NtlDebug.h"
#include "NtlSerializer.h"

// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CHlsSlotMachineItemTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CHlsSlotMachineItemTable::CHlsSlotMachineItemTable(void)
{
	Init();
}

CHlsSlotMachineItemTable::~CHlsSlotMachineItemTable(void)
{
	Destroy();
}

bool CHlsSlotMachineItemTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CHlsSlotMachineItemTable::Destroy()
{
	CTable::Destroy();
}

void CHlsSlotMachineItemTable::Init()
{
}

void* CHlsSlotMachineItemTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sHLS_SLOT_MACHINE_ITEM_TBLDAT* pNewHelp = new sHLS_SLOT_MACHINE_ITEM_TBLDAT;
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

bool CHlsSlotMachineItemTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sHLS_SLOT_MACHINE_ITEM_TBLDAT* pHelp = (sHLS_SLOT_MACHINE_ITEM_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pHelp, sizeof(*pHelp)))
			return false;

		delete pHelp;

		return true;
	}

	return false;
}

bool CHlsSlotMachineItemTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);

	sHLS_SLOT_MACHINE_ITEM_TBLDAT* pTbldat = (sHLS_SLOT_MACHINE_ITEM_TBLDAT*)pvTable;

	if (false == m_mapTableList.insert(std::pair<TBLIDX, sTBLDAT*>(pTbldat->tblidx, pTbldat)).second)
	{
		WCHAR wszFormatBuf[512];
		FormatStringToWCHAR(L"[File] : %s\r\n Table Tblidx[%u] is Duplicated ", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
		CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, pTbldat->tblidx);
		_ASSERTE(0);
		return false;
	}

	return true;
}

bool CHlsSlotMachineItemTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sHLS_SLOT_MACHINE_ITEM_TBLDAT* pHelp = (sHLS_SLOT_MACHINE_ITEM_TBLDAT*)pvTable;

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


sTBLDAT* CHlsSlotMachineItemTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;

	return (sTBLDAT*)(iter->second);
}


bool CHlsSlotMachineItemTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sHLS_SLOT_MACHINE_ITEM_TBLDAT* pTableData = new sHLS_SLOT_MACHINE_ITEM_TBLDAT;
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

	//	printf("%u|%u|%u|%u|%u|%u \n", pTableData->tblidx, pTableData->bActive, pTableData->slotMachineTblidx, pTableData->cashItemTblidx, pTableData->byStackCount, pTableData->byPercent);
		if (false == AddTable(pTableData, bReload, bUpdate))
		{
			delete pTableData;
		}

	} while (false != bLoop);

	return true;
}

bool CHlsSlotMachineItemTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin(); End() != iter; iter++)
	{
		sHLS_SLOT_MACHINE_ITEM_TBLDAT* pTableData = (sHLS_SLOT_MACHINE_ITEM_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}

