#ifndef __EVENT_SYSTEM_EVENT__
#define __EVENT_SYSTEM_EVENT__

#include <unordered_map>

#include "NtlSingleton.h"
#include "NtlSharedType.h"

class CMonster;
class CPlayer;
class CWorld;
struct sEVENT_SYSTEM_TBLDAT;

class CEventSystemEvent : public CNtlSingleton<CEventSystemEvent>
{
public:
	CEventSystemEvent();
	virtual ~CEventSystemEvent();

public:
	void Create();
	void TickProcess(DWORD dwTickDiff);
	void LoadEvent(CPlayer* pPlayer);
	void Update(CMonster* pMob, CPlayer* pPlayer);
	void OnEventMachineUse(CPlayer* pPlayer, BYTE byExtractCount);

private:
	struct sTIME_EVENT_STATE
	{
		DWORD dwNextTick;
		DWORD dwInterval;
		bool bRepeat;
		bool bDone;
	};

private:
	void Init();
	void Destroy();
	void BuildTimeSchedule();
	void ProcessTimeEvents();
	void ApplyTimeEvent(const struct sEVENT_SYSTEM_TBLDAT* pEvent);
	void HandleActionItem(const struct sEVENT_SYSTEM_TBLDAT* pEvent, CMonster* pMob, CPlayer* pPlayer);
	void HandleActionMob(const struct sEVENT_SYSTEM_TBLDAT* pEvent, CMonster* pMob);
	void HandleActionBuff(const struct sEVENT_SYSTEM_TBLDAT* pEvent, CMonster* pMob, CPlayer* pPlayer);
	void HandleActionInventory(const struct sEVENT_SYSTEM_TBLDAT* pEvent, CPlayer* pPlayer);
	void HandleEventMachineReward(const struct sEVENT_SYSTEM_TBLDAT* pEvent, CPlayer* pPlayer);
	bool IsServerFarmMatch(const struct sEVENT_SYSTEM_TBLDAT* pEvent) const;
	bool IsWorldAllowed(const struct sEVENT_SYSTEM_TBLDAT* pEvent, CWorld* pWorld) const;
	bool IsMobMatch(const struct sEVENT_SYSTEM_TBLDAT* pEvent, TBLIDX mobTblidx) const;
	bool IsLevelInRange(const struct sEVENT_SYSTEM_TBLDAT* pEvent, BYTE byLevel) const;
	bool IsProbabilityPassed(const struct sEVENT_SYSTEM_TBLDAT* pEvent) const;

private:
	static const DWORD INVALID_DWORD_VALUE = 0xFFFFFFFF;

	DWORD m_dwNextUpdateTick;
	std::unordered_map<TBLIDX, sTIME_EVENT_STATE> m_timeEvents;
};

#define GetEventSystemEvent()			CEventSystemEvent::GetInstance()
#define g_pEventSystemEvent				GetEventSystemEvent()

#endif
