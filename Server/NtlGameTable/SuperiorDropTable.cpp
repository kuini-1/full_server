//***********************************************************************************
//
//	File		:	SuperiorDropTable.cpp
//
//	Begin		:	2006-06-2
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Doo  Sup, Chung   ( john@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************

#include "stdafx.h"
#include "SuperiorDropTable.h"
#include "NtlDebug.h"
#include "NtlSerializer.h"

// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CSuperiorDropTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CSuperiorDropTable::CSuperiorDropTable(void)
{
	Init();
}

CSuperiorDropTable::~CSuperiorDropTable(void)
{
	Destroy();
}

bool CSuperiorDropTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CSuperiorDropTable::Destroy()
{
	CTable::Destroy();
}

void CSuperiorDropTable::Init()
{
}

void* CSuperiorDropTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sSUPERIOR_DROP_TBLDAT* pDrop = new sSUPERIOR_DROP_TBLDAT;
		if (NULL == pDrop)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pDrop;
			return NULL;
		}

		m_dwCodePage = dwCodePage;
		return pDrop;
	}

	return NULL;
}

bool CSuperiorDropTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sSUPERIOR_DROP_TBLDAT* pDrop = (sSUPERIOR_DROP_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pDrop, sizeof(*pDrop)))
			return false;

		delete pDrop;

		return true;
	}

	return false;
}

bool CSuperiorDropTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bUpdate);

	sSUPERIOR_DROP_TBLDAT * pTbldat = (sSUPERIOR_DROP_TBLDAT*)pvTable;
	sSUPERIOR_DROP_TBLDAT * pExistTbldat = NULL;

	if( bReload )
	{
		pExistTbldat = (sSUPERIOR_DROP_TBLDAT*) FindData( pTbldat->tblidx );
		if( pExistTbldat )
		{
			CopyMemory( pTbldat, pExistTbldat, pTbldat->GetDataSize() );
			// return false for release reloaded table data
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

bool CSuperiorDropTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sSUPERIOR_DROP_TBLDAT* pDrop = (sSUPERIOR_DROP_TBLDAT*)pvTable;

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			pDrop->tblidx = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Max_Quality"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pDrop->byMax_Quality = READ_BYTE(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Option_Rate"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pDrop->fOption_Rate = READ_FLOAT(bstrData, wszFieldNameBuf);
		}
		else
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			static const WCHAR g_wszItemTblidx[] = { 'I', 't', 'e', 'm', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 0 };
			static const WCHAR g_wszDropRate[] = { 'D', 'r', 'o', 'p', '_', 'R', 'a', 't', 'e', '_', 0 };
			static const WCHAR g_wszItemTblidxFormat[] = { 'I', 't', 'e', 'm', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', '%', 'd', 0 };
			static const WCHAR g_wszDropRateFormat[] = { 'D', 'r', 'o', 'p', '_', 'R', 'a', 't', 'e', '_', '%', 'd', 0 };
			if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszItemTblidx, WCHARLen(g_wszItemTblidx)) )
			{
				bool bFound = false;

				WCHAR szBuffer[1024] = { 0x00, };
				for( int i = 0; i < NTL_MAX_SUPERIOR_DROP; i++ )
				{
					NTL_SWPRINTF( szBuffer, 1024, g_wszItemTblidxFormat, i + 1 );

					if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
					{
						pDrop->aItem_Tblidx[ i ] = READ_DWORD( bstrData );

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
			else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszDropRate, WCHARLen(g_wszDropRate)) )
			{
				bool bFound = false;

				WCHAR szBuffer[1024] = { 0x00, };
				for( int i = 0; i < NTL_MAX_SUPERIOR_DROP; i++ )
				{
					NTL_SWPRINTF( szBuffer, 1024, g_wszDropRateFormat, i + 1 );

					if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
					{
						pDrop->afDrop_Rate[ i ] = READ_FLOAT( bstrData, wszFieldNameBuf, 0.0f );

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


sTBLDAT* CSuperiorDropTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;

	return (sTBLDAT*)(iter->second); 
}

TBLIDX CSuperiorDropTable::FindDropIndex( sSUPERIOR_DROP_TBLDAT* psTblData, BYTE byIndex)
{
	if ( NTL_MAX_SUPERIOR_DROP <= byIndex || 0 > byIndex )
		return INVALID_TBLIDX;

	return psTblData->aItem_Tblidx[byIndex];
}

float CSuperiorDropTable::FindDropRate( sSUPERIOR_DROP_TBLDAT* psTblData, BYTE byIndex)
{
	if ( NTL_MAX_SUPERIOR_DROP <= byIndex || 0 > byIndex )
		return 0.0f;

	return psTblData->afDrop_Rate[byIndex];
}

bool CSuperiorDropTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sSUPERIOR_DROP_TBLDAT* pTableData = new sSUPERIOR_DROP_TBLDAT;
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

		//  [4/26/2008 zeroera] : ???? : ????????? Load?? ??????? File Loading???? ???????
		if( false == AddTable(pTableData, bReload, bUpdate) )
		{
			delete pTableData;
		}

	} while (false != bLoop);

	return true;
}

bool CSuperiorDropTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sSUPERIOR_DROP_TBLDAT* pTableData = (sSUPERIOR_DROP_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}