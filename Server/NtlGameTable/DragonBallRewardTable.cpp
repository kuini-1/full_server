#include "stdafx.h"
#include "DragonBallRewardTable.h"
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

const WCHAR* CDragonBallRewardTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CDragonBallRewardTable::CDragonBallRewardTable(void)
{
	Init();
}

CDragonBallRewardTable::~CDragonBallRewardTable(void)
{
	Destroy();
}

bool CDragonBallRewardTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CDragonBallRewardTable::Destroy()
{
	CTable::Destroy();
}

void CDragonBallRewardTable::Init()
{
}

void* CDragonBallRewardTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sDRAGONBALL_REWARD_TBLDAT* pNewDragonBall = new sDRAGONBALL_REWARD_TBLDAT;
		if (NULL == pNewDragonBall)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pNewDragonBall;
			return NULL;
		}
		
		m_dwCodePage = dwCodePage;
		return pNewDragonBall;
	}

	return NULL;
}

bool CDragonBallRewardTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sDRAGONBALL_REWARD_TBLDAT* pDragonBall = (sDRAGONBALL_REWARD_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pDragonBall, sizeof(*pDragonBall)))
			return false;

		delete pDragonBall;

		return true;
	}

	return false;
}

bool CDragonBallRewardTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);;

	sDRAGONBALL_REWARD_TBLDAT* pTbldat = (sDRAGONBALL_REWARD_TBLDAT*) pvTable;

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

bool CDragonBallRewardTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sDRAGONBALL_REWARD_TBLDAT* pDragonBall = (sDRAGONBALL_REWARD_TBLDAT*)pvTable;

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pDragonBall->tblidx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Ball_Type"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pDragonBall->byBallType = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Reward_Category_Depth"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pDragonBall->byRewardCategoryDepth = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		//new 30.11.2014
		else if (0 == WStringCmpLiteral(*pstrDataName, L"class_bit"))
		{
			pDragonBall->dwClassBit = READ_DWORD(bstrData);
		}

		else if (0 == WStringCmpLiteral(*pstrDataName, L"Reward_Category_Name"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pDragonBall->rewardCategoryName = READ_DWORD( bstrData );
		}	
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Reward_Category_Dialog"))
		{
			pDragonBall->rewardCategoryDialog = READ_DWORD( bstrData );
		}	
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Reward_Type"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pDragonBall->byRewardType = READ_BYTE( bstrData, wszFieldNameBuf );
		}	
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Reward_Name"))
		{
			pDragonBall->rewardName = READ_DWORD( bstrData );
		}	
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Reward_Link_Tblidx"))
		{
			pDragonBall->rewardLinkTblidx = READ_DWORD( bstrData );
		}	
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Reward_Zenny"))
		{
			pDragonBall->dwRewardZenny = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Reward_Dialog_1"))
		{
			pDragonBall->rewardDialog1 = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Reward_Dialog_2"))
		{
			pDragonBall->rewardDialog2 = READ_DWORD( bstrData );
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


sTBLDAT* CDragonBallRewardTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;

	return (sTBLDAT*)(iter->second); 
}

bool CDragonBallRewardTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sDRAGONBALL_REWARD_TBLDAT* pTableData = new sDRAGONBALL_REWARD_TBLDAT;
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

bool CDragonBallRewardTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sDRAGONBALL_REWARD_TBLDAT* pTableData = (sDRAGONBALL_REWARD_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}