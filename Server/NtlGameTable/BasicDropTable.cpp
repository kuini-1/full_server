//***********************************************************************************
//
//	File		:	BasicDropTable.cpp
//
//	Begin		:	2006-03-27
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Doo  Sup, Chung   ( john@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************

#include "stdafx.h"
#include "BasicDropTable.h"
#include "NtlDebug.h"
#include "NtlSerializer.h"

// Static WCHAR arrays for string literals (Linux compatibility)
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };
static const WCHAR g_wszTblidx[] = { 'T', 'b', 'l', 'i', 'd', 'x', 0 };
static const WCHAR g_wszMax[] = { 'M', 'a', 'x', 0 };
static const WCHAR g_wszNormalTblidxRate[] = { 'N', 'o', 'r', 'm', 'a', 'l', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 'R', 'a', 't', 'e', '_', '%', 'd', 0 };
static const WCHAR g_wszNormalDropTblidx[] = { 'N', 'o', 'r', 'm', 'a', 'l', '_', 'D', 'r', 'o', 'p', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', '%', 'd', 0 };
static const WCHAR g_wszSuperiorTblidxRate[] = { 'S', 'u', 'p', 'e', 'r', 'i', 'o', 'r', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 'R', 'a', 't', 'e', '_', '%', 'd', 0 };
static const WCHAR g_wszSuperiorDropTblidx[] = { 'S', 'u', 'p', 'e', 'r', 'i', 'o', 'r', '_', 'D', 'r', 'o', 'p', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', '%', 'd', 0 };
static const WCHAR g_wszNormalTblidxRatePrefix[] = { 'N', 'o', 'r', 'm', 'a', 'l', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 'R', 'a', 't', 'e', '_', 0 };
static const WCHAR g_wszNormalDropTblidxPrefix[] = { 'N', 'o', 'r', 'm', 'a', 'l', '_', 'D', 'r', 'o', 'p', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 0 };
static const WCHAR g_wszSuperiorTblidxRatePrefix[] = { 'S', 'u', 'p', 'e', 'r', 'i', 'o', 'r', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 'R', 'a', 't', 'e', '_', 0 };
static const WCHAR g_wszSuperiorDropTblidxPrefix[] = { 'S', 'u', 'p', 'e', 'r', 'i', 'o', 'r', '_', 'D', 'r', 'o', 'p', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 0 };
static const WCHAR g_wszSuperiorDropRateControl[] = { 'S', 'u', 'p', 'e', 'r', 'i', 'o', 'r', '_', 'D', 'r', 'o', 'p', '_', 'R', 'a', 't', 'e', '_', 'C', 'o', 'n', 't', 'r', 'o', 'l', 0 };
static const WCHAR g_wszSuperiorOptionRateControl[] = { 'S', 'u', 'p', 'e', 'r', 'i', 'o', 'r', '_', 'O', 'p', 't', 'i', 'o', 'n', '_', 'R', 'a', 't', 'e', '_', 'C', 'o', 'n', 't', 'r', 'o', 'l', 0 };
static const WCHAR g_wszExcellentTblidxRate[] = { 'E', 'x', 'c', 'e', 'l', 'l', 'e', 'n', 't', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 'R', 'a', 't', 'e', '_', '%', 'd', 0 };
static const WCHAR g_wszExcellentDropTblidx[] = { 'E', 'x', 'c', 'e', 'l', 'l', 'e', 'n', 't', '_', 'D', 'r', 'o', 'p', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', '%', 'd', 0 };
static const WCHAR g_wszExcellentDropRateControl[] = { 'E', 'x', 'c', 'e', 'l', 'l', 'e', 'n', 't', '_', 'D', 'r', 'o', 'p', '_', 'R', 'a', 't', 'e', '_', 'C', 'o', 'n', 't', 'r', 'o', 'l', 0 };
static const WCHAR g_wszLegendaryTblidxRate[] = { 'L', 'e', 'g', 'e', 'n', 'd', 'a', 'r', 'y', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 'R', 'a', 't', 'e', '_', '%', 'd', 0 };
static const WCHAR g_wszLegendaryDropTblidx[] = { 'L', 'e', 'g', 'e', 'n', 'd', 'a', 'r', 'y', '_', 'D', 'r', 'o', 'p', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', '%', 'd', 0 };
static const WCHAR g_wszExcellentTblidxRatePrefix[] = { 'E', 'x', 'c', 'e', 'l', 'l', 'e', 'n', 't', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 'R', 'a', 't', 'e', '_', 0 };
static const WCHAR g_wszExcellentDropTblidxPrefix[] = { 'E', 'x', 'c', 'e', 'l', 'l', 'e', 'n', 't', '_', 'D', 'r', 'o', 'p', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 0 };
static const WCHAR g_wszLegendaryTblidxRatePrefix[] = { 'L', 'e', 'g', 'e', 'n', 'd', 'a', 'r', 'y', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 'R', 'a', 't', 'e', '_', 0 };
static const WCHAR g_wszLegendaryDropTblidxPrefix[] = { 'L', 'e', 'g', 'e', 'n', 'd', 'a', 'r', 'y', '_', 'D', 'r', 'o', 'p', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 0 };
static const WCHAR g_wszLegendaryDropRateControl[] = { 'L', 'e', 'g', 'e', 'n', 'd', 'a', 'r', 'y', '_', 'D', 'r', 'o', 'p', '_', 'R', 'a', 't', 'e', '_', 'C', 'o', 'n', 't', 'r', 'o', 'l', 0 };
static const WCHAR g_wszErrorFormat[] = { '[', 'F', 'i', 'l', 'e', ']', ' ', ':', ' ', '%', 's', '\n', '[', 'E', 'r', 'r', 'o', 'r', ']', ' ', ':', ' ', 'U', 'n', 'k', 'n', 'o', 'w', 'n', ' ', 'f', 'i', 'e', 'l', 'd', ' ', 'n', 'a', 'm', 'e', ' ', 'f', 'o', 'u', 'n', 'd', '!', '(', 'F', 'i', 'e', 'l', 'd', ' ', 'N', 'a', 'm', 'e', ' ', '=', ' ', '%', 's', ')', 0 };
static const WCHAR g_wszDuplicatedFormat[] = { '[', 'F', 'i', 'l', 'e', ']', ' ', ':', ' ', '%', 's', '\r', '\n', ' ', 'T', 'a', 'b', 'l', 'e', ' ', 'T', 'b', 'l', 'i', 'd', 'x', '[', '%', 'u', ']', ' ', 'i', 's', ' ', 'D', 'u', 'p', 'l', 'i', 'c', 'a', 't', 'e', 'd', ' ', 0 };

const WCHAR* CBasicDropTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CBasicDropTable::CBasicDropTable(void)
{
	Init();
}

CBasicDropTable::~CBasicDropTable(void)
{
	Destroy();
}

bool CBasicDropTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CBasicDropTable::Destroy()
{
	CTable::Destroy();
}

void CBasicDropTable::Init()
{
}

void* CBasicDropTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sBASIC_DROP_TBLDAT* pDrop = new sBASIC_DROP_TBLDAT;
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

bool CBasicDropTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sBASIC_DROP_TBLDAT* pDrop = (sBASIC_DROP_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pDrop, sizeof(*pDrop)))
			return false;

		delete pDrop;

		return true;
	}

	return false;
}

bool CBasicDropTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bUpdate);

	sBASIC_DROP_TBLDAT * pTbldat = (sBASIC_DROP_TBLDAT*)pvTable;
	sBASIC_DROP_TBLDAT * pExistTbldat = NULL;

	if( bReload )
	{
		pExistTbldat = (sBASIC_DROP_TBLDAT*) FindData( pTbldat->tblidx );
		if( pExistTbldat )
		{
			CopyMemory( pTbldat, pExistTbldat, pTbldat->GetDataSize() );
			return true; 
		}
	}

	if ( false == m_mapTableList.insert( std::map<TBLIDX, sTBLDAT*>::value_type(pTbldat->tblidx, pTbldat)).second )
	{
		CTable::CallErrorCallbackFunction(g_wszDuplicatedFormat, m_wszXmlFileName, pTbldat->tblidx );
		_ASSERTE( 0 );
		return false;
	}


	return true;
}

