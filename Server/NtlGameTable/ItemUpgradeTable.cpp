//***********************************************************************************
//
//	File		:	ItemUpgradeTable.cpp
//
//	Begin		:	2008-04-10
//
//	Copyright	:	¨Ï NTL-Inc Co., Ltd
//
//***********************************************************************************

#include "stdafx.h"
#include "ItemUpgradeTable.h"
#include "NtlDebug.h"
#include "NtlBattle.h"
#include "NtlSerializer.h"

WCHAR* CItemUpgradeTable::m_pwszSheetList[] =
{
	L"Table_Data_KOR",
	NULL
};

CItemUpgradeTable::CItemUpgradeTable(void)
{
	Init();
}

CItemUpgradeTable::~CItemUpgradeTable(void)
{
	Destroy();
}

bool CItemUpgradeTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CItemUpgradeTable::Destroy()
{
	CTable::Destroy();
}

void CItemUpgradeTable::Init()
{
}

void* CItemUpgradeTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == wcscmp(pwszSheetName, L"Table_Data_KOR"))
	{
		sITEM_UPGRADE_TBLDAT* pNewItem = new sITEM_UPGRADE_TBLDAT;
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

bool CItemUpgradeTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == wcscmp(pwszSheetName, L"Table_Data_KOR"))
	{
		sITEM_UPGRADE_TBLDAT* pItem = (sITEM_UPGRADE_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pItem, sizeof(*pItem)))
			return false;

		delete pItem;
		return true;
	}

	return false;
}

bool CItemUpgradeTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bUpdate);

	sITEM_UPGRADE_TBLDAT * pTbldat = (sITEM_UPGRADE_TBLDAT*) pvTable;
	sITEM_UPGRADE_TBLDAT * pExistTbldat = NULL;

	if( true == bReload )
	{
		pExistTbldat = (sITEM_UPGRADE_TBLDAT*)FindData( pTbldat->tblidx );
		if( pExistTbldat )
		{
			CopyMemory( pTbldat, pExistTbldat, pTbldat->GetDataSize() );
			return true; 
		}
	}

	if ( false == m_mapTableList.insert( std::map<TBLIDX, sTBLDAT*>::value_type(pTbldat->tblidx, pTbldat)).second )
	{
		CTable::CallErrorCallbackFunction(L"[File] : %s\r\n Table Tblidx[%u] is Duplicated ",m_wszXmlFileName, pTbldat->tblidx );
		_ASSERTE( 0 );
		return false;
	}

	return true;
}

bool CItemUpgradeTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{

	if (0 == wcscmp(pwszSheetName, L"Table_Data_KOR"))
	{
		sITEM_UPGRADE_TBLDAT* pItem = (sITEM_UPGRADE_TBLDAT*)pvTable;

		if (0 == wcscmp(pstrDataName->c_str(), L"Tblidx"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pItem->tblidx = READ_DWORD( bstrData );
		}
		else if (0 == wcscmp(pstrDataName->c_str(), L"Name_Text"))
		{
			READ_STRINGW(bstrData, pItem->wszNameText, _countof(pItem->wszNameText));
		}
		
	}
	else
	{
		return false;
	}

	return true;
}


sTBLDAT* CItemUpgradeTable::FindData(TBLIDX tblidx)
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

bool CItemUpgradeTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sITEM_UPGRADE_TBLDAT* pTableData = new sITEM_UPGRADE_TBLDAT;
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

bool CItemUpgradeTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sITEM_UPGRADE_TBLDAT* pTableData = (sITEM_UPGRADE_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}

WORD CItemUpgradeTable::GetItemUpgradeValue(BYTE byGrade, sITEM_UPGRADE_TBLDAT* tbldat)
{
	if(byGrade == 0)
		return 0;

	if( tbldat == NULL )
	{
		printf("sITEM_UPGRADE_TBLDAT not found \n" );
		return 0;
	}
	printf("sITEM_UPGRADE_TBLDAT found  %d %d\n", tbldat->tblidx, tbldat->wUp10 );
	WORD wValue;

	switch( byGrade )
	{
		case 1: wValue = tbldat->wUp1; break;
		case 2: wValue = tbldat->wUp2; break;
		case 3: wValue = tbldat->wUp3; break;
		case 4: wValue = tbldat->wUp4; break;
		case 5: wValue = tbldat->wUp5; break;
		case 6: wValue = tbldat->wUp6; break;
		case 7: wValue = tbldat->wUp7; break;
		case 8: wValue = tbldat->wUp8; break;
		case 9: wValue = tbldat->wUp9; break;
		case 10: wValue = tbldat->wUp10; break;
		case 11: wValue = tbldat->wUp11; break;
		case 12: wValue = tbldat->wUp12; break;
		case 13: wValue = tbldat->wUp13; break;
		case 14: wValue = tbldat->wUp14; break;
		case 15: wValue = tbldat->wUp15; break;
		default: wValue = 0; break;
	}

	return wValue;


}