#include "stdafx.h"
#include "DropTableEx.h"
#include "NtlRandom.h"
#include "ItemTable.h"


int CDropTableEx::CNorBag::GetDrops(CItemTable * pItemTable, LstDropInfo & lstDropInfo)
{
	DWORD dwRand = RandomRange(1, 1000000);

	for (std::vector<sTBLDAT_DropItem *>::iterator it = m_vecDroper.begin(); it != m_vecDroper.end(); it++)
	{
		sTBLDAT_DropItem* pDrop = *it;

		if (CDropTableEx::IsDrop(dwRand, pDrop->m_dwProb1))
			break;

		DropInfo di;
		di.idxItem = pDrop->m_idxItem;

		if (di.idxItem == INVALID_TBLIDX)
		{
			di.idxItem = pItemTable->GetRandomTblidx(pDrop->m_idxItemMin, pDrop->m_idxItemMax, pDrop->m_byLevelMin, pDrop->m_byLevelMax);

			if (di.idxItem == INVALID_TBLIDX)
				return 0;
		}
		printf("dwRand %u, pDrop->m_dwProb1 %u, di.idxItem %u, pDrop->m_idxItem %u \n", pDrop->m_dwProb1, dwRand, di.idxItem, pDrop->m_idxItem);

		di.idxEnchant = pDrop->m_idxEnchantRate;

		if (pDrop->m_dwInterval > 0)
		{
			if ((m_dwTimeDroped + 60000) * pDrop->m_dwInterval > GetTickCount())
			{
				return 0;
			}
			m_dwTimeDroped = GetTickCount();
		}

		lstDropInfo.push_back(di);
	}

	return 1;
}


int CDropTableEx::CGrpBag::Push(sTBLDAT_DropBag * pTBLDAT_DropBag)
{
	sGROUP_INFO groupInfo;

	if (m_byType == 1)
	{
		std::map<BYTE, sGROUP_INFO>::iterator it = m_mapGroupNo.find(pTBLDAT_DropBag->m_byGroup);
		if (it != m_mapGroupNo.end())
		{
			sGROUP_INFO& existingGroupInfo = it->second;

			groupInfo.byGroupNo = existingGroupInfo.byGroupNo;
			groupInfo.nTryCount = existingGroupInfo.nTryCount;
		}
		else
		{
			groupInfo.byGroupNo = pTBLDAT_DropBag->m_byGroup;
			groupInfo.nTryCount = pTBLDAT_DropBag->m_wTryCount;
		}
	}
	else
	{
		m_byType = 1;
		groupInfo.byGroupNo = pTBLDAT_DropBag->m_byGroup;
		groupInfo.nTryCount = pTBLDAT_DropBag->m_wTryCount;
	}
	
	m_vecDroper.push_back(pTBLDAT_DropBag);

	m_mapGroupNo.insert({ groupInfo.byGroupNo , groupInfo });

	m_mmapBagByGroupNo.insert({ groupInfo.byGroupNo , pTBLDAT_DropBag });

	return 1;
}

int CDropTableEx::CGrpBag::GetDrops(CItemTable * pItemTable, CDropTableEx * pTable, LstDropInfo & lstDropInfo)
{
	if (m_byType == 1)
	{
		GetDropsFromGrpBag_Unq(pItemTable, pTable, lstDropInfo);
		return 1;
	}

	return 0;
}

int CDropTableEx::CGrpBag::GetDropsFromGrpBag_Unq(CItemTable * pItemTable, CDropTableEx * pTable, LstDropInfo & lstDropInfo)
{
	for (std::map<BYTE, sGROUP_INFO>::iterator it = m_mapGroupNo.begin(); it != m_mapGroupNo.end(); it++)
	{
		sGROUP_INFO& groupInfo = it->second;

		std::multimap<BYTE, sTBLDAT_DropBag *>::iterator iterLower = m_mmapBagByGroupNo.lower_bound(groupInfo.byGroupNo);
		std::multimap<BYTE, sTBLDAT_DropBag *>::iterator iterUpper = m_mmapBagByGroupNo.upper_bound(groupInfo.byGroupNo);

		for (int nTryCount = 0; nTryCount < groupInfo.nTryCount; nTryCount++)
			GetDropsHelper(pItemTable, pTable, lstDropInfo, iterLower, iterUpper);
	}

	return 1;
}

int CDropTableEx::CGrpBag::GetDropsHelper(CItemTable * pItemTable, CDropTableEx * pTable, LstDropInfo & lstDropInfo, std::multimap<BYTE, sTBLDAT_DropBag*>::iterator & iterLower, std::multimap<BYTE, sTBLDAT_DropBag*>::iterator & iterUpper)
{
	if (iterLower == m_mmapBagByGroupNo.end())
		return 0;

	DWORD dwRand = RandomRange(1, 1000000);

	std::multimap<BYTE, sTBLDAT_DropBag *>::iterator iter = iterLower;

	while (iter != iterUpper)
	{
		sTBLDAT_DropBag* pDropBag = iter->second;

		if (CDropTableEx::IsDrop(dwRand, pDropBag->m_dwProb1))
			break;

		CNorBag* pNorBag = pTable->FindDropBag(pDropBag->m_idxBag);

		if (pNorBag)
		{
			return pNorBag->GetDrops(pItemTable, lstDropInfo);
		}

		++iter;
	}

	return 1;
}

