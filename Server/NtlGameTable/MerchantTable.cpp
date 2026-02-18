#include "stdafx.h"
#include "MerchantTable.h"
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

const WCHAR* CMerchantTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CMerchantTable::CMerchantTable(void)
{
	Init();
}

CMerchantTable::~CMerchantTable(void)
{
	Destroy();
}

bool CMerchantTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CMerchantTable::Destroy()
{
	CTable::Destroy();
}

void CMerchantTable::Init()
{
}

void* CMerchantTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sMERCHANT_TBLDAT* pMerchant = new sMERCHANT_TBLDAT;
		if (NULL == pMerchant)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pMerchant;
			return NULL;
		}

		m_dwCodePage = dwCodePage;
		return pMerchant;
	}

	return NULL;
}

bool CMerchantTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sMERCHANT_TBLDAT* pMerchant = (sMERCHANT_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pMerchant, sizeof(*pMerchant)))
			return false;

		delete pMerchant;

		return true;
	}

	return false;
}

bool CMerchantTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	sMERCHANT_TBLDAT * pTbldat = (sMERCHANT_TBLDAT*)pvTable;
	sMERCHANT_TBLDAT * pExistTbldat = NULL;

	if( bReload )
	{
		pExistTbldat = (sMERCHANT_TBLDAT*) FindData( pTbldat->tblidx );
		if( pExistTbldat )
		{
			CopyMemory( pTbldat, pExistTbldat, pTbldat->GetDataSize() );
			// ??????? ?????? ???? false ???
			return true; 
		}
	}

	if( bUpdate) //for tables inside localize folder
	{
		pExistTbldat = (sMERCHANT_TBLDAT*) FindData( pTbldat->tblidx );
		if( pExistTbldat )
		{ //if exist then remove
			m_mapTableList.erase( pTbldat->tblidx );
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

bool CMerchantTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sMERCHANT_TBLDAT* pMerchant = (sMERCHANT_TBLDAT*)pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		static const WCHAR g_wszItemTblidxPrefix[] = { 'I', 't', 'e', 'm', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 0 };
		static const WCHAR g_wszItemTblidxFormat[] = { 'I', 't', 'e', 'm', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', '%', 'd', 0 };
		static const WCHAR g_wszNeedItemTindexPrefix[] = { 'N', 'e', 'e', 'd', '_', 'I', 't', 'e', 'm', '_', 'T', 'i', 'n', 'd', 'e', 'x', '_', 0 };
		static const WCHAR g_wszNeedItemTindexFormat[] = { 'N', 'e', 'e', 'd', '_', 'I', 't', 'e', 'm', '_', 'T', 'i', 'n', 'd', 'e', 'x', '_', '%', 'd', 0 };
		static const WCHAR g_wszNeedItemStackPrefix[] = { 'N', 'e', 'e', 'd', '_', 'I', 't', 'e', 'm', '_', 'S', 't', 'a', 'c', 'k', '_', 0 };
		static const WCHAR g_wszNeedItemStackFormat[] = { 'N', 'e', 'e', 'd', '_', 'I', 't', 'e', 'm', '_', 'S', 't', 'a', 'c', 'k', '_', '%', 'd', 0 };
		static const WCHAR g_wszNeedZennyPrefix[] = { 'N', 'e', 'e', 'd', '_', 'Z', 'e', 'n', 'n', 'y', '_', 0 };
		static const WCHAR g_wszNeedZennyFormat[] = { 'N', 'e', 'e', 'd', '_', 'Z', 'e', 'n', 'n', 'y', '_', '%', 'd', 0 };

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			pMerchant->tblidx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Name_Text"))
		{
			READ_STRINGW(bstrData, pMerchant->wszNameText, _countof(pMerchant->wszNameText));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Sell_Type"))
		{
			pMerchant->bySell_Type = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Tab_Name"))
		{
			pMerchant->Tab_Name = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Need_Mileage"))
		{
			pMerchant->dwNeedMileage = READ_DWORD( bstrData );
		}		
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszItemTblidxPrefix, WCHARLen(g_wszItemTblidxPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_MERCHANT_COUNT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszItemTblidxFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pMerchant->aitem_Tblidx[ i ] = READ_DWORD( bstrData );

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

		//new
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszNeedItemTindexPrefix, WCHARLen(g_wszNeedItemTindexPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_MERCHANT_COUNT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszNeedItemTindexFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pMerchant->aNeedItemTblidx[ i ] = READ_DWORD( bstrData );

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

		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszNeedItemStackPrefix, WCHARLen(g_wszNeedItemStackPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_MERCHANT_COUNT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszNeedItemStackFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pMerchant->abyNeedItemStack[ i ] = READ_BYTE( bstrData, wszFieldNameBuf );

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

		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszNeedZennyPrefix, WCHARLen(g_wszNeedZennyPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_MERCHANT_COUNT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszNeedZennyFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pMerchant->adwNeedZenny[ i ] = READ_DWORD( bstrData );

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


sTBLDAT* CMerchantTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;

	return (sTBLDAT*)(iter->second); 
}

TBLIDX CMerchantTable::FindMerchantItem(sMERCHANT_TBLDAT* psTbldat, BYTE byIndex)
{
	if ( NTL_MAX_MERCHANT_COUNT <= byIndex || 0 > byIndex )
		return INVALID_TBLIDX;

	return psTbldat->aitem_Tblidx[byIndex];	
}

bool CMerchantTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sMERCHANT_TBLDAT* pTableData = new sMERCHANT_TBLDAT;
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

bool CMerchantTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sMERCHANT_TBLDAT* pTableData = (sMERCHANT_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}