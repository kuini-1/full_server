#include "stdafx.h"
#include "SetItemTable.h"
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

const WCHAR* CSetItemTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CSetItemTable::CSetItemTable(void)
{
	Init();
}

CSetItemTable::~CSetItemTable(void)
{
	Destroy();
}

bool CSetItemTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CSetItemTable::Destroy()
{
	CTable::Destroy();
}

void CSetItemTable::Init()
{
}

void* CSetItemTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sSET_ITEM_TBLDAT* pSetItem = new sSET_ITEM_TBLDAT;
		if (NULL == pSetItem)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pSetItem;
			return NULL;
		}
		
		m_dwCodePage = dwCodePage;
		return pSetItem;
	}

	return NULL;
}

bool CSetItemTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sSET_ITEM_TBLDAT* pSetItem = (sSET_ITEM_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pSetItem, sizeof(*pSetItem)))
			return false;

		delete pSetItem;

		return true;
	}

	return false;
}

bool CSetItemTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bUpdate);

	sSET_ITEM_TBLDAT * pTbldat = (sSET_ITEM_TBLDAT*) pvTable;
	sSET_ITEM_TBLDAT * pExistTbldat = NULL;

	if (false == pTbldat->bValidity_Able)
	{
		return false;
	}

	if( bReload )
	{
		pExistTbldat = (sSET_ITEM_TBLDAT*) FindData( pTbldat->tblidx );
		if( pExistTbldat )
		{
			CopyMemory( pTbldat, pExistTbldat, pTbldat->GetDataSize() );
			// ??????? ?????? ???? false ???
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

bool CSetItemTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sSET_ITEM_TBLDAT* pSetItem = (sSET_ITEM_TBLDAT*)pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		static const WCHAR g_wszItemTblidxPrefix[] = { 'I', 't', 'e', 'm', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 0 };
		static const WCHAR g_wszItemTblidxFormat[] = { 'I', 't', 'e', 'm', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', '%', 'd', 0 };

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			pSetItem->tblidx = READ_DWORD( bstrData );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Validity_Able") )
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pSetItem->bValidity_Able = READ_BOOL( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Semi_Set_Option"))
		{
			pSetItem->semiSetOption = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Full_Set_Option"))
		{
			pSetItem->fullSetOption = READ_DWORD( bstrData );
		}
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszItemTblidxPrefix, WCHARLen(g_wszItemTblidxPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_SET_ITEM_COUNT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszItemTblidxFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pSetItem->aItemTblidx[ i ] = READ_DWORD( bstrData );

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


sTBLDAT* CSetItemTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;

	return (sTBLDAT*)(iter->second); 
}

bool CSetItemTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sSET_ITEM_TBLDAT* pTableData = new sSET_ITEM_TBLDAT;
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

bool CSetItemTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sSET_ITEM_TBLDAT* pTableData = (sSET_ITEM_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}