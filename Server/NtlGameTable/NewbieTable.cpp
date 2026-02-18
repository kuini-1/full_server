
//***********************************************************************************
//
//	File		:	NewbieTable.cpp
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
#include "NewbieTable.h"
#include "NtlDebug.h"
#include "NtlCharacter.h"
#include "NtlSerializer.h"

static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) { WCharTLiteralToWCHAR(fmt, dest, destSize); }

const WCHAR* CNewbieTable::m_pwszSheetList[] =
{
	g_wszTableDataKOR,
	NULL
};
CNewbieTable::CNewbieTable(void)
{
	Init();
}

CNewbieTable::~CNewbieTable(void)
{
	Destroy();
}

bool CNewbieTable::Create(DWORD dwCodePage)
{
	return CTable::Create(dwCodePage);
}

void CNewbieTable::Destroy()
{
	::ZeroMemory(m_aNewbieTbldat, sizeof(m_aNewbieTbldat));

	CTable::Destroy();
}

void CNewbieTable::Init()
{
	ZeroMemory(m_aNewbieTbldat, sizeof(m_aNewbieTbldat) );
}

void* CNewbieTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sNEWBIE_TBLDAT* pNewSpawn = new sNEWBIE_TBLDAT;
		if (NULL == pNewSpawn)
			return NULL;

		CPINFO cpInfo;
		if(false == GetCPInfo(dwCodePage, &cpInfo))
		{
			delete pNewSpawn;
			return NULL;
		}

		m_dwCodePage = dwCodePage;
		return pNewSpawn;
	}

	return NULL;
}

bool CNewbieTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sNEWBIE_TBLDAT* pNewbie = (sNEWBIE_TBLDAT*)pvTable;
		if (FALSE != IsBadReadPtr(pNewbie, sizeof(*pNewbie)))
			return false;

		delete pNewbie;

		return true;
	}

	return false;
}

bool CNewbieTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
	UNREFERENCED_PARAMETER(bReload);
	UNREFERENCED_PARAMETER(bUpdate);

	sNEWBIE_TBLDAT* pNewbie = (sNEWBIE_TBLDAT*)pvTable;

	if (CNtlVector::ZERO == pNewbie->vSpawn_Dir)
	{
		pNewbie->vSpawn_Dir = CNtlVector::UNIT_X;
	}
	else
	{
		if ( false == (pNewbie->vSpawn_Dir).SafeNormalize())
		{
			_ASSERTE( 0 );
			return false;
		}
	}

	if (CNtlVector::ZERO == pNewbie->vBind_Dir)
	{
		pNewbie->vBind_Dir = CNtlVector::UNIT_X;
	}
	else
	{
		if ( false == (pNewbie->vBind_Dir).SafeNormalize())
		{
			_ASSERTE( 0 );
			return false;
		}
	}

	if ( false == m_mapTableList.insert(std::pair<TBLIDX, sTBLDAT*>(pNewbie->tblidx, pNewbie)).second )
	{
		WCHAR wszFormatBuf[512];
		FormatStringToWCHAR(L"[File] : %s\r\n Table Tblidx[%u] is Duplicated ", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
		CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, pNewbie->tblidx );
		_ASSERTE( 0 );
		return false;
	}

	//  [6/7/2006 john] : ??? : race, class ?? ???? tbldat ????
	if( false == SetNewbieTbldat( pNewbie->byRace, pNewbie->byClass, pNewbie ) )
	{
		_ASSERTE( 0 );
		m_mapTableList.erase(pNewbie->tblidx);
		return false;
	}


	return true;
}

