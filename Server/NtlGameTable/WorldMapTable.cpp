#include "stdafx.h"
#include "WorldMapTable.h"
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

const WCHAR* CWorldMapTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CWorldMapTable::CWorldMapTable(void)
{
	Init();
}

CWorldMapTable::~CWorldMapTable(void)
{
	Destroy();
}

bool CWorldMapTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CWorldMapTable::Destroy()
{
	CTable::Destroy();
}

void CWorldMapTable::Init()
{
}

void* CWorldMapTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sWORLD_MAP_TBLDAT* pWorld = new sWORLD_MAP_TBLDAT;
		if (NULL == pWorld)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pWorld;
			return NULL;
		}
		
		m_dwCodePage = dwCodePage;
		return pWorld;
	}

	return NULL;
}

bool CWorldMapTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sWORLD_MAP_TBLDAT* pWorld = (sWORLD_MAP_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pWorld, sizeof(*pWorld)))
			return false;

		delete pWorld;

		return true;
	}

	return false;
}

bool CWorldMapTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bUpdate);

	sWORLD_MAP_TBLDAT* pTbldat = (sWORLD_MAP_TBLDAT*)pvTable;
	sWORLD_MAP_TBLDAT * pExistTbldat = NULL;

	if( bReload )
	{
		pExistTbldat = (sWORLD_MAP_TBLDAT*) FindData( pTbldat->tblidx );
		if( pExistTbldat )
		{
			CopyMemory( pTbldat, pExistTbldat, pTbldat->GetDataSize() );
			return true; 
		}
	}
	
	if ( false == m_mapTableList.insert(std::pair<TBLIDX, sTBLDAT*>(pTbldat->tblidx, pTbldat)).second )
	{
		WCHAR wszFormatBuf[512];
		FormatStringToWCHAR(L"[File] : %s\r\n Table Tblidx[%u] is Duplicated ", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
		CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, pTbldat->tblidx );
		_ASSERTE( 0 );
		return false;
	}


	return true;
}

bool CWorldMapTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sWORLD_MAP_TBLDAT* pWorld = (sWORLD_MAP_TBLDAT*)pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		static const WCHAR g_wszWarfogPrefix[] = { 'W', 'a', 'r', 'f', 'o', 'g', '_', 0 };
		static const WCHAR g_wszWarfogFormat[] = { 'W', 'a', 'r', 'f', 'o', 'g', '_', '%', 'd', 0 };

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->tblidx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"World_Tblidx"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->World_Tblidx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Zone_Tblidx"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->Zone_Tblidx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Worldmap_Name"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->Worldmap_Name = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Name_Text"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			READ_STRINGW(bstrData, pWorld->wszNameText, _countof(pWorld->wszNameText));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Validity_Able"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->bValidityAble = READ_BOOL( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Map_Type"))
		{
			pWorld->byMapType = READ_BYTE(bstrData, wszFieldNameBuf, 0);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Standard_Loc_X"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->vStandardLoc.x = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Standard_Loc_Z"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->vStandardLoc.z = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Worldmap_Scale"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->fWorldmapScale = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Link_Map_Idx"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->dwLinkMapIdx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Combobox_Type"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pWorld->dwComboBoxType = READ_DWORD( bstrData );
		}
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszWarfogPrefix, WCHARLen(g_wszWarfogPrefix)) )
		{
			bool bFound = false;
			WCHAR szBuffer[1024] = { 0x00, };

			for( int i = 0; i < DBO_WORLD_MAP_TABLE_COUNT_WORLD_WARFOG; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszWarfogFormat, i + 1 );

				if( 0 == WCHARCmp( wszFieldNameBuf, szBuffer ) )
				{
					pWorld->wWarfog[i] = READ_WORD(bstrData, wszFieldNameBuf);
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
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Recomm_Min_Level"))
		{
			pWorld->byRecomm_Min_Level = READ_BYTE(bstrData, wszFieldNameBuf, 0);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Recomm_Max_Level"))
		{
			pWorld->byRecomm_Max_Level = READ_BYTE(bstrData, wszFieldNameBuf, 0);
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


sTBLDAT* CWorldMapTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;

	return (sTBLDAT*)(iter->second); 
}

bool CWorldMapTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sWORLD_MAP_TBLDAT* pTableData = new sWORLD_MAP_TBLDAT;
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

	//	printf("pTableData->tblidx %u \n", pTableData->tblidx);
		if( false == AddTable(pTableData, bReload, bUpdate) )
		{
			delete pTableData;
		}

	} while (false != bLoop);

	return true;
}

bool CWorldMapTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sWORLD_MAP_TBLDAT* pTableData = (sWORLD_MAP_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}