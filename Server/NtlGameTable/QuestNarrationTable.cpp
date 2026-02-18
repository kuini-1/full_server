//***********************************************************************************
//
//	File		:	QuestNarrationTable.cpp
//
//	Begin		:	2008-08-21
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Chung, DooSup (mailto:john@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************


#include "stdafx.h"
#include "QuestNarrationTable.h"
#include "NtlDebug.h"
#include "NtlSerializer.h"

static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) { WCharTLiteralToWCHAR(fmt, dest, destSize); }

const WCHAR* CQuestNarrationTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CQuestNarrationTable::CQuestNarrationTable( void )
{
	Init();
}

CQuestNarrationTable::~CQuestNarrationTable( void )
{
	Destroy();
}

bool CQuestNarrationTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CQuestNarrationTable::Destroy( void )
{
	CTable::Destroy();
}

void CQuestNarrationTable::Init( void )
{
}

void* CQuestNarrationTable::AllocNewTable( WCHAR* pwszSheetName, DWORD dwCodePage )
{
	if ( 0 == WCHARCmp( pwszSheetName, g_wszTableDataKOR ) )
	{
		sQUEST_NARRATION_TBLDAT* pNewObj = new sQUEST_NARRATION_TBLDAT;
		if ( NULL == pNewObj ) return NULL;

		CPINFO cpInfo;
		if ( !GetCPInfo( dwCodePage, &cpInfo ) )
		{
			delete pNewObj;
			return NULL;
		}

		m_dwCodePage = dwCodePage;
		return pNewObj;
	}

	return NULL;
}

bool CQuestNarrationTable::DeallocNewTable( void* pvTable, WCHAR* pwszSheetName )
{
	if ( 0 == WCHARCmp( pwszSheetName, g_wszTableDataKOR ) )
	{
		sQUEST_NARRATION_TBLDAT* pObj = (sQUEST_NARRATION_TBLDAT*)pvTable;
		if ( IsBadReadPtr( pObj, sizeof(*pObj) ) ) return false;

		delete pObj;

		return true;
	}

	return false;
}

bool CQuestNarrationTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);

	sQUEST_NARRATION_TBLDAT* pTbldat = (sQUEST_NARRATION_TBLDAT*)pvTable;
	if ( false == m_mapTableList.insert( std::pair<TBLIDX, sTBLDAT*>(pTbldat->tblidx, pTbldat) ).second )
	{
		WCHAR wszFormatBuf[512];
		FormatStringToWCHAR(L"[File] : %s\r\n Table Tblidx[%u] is Duplicated ", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
		CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, pTbldat->tblidx );
		_ASSERTE( 0 );
		return false;
	}


	return true;
}

