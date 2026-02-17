#include "stdafx.h"
#include "NtlDebug.h"

#include "HTBSetTable.h"
#include "NtlCharacter.h"
#include "NtlBattle.h"

#include "NtlSerializer.h"

// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };
static const WCHAR g_wszHTBType[] = { 'H', 'T', 'B', '_', 'T', 'y', 'p', 'e', '_', 0 };
static const WCHAR g_wszSkillTblidx[] = { 'S', 'k', 'i', 'l', 'l', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 0 };
static const WCHAR g_wszHTBTypeFormat[] = { 'H', 'T', 'B', '_', 'T', 'y', 'p', 'e', '_', '%', 'd', 0 };
static const WCHAR g_wszSkillTblidxFormat[] = { 'S', 'k', 'i', 'l', 'l', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', '%', 'd', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CHTBSetTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CHTBSetTable::CHTBSetTable(void)
{
	Init();
}

CHTBSetTable::~CHTBSetTable(void)
{
	Destroy();
}

bool CHTBSetTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CHTBSetTable::Destroy()
{
	CTable::Destroy();
}

void CHTBSetTable::Init()
{
}

void* CHTBSetTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sHTB_SET_TBLDAT* pNewHTBSet = new sHTB_SET_TBLDAT;
		if (NULL == pNewHTBSet)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pNewHTBSet;
			return NULL;
		}
		
		m_dwCodePage = dwCodePage;
		return pNewHTBSet;
	}
	
	return NULL;
}

bool CHTBSetTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sHTB_SET_TBLDAT* pHTBSet = (sHTB_SET_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pHTBSet, sizeof(*pHTBSet)))
			return false;

		delete pHTBSet;

		return true;
	}
	
	return false;
}

bool CHTBSetTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);

	sHTB_SET_TBLDAT* pTbldat = (sHTB_SET_TBLDAT*)pvTable;
		
	if ( false == pTbldat->bValidity_Able )
	{
		return false;
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

bool CHTBSetTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sHTB_SET_TBLDAT * pHTBSet = (sHTB_SET_TBLDAT*) pvTable;

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			pHTBSet->tblidx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Name_Text"))
		{
			READ_STRINGW(bstrData, pHTBSet->wszNameText, _countof(pHTBSet->wszNameText));
		}		
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Validity_Able") )
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pHTBSet->bValidity_Able = READ_BOOL( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"PC_Class_Bit_Flag"))
		{
			pHTBSet->dwPC_Class_Bit_Flag = READ_BITFLAG( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Slot_Index"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pHTBSet->bySlot_Index = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Skill_Grade"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pHTBSet->bySkill_Grade = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"HTB_Skill_Name"))
		{
			pHTBSet->HTB_Skill_Name = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Icon_Name"))
		{
			READ_STRING(bstrData, pHTBSet->szIcon_Name, _countof(pHTBSet->szIcon_Name));
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Need_EP"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pHTBSet->wNeed_EP = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Require_Train_Level"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pHTBSet->byRequire_Train_Level = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Require_Zenny"))
		{
			pHTBSet->dwRequire_Zenny = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Next_Skill_Train_Exp"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pHTBSet->wNext_Skill_Train_Exp = READ_WORD( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Cool_Time"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pHTBSet->wCool_Time = READ_WORD( bstrData, wszFieldNameBuf );
			pHTBSet->dwCoolTimeInMilliSecs = pHTBSet->wCool_Time * 1000;
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Note"))
		{
			pHTBSet->Note = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Set_Count"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pHTBSet->bySetCount = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Stop_Point"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pHTBSet->byStop_Point = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			if( 0 == WCHARNCmp(wszFieldNameBuf, g_wszHTBType, WCHARLen(g_wszHTBType)) )
			{
				bool bFound = false;

				WCHAR szBuffer[1024] = { 0x00, };
				for( int i = 0; i < NTL_HTB_MAX_SKILL_COUNT_IN_SET; i++ )
				{
					NTL_SWPRINTF( szBuffer, 1024, g_wszHTBTypeFormat, i + 1 );

					if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
					{
						pHTBSet->aHTBAction[ i ].bySkillType = READ_BYTE( bstrData, wszFieldNameBuf );

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
			else if( 0 == WCHARNCmp(wszFieldNameBuf, g_wszSkillTblidx, WCHARLen(g_wszSkillTblidx)) )
			{
				bool bFound = false;

				WCHAR szBuffer[1024] = { 0x00, };
				for( int i = 0; i < NTL_HTB_MAX_SKILL_COUNT_IN_SET; i++ )
				{
					NTL_SWPRINTF( szBuffer, 1024, g_wszSkillTblidxFormat, i + 1 );

					if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
					{
						pHTBSet->aHTBAction[ i ].skillTblidx = READ_DWORD( bstrData );

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
			else if (0 == WStringCmpLiteral(*pstrDataName, L"Require_SP"))
			{
				CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
				WCHAR wszFieldNameBuf2[256];
				WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf2, sizeof(wszFieldNameBuf2)/sizeof(WCHAR));
				pHTBSet->wRequireSP = READ_WORD(bstrData, wszFieldNameBuf2);
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


sTBLDAT* CHTBSetTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;
	
	return (sTBLDAT*)(iter->second); 
}

bool CHTBSetTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sHTB_SET_TBLDAT* pTableData = new sHTB_SET_TBLDAT;
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

bool CHTBSetTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sHTB_SET_TBLDAT* pTableData = (sHTB_SET_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}