#include "stdafx.h"
#include "EventSystemEvent.h"
#include "NtlRandom.h"
#include "EventSystemTable.h"
#include "TableContainerManager.h"
#include "SystemEffectTable.h"
#include "UseItemTable.h"
#include "ItemManager.h"
#include "ItemDrop.h"
#include "Monster.h"
#include "ObjectManager.h"
#include "CPlayer.h"
#include "World.h"
#include "GameServer.h"
#include "GameMain.h"


CEventSystemEvent::CEventSystemEvent()
{
	Init();
}

CEventSystemEvent::~CEventSystemEvent()
{
	Destroy();
}

void CEventSystemEvent::Create()
{
	BuildTimeSchedule();
}

void CEventSystemEvent::TickProcess(DWORD dwTickDiff)
{
	m_dwNextUpdateTick = UnsignedSafeDecrease<DWORD>(m_dwNextUpdateTick, dwTickDiff);
	if (m_dwNextUpdateTick > 0)
		return;

	ProcessTimeEvents();
	m_dwNextUpdateTick = 60000; // check every minute
}

void CEventSystemEvent::LoadEvent(CPlayer* pPlayer)
{
	UNREFERENCED_PARAMETER(pPlayer);
	// Server-scheduled events are applied when they trigger.
}

void CEventSystemEvent::Update(CMonster* pMob, CPlayer* pPlayer)
{
	if (pMob == NULL || pPlayer == NULL)
		return;

	CEventSystemTable* pTable = g_pTableContainer->GetEventSystemTable();
	if (pTable == NULL)
		return;

	EVENT_SYSTEM_MAP::iterator it = pTable->EventBegin();
	for (; it != pTable->EventEnd(); ++it)
	{
		sEVENT_SYSTEM_TBLDAT* pEvent = it->second;
		if (pEvent == NULL || pEvent->bOnOff == false)
			continue;

		if (pEvent->byType != EVENT_SYSTEM_TYPE_MOB_KILL &&
			pEvent->byType != EVENT_SYSTEM_TYPE_COUNTING)
		{
			continue;
		}

		if (false == IsServerFarmMatch(pEvent))
			continue;

		if (false == IsWorldAllowed(pEvent, pMob->GetCurWorld()))
			continue;

		if (false == IsMobMatch(pEvent, pMob->GetTblidx()))
			continue;

		if (false == IsLevelInRange(pEvent, pMob->GetLevel()))
			continue;

		switch (pEvent->byAction)
		{
			case EVENT_SYSTEM_ACTION_ITEM:
				HandleActionItem(pEvent, pMob, pPlayer);
				break;
			case EVENT_SYSTEM_ACTION_MOB:
				HandleActionMob(pEvent, pMob);
				break;
			case EVENT_SYSTEM_ACTION_BUFF:
				HandleActionBuff(pEvent, pMob, pPlayer);
				break;
			case EVENT_SYSTEM_ACTION_INVEN_INSERT:
				HandleActionInventory(pEvent, pPlayer);
				break;
			case EVENT_SYSTEM_ACTION_COUNTING:
			case EVENT_SYSTEM_ACTION_WPS:
			default:
				break;
		}
	}
}

void CEventSystemEvent::OnEventMachineUse(CPlayer* pPlayer, BYTE byExtractCount)
{
	if (pPlayer == NULL || pPlayer->IsInitialized() == false)
		return;

	if (byExtractCount == 0)
		return;

	CEventSystemTable* pTable = g_pTableContainer->GetEventSystemTable();
	if (pTable == NULL)
		return;

	for (BYTE i = 0; i < byExtractCount; ++i)
	{
		EVENT_SYSTEM_MAP::iterator it = pTable->EventBegin();
		for (; it != pTable->EventEnd(); ++it)
		{
			sEVENT_SYSTEM_TBLDAT* pEvent = it->second;
			if (pEvent == NULL || pEvent->bOnOff == false)
				continue;

			if (pEvent->byType != EVENT_SYSTEM_TYPE_EVENT_MACHINE)
				continue;

			if (false == IsServerFarmMatch(pEvent))
				continue;

			if (false == IsWorldAllowed(pEvent, pPlayer->GetCurWorld()))
				continue;

			if (false == IsLevelInRange(pEvent, pPlayer->GetLevel()))
				continue;

			if (false == IsProbabilityPassed(pEvent))
				continue;

			HandleEventMachineReward(pEvent, pPlayer);
		}
	}
}

