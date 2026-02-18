//***********************************************************************************
//
//	File		:	ItemRecipeTable.cpp
//
//	Begin		:	2008-11-4
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Doo Sup, Chung ( john@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************

#include "stdafx.h"
#include "ItemRecipeTable.h"
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

const WCHAR* CItemRecipeTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CItemRecipeTable::CItemRecipeTable(void)
{
	Init();
}

CItemRecipeTable::~CItemRecipeTable(void)
{
	Destroy();
}

bool CItemRecipeTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CItemRecipeTable::Destroy()
{
	CTable::Destroy();
}

void CItemRecipeTable::Init()
{
}

void* CItemRecipeTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sITEM_RECIPE_TBLDAT* pNewItem = new sITEM_RECIPE_TBLDAT;
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

bool CItemRecipeTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sITEM_RECIPE_TBLDAT* pItem = (sITEM_RECIPE_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pItem, sizeof(*pItem)))
			return false;

		delete pItem;

		return true;
	}

	return false;
}

bool CItemRecipeTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	sITEM_RECIPE_TBLDAT * pTbldat = (sITEM_RECIPE_TBLDAT*) pvTable;
	sITEM_RECIPE_TBLDAT * pExistTbldat = NULL;
		
	if( bReload )
	{
		pExistTbldat = (sITEM_RECIPE_TBLDAT*) FindData( pTbldat->tblidx );
		if( pExistTbldat )
		{
			CopyMemory( pTbldat, pExistTbldat, pTbldat->GetDataSize() );
			return true; 
		}
	}

	if( bUpdate) //for tables inside localize folder
	{
		pExistTbldat = (sITEM_RECIPE_TBLDAT*) FindData( pTbldat->tblidx );
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

bool CItemRecipeTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sITEM_RECIPE_TBLDAT* pItem = (sITEM_RECIPE_TBLDAT*)pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		static const WCHAR g_wszCreateItemTblidxPrefix[] = { 'C', 'r', 'e', 'a', 't', 'e', '_', 'I', 't', 'e', 'm', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 0 };
		static const WCHAR g_wszCreateItemTblidxFormat[] = { 'C', 'r', 'e', 'a', 't', 'e', '_', 'I', 't', 'e', 'm', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', '%', 'd', 0 };
		static const WCHAR g_wszCreateItemRatePrefix[] = { 'C', 'r', 'e', 'a', 't', 'e', '_', 'I', 't', 'e', 'm', '_', 'R', 'a', 't', 'e', '_', 0 };
		static const WCHAR g_wszCreateItemRateFormat[] = { 'C', 'r', 'e', 'a', 't', 'e', '_', 'I', 't', 'e', 'm', '_', 'R', 'a', 't', 'e', '_', '%', 'd', 0 };
		static const WCHAR g_wszMaterialTblidxPrefix[] = { 'M', 'a', 't', 'e', 'r', 'i', 'a', 'l', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 0 };
		static const WCHAR g_wszMaterialTblidxFormat[] = { 'M', 'a', 't', 'e', 'r', 'i', 'a', 'l', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', '%', 'd', 0 };
		static const WCHAR g_wszMaterialQuantityPrefix[] = { 'M', 'a', 't', 'e', 'r', 'i', 'a', 'l', '_', 'Q', 'u', 'a', 'n', 't', 'i', 't', 'y', '_', 0 };
		static const WCHAR g_wszMaterialQuantityFormat[] = { 'M', 'a', 't', 'e', 'r', 'i', 'a', 'l', '_', 'Q', 'u', 'a', 'n', 't', 'i', 't', 'y', '_', '%', 'd', 0 };

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
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->dwName = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Recipe_Type"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->byRecipeType = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Need_Mix_Level"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->byNeedMixLevel = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Need_Mix_Zenny"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->dwNeedMixZenny = READ_DWORD(bstrData);
		}
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszCreateItemTblidxPrefix, WCHARLen(g_wszCreateItemTblidxPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < DBO_MAX_COUNT_RECIPE_CREATE_ITEM; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszCreateItemTblidxFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					if ( 0 == i )
					{
						CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
					}
					pItem->asCreateItemTblidx[ i ].itemTblidx = READ_DWORD( bstrData );

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
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszCreateItemRatePrefix, WCHARLen(g_wszCreateItemRatePrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < DBO_MAX_COUNT_RECIPE_CREATE_ITEM; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszCreateItemRateFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					if ( 0 == i )
					{
						CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
					}
					pItem->asCreateItemTblidx[ i ].itemRate = READ_BYTE( bstrData , wszFieldNameBuf);

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
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszMaterialTblidxPrefix, WCHARLen(g_wszMaterialTblidxPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < DBO_MAX_COUNT_RECIPE_MATERIAL_ITEM; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszMaterialTblidxFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					if ( i == 0 )
					{
						CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
					}
					pItem->asMaterial[ i ].materialTblidx = READ_DWORD( bstrData );

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
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszMaterialQuantityPrefix, WCHARLen(g_wszMaterialQuantityPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < DBO_MAX_COUNT_RECIPE_MATERIAL_ITEM; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszMaterialQuantityFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					if ( i == 0 )
					{
						CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
					}
					pItem->asMaterial[ i ].byMaterialCount = READ_BYTE( bstrData, wszFieldNameBuf);

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


sTBLDAT* CItemRecipeTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;
	
	return (sTBLDAT*)(iter->second); 
}

bool CItemRecipeTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sITEM_RECIPE_TBLDAT* pTableData = new sITEM_RECIPE_TBLDAT;
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

bool CItemRecipeTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sITEM_RECIPE_TBLDAT* pTableData = (sITEM_RECIPE_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}