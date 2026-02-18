//***********************************************************************************
//
//	File		:	PCTable.cpp
//
//	Begin		:	2006-03-09
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Doo Sup, Chung ( john@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************

#include "stdafx.h"
#include "PCTable.h"
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

const WCHAR* CPCTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CPCTable::CPCTable(void)
{
	Init();
}

CPCTable::~CPCTable(void)
{
	Destroy();
}

bool CPCTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CPCTable::Destroy()
{
	::ZeroMemory(m_aPcTbldat, sizeof(m_aPcTbldat));

	CTable::Destroy();
}

void CPCTable::Init()
{
	ZeroMemory(m_aPcTbldat, sizeof(m_aPcTbldat) );
}

void* CPCTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sPC_TBLDAT* pNewPC = new sPC_TBLDAT;
		if (NULL == pNewPC)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pNewPC;
			return NULL;
		}
		
		m_dwCodePage = dwCodePage;
		return pNewPC;
	}

	return NULL;
}

bool CPCTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sPC_TBLDAT* pPC = (sPC_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pPC, sizeof(*pPC)))
			return false;

		delete pPC;

		return true;
	}
	
	return false;
}

bool CPCTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);

	sPC_TBLDAT * pTbldat = (sPC_TBLDAT*)pvTable;

	if( false == m_mapTableList.insert( std::pair<TBLIDX, sTBLDAT*>(pTbldat->tblidx, pTbldat) ).second )
	{
		WCHAR wszFormatBuf[512];
		FormatStringToWCHAR(L"[File] : %s\r\n Table Tblidx[%u] is Duplicated ", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
		CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, pTbldat->tblidx );
		_ASSERTE( 0 );
		return false;
	}

	if( false == SetPcTbldat( pTbldat->byRace, pTbldat->byClass, pTbldat->byGender, pTbldat ) )
	{
		_ASSERTE( 0 );
		m_mapTableList.erase(pTbldat->tblidx);
		return false;
	}
		

	return true;
}

bool CPCTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	sPC_TBLDAT* pPC = (sPC_TBLDAT*)pvTable;	

	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->tblidx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Race"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->byRace = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Gender"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->byGender = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Class"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->byClass = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Prior_Class_Tblidx"))
		{
			pPC->prior_Class_Tblidx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Class_Bit_Flag"))
		{
			pPC->dwClass_Bit_Flag = READ_BITFLAG( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Model_Child"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );

			READ_STRING(bstrData, pPC->szModel_Child, _countof(pPC->szModel_Child));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Model_Adult"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );

			READ_STRING(bstrData, pPC->szModel_Adult, _countof(pPC->szModel_Adult));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_LP"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->dwBasic_LP = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_EP"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->wBasic_EP = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_RP"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->wBasic_RP = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Physical_Offence"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->wBasic_Physical_Offence = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Physical_Defence"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->wBasic_Physical_Defence = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Energy_Offence"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->wBasic_Energy_Offence = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Energy_Defence"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->wBasic_Energy_Defence = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Energy_Defence"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->wBasic_Energy_Defence = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Str"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->wBasicStr = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Con"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->wBasicCon = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Foc"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->wBasicFoc = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Dex"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->wBasicDex = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Sol"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->wBasicSol = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Eng"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->wBasicEng = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Level_Up_LP"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->byLevel_Up_LP = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Level_Up_EP"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->byLevel_Up_EP = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Level_Up_RP"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->byLevel_Up_RP = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Level_Up_Physical_Offence"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->byLevel_Up_Physical_Offence = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Level_Up_Physical_Defence"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->byLevel_Up_Physical_Defence = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Level_Up_Energy_Offence"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->byLevel_Up_Energy_Offence = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Level_Up_Energy_Defence"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->byLevel_Up_Energy_Defence = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Level_Up_Str"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->fLevel_Up_Str = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Level_Up_Con"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->fLevel_Up_Con = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Level_Up_Foc"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->fLevel_Up_Foc = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Level_Up_Dex"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->fLevel_Up_Dex = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Level_Up_Sol"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->fLevel_Up_Sol = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Level_Up_Eng"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->fLevel_Up_Eng = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Scale"))
		{
			pPC->fScale = READ_FLOAT( bstrData, wszFieldNameBuf, 1.0f );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Child_Run_Speed_Origin"))
		{
			pPC->fChild_Run_Speed_Origin = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Child_Run_Speed"))
		{
			pPC->fChild_Run_Speed = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Adult_Run_Speed_Origin"))
		{
			pPC->fAdult_Run_Speed_Origin = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Adult_Run_Speed"))
		{
			pPC->fAdult_Run_Speed = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
//		else if (0 == WStringCmpLiteral(*pstrDataName, L"Radius"))
//		{
//			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
//			pPC->fRadius = READ_FLOAT( bstrData, pstrDataName->c_str() );
//		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Attack_Speed_Rate"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->wAttack_Speed_Rate = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Attack_Type"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->byAttack_Type = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Attack_Range"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->fAttack_Range = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Attack_Rate"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->wAttack_Rate = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Dodge_Rate"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->wDodge_Rate = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Block_Rate"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->wBlock_Rate = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Curse_Success_Rate"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->wCurse_Success_Rate = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Curse_Tolerance_Rate"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pPC->wCurse_Tolerance_Rate = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Basic_Aggro_Point"))
		{
			pPC->wBasic_Aggro_Point = READ_WORD( bstrData, wszFieldNameBuf, 0 );
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


sTBLDAT* CPCTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;
	
	return (sTBLDAT*)(iter->second); 
}


//-----------------------------------------------------------------------------------
//		Purpose	:	
//		Return	:	race, class, gender?? ???? table?? ???
//-----------------------------------------------------------------------------------
sTBLDAT * CPCTable::GetPcTbldat(BYTE byRace, BYTE byClass, BYTE byGender)					
{
	if( byRace > RACE_LAST )
	{
		return NULL;
	}

	if( byClass > PC_CLASS_LAST )
	{
		return NULL;
	}

	if( byGender > GENDER_LAST )
	{
		return NULL;
	}


	return m_aPcTbldat[byRace][byClass][byGender];
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CPCTable::SetPcTbldat(BYTE byRace, BYTE byClass, BYTE byGender, sTBLDAT * pTbldat)
{
	if( byRace > RACE_LAST )
	{
		return false;
	}

	if( byClass > PC_CLASS_LAST )
	{
		return false;
	}

	if( byGender > GENDER_LAST )
	{
		return false;
	}

	// already registered?
	if( NULL != m_aPcTbldat[byRace][byClass][byGender] )
	{
		return false;
	}

	m_aPcTbldat[byRace][byClass][byGender] = pTbldat;

	return true;
}

bool CPCTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sPC_TBLDAT* pTableData = new sPC_TBLDAT;
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

//		printf("pTableData->tblidx %d  dwWeightLimit %d \n", pTableData->tblidx, pTableData->dwWeightLimit);
		if( false == AddTable(pTableData, bReload, bUpdate) )
		{
			delete pTableData;
		}

	} while (false != bLoop);

	return true;
}

bool CPCTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sPC_TBLDAT* pTableData = (sPC_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}