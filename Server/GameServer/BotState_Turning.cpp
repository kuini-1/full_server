#include "stdafx.h"
#include "BotState_Turning.h"
#include "char.h"


CBotState_Turning::CBotState_Turning(CCharacter* pParent)
	:CCharState(CHARSTATE_TURNING, pParent)
{
}

CBotState_Turning::~CBotState_Turning()
{
}


void CBotState_Turning::OnEnter()
{
}

void CBotState_Turning::OnExit()
{
	CObjMsg_CharTurningEnd pMsg;
	pMsg.directTblidx = m_directTblidx;
	m_pParent->SendObjectMsg(&pMsg);
}

int CBotState_Turning::OnUpdate(DWORD dwTickDiff, float fMultiple)
{
	return NULL;
}

int CBotState_Turning::OnObjectMsg(CObjectMsg * pObjectMsg)
{
	return NULL;
}

bool CBotState_Turning::CanTransition(BYTE stateID)
{
	return true;
}

int CBotState_Turning::CopyTo(sCHARSTATE_DETAIL* pStateDetail)
{
	pStateDetail->sCharStateTurning.directTblidx = m_directTblidx;
	pStateDetail->sCharStateTurning.vDestDir = m_vDestDir;

	return sizeof(sCHARSTATE_TURNING);
}

void CBotState_Turning::CopyFrom(sCHARSTATE_DETAIL* pStateDetail)
{
	m_directTblidx = pStateDetail->sCharStateTurning.directTblidx;
	m_vDestDir = pStateDetail->sCharStateTurning.vDestDir;
}