void CEventSystemEvent::Init()
{
	m_dwNextUpdateTick = 10000;
	m_timeEvents.clear();
}

void CEventSystemEvent::Destroy()
{
	m_timeEvents.clear();
}

void CEventSystemEvent::BuildTimeSchedule()
{
	CEventSystemTable* pTable = g_pTableContainer->GetEventSystemTable();
	if (pTable == NULL)
		return;

	EVENT_SYSTEM_MAP::iterator it = pTable->EventBegin();
	for (; it != pTable->EventEnd(); ++it)
	{
		sEVENT_SYSTEM_TBLDAT* pEvent = it->second;
		if (pEvent == NULL || pEvent->bOnOff == false)
			continue;

		if (pEvent->byType != EVENT_SYSTEM_TYPE_TIME_ONE &&
			pEvent->byType != EVENT_SYSTEM_TYPE_TIME_EACH)
		{
			continue;
		}

		if (false == IsServerFarmMatch(pEvent))
			continue;

		if (pEvent->dwConnectionTime == INVALID_DWORD_VALUE)
			continue;

		sTIME_EVENT_STATE state = {};
		state.dwInterval = pEvent->dwConnectionTime * 60000; // minutes to ms
		state.dwNextTick = GetTickCount() + state.dwInterval;
		state.bRepeat = (pEvent->byType == EVENT_SYSTEM_TYPE_TIME_EACH);
		state.bDone = false;

		m_timeEvents[pEvent->tblidx] = state;
	}
}

void CEventSystemEvent::ProcessTimeEvents()
{
	if (m_timeEvents.empty())
		BuildTimeSchedule();

	if (m_timeEvents.empty())
		return;

	DWORD dwNow = GetTickCount();
	CEventSystemTable* pTable = g_pTableContainer->GetEventSystemTable();
	if (pTable == NULL)
		return;

	for (auto& itState : m_timeEvents)
	{
		sTIME_EVENT_STATE& state = itState.second;
		if (state.bDone)
			continue;
		if (dwNow < state.dwNextTick)
			continue;

		sEVENT_SYSTEM_TBLDAT* pEvent = (sEVENT_SYSTEM_TBLDAT*)pTable->FindData(itState.first);
		if (pEvent == NULL || pEvent->bOnOff == false)
			continue;

		ApplyTimeEvent(pEvent);

		if (state.bRepeat)
			state.dwNextTick = dwNow + state.dwInterval;
		else
			state.bDone = true;
	}
}

void CEventSystemEvent::ApplyTimeEvent(const sEVENT_SYSTEM_TBLDAT* pEvent)
{
	if (pEvent == NULL)
		return;

	const auto& players = g_pObjectManager->GetPlayerMap();
	for (auto it = players.begin(); it != players.end(); ++it)
	{
		CPlayer* pPlayer = it->second;
		if (pPlayer == NULL || pPlayer->IsInitialized() == false)
			continue;

		if (false == IsLevelInRange(pEvent, pPlayer->GetLevel()))
			continue;

		if (false == IsProbabilityPassed(pEvent))
			continue;

		switch (pEvent->byAction)
		{
			case EVENT_SYSTEM_ACTION_ITEM:
			case EVENT_SYSTEM_ACTION_INVEN_INSERT:
				HandleActionInventory(pEvent, pPlayer);
				break;
			case EVENT_SYSTEM_ACTION_BUFF:
				HandleActionBuff(pEvent, NULL, pPlayer);
				break;
			case EVENT_SYSTEM_ACTION_MOB:
			case EVENT_SYSTEM_ACTION_COUNTING:
			case EVENT_SYSTEM_ACTION_WPS:
			default:
				break;
		}
	}
}