void CDropTableEx::Init(CTableTmp<sTBLDAT_DropItem>* pDropItem, CTableTmp<sTBLDAT_DropBag>* pDropBag)
{
	Init(pDropItem);
	Init(pDropBag);
}

void CDropTableEx::Init(CTableTmp<sTBLDAT_DropItem>* pDropItem)
{
	if (pDropItem)
	{
		ClearNorBag();
	//	printf("pDropItem->GetNumberOfTables() %I64u \n", pDropItem->GetNumberOfTables());
		for (CTable::TABLEIT iter = pDropItem->Begin(); iter != pDropItem->End(); iter++)
		{
			sTBLDAT_DropItem* pItem = (sTBLDAT_DropItem*)iter->second;

			if(!Add(pItem))
				CTable::CallErrorCallbackFunction("sTBLDAT_DropItem : Can't add (Idx = %u)", pItem->m_idxBag);
		}
	}
}

void CDropTableEx::Init(CTableTmp<sTBLDAT_DropBag> * pDropBag)
{
	if (pDropBag)
	{
		ClearGrpBag();
	//	printf("pDropBag->GetNumberOfTables() %I64u \n", pDropBag->GetNumberOfTables());
		for (CTable::TABLEIT iter = pDropBag->Begin(); iter != pDropBag->End(); iter++)
		{
			sTBLDAT_DropBag* pItem = (sTBLDAT_DropBag*)iter->second;

			if (!Add(pItem))
				CTable::CallErrorCallbackFunction("sTBLDAT_DropBag : Can't add (Idx = %u)", pItem->m_idxBigBag);
		}
	}
}

void CDropTableEx::ClearNorBag()
{
	for (std::map<TBLIDX, CNorBag*>::iterator it = m_mapNorBag.begin(); it != m_mapNorBag.end(); )
	{
		CDropTableEx::CNorBag* pBag = it->second;

		delete pBag;

		it = m_mapNorBag.erase(it);
	}
}

void CDropTableEx::ClearGrpBag()
{
	for (std::map<TBLIDX, CGrpBag*>::iterator it = m_mapGrpBag.begin(); it != m_mapGrpBag.end(); )
	{
		CDropTableEx::CGrpBag* pBag = it->second;

		delete pBag;

		it = m_mapGrpBag.erase(it);
	}
}

int CDropTableEx::Add(sTBLDAT_DropItem* pObj)
{
	CNorBag* pBag = FindDropBag(pObj->m_idxBag);
	if (pBag)
	{
		pBag->m_vecDroper.push_back(pObj);
	}
	else
	{
		pBag = new CNorBag(pObj->m_idxBag);

		if (m_mapNorBag.insert(std::make_pair(pObj->m_idxBag, pBag)).second == true)
		{
			pBag->m_vecDroper.push_back(pObj);
		}
	}

	return 1;
}

int CDropTableEx::Add(sTBLDAT_DropBag* pObj)
{
	CGrpBag* pBag = FindDropBigBag(pObj->m_idxBigBag);
	if (pBag)
	{
		return pBag->Push(pObj);
	}
	else
	{
		pBag = new CGrpBag(pObj->m_idxBigBag);

		if (m_mapGrpBag.insert(std::make_pair(pObj->m_idxBigBag, pBag)).second == true)
		{
			return pBag->Push(pObj);
		}
	}

	return 0;
}

void CDropTableEx::Clear()
{
	ClearNorBag();
	ClearGrpBag();
}

int CDropTableEx::GetDrops(CItemTable * pItemTable, TBLIDX idxBag, LstDropInfo & lstDropInfo)
{
	CGrpBag* pBag = FindDropBigBag(idxBag);
	if (pBag)
		return pBag->GetDrops(pItemTable, this, lstDropInfo);
	printf("could not find bigbag %u \n", idxBag);
	return 0;
}

bool CDropTableEx::HasAnyDropBag(TBLIDX tblidx)
{
	return FindDropBag(tblidx) != NULL;
}

bool CDropTableEx::HasAnyDropBigBag(TBLIDX tblidx)
{
	return FindDropBigBag(tblidx) != NULL;
}

int CDropTableEx::IsDrop(DWORD & rdwRand, DWORD dwProb)
{
	if (dwProb < rdwRand)
	{
		rdwRand -= dwProb;
		return 0;
	}

	return 1;
}

CDropTableEx::CNorBag * CDropTableEx::FindDropBag(TBLIDX idx)
{
	std::map<TBLIDX, CNorBag*>::iterator it = m_mapNorBag.find(idx);

	if (it != m_mapNorBag.end())
		return it->second;

	return NULL;
}

CDropTableEx::CGrpBag * CDropTableEx::FindDropBigBag(TBLIDX idx)
{
	std::map<TBLIDX, CGrpBag*>::iterator it = m_mapGrpBag.find(idx);

	if (it != m_mapGrpBag.end())
		return it->second;

	return NULL;
}
