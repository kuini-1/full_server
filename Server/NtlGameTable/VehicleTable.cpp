//***********************************************************************************
//
//	File		:	VehicleTable.cpp
//
//	Begin		:	2008-11-04
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//***********************************************************************************

#include "stdafx.h"
#include "VehicleTable.h"
#include "NtlDebug.h"
#include "NtlBattle.h"
#include "NtlSerializer.h"

// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CVehicleTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CVehicleTable::CVehicleTable(void)
{
	Init();
}

CVehicleTable::~CVehicleTable(void)
{
	Destroy();
}

bool CVehicleTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CVehicleTable::Destroy()
{
	CTable::Destroy();
}

void CVehicleTable::Init()
{
}

void* CVehicleTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sVEHICLE_TBLDAT* pNewItem = new sVEHICLE_TBLDAT;
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

bool CVehicleTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sVEHICLE_TBLDAT* pItem = (sVEHICLE_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pItem, sizeof(*pItem)))
			return false;

		delete pItem;
		return true;
	}

	return false;
}

bool CVehicleTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bUpdate);

	sVEHICLE_TBLDAT * pTbldat = (sVEHICLE_TBLDAT*) pvTable;
	sVEHICLE_TBLDAT * pExistTbldat = NULL;

	if( true == bReload )
	{
		pExistTbldat = (sVEHICLE_TBLDAT*)FindData( pTbldat->tblidx );
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

bool CVehicleTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{

	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sVEHICLE_TBLDAT* pItem = (sVEHICLE_TBLDAT*)pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->tblidx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Model_Name"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			READ_STRING( bstrData, pItem->szModelName, _countof(pItem->szModelName) );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"SRP_Type"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->bySRPType = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Speed"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->bySpeed = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		//else if (0 == WStringCmpLiteral(*pstrDataName, L"Fuel_Efficiency"))
		//{
		//	CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
		//	pItem->byFuelEfficiency = READ_BYTE( bstrData, wszFieldNameBuf );
		//}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Vehicle_Type"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->byVehicleType = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Run_Height") )
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->wRunHeight = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Personnel"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->byPersonnel = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		//else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Name") )
		//{
		//	CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
		//	pItem->dwName = READ_DWORD( bstrData );
		//}
	}
	else
	{
		return false;
	}

	return true;
}


sTBLDAT* CVehicleTable::FindData(TBLIDX tblidx)
{
	if ( 0 == tblidx )
	{
		return NULL;
	}

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if ( End() == iter )
	{
		return NULL;
	}

	return (sTBLDAT*)(iter->second); 
}

bool CVehicleTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sVEHICLE_TBLDAT* pTableData = new sVEHICLE_TBLDAT;
		if ( NULL == pTableData )
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

bool CVehicleTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sVEHICLE_TBLDAT* pTableData = (sVEHICLE_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}