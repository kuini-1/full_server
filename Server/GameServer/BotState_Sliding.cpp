#include "stdafx.h"
#include "BotState_Sliding.h"
#include "char.h"
#include "GameServer.h"
#include "GameMain.h"


CBotState_Sliding::CBotState_Sliding(CCharacter* pParent)
	:CCharState(CHARSTATE_SLIDING, pParent)
{
}

CBotState_Sliding::~CBotState_Sliding()
{
}


void CBotState_Sliding::OnEnter()
{
	m_pParent->InitKnockDown();

	if (m_pParent->GetBotController())
	{
		m_pParent->GetBotController()->SetControlBlock(true);
	}

	if (GetPrevStateID() == CHARSTATE_SLEEPING)
		m_pParent->GetBuffManager()->EndBuff(ACTIVE_SLEEP);
}

void CBotState_Sliding::OnExit()
{
	if (m_pParent->GetBotController())
	{
		m_pParent->GetBotController()->SetControlBlock(false);
	}
}

int CBotState_Sliding::OnUpdate(DWORD dwTickDiff, float fMultiple)
{
	m_pParent->IncKnockDownTime(dwTickDiff);

	if (m_pParent->GetKnockDownTime() >= NTL_BATTLE_SLIDING_END_TIME)
	{
		if (m_pParent->GetBuffManager()->CheckAndApplyOtherStun(CHARSTATE_SLIDING, INVALID_WORD) == false) //check if any previous stuns exist. If yes, then apply them and return true. Otherwise return false.
			m_pParent->SendCharStateStanding();
	}

	return 0;
}

int CBotState_Sliding::OnObjectMsg(CObjectMsg * pObjectMsg)
{
	return 0;
}

bool CBotState_Sliding::CanTransition(BYTE stateID)
{
	if (m_pParent->IsPC())
	{
		if (stateID == CHARSTATE_LEAVING || stateID == CHARSTATE_STANDING || stateID == CHARSTATE_DESPAWNING || stateID == CHARSTATE_TELEPORTING || stateID == CHARSTATE_FAINTING || stateID == CHARSTATE_PARALYZED || stateID == CHARSTATE_STUNNED)
			return true;
	}
	else
		return true;

	return false;
}

int CBotState_Sliding::CopyTo(sCHARSTATE_DETAIL* pStateDetail)
{
	return NULL;
}

void CBotState_Sliding::CopyFrom(sCHARSTATE_DETAIL* pStateDetail)
{
	if (m_pParent->GetObjType() >= OBJTYPE_PC)
	{
		if (m_pParent->GetCurWorld())
		{
			CGameServer* app = (CGameServer*)g_pApp;

			CNtlVector rvShift(pStateDetail->sCharStateSliding.vShift);
			CNtlVector rvDestLoc;
			CNtlVector rvNewDir;

			if (app->GetGameMain()->GetWorldManager()->GetDestLocAfterCollision(m_pParent->GetCurWorld(), m_pParent, rvShift, rvDestLoc, rvNewDir))
			{
				m_pParent->SetCurLoc(rvDestLoc, m_pParent->GetCurWorld());
				m_pParent->SetCurDir(rvNewDir);
			}
			/*else
			{
			ERR_LOG(LOG_USER,"GetDestLocAfterCollision() failed., pWorld->GetID() = %u, m_pParent->GetCurLoc() = (%f, %f, %f)");
			}*/
		}
		else ERR_LOG(LOG_USER, "The bot doesn't belong to any world.(NULL == pWorld)., m_pParent->GetID() = %u", m_pParent->GetID());
	}
}