bool CQuestNarrationTable::SetTableData( void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData )
{
	if ( 0 == WCHARCmp( pwszSheetName, g_wszTableDataKOR ) )
	{
		sQUEST_NARRATION_TBLDAT* pTbldat = (sQUEST_NARRATION_TBLDAT*)pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		static const WCHAR g_wszUIShowHidePrefix[] = { 'U', 'I', 'S', 'h', 'o', 'w', 'H', 'i', 'd', 'e', 'D', 'i', 'r', 'e', 'c', 't', 'i', 'o', 'n', '_', 0 };
		static const WCHAR g_wszUIShowHideFormat[] = { 'U', 'I', 'S', 'h', 'o', 'w', 'H', 'i', 'd', 'e', 'D', 'i', 'r', 'e', 'c', 't', 'i', 'o', 'n', '_', '%', 'd', 0 };
		static const WCHAR g_wszOwnerTypePrefix[] = { 'O', 'w', 'n', 'e', 'r', 'T', 'y', 'p', 'e', '_', 0 };
		static const WCHAR g_wszOwnerTypeFormat[] = { 'O', 'w', 'n', 'e', 'r', 'T', 'y', 'p', 'e', '_', '%', 'd', 0 };
		static const WCHAR g_wszOwnerPrefix[] = { 'O', 'w', 'n', 'e', 'r', '_', 0 };
		static const WCHAR g_wszOwnerFormat[] = { 'O', 'w', 'n', 'e', 'r', '_', '%', 'd', 0 };
		static const WCHAR g_wszConditionPrefix[] = { 'C', 'o', 'n', 'd', 'i', 't', 'i', 'o', 'n', '_', 0 };
		static const WCHAR g_wszConditionFormat[] = { 'C', 'o', 'n', 'd', 'i', 't', 'i', 'o', 'n', '_', '%', 'd', 0 };
		static const WCHAR g_wszDirectionPrefix[] = { 'D', 'i', 'r', 'e', 'c', 't', 'i', 'o', 'n', '_', 0 };
		static const WCHAR g_wszDirectionFormat[] = { 'D', 'i', 'r', 'e', 'c', 't', 'i', 'o', 'n', '_', '%', 'd', 0 };
		static const WCHAR g_wszDialogPrefix[] = { 'D', 'i', 'a', 'l', 'o', 'g', '_', 0 };
		static const WCHAR g_wszDialogFormat[] = { 'D', 'i', 'a', 'l', 'o', 'g', '_', '%', 'd', 0 };
		static const WCHAR g_wszUiTypePrefix[] = { 'U', 'i', 'T', 'y', 'p', 'e', '_', 0 };
		static const WCHAR g_wszUiTypeFormat[] = { 'U', 'i', 'T', 'y', 'p', 'e', '_', '%', 'd', 0 };
		static const WCHAR g_wszUIDirectionPrefix[] = { 'U', 'I', 'D', 'i', 'r', 'e', 'c', 't', 'i', 'o', 'n', '_', 0 };
		static const WCHAR g_wszUIDirectionFormat[] = { 'U', 'I', 'D', 'i', 'r', 'e', 'c', 't', 'i', 'o', 'n', '_', '%', 'd', 0 };

		if ( 0 == WStringCmpLiteral( *pstrDataName, L"Tblidx" ) )
		{
			pTbldat->tblidx = READ_TBLIDX( bstrData );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Type" ) )
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pTbldat->bType = READ_BOOL( bstrData, wszFieldNameBuf );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Number" ) )
		{
			pTbldat->byNumber = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if ( 0 == WStringCmpLiteral( *pstrDataName, L"Time" ) )
		{
			pTbldat->byTime = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszUIShowHidePrefix, WCHARLen(g_wszUIShowHidePrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < DBO_MAX_COUNT_OF_NARRATION; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszUIShowHideFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pTbldat->asData[i].byUIShowHideDirection = READ_BYTE( bstrData, wszFieldNameBuf );

					bFound = true;
					break;
				}
			}

			if( false == bFound )
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
		}
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszOwnerTypePrefix, WCHARLen(g_wszOwnerTypePrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < DBO_MAX_COUNT_OF_NARRATION; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszOwnerTypeFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pTbldat->asData[i].byOwnerType = READ_BYTE( bstrData, wszFieldNameBuf );

					bFound = true;
					break;
				}
			}

			if( false == bFound )
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
		}
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszOwnerPrefix, WCHARLen(g_wszOwnerPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < DBO_MAX_COUNT_OF_NARRATION; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszOwnerFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pTbldat->asData[i].dwOwner = READ_DWORD( bstrData );

					bFound = true;
					break;
				}
			}

			if( false == bFound )
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
		}
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszConditionPrefix, WCHARLen(g_wszConditionPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < DBO_MAX_COUNT_OF_NARRATION; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszConditionFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pTbldat->asData[i].byCondition = READ_BYTE( bstrData, wszFieldNameBuf );

					bFound = true;
					break;
				}
			}

			if( false == bFound )
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
		}
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszDirectionPrefix, WCHARLen(g_wszDirectionPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < DBO_MAX_COUNT_OF_NARRATION; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszDirectionFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pTbldat->asData[i].byDirection = READ_BYTE( bstrData, wszFieldNameBuf );

					bFound = true;
					break;
				}
			}

			if( false == bFound )
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
		}
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszDialogPrefix, WCHARLen(g_wszDialogPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < DBO_MAX_COUNT_OF_NARRATION; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszDialogFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pTbldat->asData[i].dwDialog = READ_DWORD( bstrData );

					bFound = true;
					break;
				}
			}

			if( false == bFound )
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
		}
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszUiTypePrefix, WCHARLen(g_wszUiTypePrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < DBO_MAX_COUNT_OF_NARRATION; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszUiTypeFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pTbldat->asData[i].byUiType = READ_BYTE( bstrData, wszFieldNameBuf );

					bFound = true;
					break;
				}
			}

			if( false == bFound )
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
		}
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszUIDirectionPrefix, WCHARLen(g_wszUIDirectionPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < DBO_MAX_COUNT_OF_NARRATION; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszUIDirectionFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pTbldat->asData[i].byUIDirection = READ_BYTE( bstrData, wszFieldNameBuf );

					bFound = true;
					break;
				}
			}

			if( false == bFound )
			{
				WCHAR wszFormatBuf[512];
				FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
				return false;
			}
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


sTBLDAT* CQuestNarrationTable::FindData( TBLIDX tblidx )
{
	if ( 0 == tblidx ) return NULL;

	TABLEIT iter = m_mapTableList.find( tblidx );
	if ( m_mapTableList.end() == iter ) return NULL;

	return (sTBLDAT*)(iter->second); 
}

bool CQuestNarrationTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sQUEST_NARRATION_TBLDAT* pTableData = new sQUEST_NARRATION_TBLDAT;
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

bool CQuestNarrationTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sQUEST_NARRATION_TBLDAT* pTableData = (sQUEST_NARRATION_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}