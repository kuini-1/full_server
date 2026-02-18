//***********************************************************************************
//
//	File		:	ItemMixMachineTable.cpp
//
//	Begin		:	2009-04-24
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Chung Doo sup   ( john@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************

#include "stdafx.h"
#include "ItemMixMachineTable.h"
#include "NtlDebug.h"
#include "NtlBattle.h"
#include "NtlSerializer.h"

//- yoshiki : Let's consider of implementing NtlAssert series.
//#include "NtlAssert.h"

// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CItemMixMachineTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CItemMixMachineTable::CItemMixMachineTable(void)
{
	Init();
}

CItemMixMachineTable::~CItemMixMachineTable(void)
{
	Destroy();
}

bool CItemMixMachineTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CItemMixMachineTable::Destroy()
{
	CTable::Destroy();
}

void CItemMixMachineTable::Init()
{
}

void* CItemMixMachineTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sITEM_MIX_MACHINE_TBLDAT* pNewItem = new sITEM_MIX_MACHINE_TBLDAT;
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

bool CItemMixMachineTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sITEM_MIX_MACHINE_TBLDAT* pItem = (sITEM_MIX_MACHINE_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pItem, sizeof(*pItem)))
			return false;

		delete pItem;

		return true;
	}

	return false;
}

bool CItemMixMachineTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bUpdate);
	sITEM_MIX_MACHINE_TBLDAT * pTbldat = (sITEM_MIX_MACHINE_TBLDAT*) pvTable;
	sITEM_MIX_MACHINE_TBLDAT * pExistTbldat = NULL;
		
	if( bReload )
	{
		pExistTbldat = (sITEM_MIX_MACHINE_TBLDAT*) FindData( pTbldat->tblidx );
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

bool CItemMixMachineTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sITEM_MIX_MACHINE_TBLDAT* pItem = (sITEM_MIX_MACHINE_TBLDAT*)pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		static const WCHAR g_wszBuiltInRecipeTblidxPrefix[] = { 'B', 'u', 'i', 'l', 't', '_', 'I', 'n', '_', 'R', 'e', 'c', 'i', 'p', 'e', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 0 };
		static const WCHAR g_wszBuiltInRecipeTblidxFormat[] = { 'B', 'u', 'i', 'l', 't', '_', 'I', 'n', '_', 'R', 'e', 'c', 'i', 'p', 'e', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', '%', 'd', 0 };

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->tblidx = READ_DWORD( bstrData );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Validity_Able") )
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->bValidityAble = READ_BOOL( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Name"))
		{
			pItem->name = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Machine_Type"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->byMachineType = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Function_Bit_Flag"))
		{
			pItem->wFunctionBitFlag = (WORD)READ_BITFLAG( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Mix_Zenny_Discount_Rate"))
		{
			pItem->byMixZennyDiscountRate = READ_BYTE(bstrData, wszFieldNameBuf);
		}		
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Dynamic_Object_Tblidx"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->dynamicObjectTblidx = READ_DWORD( bstrData );
		}
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszBuiltInRecipeTblidxPrefix, WCHARLen(g_wszBuiltInRecipeTblidxPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < DBO_MAX_COUNT_RECIPE_MATERIAL_ITEM; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszBuiltInRecipeTblidxFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pItem->aBuiltInRecipeTblidx[ i ] = READ_DWORD( bstrData );
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


sTBLDAT* CItemMixMachineTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;
	
	return (sTBLDAT*)(iter->second); 
}

bool CItemMixMachineTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sITEM_MIX_MACHINE_TBLDAT* pTableData = new sITEM_MIX_MACHINE_TBLDAT;
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

bool CItemMixMachineTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sITEM_MIX_MACHINE_TBLDAT* pTableData = (sITEM_MIX_MACHINE_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}