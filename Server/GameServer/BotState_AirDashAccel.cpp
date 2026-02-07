#include "stdafx.h"
#include "BotState_AirDashAccel.h"
#include "char.h"

CBotState_AirDashAccel::CBotState_AirDashAccel(CCharacter* pParent)
	:CCharState(CHARSTATE_AIR_DASH_ACCEL, pParent)
{
}

CBotState_AirDashAccel::~CBotState_AirDashAccel()
{
}


void CBotState_AirDashAccel::OnEnter()
{
}

void CBotState_AirDashAccel::OnExit()
{
}

int CBotState_AirDashAccel::OnUpdate(DWORD dwTickDiff, float fMultiple)
{
	m_pParent->UpdateMove(dwTickDiff, fMultiple);

	return 0;
}

int CBotState_AirDashAccel::OnObjectMsg(CObjectMsg * pObjectMsg)
{
	return 0;
}

bool CBotState_AirDashAccel::CanTransition(BYTE stateID)
{
	if (stateID == CHARSTATE_GUARD || stateID == CHARSTATE_DASH_PASSIVE)
		return false;

	return true;
}

int CBotState_AirDashAccel::CopyTo(sCHARSTATE_DETAIL* pStateDetail)
{
	pStateDetail->sCharStateAirAccel.byMoveDirection = m_pParent->GetMoveDirection();
	return sizeof(sCHARSTATE_AIRACCEL);
}

void CBotState_AirDashAccel::CopyFrom(sCHARSTATE_DETAIL* pStateDetail)
{
	m_pParent->SetMoveDirection(pStateDetail->sCharStateAirAccel.byMoveDirection);
	m_pParent->SetMoveFlag(NTL_MOVE_FLAG_FLY_ACCEL);
}

