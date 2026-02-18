#include "stdafx.h"
#include "MobMovePatternTable.h"
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

const WCHAR* CMobMovePatternTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CMobMovePatternTable::CMobMovePatternTable(void)
{
	Init();
}

CMobMovePatternTable::~CMobMovePatternTable(void)
{
	Destroy();
}

bool CMobMovePatternTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CMobMovePatternTable::Destroy()
{
	CTable::Destroy();
}

void CMobMovePatternTable::Init()
{
}

void* CMobMovePatternTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sMOVE_PATTERN_TBLDAT* pPattern = new sMOVE_PATTERN_TBLDAT;
		if (NULL == pPattern)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pPattern;
			return NULL;
		}
		
		m_dwCodePage = dwCodePage;
		return pPattern;
	}

	return NULL;
}

bool CMobMovePatternTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sMOVE_PATTERN_TBLDAT* pPattern = (sMOVE_PATTERN_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pPattern, sizeof(*pPattern)))
			return false;

		delete pPattern;

		return true;
	}

	return false;
}

bool CMobMovePatternTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bUpdate);

	sMOVE_PATTERN_TBLDAT * pTbldat = (sMOVE_PATTERN_TBLDAT*) pvTable;
	sMOVE_PATTERN_TBLDAT * pExistTbldat = NULL;

	if( bReload )
	{
		pExistTbldat = (sMOVE_PATTERN_TBLDAT*) FindData( pTbldat->tblidx );
		if( pExistTbldat )
		{
			CopyMemory( pTbldat, pExistTbldat, pTbldat->GetDataSize() );
			return true; 
		}
	}

	if ( false == m_mapTableList.insert( std::map<TBLIDX, sTBLDAT*>::value_type(pTbldat->tblidx, pTbldat)).second )
	{
		WCHAR wszFormatBuf[512];
		FormatStringToWCHAR(L"[File] : %s\r\n Table Tblidx[%u] is Duplicated ", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
		CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, pTbldat->tblidx );
		_ASSERTE( 0 );
		return false;
	}

	return true;
}

bool CMobMovePatternTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sMOVE_PATTERN_TBLDAT* pPattern = (sMOVE_PATTERN_TBLDAT*)pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		static const WCHAR g_wszPatternPrefix[] = { 'P', 'a', 't', 't', 'e', 'r', 'n', '_', 0 };
		static const WCHAR g_wszPatternFormat[] = { 'P', 'a', 't', 't', 'e', 'r', 'n', '_', '%', 'd', 0 };

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			pPattern->tblidx = READ_DWORD( bstrData );
		}
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszPatternPrefix, WCHARLen(g_wszPatternPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < DBO_MAX_COUNT_MOVE_PATTERN; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszPatternFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pPattern->abyPattern[ i ] = READ_BYTE( bstrData, wszFieldNameBuf );

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
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Note"))
		{
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


sTBLDAT* CMobMovePatternTable::FindData(TBLIDX tblidx)
{
	if (INVALID_TBLIDX == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;

	return (sTBLDAT*)(iter->second); 
}

bool CMobMovePatternTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sMOVE_PATTERN_TBLDAT* pTableData = new sMOVE_PATTERN_TBLDAT;
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

bool CMobMovePatternTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sMOVE_PATTERN_TBLDAT* pTableData = (sMOVE_PATTERN_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}