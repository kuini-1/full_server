#include "stdafx.h"
#include "NtlDebug.h"

#include "QuestProbabilityTable.h"
#include "NtlSerializer.h"

// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CQuestProbabilityTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CQuestProbabilityTable::CQuestProbabilityTable(void)
{
	Init();
}

CQuestProbabilityTable::~CQuestProbabilityTable(void)
{
	Destroy();
}

bool CQuestProbabilityTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CQuestProbabilityTable::Destroy()
{
	CTable::Destroy();
}

void CQuestProbabilityTable::Init()
{
}

void* CQuestProbabilityTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sQUEST_PROBABILITY_TBLDAT * pQuestProbability = new sQUEST_PROBABILITY_TBLDAT;
		if (NULL == pQuestProbability)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pQuestProbability;
			return NULL;
		}

		m_dwCodePage = dwCodePage;
		return pQuestProbability;
	}

	return NULL;
}

bool CQuestProbabilityTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sQUEST_PROBABILITY_TBLDAT * pQuestProbability = (sQUEST_PROBABILITY_TBLDAT *)pvTable;
		if (FALSE != IsBadReadPtr(pQuestProbability, sizeof(*pQuestProbability)))
			return false;

		delete pQuestProbability;

		return true;
	}

	return false;
}

bool CQuestProbabilityTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER( bReload );

	sQUEST_PROBABILITY_TBLDAT * pTbldat = (sQUEST_PROBABILITY_TBLDAT *)pvTable;
	sQUEST_PROBABILITY_TBLDAT * pExistTbldat = NULL;

	for( BYTE bySlot = 0; bySlot < NTL_QUEST_PROBABILITY_MAX_COUNT; bySlot++ )
	{
		if( 0xff == pTbldat->asProbabilityData[bySlot].byType )
		{
			break;
		}

		if( INVALID_TBLIDX == pTbldat->asProbabilityData[bySlot].tblidx
			&& INVALID_TBLIDX == pTbldat->asProbabilityData[bySlot].dwMaxValue
			&& INVALID_TBLIDX == pTbldat->asProbabilityData[bySlot].dwMinValue )
		{
			WCHAR wszFormatBuf[512];
			FormatStringToWCHAR(L"[File] : %s\r\n Table Tblidx[%u] entity index[%u] : invalid value", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
			CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, pTbldat->tblidx, bySlot );
			_ASSERTE( 0 );

			return false;
		}
	}

	if( bUpdate) //for tables inside localize folder
	{
		pExistTbldat = (sQUEST_PROBABILITY_TBLDAT*) FindData( pTbldat->tblidx );
		if( pExistTbldat )
		{ //if exist then remove
			m_mapTableList.erase( pTbldat->tblidx );
		}
	}


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

bool CQuestProbabilityTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sQUEST_PROBABILITY_TBLDAT * pProbabilityTbldat = (sQUEST_PROBABILITY_TBLDAT*)pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		static const WCHAR g_wszRewardTypeFormat[] = { 'R', 'e', 'w', 'a', 'r', 'd', '_', 'T', 'y', 'p', 'e', '%', 'd', 0 };
		static const WCHAR g_wszRewardTblidxFormat[] = { 'R', 'e', 'w', 'a', 'r', 'd', '_', 'T', 'b', 'l', 'i', 'd', 'x', '%', 'd', 0 };
		static const WCHAR g_wszMinValueFormat[] = { 'M', 'i', 'n', '_', 'V', 'a', 'l', 'u', 'e', '%', 'd', 0 };
		static const WCHAR g_wszMaxValueFormat[] = { 'M', 'a', 'x', '_', 'V', 'a', 'l', 'u', 'e', '%', 'd', 0 };
		static const WCHAR g_wszDropRateFormat[] = { 'D', 'r', 'o', 'p', '_', 'R', 'a', 't', 'e', '%', 'd', 0 };

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			pProbabilityTbldat->tblidx = READ_TBLIDX(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Name"))
		{
			if( false == READ_STRINGW(bstrData, pProbabilityTbldat->wszName, _countof(pProbabilityTbldat->wszName)) )
			{
				return false;
			}
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Note"))
		{
			if( false == READ_STRINGW(bstrData, pProbabilityTbldat->wszNote, _countof(pProbabilityTbldat->wszNote)) )
			{
				return false;
			}
		}


		else if (0 == WStringCmpLiteral(*pstrDataName, L"Allow_Blank"))
		{
			pProbabilityTbldat->bAllowBlank = READ_BOOL(bstrData, wszFieldNameBuf);
		}


		else if (0 == WStringCmpLiteral(*pstrDataName, L"Probability_Type"))
		{
			pProbabilityTbldat->byProbabilityType = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else
		{
			WCHAR szBuffer[1024] = { 0x00, };

			// Reward Type
			for( int i = 0; i < NTL_QUEST_PROBABILITY_MAX_COUNT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszRewardTypeFormat, i + 1);

				if( 0 == WCHARCmp( wszFieldNameBuf, szBuffer ) )
				{
					pProbabilityTbldat->asProbabilityData[i].byType = READ_BYTE( bstrData, wszFieldNameBuf );
					return true;
				}
			}

			// Reward Index
			for( int i = 0; i < NTL_QUEST_PROBABILITY_MAX_COUNT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszRewardTblidxFormat, i + 1);

				if( 0 == WCHARCmp( wszFieldNameBuf, szBuffer ) )
				{
					pProbabilityTbldat->asProbabilityData[i].tblidx = READ_TBLIDX( bstrData );
					return true;
				}
			}

			// Reward Index
			for( int i = 0; i < NTL_QUEST_PROBABILITY_MAX_COUNT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszMinValueFormat, i + 1);

				if( 0 == WCHARCmp( wszFieldNameBuf, szBuffer ) )
				{
					pProbabilityTbldat->asProbabilityData[i].dwMinValue = READ_DWORD( bstrData );
					return true;
				}
			}

			// Reward Index
			for( int i = 0; i < NTL_QUEST_PROBABILITY_MAX_COUNT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszMaxValueFormat, i + 1);

				if( 0 == WCHARCmp( wszFieldNameBuf, szBuffer ) )
				{
					pProbabilityTbldat->asProbabilityData[i].dwMaxValue = READ_DWORD( bstrData );
					return true;
				}
			}

			// Rate
			for( int i = 0; i < NTL_QUEST_PROBABILITY_MAX_COUNT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszDropRateFormat, i + 1);

				if( 0 == WCHARCmp( wszFieldNameBuf, szBuffer ) )
				{
					pProbabilityTbldat->asProbabilityData[i].dwRate = READ_DWORD( bstrData );
					return true;
				}
			}

			WCHAR wszFormatBuf[512];
			FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
			CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
			return false;

		} // end if
	} // end if
	else
	{
		return false;
	}

	return true;
}


sTBLDAT* CQuestProbabilityTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;

	return (sTBLDAT*)(iter->second); 
}

bool CQuestProbabilityTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
{
	if( false == bReload && bUpdate == false )
	{
		Reset();
	}

	BYTE byMargin = 1;
	serializer >> byMargin;

	bool bLoop = true;

	do
	{

		sQUEST_PROBABILITY_TBLDAT* pTableData = new sQUEST_PROBABILITY_TBLDAT;
		if (NULL == pTableData)
		{
			// Log : [11/29/2007 niam]

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

bool CQuestProbabilityTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sQUEST_PROBABILITY_TBLDAT* pTableData = (sQUEST_PROBABILITY_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}
