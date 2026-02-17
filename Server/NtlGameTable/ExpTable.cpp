#include "stdafx.h"
#include "ExpTable.h"
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

const WCHAR* CExpTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CExpTable::CExpTable(void)
{
	Init();
}

CExpTable::~CExpTable(void)
{
	Destroy();
}

bool CExpTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CExpTable::Destroy()
{
	CTable::Destroy();
}

void CExpTable::Init()
{
}

void* CExpTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sEXP_TBLDAT* pNewExp = new sEXP_TBLDAT;
		if (NULL == pNewExp)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pNewExp;
			return NULL;
		}
		
		m_dwCodePage = dwCodePage;
		return pNewExp;
	}

	return NULL;
}

bool CExpTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sEXP_TBLDAT* pExp = (sEXP_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pExp, sizeof(*pExp)))
			return false;

		delete pExp;

		return true;
	}

	return false;
}



bool CExpTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bUpdate);

	sEXP_TBLDAT * pTbldat = (sEXP_TBLDAT*) pvTable;
	sEXP_TBLDAT * pExistTbldat = NULL;

	// Reload???? Data?? ??? Update?????
	if( bReload )
	{
		pExistTbldat = (sEXP_TBLDAT*) FindData( pTbldat->tblidx );
		if( pExistTbldat )
		{
			CopyMemory( pTbldat, pExistTbldat, pTbldat->GetDataSize() );
			// ??????? ?????? ???? false ???
			return true; 
		}
	}
	
	if ( false == m_mapTableList.insert( std::map<TBLIDX, sTBLDAT*>::value_type(pTbldat->tblidx, pTbldat)).second )
	{
		printf("[File] : %ws\r\n Table Tblidx[%u] is Duplicated ",m_wszXmlFileName, pTbldat->tblidx );
		_ASSERTE( 0 );
		return false;
	}
	
	return true;
}

bool CExpTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{

	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sEXP_TBLDAT* pExp = (sEXP_TBLDAT*)pvTable;

		if (0 == WStringCmpLiteral(*pstrDataName, L"Level"))
		{
			pExp->tblidx = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"EXP"))
		{
			pExp->dwExp = READ_DWORD(bstrData);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Need_EXP"))
		{
			pExp->dwNeed_Exp = READ_DWORD(bstrData);
		}
		//new
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Normal_Race"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pExp->wNormal_Race = READ_WORD(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Super_Race"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pExp->wSuperRace = READ_WORD(bstrData, wszFieldNameBuf);
		}

		else if (0 == WStringCmpLiteral(*pstrDataName, L"Mob_Exp"))
		{
			pExp->dwMobExp = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Phy_Defence_Ref"))
		{
			pExp->dwPhyDefenceRef = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Eng_Defence_Ref"))
		{
			pExp->dwEngDefenceRef = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Mob_Zenny"))
		{
			pExp->dwMobZenny = READ_DWORD( bstrData );
		}
		else
		{
			printf("[File] : %ws\n[Error] : Unknown field name found!(Field Name = %ls)", m_wszXmlFileName, pstrDataName->c_str());
			return false;
		}
	}
	else
	{
		return false;
	}

	return true;
}


sTBLDAT* CExpTable::FindData(TBLIDX tblidx)
{
	if (tblidx == 0)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (iter == End())
		return NULL;

	return (sTBLDAT*)(iter->second); 
}

bool CExpTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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

		sEXP_TBLDAT* pTableData = new sEXP_TBLDAT;

		if (pTableData == NULL)
		{
			printf("sEXP_TBLDAT* pTableData = new sEXP_TBLDAT is empty - destroy! \n");
			Destroy();
			return false;
		}

		/*This function adds data into pTableData*/
		if (pTableData->LoadFromBinary(serializer) == false)
		{
			delete pTableData;
			bLoop = false;
			break;
		}

	//	printf("EXP TABLE: pTableData->tblidx %d \n", pTableData->tblidx);
		if( AddTable(pTableData, bReload, bUpdate) == false)
		{
			printf("del exp\n");
			delete pTableData;
		}

	} while (bLoop != false);

	return true;
}

bool CExpTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sEXP_TBLDAT* pTableData = (sEXP_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}