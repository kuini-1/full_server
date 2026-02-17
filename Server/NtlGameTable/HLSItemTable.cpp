//***********************************************************************************
//
//	File		:	HLSItemTable.h
//
//	Begin		:	2009-8-14
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Chung Doo sup   ( john@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************

#include "stdafx.h"
#include "HLSItemTable.h"
#include "NtlDebug.h"
#include "NtlBattle.h"
#include "NtlSerializer.h"

//- yoshiki : Let's consider of implementing NtlAssert series.
//#include "NtlAssert.h"

// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };
static const WCHAR g_wszSubItemTblidx[] = { 'S', 'u', 'b', '_', 'I', 't', 'e', 'm', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 0 };
static const WCHAR g_wszSubItemCount[] = { 'S', 'u', 'b', '_', 'I', 't', 'e', 'm', '_', 'C', 'o', 'u', 'n', 't', '_', 0 };
static const WCHAR g_wszSubItemTblidxFormat[] = { 'S', 'u', 'b', '_', 'I', 't', 'e', 'm', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', '%', 'd', 0 };
static const WCHAR g_wszSubItemCountFormat[] = { 'S', 'u', 'b', '_', 'I', 't', 'e', 'm', '_', 'C', 'o', 'u', 'n', 't', '_', '%', 'd', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CHLSItemTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CHLSItemTable::CHLSItemTable(void)
{
	Init();
}

CHLSItemTable::~CHLSItemTable(void)
{
	Destroy();
}

bool CHLSItemTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CHLSItemTable::Destroy()
{
	CTable::Destroy();
}

void CHLSItemTable::Init()
{
}

void* CHLSItemTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sHLS_ITEM_TBLDAT* pNewItem = new sHLS_ITEM_TBLDAT;
		if (NULL == pNewItem)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pNewItem;
			return NULL;
		}
		
		m_dwCodePage = dwCodePage;
		return pNewItem;
	}

	return NULL;
}

bool CHLSItemTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sHLS_ITEM_TBLDAT* pItem = (sHLS_ITEM_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pItem, sizeof(*pItem)))
			return false;

		delete pItem;

		return true;
	}

	return false;
}

bool CHLSItemTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bUpdate);

	sHLS_ITEM_TBLDAT * pTbldat = (sHLS_ITEM_TBLDAT*) pvTable;
	sHLS_ITEM_TBLDAT * pExistTbldat = NULL;
		
	if( bReload )
	{
		pExistTbldat = (sHLS_ITEM_TBLDAT*) FindData( pTbldat->tblidx );
		if( pExistTbldat )
		{
			CopyMemory( pTbldat, pExistTbldat, pTbldat->GetDataSize() );
			// ??????? ????? ?????? ???? true ???
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

bool CHLSItemTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{

	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sHLS_ITEM_TBLDAT* pItem = (sHLS_ITEM_TBLDAT*)pvTable;

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->tblidx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Name_Text"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			READ_STRINGW(bstrData, pItem->wszName, _countof(pItem->wszName));
		}

	//	else if (0 == WStringCmpLiteral(*pstrDataName, L"Icon_Name"))
	//	{
	//		READ_STRING(bstrData, pItem->szIcon_Name, _countof(pItem->szIcon_Name));
	//	}


		else if (0 == WStringCmpLiteral(*pstrDataName, L"CJI_Product_ID"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			READ_STRINGW(bstrData, pItem->wszCJIProductID, _countof(pItem->wszCJIProductID));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"HLS_Item_Type"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pItem->wHLSItemType = READ_WORD(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Item_Tblidx"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->itemTblidx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"On_Sale"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pItem->bOnSale = READ_BOOL(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Sell_Type"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pItem->bySellType = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Cash"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->dwCash = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Discount"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pItem->byDiscount = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Item_Stack_Count"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pItem->byStackCount = READ_BYTE(bstrData, wszFieldNameBuf);
		}

		else if (0 == WStringCmpLiteral(*pstrDataName, L"Display_Bit_Flag"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->wDisplayBitFlag = (WORD)READ_BITFLAG( bstrData );
		}
		else
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszSubItemTblidx, WCHARLen(g_wszSubItemTblidx)) )
			{
				bool bFound = false;

				WCHAR szBuffer[1024] = { 0x00, };
				for( int i = 0; i < DBO_MAX_COUNT_HLSITEM_SUB_ITEM; i++ )
				{
					NTL_SWPRINTF( szBuffer, 1024, g_wszSubItemTblidxFormat, i + 1 );

					if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
					{
						pItem->asSubItem[ i ].itemTblidx = READ_DWORD( bstrData );

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
			else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszSubItemCount, WCHARLen(g_wszSubItemCount)) )
			{
				bool bFound = false;

				WCHAR szBuffer[1024] = { 0x00, };
				for( int i = 0; i < DBO_MAX_COUNT_HLSITEM_SUB_ITEM; i++ )
				{
					NTL_SWPRINTF( szBuffer, 1024, g_wszSubItemCountFormat, i + 1 );

					if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
					{
						pItem->asSubItem[ i ].byStackCount = READ_BYTE(bstrData, wszFieldNameBuf);

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
	}
	else
	{
		return false;
	}

	return true;
}


sTBLDAT* CHLSItemTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;
	
	return (sTBLDAT*)(iter->second); 
}

bool CHLSItemTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sHLS_ITEM_TBLDAT* pTableData = new sHLS_ITEM_TBLDAT;
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

	//	printf("pTableData->tblidx %u \n", pTableData->tblidx );
		if( false == AddTable(pTableData, bReload, bUpdate) )
		{
			delete pTableData;
		}

	} while (false != bLoop);

	return true;
}

bool CHLSItemTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sHLS_ITEM_TBLDAT* pTableData = (sHLS_ITEM_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}
