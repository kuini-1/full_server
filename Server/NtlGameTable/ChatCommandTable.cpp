//***********************************************************************************
//
//	File		:	ChatCommandTable.cpp
//
//	Begin		:	2006-08-29
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Doo Sup, Chung   ( john@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************

#include "stdafx.h"
#include "NtlDebug.h"

#include "ChatCommandTable.h"

#include "NtlSerializer.h"

// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };
static const WCHAR g_wszChatCommand[] = { 'C', 'h', 'a', 't', '_', 'C', 'o', 'm', 'm', 'a', 'n', 'd', '_', 0 };
static const WCHAR g_wszChatCommandFormat[] = { 'C', 'h', 'a', 't', '_', 'C', 'o', 'm', 'm', 'a', 'n', 'd', '_', '%', 'd', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
	WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CChatCommandTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};

CChatCommandTable::CChatCommandTable(void)
{
	Init();
}

CChatCommandTable::~CChatCommandTable(void)
{
	Destroy();
}

bool CChatCommandTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CChatCommandTable::Destroy()
{
	CTable::Destroy();
}

void CChatCommandTable::Init()
{
}

void* CChatCommandTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sCHAT_COMMAND_TBLDAT* pNewChatCommand = new sCHAT_COMMAND_TBLDAT;
		if (NULL == pNewChatCommand)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pNewChatCommand;
			return NULL;
		}

		m_dwCodePage = dwCodePage;
		return pNewChatCommand;
	}

	return NULL;
}

bool CChatCommandTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sCHAT_COMMAND_TBLDAT* pChatCommand = (sCHAT_COMMAND_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pChatCommand, sizeof(*pChatCommand)))
			return false;

		delete pChatCommand;

		return true;
	}

	return false;
}

bool CChatCommandTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);

	sCHAT_COMMAND_TBLDAT* pTbldat = (sCHAT_COMMAND_TBLDAT*)pvTable;
		
	if ( false == pTbldat->bValidity_Able )
	{
		return false;
	}


	if( false == m_mapTableList.insert(std::pair<TBLIDX, sTBLDAT*>(pTbldat->tblidx, pTbldat)).second )
	{
		WCHAR wszFormatBuf[512];
		FormatStringToWCHAR(L"[File] : %s\r\n Table Tblidx[%u] is Duplicated ", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
		CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, pTbldat->tblidx );
		_ASSERTE( 0 );
		return false;
	}


	return true;
}

bool CChatCommandTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sCHAT_COMMAND_TBLDAT* pChatCommand = (sCHAT_COMMAND_TBLDAT*)pvTable;

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			pChatCommand->tblidx = READ_DWORD(bstrData);
		}
		else if ( 0 == WStringCmpLiteral(*pstrDataName, L"Validity_Able") )
		{
			CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pChatCommand->bValidity_Able = READ_BOOL(bstrData, wszFieldNameBuf);
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Action_Animation_Index"))
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			pChatCommand->wAction_Animation_Index = READ_WORD(bstrData, wszFieldNameBuf);
		}
		else
		{
			WCHAR wszFieldNameBuf[256];
			WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
			if ( 0 == WCHARNCmp( wszFieldNameBuf, g_wszChatCommand, WCHARLen(g_wszChatCommand) ) )
			{
				bool bFound = false;
				WCHAR szBuffer[1024] = { 0x00, };
				for( int i = 0; i < NTL_MAX_CHAT_COMMAND; i++ )
				{
					NTL_SWPRINTF( szBuffer, 1024, g_wszChatCommandFormat, i + 1 );

					if( 0 == WCHARCmp( wszFieldNameBuf, szBuffer) )
					{
						if ( 0 == i )
							CheckNegativeInvalid( pstrDataName->c_str(), bstrData );
						pChatCommand->aChat_Command[ i ] = READ_DWORD( bstrData);

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
		_ASSERT(0);
		return false;
	}

	return true;
}


sTBLDAT* CChatCommandTable::FindData(TBLIDX tblidx)
{
	if (0 == tblidx)
		return NULL;

	TABLEIT iter;
	iter = m_mapTableList.find(tblidx);
	if (End() == iter)
		return NULL;

	return (sTBLDAT*)(iter->second); 
}

bool CChatCommandTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sCHAT_COMMAND_TBLDAT* pTableData = new sCHAT_COMMAND_TBLDAT;
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
		//  [4/26/2008 zeroera] : ???? : ????????? Load?? ??????? File Loading???? ???????
		if( false == AddTable(pTableData, bReload, bUpdate) )
		{
			delete pTableData;
		}

	} while (false != bLoop);

	return true;
}

bool CChatCommandTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sCHAT_COMMAND_TBLDAT* pTableData = (sCHAT_COMMAND_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}