bool CBasicDropTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sBASIC_DROP_TBLDAT* pDrop = (sBASIC_DROP_TBLDAT*)pvTable;

		// Convert std::wstring::c_str() to WCHAR* for comparisons
		WCHAR wszDataNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszDataNameBuf, sizeof(wszDataNameBuf)/sizeof(WCHAR));

		if (0 == WCHARCmp(wszDataNameBuf, g_wszTblidx))
		{
			// Convert WCHAR* to wchar_t* for CheckNegativeInvalid
			wchar_t wbuf[256];
			for (size_t i = 0; i < 255 && wszDataNameBuf[i] != 0; i++) {
				wbuf[i] = (wchar_t)wszDataNameBuf[i];
			}
			wbuf[255] = L'\0';
			CheckNegativeInvalid( wbuf, bstrData );
			pDrop->tblidx = READ_DWORD( bstrData );
		}
		else if (0 == WCHARCmp(wszDataNameBuf, g_wszMax))
		{
			// Convert WCHAR* to wchar_t* for CheckNegativeInvalid
			wchar_t wbuf[256];
			for (size_t i = 0; i < 255 && wszDataNameBuf[i] != 0; i++) {
				wbuf[i] = (wchar_t)wszDataNameBuf[i];
			}
			wbuf[255] = L'\0';
			CheckNegativeInvalid( wbuf, bstrData );
			pDrop->byMax = READ_BYTE( bstrData, wszDataNameBuf );
		}
		else if ( 0 == WCHARNCmp(wszDataNameBuf, g_wszNormalTblidxRatePrefix, WCHARLen(g_wszNormalTblidxRatePrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_DROP_TABLE_SELECT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszNormalTblidxRate, i + 1 );

				if( 0 == WCHARCmp(wszDataNameBuf, szBuffer) )
				{
					pDrop->afNoramalTblidxRate[ i ] = READ_FLOAT( bstrData, wszDataNameBuf, 0.0f );

					bFound = true;
					break;
				}
			}

			if( false == bFound )
			{
				CTable::CallErrorCallbackFunction(g_wszErrorFormat, m_wszXmlFileName, wszDataNameBuf);
				return false;
			}
		}
		else if ( 0 == WCHARNCmp(wszDataNameBuf, g_wszNormalDropTblidxPrefix, WCHARLen(g_wszNormalDropTblidxPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_DROP_TABLE_SELECT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszNormalDropTblidx, i + 1 );

				if( 0 == WCHARCmp(wszDataNameBuf, szBuffer) )
				{
					pDrop->aNoramalDropTblidx[ i ] = READ_DWORD( bstrData );

					bFound = true;
					break;
				}
			}

			if( false == bFound )
			{
				CTable::CallErrorCallbackFunction(g_wszErrorFormat, m_wszXmlFileName, wszDataNameBuf);
				return false;
			}
		}
		else if ( 0 == WCHARNCmp(wszDataNameBuf, g_wszSuperiorTblidxRatePrefix, WCHARLen(g_wszSuperiorTblidxRatePrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_DROP_TABLE_SELECT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszSuperiorTblidxRate, i + 1 );

				if( 0 == WCHARCmp(wszDataNameBuf, szBuffer) )
				{
					pDrop->afSuperiorTblidxRate[ i ] = READ_FLOAT( bstrData, wszDataNameBuf, 0.0f );

					bFound = true;
					break;
				}
			}

			if( false == bFound )
			{
				CTable::CallErrorCallbackFunction(g_wszErrorFormat, m_wszXmlFileName, wszDataNameBuf);
				return false;
			}
		}
		else if ( 0 == WCHARNCmp(wszDataNameBuf, g_wszSuperiorDropTblidxPrefix, WCHARLen(g_wszSuperiorDropTblidxPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_DROP_TABLE_SELECT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszSuperiorDropTblidx, i + 1 );

				if( 0 == WCHARCmp(wszDataNameBuf, szBuffer) )
				{
					pDrop->aSuperiorDropTblidx[ i ] = READ_DWORD( bstrData );

					bFound = true;
					break;
				}
			}

			if( false == bFound )
			{
				CTable::CallErrorCallbackFunction(g_wszErrorFormat, m_wszXmlFileName, wszDataNameBuf);
				return false;
			}
		}
		else if (0 == WCHARCmp(wszDataNameBuf, g_wszSuperiorDropRateControl))
		{
			pDrop->fSuperior_Drop_Rate_Control = READ_FLOAT( bstrData, wszDataNameBuf, 1.0f );
		}
		else if (0 == WCHARCmp(wszDataNameBuf, g_wszSuperiorOptionRateControl))
		{
			pDrop->fSuperior_Option_Rate_Control = READ_FLOAT( bstrData, wszDataNameBuf, 1.0f );
		}
		else if ( 0 == WCHARNCmp(wszDataNameBuf, g_wszExcellentTblidxRatePrefix, WCHARLen(g_wszExcellentTblidxRatePrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_DROP_TABLE_SELECT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszExcellentTblidxRate, i + 1 );

				if( 0 == WCHARCmp(wszDataNameBuf, szBuffer) )
				{
					pDrop->afExcellentTblidxRate[ i ] = READ_FLOAT( bstrData, wszDataNameBuf, 0.0f );

					bFound = true;
					break;
				}
			}

			if( false == bFound )
			{
				CTable::CallErrorCallbackFunction(g_wszErrorFormat, m_wszXmlFileName, wszDataNameBuf);
				return false;
			}
		}
		else if ( 0 == WCHARNCmp(wszDataNameBuf, g_wszExcellentDropTblidxPrefix, WCHARLen(g_wszExcellentDropTblidxPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_DROP_TABLE_SELECT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszExcellentDropTblidx, i + 1 );

				if( 0 == WCHARCmp(wszDataNameBuf, szBuffer) )
				{
					pDrop->aExcellentDropTblidx[ i ] = READ_DWORD( bstrData );

					bFound = true;
					break;
				}
			}

			if( false == bFound )
			{
				CTable::CallErrorCallbackFunction(g_wszErrorFormat, m_wszXmlFileName, wszDataNameBuf);
				return false;
			}
		}		
		else if (0 == WCHARCmp(wszDataNameBuf, g_wszExcellentDropRateControl))
		{
			pDrop->fExcellent_Drop_Rate_Control = READ_FLOAT( bstrData, wszDataNameBuf, 1.0f );
		}
		else if ( 0 == WCHARNCmp(wszDataNameBuf, g_wszLegendaryTblidxRatePrefix, WCHARLen(g_wszLegendaryTblidxRatePrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_DROP_TABLE_SELECT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszLegendaryTblidxRate, i + 1 );

				if( 0 == WCHARCmp(wszDataNameBuf, szBuffer) )
				{
					pDrop->afLegendaryTblidxRate[ i ] = READ_FLOAT( bstrData, wszDataNameBuf, 0.0f );

					bFound = true;
					break;
				}
			}

			if( false == bFound )
			{
				CTable::CallErrorCallbackFunction(g_wszErrorFormat, m_wszXmlFileName, wszDataNameBuf);
				return false;
			}
		}
		else if ( 0 == WCHARNCmp(wszDataNameBuf, g_wszLegendaryDropTblidxPrefix, WCHARLen(g_wszLegendaryDropTblidxPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_DROP_TABLE_SELECT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszLegendaryDropTblidx, i + 1 );

				if( 0 == WCHARCmp(wszDataNameBuf, szBuffer) )
				{
					pDrop->aLegendaryDropTblidx[ i ] = READ_DWORD( bstrData );

					bFound = true;
					break;
				}
			}

			if( false == bFound )
			{
				CTable::CallErrorCallbackFunction(g_wszErrorFormat, m_wszXmlFileName, wszDataNameBuf);
				return false;
			}
		}		
		else if (0 == WCHARCmp(wszDataNameBuf, g_wszLegendaryDropRateControl))
		{
			pDrop->fLegendary_Drop_Rate_Control = READ_FLOAT( bstrData, wszDataNameBuf, 1.0f );
		}		
		else
		{
			CTable::CallErrorCallbackFunction(g_wszErrorFormat, m_wszXmlFileName, wszDataNameBuf);
			return false;
		}
	}
	else
	{
		return false;
	}

	return true;
}


