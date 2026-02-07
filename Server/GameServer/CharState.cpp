#include "stdafx.h"
#include "CharState.h"


CCharState::CCharState(BYTE stateID, CCharacter* pParent)
	:CStateBase(stateID)
{
	m_pParent = pParent;
}

CCharState::~CCharState()
{

}
