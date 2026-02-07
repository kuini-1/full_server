#include "stdafx.h"
#include "BotState_KeepingEffect.h"
#include "char.h"


CBotState_KeepingEffect::CBotState_KeepingEffect(CCharacter* pParent)
	:CCharState(CHARSTATE_KEEPING_EFFECT, pParent)
{
}

CBotState_KeepingEffect::~CBotState_KeepingEffect()
{
}


void CBotState_KeepingEffect::OnEnter()
{
}

void CBotState_KeepingEffect::OnExit()
{
	m_pParent->RemoveKeepEffectTargets();

	if(m_pParent->IsPC() == false) //only apply on PC. Otherwise if pc is in this state and use skill which has casting, then he stays forever in casting
		m_pParent->GetSkillManager()->CancelCasting(); //if this not here then mobs wont use any skills anymore after using a skill which set them into keeping effect
}

int CBotState_KeepingEffect::OnUpdate(DWORD dwTickDiff, float fMultiple)
{
	if (m_pParent->HasKeepEffectTargets() == false)
	{
		m_pParent->SendCharStateStanding();
	}

	return 0;
}

int CBotState_KeepingEffect::OnObjectMsg(CObjectMsg * pObjectMsg)
{
	return 0;
}

bool CBotState_KeepingEffect::CanTransition(BYTE stateID)
{
	if (stateID == CHARSTATE_GUARD || stateID == CHARSTATE_RIDEON)
		return false;

	return true;
}

int CBotState_KeepingEffect::CopyTo(sCHARSTATE_DETAIL* pStateDetail)
{
	return 0;
}

void CBotState_KeepingEffect::CopyFrom(sCHARSTATE_DETAIL* pStateDetail)
{
}

