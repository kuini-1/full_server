#include "stdafx.h"
#include "NtlDebug.h"

#include "QuestDropTable.h"

#include "NtlSerializer.h"

// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CQuestDropTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CQuestDropTable::CQuestDropTable(void)
{
	Init();
}

CQuestDropTable::~CQuestDropTable(void)
{
	Destroy();
}

bool CQuestDropTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CQuestDropTable::Destroy()
{
	CTable::Destroy();
}

void CQuestDropTable::Init()
{
}

void* CQuestDropTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sQUEST_DROP_TBLDAT* pQuestDrop = new sQUEST_DROP_TBLDAT;
		if (NULL == pQuestDrop)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pQuestDrop;
			return NULL;
		}
		
		m_dwCodePage = dwCodePage;
		return pQuestDrop;
	}
	
	return NULL;
}

bool CQuestDropTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sQUEST_DROP_TBLDAT* pQuestDrop = (sQUEST_DROP_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pQuestDrop, sizeof(*pQuestDrop)))
			return false;

		delete pQuestDrop;

		return true;
	}
	
	return false;
}

bool CQuestDropTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);

	sQUEST_DROP_TBLDAT* pTbldat = (sQUEST_DROP_TBLDAT*)pvTable;
		
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

bool CQuestDropTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sQUEST_DROP_TBLDAT* pQuestDrop = (sQUEST_DROP_TBLDAT*)pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		if (0 == WStringCmpLiteral(*pstrDataName, L"Quest_Drop_Tblidx"))
		{
			pQuestDrop->tblidx = READ_TBLIDX(bstrData);
		}

		// 1
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Quest_Item_Index1"))
		{
			pQuestDrop->aQuestItemTblidx[0] = READ_TBLIDX(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Drop_Rate_1"))
		{
			pQuestDrop->aDropRate[0] = READ_FLOAT(bstrData, wszFieldNameBuf);
		}

		// 2
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Quest_Item_Index2"))
		{
			pQuestDrop->aQuestItemTblidx[1] = READ_TBLIDX(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Drop_Rate_2"))
		{
			pQuestDrop->aDropRate[1] = READ_FLOAT(bstrData, wszFieldNameBuf);
		}

		// 3
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Quest_Item_Index3"))
		{
			pQuestDrop->aQuestItemTblidx[2] = READ_TBLIDX(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Drop_Rate_3"))
		{
			pQuestDrop->aDropRate[2] = READ_FLOAT(bstrData, wszFieldNameBuf);
		}

		// 4
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Quest_Item_Index4"))
		{
			pQuestDrop->aQuestItemTblidx[3] = READ_TBLIDX(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Drop_Rate_4"))
		{
			pQuestDrop->aDropRate[3] = READ_FLOAT(bstrData, wszFieldNameBuf);
		}

		// 5
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Quest_Item_Index5"))
		{
			pQuestDrop->aQuestItemTblidx[4] = READ_TBLIDX(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Drop_Rate_5"))
		{
			pQuestDrop->aDropRate[4] = READ_FLOAT(bstrData, wszFieldNameBuf);
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


sTBLDAT* CQuestDropTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;
	
	return (sTBLDAT*)(iter->second); 
}

bool CQuestDropTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sQUEST_DROP_TBLDAT* pTableData = new sQUEST_DROP_TBLDAT;
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

bool CQuestDropTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sQUEST_DROP_TBLDAT* pTableData = (sQUEST_DROP_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}