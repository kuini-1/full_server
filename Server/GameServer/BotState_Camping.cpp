#include "stdafx.h"
#include "BotState_Camping.h"

CBotState_Camping::CBotState_Camping(CCharacter* pParent)
	:CCharState(CHARSTATE_CAMPING, pParent)
{
}

CBotState_Camping::~CBotState_Camping()
{
}


void CBotState_Camping::OnEnter()
{
}

void CBotState_Camping::OnExit()
{
}

int CBotState_Camping::OnUpdate(DWORD dwTickDiff, float fMultiple)
{
	return NULL;
}

int CBotState_Camping::OnObjectMsg(CObjectMsg * pObjectMsg)
{
	return NULL;
}


bool CBotState_Camping::CanTransition(BYTE stateID)
{
	if (stateID == CHARSTATE_GUARD)
		return false;

	return true;
}

int CBotState_Camping::CopyTo(sCHARSTATE_DETAIL* pStateDetail)
{
	return NULL;
}

void CBotState_Camping::CopyFrom(sCHARSTATE_DETAIL* pStateDetail)
{
}

