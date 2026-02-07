#include "stdafx.h"
#include "BotState_SkillAffecting.h"
#include "ObjectManager.h"
#include "char.h"
#include "TableContainerManager.h"
#include "SystemEffectTable.h"
#include "NtlPacketGU.h"
#include "NtlMovement.h"
#include "StateManager.h"


CBotState_SkillAffecting::CBotState_SkillAffecting(CCharacter* pParent)
	:CCharState(CHARSTATE_SKILL_AFFECTING, pParent)
{
	m_hTarget = INVALID_HOBJECT;
	m_skillId = INVALID_TBLIDX;
	m_dwTime = 0;
	m_dwAffectingTimeRemaining = INVALID_DWORD;
}

CBotState_SkillAffecting::~CBotState_SkillAffecting()
{
}


void CBotState_SkillAffecting::OnEnter()
{
}

void CBotState_SkillAffecting::OnExit()
{
	m_pParent->GetSkillManager()->CancelCasting(); //do this or monster keep freezing forever

	if (m_pSkill)
	{
		//printf("m_dwTime: %u, m_pSkill->GetAffectingTimeRemaining(); %u \n", m_dwTime, m_pSkill->GetAffectingTimeRemaining());

		if (m_pSkill->GetAffectingTimeRemaining() > 0)
			m_pSkill->OnAffectedCanceled();
	}

	//NTL_PRINT(PRINT_APP, "LEAVE SKILL AFFECTING STATE");

	m_hTarget = INVALID_HOBJECT;
	m_skillId = INVALID_TBLIDX;
	m_dwTime = INVALID_DWORD;
	m_pSkill = NULL;
}

int CBotState_SkillAffecting::OnUpdate(DWORD dwTickDiff, float fMultiple)
{
	m_pParent->UpdateMove(dwTickDiff, fMultiple);

	m_dwTime = UnsignedSafeDecrease<DWORD>(m_dwTime, dwTickDiff);

	if (m_dwTime == 0) //in OnAffected player could go another state
	{
	//	if(m_pParent->IsPC())
	//		NTL_PRINT(PRINT_APP, "%u go standing | %f %f %f | %u %u %u", m_pParent->GetID(), m_pParent->GetCurLoc().x, m_pParent->GetCurLoc().y, m_pParent->GetCurLoc().z, m_pParent->GetMoveDirection(), m_pParent->GetMoveFlag(), m_pParent->GetMoveStatus());
		if (m_pParent->GetCharStateID() == CHARSTATE_SKILL_AFFECTING)
		{
			if (m_pSkill)
				m_pSkill->OnAffectingEnd();

			if (m_pParent->GetStateManager()->IsCharCondition(CHARCOND_NULLIFIED_DAMAGE) == true)
				m_pParent->GetStateManager()->RemoveConditionState(CHARCOND_NULLIFIED_DAMAGE, NULL, false); // if player somehow die while skill usage.. Just to make 100% sure, no immortal bug going to happen

			m_pParent->SendCharStateStanding();
		}

	}

	return 0;
}

int CBotState_SkillAffecting::OnObjectMsg(CObjectMsg * pObjectMsg)
{
	return 0;
}

bool CBotState_SkillAffecting::CanTransition(BYTE stateID)
{
	if (m_pParent->IsPC())
	{
		if (stateID == CHARSTATE_SITTING
			|| stateID == CHARSTATE_CAMPING
			|| stateID == CHARSTATE_MOVING
			|| stateID == CHARSTATE_DESTMOVE
			|| stateID == CHARSTATE_FOLLOWING
			|| stateID == CHARSTATE_DASH_PASSIVE
			|| stateID == CHARSTATE_FOCUSING
			|| stateID == CHARSTATE_CASTING
			|| stateID == CHARSTATE_CASTING_ITEM
			|| stateID == CHARSTATE_PRIVATESHOP
			|| stateID == CHARSTATE_RIDEON
			|| stateID == CHARSTATE_AIR_JUMP
			|| stateID == CHARSTATE_AIR_DASH_ACCEL
			)
			return false;
	}

	return true;
}

int CBotState_SkillAffecting::CopyTo(sCHARSTATE_DETAIL* pStateDetail)
{
	pStateDetail->sCharStateSkillAffecting.hTarget = m_hTarget;
	pStateDetail->sCharStateSkillAffecting.skillId = m_skillId;

	return sizeof(sCHARSTATE_SKILL_AFFECTING);
}