bool CNewbieTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
	if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
	{
		sNEWBIE_TBLDAT* pNewbie = (sNEWBIE_TBLDAT*)pvTable;

		WCHAR wszFieldNameBuf[256];
		WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));

		static const WCHAR g_wszItemTblidxPrefix[] = { 'I', 't', 'e', 'm', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 0 };
		static const WCHAR g_wszItemTblidxFormat[] = { 'I', 't', 'e', 'm', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', '%', 'd', 0 };
		static const WCHAR g_wszPositionPrefix[] = { 'P', 'o', 's', 'i', 't', 'i', 'o', 'n', '_', 0 };
		static const WCHAR g_wszPositionFormat[] = { 'P', 'o', 's', 'i', 't', 'i', 'o', 'n', '_', '%', 'd', 0 };
		static const WCHAR g_wszStackQuantityPrefix[] = { 'S', 't', 'a', 'c', 'k', '_', 'Q', 'u', 'a', 'n', 't', 'i', 't', 'y', '_', 0 };
		static const WCHAR g_wszStackQuantityFormat[] = { 'S', 't', 'a', 'c', 'k', '_', 'Q', 'u', 'a', 'n', 't', 'i', 't', 'y', '_', '%', 'd', 0 };
		static const WCHAR g_wszSkillTblidxPrefix[] = { 'S', 'k', 'i', 'l', 'l', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', 0 };
		static const WCHAR g_wszSkillTblidxFormat[] = { 'S', 'k', 'i', 'l', 'l', '_', 'T', 'b', 'l', 'i', 'd', 'x', '_', '%', 'd', 0 };
		static const WCHAR g_wszQuickTblidxPrefix[] = { 'Q', 'u', 'i', 'c', 'k', '_', 'T', 'b', 'l', 'i', 'd', 'x', 0 };
		static const WCHAR g_wszQuickTblidxFormat[] = { 'Q', 'u', 'i', 'c', 'k', '_', 'T', 'b', 'l', 'i', 'd', 'x', '%', 'd', 0 };
		static const WCHAR g_wszQuickTypePrefix[] = { 'Q', 'u', 'i', 'c', 'k', '_', 'T', 'y', 'p', 'e', 0 };
		static const WCHAR g_wszQuickTypeFormat[] = { 'Q', 'u', 'i', 'c', 'k', '_', 'T', 'y', 'p', 'e', '%', 'd', 0 };
		static const WCHAR g_wszQuickPositionPrefix[] = { 'Q', 'u', 'i', 'c', 'k', '_', 'P', 'o', 's', 'i', 't', 'i', 'o', 'n', 0 };
		static const WCHAR g_wszQuickPositionFormat[] = { 'Q', 'u', 'i', 'c', 'k', '_', 'P', 'o', 's', 'i', 't', 'i', 'o', 'n', '%', 'd', 0 };

		if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
		{
			pNewbie->tblidx = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Race"))
		{
			pNewbie->byRace = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Class"))
		{
			pNewbie->byClass = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"World_Id"))
		{
			pNewbie->world_Id = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Tutorial_World_Tblidx"))
		{
			pNewbie->tutorialWorld = READ_DWORD( bstrData );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Spawn_Loc_X"))
		{
			pNewbie->vSpawn_Loc.x = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Spawn_Loc_Y"))
		{
			pNewbie->vSpawn_Loc.y = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Spawn_Loc_Z"))
		{
			pNewbie->vSpawn_Loc.z = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Spawn_Dir_X"))
		{
			pNewbie->vSpawn_Dir.x = READ_FLOAT( bstrData, wszFieldNameBuf, 0.0f );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Spawn_Dir_Z"))
		{
			pNewbie->vSpawn_Dir.z = READ_FLOAT( bstrData, wszFieldNameBuf, 0.0f );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Bind_Loc_X"))
		{
			pNewbie->vBind_Loc.x = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Bind_Loc_Y"))
		{
			pNewbie->vBind_Loc.y = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Bind_Loc_Z"))
		{
			pNewbie->vBind_Loc.z = READ_FLOAT( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Bind_Dir_X"))
		{
			pNewbie->vBind_Dir.x = READ_FLOAT( bstrData, wszFieldNameBuf, 0.0f );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Bind_Dir_Z"))
		{
			pNewbie->vBind_Dir.z = READ_FLOAT( bstrData, wszFieldNameBuf, 0.0f );
		}
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszItemTblidxPrefix, WCHARLen(g_wszItemTblidxPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_NEWBIE_ITEM; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszItemTblidxFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pNewbie->aitem_Tblidx[ i ] = READ_DWORD( bstrData );

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
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszPositionPrefix, WCHARLen(g_wszPositionPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_NEWBIE_ITEM; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszPositionFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pNewbie->abyPos[ i ] = READ_BYTE( bstrData, wszFieldNameBuf, EQUIP_SLOT_TYPE_UNKNOWN );

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
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszStackQuantityPrefix, WCHARLen(g_wszStackQuantityPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_NEWBIE_ITEM; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszStackQuantityFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pNewbie->abyStack_Quantity[ i ] = READ_BYTE( bstrData, wszFieldNameBuf, 1 );

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
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszSkillTblidxPrefix, WCHARLen(g_wszSkillTblidxPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_NEWBIE_SKILL; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszSkillTblidxFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pNewbie->aSkillTblidx[ i ] = READ_DWORD( bstrData );

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
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszQuickTblidxPrefix, WCHARLen(g_wszQuickTblidxPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_NEWBIE_QUICKSLOT_COUNT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszQuickTblidxFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pNewbie->asQuickData[ i ].tbilidx = READ_DWORD( bstrData );

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
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszQuickTypePrefix, WCHARLen(g_wszQuickTypePrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_NEWBIE_QUICKSLOT_COUNT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszQuickTypeFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pNewbie->asQuickData[ i ].byType = READ_BYTE( bstrData, wszFieldNameBuf );

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
		else if ( 0 == WCHARNCmp(wszFieldNameBuf, g_wszQuickPositionPrefix, WCHARLen(g_wszQuickPositionPrefix)) )
		{
			bool bFound = false;

			WCHAR szBuffer[1024] = { 0x00, };
			for( int i = 0; i < NTL_MAX_NEWBIE_QUICKSLOT_COUNT; i++ )
			{
				NTL_SWPRINTF( szBuffer, 1024, g_wszQuickPositionFormat, i + 1 );

				if( 0 == WCHARCmp(wszFieldNameBuf, szBuffer) )
				{
					pNewbie->asQuickData[ i ].byQuickSlot = READ_BYTE( bstrData, wszFieldNameBuf );

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
		else if (0 == WStringCmpLiteral(*pstrDataName, L"Map_Name_Tblidx"))
		{
			pNewbie->mapNameTblidx = READ_DWORD( bstrData );
		}	
		else if (0 == WStringCmpLiteral(*pstrDataName, L"QItem_Tblidx_1"))
		{
			pNewbie->qItemTblidx1 = READ_DWORD( bstrData );
		}	
		else if (0 == WStringCmpLiteral(*pstrDataName, L"QPosition_1"))
		{
			pNewbie->byQPosition1 = READ_BYTE( bstrData, wszFieldNameBuf );
		}
		else if (0 == WStringCmpLiteral(*pstrDataName, L"QStack_Quantity_1"))
		{
			pNewbie->byQStackQuantity1 = READ_BYTE( bstrData , wszFieldNameBuf );
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


sTBLDAT* CNewbieTable::FindData(TBLIDX tblidx)
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
//		Return	:	return tbl data using race and class
//-----------------------------------------------------------------------------------
sTBLDAT * CNewbieTable::GetNewbieTbldat(BYTE byRace, BYTE byClass)					
{
	return m_aNewbieTbldat[byRace][byClass];
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CNewbieTable::SetNewbieTbldat(BYTE byRace, BYTE byClass, sTBLDAT * pTbldat)
{
	if( byRace > RACE_LAST )
	{
		return false;
	}

	if( byClass > PC_CLASS_LAST )
	{
		return false;
	}

	// already registered?
	if( NULL != m_aNewbieTbldat[byRace][byClass] )
	{
		return false;
	}

	m_aNewbieTbldat[byRace][byClass] = pTbldat;

	return true;
}

bool CNewbieTable::LoadFromBinary(CNtlSerializer& serializer, bool bReload, bool bUpdate)
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
		sNEWBIE_TBLDAT* pTableData = new sNEWBIE_TBLDAT;
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

	//	printf("NEWBIE TABLE: pTableData->tblidx %d %d\n", pTableData->tblidx, pTableData->wUnknown);
		if( false == AddTable(pTableData, bReload, bUpdate) )
		{
			delete pTableData;
		}

	} while (false != bLoop);

	return true;
}

bool CNewbieTable::SaveToBinary(CNtlSerializer& serializer)
{
	serializer.Refresh();

	BYTE byMargin = 1;
	serializer << byMargin;

	TABLE::iterator iter;
	for (iter = Begin() ; End() != iter ; iter++)
	{
		sNEWBIE_TBLDAT* pTableData = (sNEWBIE_TBLDAT*)(iter->second);

		pTableData->SaveToBinary(serializer);
	}

	return true;
}