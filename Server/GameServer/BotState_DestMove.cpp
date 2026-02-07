#include "stdafx.h"
#include "BotState_DestMove.h"
#include "char.h"


CBotState_DestMove::CBotState_DestMove(CCharacter* pParent)
	:CCharState(CHARSTATE_DESTMOVE, pParent)
{
}

CBotState_DestMove::~CBotState_DestMove()
{
}


void CBotState_DestMove::OnEnter()
{
	m_pParent->SetMoveDirection(NTL_MOVE_MOUSE_MOVEMENT);
}

void CBotState_DestMove::OnExit()
{
	m_pParent->SetMoveDirection(NTL_MOVE_NONE);
}

int CBotState_DestMove::OnUpdate(DWORD dwTickDiff, float fMultiple)
{
	//printf("CBotState_DestMove::OnUpdate TO DO \n");
	m_pParent->UpdateMove(dwTickDiff, fMultiple);

	if (m_pParent->GetAspectStateId() == ASPECTSTATE_VEHICLE)
		return 0;

	//((void(__fastcall *)(CBot *, _QWORD))v8->m_pParent->vfptr[4].__vecDelDtor)(v8->m_pParent, v9);
	//((void(__fastcall *)(CBot *, _QWORD))v8->m_pParent->vfptr[3].SendPacket)(v8->m_pParent, v9);


	if (!m_pParent->IsMoveComplete())
		return 0;

	if (m_pParent->GetHaveSecondDestLoc())
		return 0;

	if (m_pParent->GetStateManager()->ChangeCharState(CHARSTATE_STANDING, NULL, true))
		return 2;

	return 0;
}

int CBotState_DestMove::OnObjectMsg(CObjectMsg * pObjectMsg)
{
	return 0;
}

bool CBotState_DestMove::CanTransition(BYTE stateID)
{
	return true;
}

int CBotState_DestMove::CopyTo(sCHARSTATE_DETAIL* pStateDetail)
{
	pStateDetail->sCharStateDestMove.actionPatternIndex = BOTAP_MOVE;
	m_pParent->GetDestLoc().CopyTo(pStateDetail->sCharStateDestMove.avDestLoc[0]);
	pStateDetail->sCharStateDestMove.bHaveSecondDestLoc = m_pParent->GetHaveSecondDestLoc();
	pStateDetail->sCharStateDestMove.byDestLocCount = m_pParent->GetDestLocCount();
	pStateDetail->sCharStateDestMove.byMoveFlag = m_pParent->GetMoveFlag();
	m_pParent->GetSecondDestLoc().CopyTo(pStateDetail->sCharStateDestMove.vSecondDestLoc);
	return sizeof(sCHARSTATE_DESTMOVE);
}

void CBotState_DestMove::CopyFrom(sCHARSTATE_DETAIL* pStateDetail)
{
	m_pParent->SetMoveDirection(NTL_MOVE_MOUSE_MOVEMENT);
	m_pParent->SetMoveFlag(pStateDetail->sCharStateDestMove.byMoveFlag);
	m_pParent->SetHaveSecondDestLoc(pStateDetail->sCharStateDestMove.bHaveSecondDestLoc);
	m_pParent->SetSecondDestLoc(pStateDetail->sCharStateDestMove.vSecondDestLoc);
	m_pParent->SetMultipleDestLoc(pStateDetail->sCharStateDestMove.byDestLocCount, pStateDetail->sCharStateDestMove.avDestLoc);
}