void CBotState_SkillAffecting::CopyFrom(sCHARSTATE_DETAIL* pStateDetail)
{
	m_hTarget = pStateDetail->sCharStateSkillAffecting.hTarget;
	m_skillId = pStateDetail->sCharStateSkillAffecting.skillId;

	m_pSkill = m_pParent->GetSkillManager()->FindSkill(m_skillId);
	if (m_pSkill)
	{
		sCHAR_DATA_INFO* pCharInfo = m_pParent->GetAniTbldat()->GetAnim(0, m_pSkill->GetOriginalTableData()->wAction_Animation_Index);
		if (pCharInfo == NULL)
		{
			ERR_LOG(LOG_SYSTEM, "Animation: %u could not be found", m_pSkill->GetOriginalTableData()->wAction_Animation_Index);
			m_dwAffectingTimeRemaining = 1000;
			m_dwTime = 1000;
			m_pSkill->SetAffectingTimeRemaining(m_dwAffectingTimeRemaining);
			return;
		}

		DWORD dwFirstHitTick = DWORD(pCharInfo->aHitTime[0] * 1000.f);
		DWORD dwLastHitTick = (pCharInfo->byHitCount > 1) ? DWORD(pCharInfo->aHitTime[pCharInfo->byHitCount - 1] * 1000.f) : dwFirstHitTick;

		DWORD dwSkillCancelTime = m_pParent->GetAniTbldat()->GetSkillAnimationSkillCancelTime(m_pSkill->GetOriginalTableData()->wAction_Animation_Index);

		m_dwAffectingTimeRemaining = 0;

		if (dwSkillCancelTime > 0)
		{
			m_dwTime = DWORD((float)dwSkillCancelTime * (100.f / m_pParent->GetCharAtt()->GetLastSkillAnimationSpeedModifier()));
			m_dwAffectingTimeRemaining = DWORD((float)dwFirstHitTick * (100.f / m_pParent->GetCharAtt()->GetLastSkillAnimationSpeedModifier()));
		}
		else
			m_dwTime = DWORD((float)dwLastHitTick * (100.f / m_pParent->GetCharAtt()->GetLastSkillAnimationSpeedModifier()));

		//printf("m_dwTime %u, m_dwAffectingTimeRemaining %u, dwFirstHitTick %u, dwLastHitTick %u, dwSkillCancelTime %u \n", 
		//	m_dwTime, m_dwAffectingTimeRemaining, dwFirstHitTick, dwLastHitTick, dwSkillCancelTime);

		if (BIT_FLAG_TEST(m_pSkill->GetOriginalTableData()->dwFunction_Bit_Flag, SKILL_FUNCTION_FLAG_IGNORE_AFFECT_TIME))
		{
			m_dwAffectingTimeRemaining = 0;
		}
		else if (m_pSkill->GetEffectCode(0) == ACTIVE_WARP_STUN || m_pSkill->GetEffectCode(1) == ACTIVE_WARP_STUN)
		{
			m_dwAffectingTimeRemaining = DWORD((float)dwLastHitTick * (100.f / m_pParent->GetCharAtt()->GetLastSkillAnimationSpeedModifier()));
		}
		else if (Dbo_IsSystemEffectForBodyCurse(m_pSkill->GetEffectCode(0)) || Dbo_IsSystemEffectForBodyCurse(m_pSkill->GetEffectCode(1))) //if it is a skill like kidney shot which stun target, then it is applied right after usage.
		{
			m_dwAffectingTimeRemaining = 0;
		}
			
		if (m_dwAffectingTimeRemaining > m_dwTime)
			m_dwTime = m_dwAffectingTimeRemaining;

		//NTL_PRINT(PRINT_APP,"m_dwTime %u, m_dwAffectingTimeRemaining %u byHitCount %u, aHitTime %f, dwSkillCancelTime %u, dwFirstHitTick %u, dwLastHitTick %u, byKnockDownCount %u, fDurationTime %f", 
		//m_dwTime, m_dwAffectingTimeRemaining, pCharInfo->byHitCount, pCharInfo->aHitTime[pCharInfo->byHitCount - 1], dwSkillCancelTime, dwFirstHitTick, dwLastHitTick, pCharInfo->byKnockDownCount, pCharInfo->fDurationTime);

		m_pSkill->SetAffectingTimeRemaining(m_dwAffectingTimeRemaining);

		if (m_dwAffectingTimeRemaining == 0)
		{
			m_pSkill->OnAffected();
		}
	}
}

