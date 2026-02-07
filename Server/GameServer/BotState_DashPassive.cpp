#include "stdafx.h"
#include "BotState_DashPassive.h"
#include "char.h"

CBotState_DashPassive::CBotState_DashPassive(CCharacter* pParent)
	:CCharState(CHARSTATE_DASH_PASSIVE, pParent)
{
}

CBotState_DashPassive::~CBotState_DashPassive()
{
}


void CBotState_DashPassive::OnEnter()
{
}

void CBotState_DashPassive::OnExit()
{
}

int CBotState_DashPassive::OnUpdate(DWORD dwTickDiff, float fMultiple)
{
	m_pParent->UpdateMove(dwTickDiff, fMultiple);

	return 0;
}

int CBotState_DashPassive::OnObjectMsg(CObjectMsg * pObjectMsg)
{
	return 0;
}

bool CBotState_DashPassive::CanTransition(BYTE stateID)
{
	if (stateID == CHARSTATE_GUARD || stateID == CHARSTATE_DASH_PASSIVE || stateID == CHARSTATE_RIDEON)
		return false;

	return true;
}

int CBotState_DashPassive::CopyTo(sCHARSTATE_DETAIL* pStateDetail)
{
	pStateDetail->sCharStateDashPassive.byMoveDirection = m_pParent->GetMoveDirection();
	pStateDetail->sCharStateDashPassive.byMoveFlag = m_pParent->GetMoveFlag();
	m_pParent->GetDestLoc().CopyTo(pStateDetail->sCharStateDashPassive.vDestLoc);

	return sizeof(sCHARSTATE_DASH_PASSIVE);
}

void CBotState_DashPassive::CopyFrom(sCHARSTATE_DETAIL* pStateDetail)
{
	m_pParent->SetMoveDirection(pStateDetail->sCharStateDashPassive.byMoveDirection);
	m_pParent->SetMoveFlag(pStateDetail->sCharStateDashPassive.byMoveFlag);
	m_pParent->SetDestLoc(pStateDetail->sCharStateDashPassive.vDestLoc);
	m_pParent->SetMoveStatus(NTL_MOVE_STATUS_DASH);
}

