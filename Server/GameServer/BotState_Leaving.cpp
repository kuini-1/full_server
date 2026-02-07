#include "stdafx.h"
#include "BotState_Leaving.h"

CBotState_Leaving::CBotState_Leaving(CCharacter* pParent)
	:CCharState(CHARSTATE_LEAVING, pParent)
{
	byLeavingType = INVALID_BYTE;
}

CBotState_Leaving::~CBotState_Leaving()
{
}


void CBotState_Leaving::OnEnter()
{
}

void CBotState_Leaving::OnExit()
{
}

int CBotState_Leaving::OnUpdate(DWORD dwTickDiff, float fMultiple)
{
	return NULL;
}

int CBotState_Leaving::OnObjectMsg(CObjectMsg * pObjectMsg)
{
	return NULL;
}

bool CBotState_Leaving::CanTransition(BYTE stateID)
{
	return true;
}

int CBotState_Leaving::CopyTo(sCHARSTATE_DETAIL* pStateDetail)
{
	pStateDetail->sCharStateLeaving.byLeavingType = byLeavingType;

	return sizeof(sCHARSTATE_LEAVING);
}

void CBotState_Leaving::CopyFrom(sCHARSTATE_DETAIL* pStateDetail)
{
	byLeavingType = pStateDetail->sCharStateLeaving.byLeavingType;
}

