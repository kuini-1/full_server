#include "stdafx.h"
#include "DwcTable.h"
#include "NtlDebug.h"
#include "NtlSerializer.h"

// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };
static const WCHAR g_wszConditionTblidx[] = { 'C', 'o', 'n', 'd', 'i', 't', 'i', 'o', 'n', '_', 'T', 'b', 'l', 'i', 'd', 'x', 0 };
static const WCHAR g_wszMissionTblidx[] = { 'M', 'i', 's', 's', 'i', 'o', 'n', '_', 'T', 'b', 'l', 'i', 'd', 'x', 0 };
static const WCHAR g_wszConditionTblidxFormat[] = { 'C', 'o', 'n', 'd', 'i', 't', 'i', 'o', 'n', '_', 'T', 'b', 'l', 'i', 'd', 'x', '%', 'd', 0 };
static const WCHAR g_wszMissionTblidxFormat[] = { 'M', 'i', 's', 's', 'i', 'o', 'n', '_', 'T', 'b', 'l', 'i', 'd', 'x', '%', 'd', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CDwcTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CDwcTable::CDwcTable(void)
{
	Init();
}

CDwcTable::~CDwcTable(void)
{
	Destroy();
}

bool CDwcTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CDwcTable::Destroy()
{
	CTable::Destroy();
}

void CDwcTable::Init()
{
}

void* CDwcTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sDWC_TBLDAT* pNewHelp = new sDWC_TBLDAT;
		if (NULL == pNewHelp)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pNewHelp;
			return NULL;
		}
		
		m_dwCodePage = dwCodePage;
		return pNewHelp;
	}

	return NULL;
}

bool CDwcTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sDWC_TBLDAT* pHelp = (sDWC_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pHelp, sizeof(*pHelp)))
			return false;

		delete pHelp;

		return true;
	}

	return false;
}

bool CDwcTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);

	sDWC_TBLDAT* pTbldat = (sDWC_TBLDAT*) pvTable;

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

bool CDwcTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sDWC_TBLDAT* pHelp = (sDWC_TBLDAT*)pvTable;

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			pHelp->tblidx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Name"))
		{
			pHelp->tblNameIndex = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Level_Min"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pHelp->byLevel_Min = READ_BYTE( bstrData, wszFieldNameBuf, 0);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Level_Max"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pHelp->byLevel_Max = READ_BYTE( bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Admission_Bit_Flag"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pHelp->wAdmission_Bit_Flag = (WORD)READ_BITFLAG( bstrData, 0 );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Admission_Num_Min"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pHelp->byAdmission_Num_Min = READ_BYTE( bstrData, wszFieldNameBuf, 1);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Admission_Num_Max"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pHelp->byAdmission_Num_Max = READ_BYTE( bstrData, wszFieldNameBuf);
		}
		else
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszConditionTblidx, WCHARLen(g_wszConditionTblidx)) )
			{
				bool bFound = false;

				WCHAR szBuffer[1024] = { 0x00, };
				for( int i = 0; i < MAX_DWC_ADMISSION_CONDITION_COUNT; i++ )
				{
					NTL_SWPRINTF( szBuffer, 1024, g_wszConditionTblidxFormat, i + 1 );

					if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
					{
						pHelp->aConditionTblidx[ i ] = READ_DWORD( bstrData );

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
			else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszMissionTblidx, WCHARLen(g_wszMissionTblidx)) )
			{
				bool bFound = false;

				WCHAR szBuffer[1024] = { 0x00, };
				for( int i = 0; i < MAX_DWC_MISSION_COUNT_PER_SCENARIO; i++ )
				{
					NTL_SWPRINTF( szBuffer, 1024, g_wszMissionTblidxFormat, i + 1 );

					if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
					{
						pHelp->aMissionTblidx[ i ] = READ_DWORD( bstrData );

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
			else if (0 == WStringCmpLiteral(*pstrDataName, L"Prologue_Cinematic_Tblidx"))
			{
				pHelp->prologueCinematicTblidx = READ_DWORD( bstrData );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"Prologue_Text"))
			{
				pHelp->prologueTblidx = READ_DWORD( bstrData );
			}
			else if (0 == WStringCmpLiteral(*pstrDataName, L"World_Tblidx"))
			{
				pHelp->worldTblidx = READ_DWORD( bstrData );
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


sTBLDAT* CDwcTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;

	return (sTBLDAT*)(iter->second); 
}

bool CDwcTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sDWC_TBLDAT* pTableData = new sDWC_TBLDAT;
		if (NULL == pTableData)
		{
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

bool CDwcTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sDWC_TBLDAT* pTableData = (sDWC_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}