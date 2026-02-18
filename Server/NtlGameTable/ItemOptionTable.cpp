#include "stdafx.h"
#include "ItemOptionTable.h"
#include "NtlDebug.h"
#include "NtlSkill.h"
#include "NtlSerializer.h"

//- yoshiki : Let's consider of implementing NtlAssert series.
//#include "NtlAssert.h"

// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CItemOptionTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CItemOptionTable::CItemOptionTable(void)
{
	Init();
}

CItemOptionTable::~CItemOptionTable(void)
{
	Destroy();
}

bool CItemOptionTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CItemOptionTable::Destroy()
{
	CTable::Destroy();
}

void CItemOptionTable::Init()
{
	//ZeroMemory(m_aOptionTbldat, sizeof(m_aOptionTbldat) );
}

void* CItemOptionTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sITEM_OPTION_TBLDAT* pNewItem = new sITEM_OPTION_TBLDAT;
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

bool CItemOptionTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sITEM_OPTION_TBLDAT* pItem = (sITEM_OPTION_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pItem, sizeof(*pItem)))
			return false;

		delete pItem;

		return true;
	}
	
	return false;
}

bool CItemOptionTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	sITEM_OPTION_TBLDAT * pTbldat = (sITEM_OPTION_TBLDAT*) pvTable;
	sITEM_OPTION_TBLDAT * pExistTbldat = NULL;


	if (false == pTbldat->bValidity_Able)
	{
		return false;
	}

	if( bReload )
	{
		pExistTbldat = (sITEM_OPTION_TBLDAT*) FindData( pTbldat->tblidx );
		if( pExistTbldat )
		{
			CopyMemory( pTbldat, pExistTbldat, pTbldat->GetDataSize() );

			return true; 
		}
	}

	if( bUpdate) //for tables inside localize folder
	{
	//	printf("UPDATE TABLE %d ! \n", pTbldat->tblidx);
		pExistTbldat = (sITEM_OPTION_TBLDAT*) FindData( pTbldat->tblidx );
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

bool CItemOptionTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sITEM_OPTION_TBLDAT* pItem = (sITEM_OPTION_TBLDAT*)pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		static const WCHAR g_wszSystemEffectPrefix[] = { 'S', 'y', 's', 't', 'e', 'm', '_', 'E', 'f', 'f', 'e', 'c', 't', '_', 0 };
		static const WCHAR g_wszSystemEffectFormat[] = { 'S', 'y', 's', 't', 'e', 'm', '_', 'E', 'f', 'f', 'e', 'c', 't', '_', '%', 'd', 0 };
		static const WCHAR g_wszTypePrefix[] = { 'T', 'y', 'p', 'e', '_', 0 };
		static const WCHAR g_wszTypeFormat[] = { 'T', 'y', 'p', 'e', '_', '%', 'd', 0 };
		static const WCHAR g_wszValuePrefix[] = { 'V', 'a', 'l', 'u', 'e', '_', 0 };
		static const WCHAR g_wszValueFormat[] = { 'V', 'a', 'l', 'u', 'e', '_', '%', 'd', 0 };
		static const WCHAR g_wszScouterInfoPrefix[] = { 'S', 'c', 'o', 'u', 't', 'e', 'r', '_', 'I', 'n', 'f', 'o', '_', 0 };
		static const WCHAR g_wszScouterInfoFormat[] = { 'S', 'c', 'o', 'u', 't', 'e', 'r', '_', 'I', 'n', 'f', 'o', '_', '%', 'd', 0 };

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->tblidx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Option_Name"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );

			READ_STRINGW(bstrData, pItem->wszOption_Name, _countof(pItem->wszOption_Name));
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Validity_Able") )
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->bValidity_Able = READ_BOOL( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Option_Rank"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->byOption_Rank = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Item_Group"))
		{
			pItem->byItem_Group = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Max_Quality"))
		{
			pItem->byMaxQuality = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Quality"))
		{
			pItem->byQuality = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Quality_Index"))
		{
			pItem->byQualityIndex = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Cost"))
		{
			pItem->dwCost = READ_DWORD( bstrData, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Level"))
		{
			pItem->byLevel = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszSystemEffectPrefix, WCHARLen(g_wszSystemEffectPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_SYSTEM_EFFECT_COUNT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszSystemEffectFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pItem->system_Effect[ i ] = READ_DWORD( bstrData );

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
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszTypePrefix, WCHARLen(g_wszTypePrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_SYSTEM_EFFECT_COUNT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszTypeFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pItem->bAppliedInPercent[ i ] = READ_BOOL( bstrData, wszFieldNameBuf );

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
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszValuePrefix, WCHARLen(g_wszValuePrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_SYSTEM_EFFECT_COUNT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszValueFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pItem->nValue[ i ] = READ_DWORD( bstrData );

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
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Active_Effect"))
		{
			pItem->activeEffect = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Active_Rate"))
		{
			pItem->fActiveRate = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Note"))
		{
			READ_STRING(bstrData, pItem->szNote, _countof(pItem->szNote));
		}
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszScouterInfoPrefix, WCHARLen(g_wszScouterInfoPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_SYSTEM_EFFECT_COUNT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszScouterInfoFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pItem->byScouterInfo[ i ] = READ_BYTE( bstrData, wszFieldNameBuf, INVALID_BYTE );

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


sTBLDAT* CItemOptionTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;
	
	return (sTBLDAT*)(iter->second); 
}

bool CItemOptionTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sITEM_OPTION_TBLDAT* pTableData = new sITEM_OPTION_TBLDAT;
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

	//	printf("CItemOptionTable: pTableData->tblidx %d %d\n", pTableData->tblidx, pTableData->system_Effect[0]);
		if( false == AddTable(pTableData, bReload, bUpdate) )
		{
			delete pTableData;
		}

	} while (false != bLoop);

	return true;
}

bool CItemOptionTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sITEM_OPTION_TBLDAT* pTableData = (sITEM_OPTION_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}