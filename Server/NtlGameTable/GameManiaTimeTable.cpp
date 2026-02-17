#include "stdafx.h"
#include "GameManiaTimeTable.h"
#include "NtlDebug.h"
#include "NtlSerializer.h"

//- yoshiki : Let's consider of implementing NtlAssert series.
//#include "NtlAssert.h"

// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };
static const WCHAR g_wszEffectDurationTime[] = { 'E', 'f', 'f', 'e', 'c', 't', '_', 'D', 'u', 'r', 'a', 't', 'i', 'o', 'n', '_', 'T', 'i', 'm', 'e', 0 };
static const WCHAR g_wszEffectType[] = { 'E', 'f', 'f', 'e', 'c', 't', '_', 'T', 'y', 'p', 'e', 0 };
static const WCHAR g_wszEffectValue[] = { 'E', 'f', 'f', 'e', 'c', 't', '_', 'V', 'a', 'l', 'u', 'e', 0 };
static const WCHAR g_wszEffectTimeNum[] = { 'E', 'f', 'f', 'e', 'c', 't', '_', 'T', 'i', 'm', 'e', '_', 'N', 'u', 'm', 0 };
static const WCHAR g_wszEffectTimeSet1[] = { 'E', 'f', 'f', 'e', 'c', 't', '_', 'T', 'i', 'm', 'e', '_', 'S', 'e', 't', '1', 0 };
static const WCHAR g_wszEffectTimeSet2[] = { 'E', 'f', 'f', 'e', 'c', 't', '_', 'T', 'i', 'm', 'e', '_', 'S', 'e', 't', '2', 0 };
static const WCHAR g_wszEffectTimeSet3[] = { 'E', 'f', 'f', 'e', 'c', 't', '_', 'T', 'i', 'm', 'e', '_', 'S', 'e', 't', '3', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CGameManiaTimeTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CGameManiaTimeTable::CGameManiaTimeTable(void)
{
	Init();
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CGameManiaTimeTable::~CGameManiaTimeTable(void)
{
	Destroy();
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CGameManiaTimeTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CGameManiaTimeTable::Destroy()
{
	CTable::Destroy();
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CGameManiaTimeTable::Init()
{
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void* CGameManiaTimeTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sGMT_TBLDAT* pGMT = new sGMT_TBLDAT;
		if (NULL == pGMT)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pGMT;
			return NULL;
		}

		m_dwCodePage = dwCodePage;
		return pGMT;
	}

	return NULL;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CGameManiaTimeTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sGMT_TBLDAT* pGMT = (sGMT_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pGMT, sizeof(*pGMT)))
			return false;

		delete pGMT;

		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CGameManiaTimeTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);

	sGMT_TBLDAT* pTbldat = (sGMT_TBLDAT*) pvTable;

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


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CGameManiaTimeTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sGMT_TBLDAT* pGMT = (sGMT_TBLDAT*)pvTable;

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			pGMT->tblidx = READ_TBLIDX( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Effect_Duration_Time"))
		{
			pGMT->byEffectTerm = READ_BYTE( bstrData, g_wszEffectDurationTime );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Effect_Type"))
		{
			pGMT->byEffectType = READ_BYTE( bstrData, g_wszEffectType );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Effect_Value"))
		{
			pGMT->byEffectValue = READ_BYTE( bstrData, g_wszEffectValue );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Effect_Time_Num"))
		{
			pGMT->byMaxTimeNumber = READ_BYTE( bstrData, g_wszEffectTimeNum );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Effect_Time_Set1"))
		{
			pGMT->abyTimeSet[0] = READ_BYTE( bstrData, g_wszEffectTimeSet1 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Effect_Time_Set2"))
		{
			pGMT->abyTimeSet[1] = READ_BYTE( bstrData, g_wszEffectTimeSet2 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Effect_Time_Set3"))
		{
			pGMT->abyTimeSet[2] = READ_BYTE( bstrData, g_wszEffectTimeSet3 );
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


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
sTBLDAT* CGameManiaTimeTable::FindData(TBLIDX tblidx)
{
	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;

	return (sTBLDAT*)(iter->second); 
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CGameManiaTimeTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sGMT_TBLDAT* pTableData = new sGMT_TBLDAT;
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


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CGameManiaTimeTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sGMT_TBLDAT* pTableData = (sGMT_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}