void CEventSystemEvent::HandleActionItem(const sEVENT_SYSTEM_TBLDAT* pEvent, CMonster* pMob, CPlayer* pPlayer)
{
	if (pEvent == NULL || pMob == NULL || pPlayer == NULL)
		return;

	CItemDrop* pDrop = g_pItemManager->CreateSingleDrop(pEvent->fRate, pEvent->aIndex);
	if (pDrop == NULL)
		return;

	sVECTOR3 pos;
	pos.x = pMob->GetCurLoc().x + RandomRangeF(-2.0f, 2.0f);
	pos.y = pMob->GetCurLoc().y;
	pos.z = pMob->GetCurLoc().z + RandomRangeF(-2.0f, 2.0f);

	pDrop->SetOwnership(pPlayer->GetID(), pPlayer->GetPartyID());
	pDrop->StartDestroyEvent();
	pDrop->AddToGround(pMob->GetWorldID(), pos);
}

void CEventSystemEvent::HandleActionMob(const sEVENT_SYSTEM_TBLDAT* pEvent, CMonster* pMob)
{
	if (pEvent == NULL || pMob == NULL)
		return;

	if (false == Dbo_CheckProbabilityF(pEvent->fRate))
		return;

	sMOB_TBLDAT* pTbldat = (sMOB_TBLDAT*)g_pTableContainer->GetMobTable()->FindData(pEvent->aIndex);
	if (pTbldat == NULL)
	{
		ERR_LOG(LOG_SCRIPT, "EventSystem: Could not find MOB-TBLDAT. Tblidx %u", pEvent->aIndex);
		return;
	}

	sMOB_DATA data;
	InitMobData(data);

	data.worldID = pMob->GetWorldID();
	data.worldtblidx = pMob->GetWorldTblidx();
	data.tblidx = pTbldat->tblidx;

	pMob->GetCurLoc().CopyTo(data.vCurLoc);
	pMob->GetCurLoc().CopyTo(data.vSpawnLoc);
	pMob->GetCurDir().CopyTo(data.vCurDir);
	pMob->GetCurDir().CopyTo(data.vSpawnDir);

	data.actionpatternTblIdx = 1;

	CMonster* pNewMob = (CMonster*)g_pObjectManager->CreateCharacter(OBJTYPE_MOB);
	if (pNewMob && pNewMob->CreateDataAndSpawn(data, pTbldat))
		pNewMob->SetStandAlone(true);
}

void CEventSystemEvent::HandleActionBuff(const sEVENT_SYSTEM_TBLDAT* pEvent, CMonster* pMob, CPlayer* pPlayer)
{
	if (pEvent == NULL || pPlayer == NULL)
		return;

	if (false == Dbo_CheckProbabilityF(pEvent->fRate))
		return;

	sUSE_ITEM_TBLDAT* pUseItemTbldat = (sUSE_ITEM_TBLDAT*)g_pTableContainer->GetUseItemTable()->FindData(pEvent->aIndex);
	if (pUseItemTbldat == NULL)
		return;

	sBUFF_INFO buffInfo;
	buffInfo.buffIndex = INVALID_BYTE;
	buffInfo.sourceTblidx = pEvent->aIndex;
	buffInfo.dwTimeRemaining = pUseItemTbldat->dwKeepTimeInMilliSecs;
	buffInfo.dwInitialDuration = pUseItemTbldat->dwKeepTimeInMilliSecs;
	buffInfo.bySourceType = DBO_OBJECT_SOURCE_ITEM;

	eSYSTEM_EFFECT_CODE effectCode[NTL_MAX_EFFECT_IN_ITEM];
	effectCode[0] = g_pTableContainer->GetSystemEffectTable()->GetEffectCodeWithTblidx(pUseItemTbldat->aSystem_Effect[0]);
	effectCode[1] = g_pTableContainer->GetSystemEffectTable()->GetEffectCodeWithTblidx(pUseItemTbldat->aSystem_Effect[1]);

	for (int x = 0; x < NTL_MAX_EFFECT_IN_ITEM; x++)
	{
		switch (effectCode[x])
		{
			case ACTIVE_HEAL_OVER_TIME:
			case ACTIVE_EP_OVER_TIME:
				buffInfo.aBuffParameter[x].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_HOT;
				buffInfo.aBuffParameter[x].buffParameter.dwRemainTime = pUseItemTbldat->dwKeepTimeInMilliSecs;
				buffInfo.aBuffParameter[x].buffParameter.fParameter = (float)pUseItemTbldat->aSystem_Effect_Value[x];
				break;
			default:
				break;
		}
	}

	WORD wTemp;
	pPlayer->GetBuffManager()->RegisterSubBuff(&buffInfo, effectCode, pPlayer->GetID(), pUseItemTbldat->byBuff_Group, wTemp, pUseItemTbldat->abySystem_Effect_Type);
}