sTBLDAT* CBasicDropTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;

	return (sTBLDAT*)(iter->second); 
}

TBLIDX CBasicDropTable::FindNormalDropIndex( sBASIC_DROP_TBLDAT* psTblData, BYTE byIndex)
{
	if ( NTL_MAX_DROP_TABLE_SELECT <= byIndex || 0 > byIndex )
		return INVALID_TBLIDX;

	return psTblData->aNoramalDropTblidx[byIndex];
}

float CBasicDropTable::FindNormalDropRate( sBASIC_DROP_TBLDAT* psTblData, BYTE byIndex)
{
	if ( NTL_MAX_DROP_TABLE_SELECT <= byIndex || 0 > byIndex )
		return 0.0f;

	return psTblData->afNoramalTblidxRate[byIndex];
}

TBLIDX CBasicDropTable::FindSuperiorDropIndex( sBASIC_DROP_TBLDAT* psTblData, BYTE byIndex)
{
	if ( NTL_MAX_DROP_TABLE_SELECT <= byIndex || 0 > byIndex )
		return INVALID_TBLIDX;

	return psTblData->aSuperiorDropTblidx[byIndex];
}

float CBasicDropTable::FindSuperiorDropRate( sBASIC_DROP_TBLDAT* psTblData, BYTE byIndex)
{
	if ( NTL_MAX_DROP_TABLE_SELECT <= byIndex || 0 > byIndex )
		return 0.0f;

	return psTblData->afSuperiorTblidxRate[byIndex];
}

