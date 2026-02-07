#include "stdafx.h"
#include "BotState_Operating.h"


CBotState_Operating::CBotState_Operating(CCharacter* pParent)
	:CCharState(CHARSTATE_OPERATING, pParent)
{
}

CBotState_Operating::~CBotState_Operating()
{
}


void CBotState_Operating::OnEnter()
{
}

void CBotState_Operating::OnExit()
{
}

int CBotState_Operating::OnUpdate(DWORD dwTickDiff, float fMultiple)
{
	return NULL;
}

int CBotState_Operating::OnObjectMsg(CObjectMsg * pObjectMsg)
{
	return NULL;
}

bool CBotState_Operating::CanTransition(BYTE stateID)
{
	if (stateID == CHARSTATE_GUARD || stateID == CHARSTATE_RIDEON)
		return false;

	return true;
}

int CBotState_Operating::CopyTo(sCHARSTATE_DETAIL* pStateDetail)
{
	pStateDetail->sCharStateOperating.hTargetObject = hTargetObject;
	pStateDetail->sCharStateOperating.dwOperateTime = dwOperateTime;
	pStateDetail->sCharStateOperating.directTblidx = directTblidx;
	return sizeof(sCHARSTATE_OPERATING);
}

void CBotState_Operating::CopyFrom(sCHARSTATE_DETAIL* pStateDetail)
{
	hTargetObject = pStateDetail->sCharStateOperating.hTargetObject;
	dwOperateTime = pStateDetail->sCharStateOperating.dwOperateTime;
	directTblidx = pStateDetail->sCharStateOperating.directTblidx;
}

