#include "stdafx.h"
#include "BotState_Follow.h"
#include "char.h"
#include "ObjectManager.h"


CBotState_Follow::CBotState_Follow(CCharacter* pParent)
	:CCharState(CHARSTATE_FOLLOWING, pParent)
{
}

CBotState_Follow::~CBotState_Follow()
{
}


void CBotState_Follow::OnEnter()
{
}

void CBotState_Follow::OnExit()
{
}

int CBotState_Follow::OnUpdate(DWORD dwTickDiff, float fMultiple)
{
	/*
	  ((void (__fastcall *)(CBot *, _QWORD))v7->m_pParent->vfptr[4].__vecDelDtor)(v7->m_pParent, dwTickTime);
	  ((void (__fastcall *)(CBot *, _QWORD))v7->m_pParent->vfptr[3].SendPacket)(v7->m_pParent, v8);
	 */

	if (m_pParent->GetAspectStateId() == ASPECTSTATE_VEHICLE)
		return 0;

	if (!m_pParent->UpdateFollow(dwTickDiff, fMultiple) && m_pParent->IsPC() == false) //dont do for pc because client will decide (which sucks)
	{
		SetNextStateID(CHARSTATE_STANDING);
		SetFinish(true);
	}


	return 0;
}

int CBotState_Follow::OnObjectMsg(CObjectMsg * pObjectMsg)
{
	return 0;
}

bool CBotState_Follow::CanTransition(BYTE stateID)
{
	return true;
}

int CBotState_Follow::CopyTo(sCHARSTATE_DETAIL* pStateDetail)
{
	pStateDetail->sCharStateFollwing.dwTimeStamp = GetStateTime();
	pStateDetail->sCharStateFollwing.byMoveFlag = m_pParent->GetMoveFlag();
	pStateDetail->sCharStateFollwing.hTarget = m_pParent->GetTargetHandle();
	pStateDetail->sCharStateFollwing.fDistance = m_pParent->GetFollowDistance();
	pStateDetail->sCharStateFollwing.byMoveStatus = m_pParent->GetMoveStatus();
	m_pParent->GetDestLoc().CopyTo(pStateDetail->sCharStateFollwing.vDestLoc);
	pStateDetail->sCharStateFollwing.byMovementReason = m_pParent->GetFollowMoveReason();

	return sizeof(sCHARSTATE_FOLLOWING);
}

void CBotState_Follow::CopyFrom(sCHARSTATE_DETAIL* pStateDetail)
{
	m_pParent->SetMoveFlag(pStateDetail->sCharStateFollwing.byMoveFlag);

	m_pParent->ChangeTarget(pStateDetail->sCharStateFollwing.hTarget);

	m_pParent->SetFollowDistance(pStateDetail->sCharStateFollwing.fDistance);
	m_pParent->SetMoveStatus(pStateDetail->sCharStateFollwing.byMoveStatus);
	m_pParent->SetMoveDirection(NTL_MOVE_FOLLOW_MOVEMENT);
	m_pParent->SetDestLoc(pStateDetail->sCharStateFollwing.vDestLoc);
	m_pParent->SetFollowMoveReason(pStateDetail->sCharStateFollwing.byMovementReason);
}