void CEventSystemEvent::HandleActionInventory(const sEVENT_SYSTEM_TBLDAT* pEvent, CPlayer* pPlayer)
{
	if (pEvent == NULL || pPlayer == NULL)
		return;

	if (pPlayer->GetPlayerItemContainer()->CountEmptyInventory() == 0)
		return;

	BYTE byCount = 1;
	if (pEvent->adwSetting[0] != INVALID_DWORD_VALUE && pEvent->adwSetting[0] > 0)
		byCount = (BYTE)pEvent->adwSetting[0];

	g_pItemManager->CreateItem(pPlayer, pEvent->aIndex, byCount);
}

void CEventSystemEvent::HandleEventMachineReward(const sEVENT_SYSTEM_TBLDAT* pEvent, CPlayer* pPlayer)
{
	if (pEvent == NULL || pPlayer == NULL)
		return;

	switch (pEvent->byAction)
	{
		case EVENT_SYSTEM_ACTION_ITEM:
		case EVENT_SYSTEM_ACTION_INVEN_INSERT:
			HandleActionInventory(pEvent, pPlayer);
			break;
		case EVENT_SYSTEM_ACTION_BUFF:
			HandleActionBuff(pEvent, NULL, pPlayer);
			break;
		default:
			break;
	}
}

bool CEventSystemEvent::IsServerFarmMatch(const sEVENT_SYSTEM_TBLDAT* pEvent) const
{
	if (pEvent == NULL)
		return false;

	if (pEvent->byServerFarm == 255)
		return true;

	CGameServer* app = (CGameServer*)g_pApp;
	return (pEvent->byServerFarm == app->GetGsServerId());
}

bool CEventSystemEvent::IsWorldAllowed(const sEVENT_SYSTEM_TBLDAT* pEvent, CWorld* pWorld) const
{
	if (pEvent == NULL || pWorld == NULL)
		return false;

	if (pEvent->dwContentRestrictionBitFlag == 0 ||
		pEvent->dwContentRestrictionBitFlag == INVALID_DWORD_VALUE)
	{
		return true;
	}

	BYTE byRuleType = pWorld->GetRuleType();

	// Field-only flags in table use 65534, dungeon-only use 61343 or 28575.
	if (pEvent->dwContentRestrictionBitFlag == 65534)
		return (byRuleType == GAMERULE_NORMAL);

	if (pEvent->dwContentRestrictionBitFlag == 61343 ||
		pEvent->dwContentRestrictionBitFlag == 28575)
	{
		return (byRuleType != GAMERULE_NORMAL);
	}

	return true;
}

bool CEventSystemEvent::IsMobMatch(const sEVENT_SYSTEM_TBLDAT* pEvent, TBLIDX mobTblidx) const
{
	if (pEvent == NULL)
		return false;

	if (pEvent->tIndex == INVALID_TBLIDX || pEvent->tIndex == 0)
		return true;

	return (pEvent->tIndex == mobTblidx);
}

bool CEventSystemEvent::IsLevelInRange(const sEVENT_SYSTEM_TBLDAT* pEvent, BYTE byLevel) const
{
	if (pEvent == NULL)
		return false;

	DWORD dwMin = pEvent->adwSetting[0];
	DWORD dwMax = pEvent->adwSetting[1];

	if (dwMin != INVALID_DWORD_VALUE && byLevel < dwMin)
		return false;
	if (dwMax != INVALID_DWORD_VALUE && byLevel > dwMax)
		return false;

	return true;
}

bool CEventSystemEvent::IsProbabilityPassed(const sEVENT_SYSTEM_TBLDAT* pEvent) const
{
	if (pEvent == NULL)
		return false;

	return Dbo_CheckProbabilityF(pEvent->fRate);
}
