#include "stdafx.h"
#include "CharacterAttBot.h"
#include "Monster.h"
#include "GameServer.h"
#include "FormulaTable.h"
#include "calcs.h"
#include "World.h"


CCharacterAttBot::CCharacterAttBot()
{
}

CCharacterAttBot::~CCharacterAttBot()
{
}


bool CCharacterAttBot::Create(CNpc* pBot)
{
	if (CCharacterAtt::Create(pBot))
	{
		m_pBotRef = pBot;
		return true;
	}

	return false;
}


void CCharacterAttBot::Reset()
{
	CCharacterAtt::Reset();

	sBOT_TBLDAT* pBotTbldat = m_pBotRef->GetTbldat();

	m_pAttribute.fBaseRunSpeed = pBotTbldat->fWalk_Speed; //walk speed
	m_pAttribute.fLastRunSpeed = pBotTbldat->fRun_Speed; //run speed
}


void CCharacterAttBot::CalculateBaseAtt()
{
	sCHAR_TBLDAT* charTbldat = m_pBotRef->GetTbldat();
	sBOT_TBLDAT* pBotTbldat = m_pBotRef->GetTbldat();

	BYTE byGrade = pBotTbldat->byGrade;
	BYTE byLv = pBotTbldat->byLevel;
	BYTE byEfLv = m_pBotRef->GetEffectiveLevel();

	/*sMOB_TBLDAT* pMobTbldat = NULL;
	if (m_pBotRef->IsMonster())
	{
		pMobTbldat = (sMOB_TBLDAT*)m_pBotRef->GetTbldat();
	}*/

	m_pAttribute.baseMaxLp = (int)charTbldat->dwBasic_LP;
	m_pAttribute.baseMaxLp += (byEfLv - byLv) * ((m_pAttribute.baseMaxLp / byLv) * (byGrade + 1));

	m_pAttribute.wBaseMaxEP = charTbldat->wBasic_EP;
	m_pAttribute.wBaseMaxEP += (byEfLv - byLv) * ((m_pAttribute.wBaseMaxEP / byLv) * (byGrade + 1));

	m_pAttribute.wBasePhysicalDefence = charTbldat->wBasic_Physical_Defence;
	m_pAttribute.wBasePhysicalDefence += (byEfLv - byLv) * ((m_pAttribute.wBasePhysicalDefence / byLv) * (byGrade + 1));

	m_pAttribute.wBaseEnergyDefence = charTbldat->wBasic_Energy_Defence;
	m_pAttribute.wBaseEnergyDefence += (byEfLv - byLv) * ((m_pAttribute.wBaseEnergyDefence / byLv) * (byGrade + 1));

	m_pAttribute.wBasePhysicalOffence = pBotTbldat->wBasic_Physical_Offence;
	m_pAttribute.wBasePhysicalOffence += (byEfLv - byLv) * ((m_pAttribute.wBasePhysicalOffence / byLv) * (byGrade + 1));

	m_pAttribute.wBaseEnergyOffence = pBotTbldat->wBasic_Energy_Offence;
	m_pAttribute.wBaseEnergyOffence += (byEfLv - byLv) * ((m_pAttribute.wBaseEnergyOffence / byLv) * (byGrade + 1));

	m_pAttribute.baseStr = charTbldat->wBasicStr;
	m_pAttribute.baseCon = charTbldat->wBasicCon;
	m_pAttribute.baseFoc = charTbldat->wBasicFoc;
	m_pAttribute.baseDex = charTbldat->wBasicDex;
	m_pAttribute.baseSol = charTbldat->wBasicSol;
	m_pAttribute.baseEng = charTbldat->wBasicEng;
	m_pAttribute.wBaseAttackSpeedRate = charTbldat->wAttack_Speed_Rate;
	m_pAttribute.fBaseAttackRange = charTbldat->fAttack_Range;

	//m_pAttribute.wBaseBlockRate = charTbldat->wBlock_Rate;
	//m_pAttribute.wBaseCurseSuccessRate = charTbldat->wCurse_Success_Rate;
	//m_pAttribute.wBaseCurseToleranceRate = charTbldat->wCurse_Tolerance_Rate;

	m_pAttribute.wBaseEpRegen = pBotTbldat->wEP_Regeneration;
	m_pAttribute.wBaseEpSitdownRegen = m_pAttribute.wBaseMaxEP * 20 / 100;
	m_pAttribute.wBaseEpBattleRegen = pBotTbldat->wEP_Regeneration;
	m_abyBattle_Attribute[0] = pBotTbldat->byBattle_Attribute;
	m_abyBattle_Attribute[1] = pBotTbldat->byBattle_Attribute;

	m_pAttribute.fBaseRunSpeed = pBotTbldat->fWalk_Speed;
	m_pAttribute.fBaseFlySpeed = pBotTbldat->fWalk_Speed;
	m_pAttribute.fBaseFlyDashSpeed = pBotTbldat->fWalk_Speed;
	m_pAttribute.fBaseFlyAccelSpeed = pBotTbldat->fWalk_Speed;

	m_pAttribute.wStomachacheDefenceBase = pBotTbldat->wStomachacheDefence;
	m_pAttribute.wPoisonDefenceBase = pBotTbldat->wPoisonDefence;
	m_pAttribute.wBleedDefenceBase = pBotTbldat->wBleedDefence;
	m_pAttribute.wBurnDefenceBase = pBotTbldat->wBurnDefence;

	m_pAttribute.fBaseSkillAnimationSpeedModifier = 100.0f;

	m_pAttribute.wBaseAttackRate = WORD(m_pAttribute.baseFoc / CFormulaTable::m_afRate[600][1] * CFormulaTable::m_afRate[600][2]);
	m_pAttribute.wBaseDodgeRate = WORD(m_pAttribute.baseDex / CFormulaTable::m_afRate[800][1] * CFormulaTable::m_afRate[800][2]);

	m_pAttribute.wBaseBlockRate = WORD(CFormulaTable::m_afRate[919][1] + m_pAttribute.baseDex * CFormulaTable::m_afRate[919][2] + m_pAttribute.baseCon * CFormulaTable::m_afRate[919][2]);
	m_pAttribute.wGuardRateBase = WORD(CFormulaTable::m_afRate[1000][1] + m_pAttribute.baseDex * CFormulaTable::m_afRate[1000][2]);

	m_pAttribute.wBaseCurseSuccessRate = WORD(m_pAttribute.baseFoc / CFormulaTable::m_afRate[700][1] * CFormulaTable::m_afRate[700][2]);
	m_pAttribute.wBaseCurseToleranceRate = WORD(m_pAttribute.baseDex / CFormulaTable::m_afRate[1200][1] * CFormulaTable::m_afRate[1200][2]);

	m_pAttribute.wBaseLpRegen = pBotTbldat->wLP_Regeneration;
	m_pAttribute.wBaseLpSitdownRegen = WORD(m_pAttribute.baseMaxLp * 20 / 100);
	m_pAttribute.wBaseLpBattleRegen = m_pAttribute.wBaseLpRegen;

	if (m_pBotRef->IsMonster())
	{
		CGameServer* app = (CGameServer*)g_pApp;
		BYTE worldRuleType = GAMERULE_NORMAL;
		if (m_pBotRef->GetCurWorld())
			worldRuleType = m_pBotRef->GetCurWorld()->GetRuleType();
		
		const sMONSTER_STAT_BONUS& bonus = app->GetMonsterStatBonus(worldRuleType);

		if (bonus.fMaxLpPercent != 0.0f)
			m_pAttribute.baseMaxLp += int((float)m_pAttribute.baseMaxLp * bonus.fMaxLpPercent / 100.f);
		if (bonus.fMaxEpPercent != 0.0f)
			m_pAttribute.wBaseMaxEP = WORD((float)m_pAttribute.wBaseMaxEP + (float)m_pAttribute.wBaseMaxEP * bonus.fMaxEpPercent / 100.f);
		if (bonus.fPhysicalOffencePercent != 0.0f)
			m_pAttribute.wBasePhysicalOffence = WORD((float)m_pAttribute.wBasePhysicalOffence + (float)m_pAttribute.wBasePhysicalOffence * bonus.fPhysicalOffencePercent / 100.f);
		if (bonus.fEnergyOffencePercent != 0.0f)
			m_pAttribute.wBaseEnergyOffence = WORD((float)m_pAttribute.wBaseEnergyOffence + (float)m_pAttribute.wBaseEnergyOffence * bonus.fEnergyOffencePercent / 100.f);
		if (bonus.fPhysicalDefencePercent != 0.0f)
			m_pAttribute.wBasePhysicalDefence = WORD((float)m_pAttribute.wBasePhysicalDefence + (float)m_pAttribute.wBasePhysicalDefence * bonus.fPhysicalDefencePercent / 100.f);
		if (bonus.fEnergyDefencePercent != 0.0f)
			m_pAttribute.wBaseEnergyDefence = WORD((float)m_pAttribute.wBaseEnergyDefence + (float)m_pAttribute.wBaseEnergyDefence * bonus.fEnergyDefencePercent / 100.f);
		if (bonus.fAttackRatePercent != 0.0f)
			m_pAttribute.wBaseAttackRate = WORD((float)m_pAttribute.wBaseAttackRate + (float)m_pAttribute.wBaseAttackRate * bonus.fAttackRatePercent / 100.f);
		if (bonus.fDodgeRatePercent != 0.0f)
			m_pAttribute.wBaseDodgeRate = WORD((float)m_pAttribute.wBaseDodgeRate + (float)m_pAttribute.wBaseDodgeRate * bonus.fDodgeRatePercent / 100.f);
	}
}


void CCharacterAttBot::CalculateLastAtt()
{
	Reset(); //set last to base

	m_pOwnerRef->GetBuffManager()->CopyBuffAttributesTo(this);

	//printf("");

	CalculatePercentValues();

	CCharacterAtt::CalculateLastAtt();
}

void CCharacterAttBot::CalculateLastRunSpeed(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.fLastRunSpeed += fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_pAttribute.fLastRunSpeed += m_pAttribute.fLastRunSpeed * fValue / 100.f;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.fLastRunSpeed -= fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_pAttribute.fLastRunSpeed -= m_pAttribute.fLastRunSpeed * fValue / 100.f;
		}

		if (m_pAttribute.fLastRunSpeed < 0.f)
			m_pAttribute.fLastRunSpeed = 0.f;
	}
}
