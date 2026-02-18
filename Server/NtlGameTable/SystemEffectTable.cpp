#include "stdafx.h"

#include "NtlDebug.h"
#include "NtlSkill.h"

#include "SystemEffectTable.h"

#include "NtlSerializer.h"

//- yoshiki : Let's consider of implementing NtlAssert series.
//#include "NtlAssert.h"

// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CSystemEffectTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CSystemEffectTable::CSystemEffectTable(void)
{
	Init();
}

CSystemEffectTable::~CSystemEffectTable(void)
{
	Destroy();
}

bool CSystemEffectTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CSystemEffectTable::Destroy()
{
	CTable::Destroy();
}

void CSystemEffectTable::Init()
{
	memset( m_aEffectCodeMatchTable, 0xFF, sizeof(TBLIDX) * MAX_SYSTEM_EFFECT_CODE );
}

void* CSystemEffectTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sSYSTEM_EFFECT_TBLDAT* pNewSystemEffect = new sSYSTEM_EFFECT_TBLDAT;
		if (NULL == pNewSystemEffect)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pNewSystemEffect;
			return NULL;
		}
		
		m_dwCodePage = dwCodePage;
		return pNewSystemEffect;
	}

	return NULL;
}

bool CSystemEffectTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sSYSTEM_EFFECT_TBLDAT* pSystemEffect = (sSYSTEM_EFFECT_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pSystemEffect, sizeof(*pSystemEffect)))
			return false;

		delete pSystemEffect;

		return true;
	}

	return false;
}
#include "NtlStringHandler.h"
bool CSystemEffectTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);

	sSYSTEM_EFFECT_TBLDAT * pTbldat = (sSYSTEM_EFFECT_TBLDAT*) pvTable;

	//  [8/31/2006 zeroera] : ??? : System Effect Code MatchUp Table
	{
		char buffer[256] = { 0x00, };
		WideCharToMultiByte( ::GetACP(), 0, pTbldat->wszName, -1, buffer, 256, NULL, NULL );

		bool bFound = false;
		for( int i = 0; i < MAX_SYSTEM_EFFECT_CODE; i++ )
		{
			const char * pBuffer = NtlGetSystemEffectString( i );
			if( 0 == strcmp( buffer, pBuffer ) )
			{
				if( INVALID_TBLIDX != m_aEffectCodeMatchTable[ i ] )
				{
					CTable::CallErrorCallbackFunction(L"[File] : %s\n[Error] : Duplicated system effect found!(System Effect Name = %s)", m_wszXmlFileName, pTbldat->wszName);
					_ASSERTE( !"Effect Code Already Registered" );
					return false;
				}
				else
				{
					m_aEffectCodeMatchTable[ i ] = pTbldat->tblidx;
					pTbldat->effectCode = (eSYSTEM_EFFECT_CODE) i;

					bFound = true;
					break;
				}
			}
		}

		if( false == bFound )
		{
			printf("[File] : %ls\n[Error] : Unknown system effect found!(System Effect Name = %s id %d)", m_wszXmlFileName, Ntl_WC2MB(pTbldat->wszName), pTbldat->tblidx);
			_ASSERTE( !"Cant Find System Effect Code" );
			return false;
		}
	}


	if( false == m_mapTableList.insert( std::pair<TBLIDX, sTBLDAT*>(pTbldat->tblidx, pTbldat) ).second )
	{
		CTable::CallErrorCallbackFunction(L"[File] : %s\r\n Table Tblidx[%u] is Duplicated ",m_wszXmlFileName, pTbldat->tblidx );
		_ASSERTE( 0 );
		return false;
	}


	return true;
}

bool CSystemEffectTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sSYSTEM_EFFECT_TBLDAT* pSystemEffect = (sSYSTEM_EFFECT_TBLDAT*)pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pSystemEffect->tblidx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Name"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );

			READ_STRINGW(bstrData, pSystemEffect->wszName, _countof(pSystemEffect->wszName));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Effect_Type"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pSystemEffect->byEffect_Type = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Active_Effect_Type"))
		{
			pSystemEffect->byActive_Effect_Type = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Effect_Info_Text"))
		{
			pSystemEffect->Effect_Info_Text = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Keep_Effect_Name"))
		{
			pSystemEffect->Keep_Effect_Name = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Target_Effect_Position"))
		{
			pSystemEffect->byTarget_Effect_Position = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Success_Effect_Name"))
		{
			READ_STRING(bstrData, pSystemEffect->szSuccess_Effect_Name, _countof(pSystemEffect->szSuccess_Effect_Name));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Success_Projectile_Type"))
		{
			pSystemEffect->bySuccess_Projectile_Type = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Success_Effect_Position"))
		{
			pSystemEffect->bySuccess_Effect_Position = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Success_End_Effect_Name"))
		{
			READ_STRING(bstrData, pSystemEffect->szSuccess_End_Effect_Name, _countof(pSystemEffect->szSuccess_End_Effect_Name));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"End_Effect_Position"))
		{
			pSystemEffect->byEnd_Effect_Position = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Keep_Animation_Index"))
		{
			pSystemEffect->wKeep_Animation_Index = READ_WORD( bstrData, wszFieldNameBuf );
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


sTBLDAT* CSystemEffectTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;
	
	return (sTBLDAT*)(iter->second); 
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
eSYSTEM_EFFECT_CODE CSystemEffectTable::GetEffectCodeWithTblidx(TBLIDX tblidx)
{
	sSYSTEM_EFFECT_TBLDAT* pEffectTableData = (sSYSTEM_EFFECT_TBLDAT*)(FindData(tblidx));
	if (NULL == pEffectTableData)
	{
		return INVALID_SYSTEM_EFFECT_CODE;
	}

	return pEffectTableData->effectCode;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
sSYSTEM_EFFECT_TBLDAT * CSystemEffectTable::FindDataWithEffectCode(DWORD dwEffectCode)
{
	TBLIDX tblidx = GetEffectTblidx( dwEffectCode );
	if( INVALID_TBLIDX != tblidx )
	{
		return (sSYSTEM_EFFECT_TBLDAT *) FindData( tblidx );
	}

	return NULL;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
TBLIDX CSystemEffectTable::GetEffectTblidx(DWORD dwEffectCode)
{
	if( dwEffectCode >= MAX_SYSTEM_EFFECT_CODE )
	{
		return INVALID_TBLIDX;
	}

	return m_aEffectCodeMatchTable[ dwEffectCode ];
}

bool CSystemEffectTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sSYSTEM_EFFECT_TBLDAT* pTableData = new sSYSTEM_EFFECT_TBLDAT;
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

bool CSystemEffectTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sSYSTEM_EFFECT_TBLDAT* pTableData = (sSYSTEM_EFFECT_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}