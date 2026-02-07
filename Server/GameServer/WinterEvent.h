#ifndef __WINTER_EVENT_SYSTEM__
#define __WINTER_EVENT_SYSTEM__

#include "NtlSingleton.h"
#include "NtlSharedType.h"


class CMonster;
class CCharacter;

class CWinterEvent : public CNtlSingleton<CWinterEvent>
{

public:

	CWinterEvent();
	virtual ~CWinterEvent();

private:

	void				Init();

public:

	void				StartEvent();

	void				EndEvent();

	void				LoadEvent(HSESSION hSession);

public:

	void				TickProcess(DWORD dwTick);

	void				Update(CMonster* pMob, CCharacter* pPlayer);

private:

	bool				m_bOn;

	DBOTIME				m_timeStart;

	DBOTIME				m_timeEnd;

	DWORD				m_dwNextUpdateTick;

	int					m_nMonsterSummoned;

};

#define GetWinterEvent()			CWinterEvent::GetInstance()
#define g_pWinterEvent				GetWinterEvent()

#endif