TBLIDX CBasicDropTable::FindLegendaryDropIndex( sBASIC_DROP_TBLDAT* psTblData, BYTE byIndex)
{
	if ( NTL_MAX_DROP_TABLE_SELECT <= byIndex || 0 > byIndex )
		return INVALID_TBLIDX;

	return psTblData->aLegendaryDropTblidx[byIndex];
}

float CBasicDropTable::FindLegendaryDropRate( sBASIC_DROP_TBLDAT* psTblData, BYTE byIndex)
{
	if ( NTL_MAX_DROP_TABLE_SELECT <= byIndex || 0 > byIndex )
		return 0.0f;

	return psTblData->afLegendaryTblidxRate[byIndex];
}

TBLIDX CBasicDropTable::FindExcellentDropIndex( sBASIC_DROP_TBLDAT* psTblData, BYTE byIndex)
{
	if ( NTL_MAX_DROP_TABLE_SELECT <= byIndex || 0 > byIndex )
		return INVALID_TBLIDX;

	return psTblData->aExcellentDropTblidx[byIndex];
}

float CBasicDropTable::FindExcellentDropRate( sBASIC_DROP_TBLDAT* psTblData, BYTE byIndex)
{
	if ( NTL_MAX_DROP_TABLE_SELECT <= byIndex || 0 > byIndex )
		return 0.0f;

	return psTblData->afExcellentTblidxRate[byIndex];
}
bool CBasicDropTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sBASIC_DROP_TBLDAT* pTableData = new sBASIC_DROP_TBLDAT;
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

		
		if( false == AddTable(pTableData, bReload, bUpdate) )
		{
			delete pTableData;
		}

	} while (false != bLoop);

	return true;
}

bool CBasicDropTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sBASIC_DROP_TBLDAT* pTableData = (sBASIC_DROP_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}