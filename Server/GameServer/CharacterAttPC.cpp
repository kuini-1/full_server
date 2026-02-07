#include "stdafx.h"
#include "CharacterAttPC.h"
#include "CPlayer.h"
#include "item.h"
#include "VehicleTable.h"
#include "ItemOptionTable.h"
#include "TableContainerManager.h"
#include "CharTitleTable.h"
#include "calcs.h"
#include "StatusTransformTable.h"
#include "CommonConfigTable.h"
#include "FormulaTable.h"
#include "GameServer.h"
#include "SummonPet.h"
#include "UseItemTable.h"
#include "BattleRoyaleEvent.h"


CCharacterAttPC::CCharacterAttPC()
{
}

CCharacterAttPC::~CCharacterAttPC()
{
}


bool CCharacterAttPC::Create(CPlayer* pPlayer)
{
	if (CCharacterAtt::Create(pPlayer))
	{
		m_pPlayerRef = pPlayer;
		return true;
	}

	return false;
}


void CCharacterAttPC::Reset()
{
	CCharacterAtt::Reset();

	m_pAttribute.fLastRunSpeed = m_pAttribute.fBaseRunSpeed;
}


void CCharacterAttPC::CalculateBaseAtt()
{
	sCHAR_TBLDAT* charTbldat = m_pPlayerRef->GetTbldat();
	sPC_TBLDAT* pcTbldat = m_pPlayerRef->GetTbldat();
	float byLevel = (float)m_pPlayerRef->GetLevel();

	m_pAttribute.baseMaxLp = (int)charTbldat->dwBasic_LP;
	m_pAttribute.wBaseMaxEP = charTbldat->wBasic_EP;
	m_pAttribute.wBasePhysicalDefence = charTbldat->wBasic_Physical_Defence;
	m_pAttribute.wBaseEnergyDefence = charTbldat->wBasic_Energy_Defence;
	m_pAttribute.wBaseAttackSpeedRate = charTbldat->wAttack_Speed_Rate;
	m_pAttribute.fBaseAttackRange = charTbldat->fAttack_Range;
	m_pAttribute.wBaseAttackRate = charTbldat->wAttack_Rate;
	m_pAttribute.wBaseDodgeRate = charTbldat->wDodge_Rate;
	m_pAttribute.wBaseBlockRate = charTbldat->wBlock_Rate;
	m_pAttribute.wBaseCurseSuccessRate = charTbldat->wCurse_Success_Rate;
	m_pAttribute.wBaseCurseToleranceRate = charTbldat->wCurse_Tolerance_Rate;

	m_pAttribute.baseStr = (WORD)(charTbldat->wBasicStr + pcTbldat->fLevel_Up_Str * byLevel);
	m_pAttribute.baseCon = (WORD)(charTbldat->wBasicCon + pcTbldat->fLevel_Up_Con * byLevel);
	m_pAttribute.baseFoc = (WORD)(charTbldat->wBasicFoc + pcTbldat->fLevel_Up_Foc * byLevel);
	m_pAttribute.baseDex = (WORD)(charTbldat->wBasicDex + pcTbldat->fLevel_Up_Dex * byLevel);
	m_pAttribute.baseSol = (WORD)(charTbldat->wBasicSol + pcTbldat->fLevel_Up_Sol * byLevel);
	m_pAttribute.baseEng = (WORD)(charTbldat->wBasicEng + pcTbldat->fLevel_Up_Eng * byLevel);
	
	// Battle Royale: Add +10 to all stats per level gained (player starts at level 1 in BR)
	// So if player is level 3, they gained 2 levels, so add 2*10 = 20 to each stat
	if (g_pBattleRoyaleEvent && g_pBattleRoyaleEvent->IsPlayerInBattleRoyale(m_pPlayerRef))
	{
		BYTE brLevelsGained = (byLevel > 1) ? (byLevel - 1) : 0;
		if (brLevelsGained > 0)
		{
			WORD bonusStats = brLevelsGained * 10;
			m_pAttribute.baseStr = UnsignedSafeIncrease<WORD>(m_pAttribute.baseStr, bonusStats);
			m_pAttribute.baseCon = UnsignedSafeIncrease<WORD>(m_pAttribute.baseCon, bonusStats);
			m_pAttribute.baseFoc = UnsignedSafeIncrease<WORD>(m_pAttribute.baseFoc, bonusStats);
			m_pAttribute.baseDex = UnsignedSafeIncrease<WORD>(m_pAttribute.baseDex, bonusStats);
			m_pAttribute.baseSol = UnsignedSafeIncrease<WORD>(m_pAttribute.baseSol, bonusStats);
			m_pAttribute.baseEng = UnsignedSafeIncrease<WORD>(m_pAttribute.baseEng, bonusStats);
		}
	}

	m_pAttribute.fBaseSkillAnimationSpeedModifier = 100.0f;
	m_pAttribute.dwBaseWeightLimit = pcTbldat->dwWeightLimit;
	m_pAttribute.baseMaxAp = 450000;
	m_pAttribute.wBaseMaxRP = WORD(CFormulaTable::m_afRate[2300][1] + byLevel * CFormulaTable::m_afRate[2300][2]);
	m_pAttribute.wBaseRpRegen = 250;
	m_pAttribute.wLastRpDimimutionRate = 6;
	m_pAttribute.wBaseApRegen = 5000;
	m_pAttribute.wBaseApSitdownRegen = 10000;
	m_pAttribute.wBaseApBattleRegen = 1000;
	m_pAttribute.wBaseApDegen = 5000;
	m_pAttribute.wBaseApBattleDegen = 2000;

	m_pAttribute.fBaseRunSpeed = pcTbldat->fAdult_Run_Speed * 1.2;
	m_pAttribute.fBaseFlySpeed = pcTbldat->fAdult_Fly_Speed;
	m_pAttribute.fBaseFlyDashSpeed = pcTbldat->fAdult_Dash_Speed;
	m_pAttribute.fBaseFlyAccelSpeed = pcTbldat->fAdult_Accel_Speed;
	
	m_pAttribute.baseMaxLp += int(CFormulaTable::m_afRate[200 + pcTbldat->byClass + 1][1] + m_pAttribute.baseCon * CFormulaTable::m_afRate[200 + pcTbldat->byClass + 1][2]);
	m_pAttribute.wBaseMaxEP += WORD(CFormulaTable::m_afRate[1300 + pcTbldat->byClass + 1][1] + m_pAttribute.baseEng * CFormulaTable::m_afRate[1300 + pcTbldat->byClass + 1][2]);
	m_pAttribute.wBaseAttackRate += WORD(m_pAttribute.baseFoc / CFormulaTable::m_afRate[600][1] * CFormulaTable::m_afRate[600][2]);
	m_pAttribute.wBaseDodgeRate += WORD(m_pAttribute.baseDex / CFormulaTable::m_afRate[800][1] * CFormulaTable::m_afRate[800][2]);

	m_pAttribute.wBasePhysicalOffence += WORD(byLevel * CFormulaTable::m_afRate[100 + pcTbldat->byClass + 1][1] + m_pAttribute.baseStr * CFormulaTable::m_afRate[100 + pcTbldat->byClass + 1][2]);
	m_pAttribute.wBasePhysicalOffence += WORD(m_pAttribute.baseDex * CFormulaTable::m_afRate[100 + pcTbldat->byClass + 1][3]);

	m_pAttribute.wBaseBlockRate += WORD(CFormulaTable::m_afRate[900][1] + m_pAttribute.baseDex * CFormulaTable::m_afRate[900][2] + m_pAttribute.baseCon * CFormulaTable::m_afRate[900][2]);
	m_pAttribute.wGuardRateBase += WORD(CFormulaTable::m_afRate[1000][1] + m_pAttribute.baseDex * CFormulaTable::m_afRate[1000][2]);


	m_pAttribute.wBaseEnergyOffence += WORD(byLevel * CFormulaTable::m_afRate[1100 + pcTbldat->byClass + 1][1] + m_pAttribute.baseSol * CFormulaTable::m_afRate[1100 + pcTbldat->byClass + 1][2]);
	m_pAttribute.wBaseEnergyOffence += WORD(m_pAttribute.baseFoc * CFormulaTable::m_afRate[1100 + pcTbldat->byClass + 1][3]);

	m_pAttribute.wBaseLpRegen += WORD(CFormulaTable::m_afRate[300][1] + m_pAttribute.baseCon * CFormulaTable::m_afRate[300][2]);
	m_pAttribute.wBaseLpSitdownRegen += WORD(m_pAttribute.wBaseLpRegen * CFormulaTable::m_afRate[400][1]);
	m_pAttribute.wBaseLpBattleRegen = WORD(m_pAttribute.wBaseLpRegen * CFormulaTable::m_afRate[500][1]);
	m_pAttribute.wBaseEpRegen += WORD(CFormulaTable::m_afRate[1400][1] + m_pAttribute.baseEng * CFormulaTable::m_afRate[1400][2]);
	m_pAttribute.wBaseEpSitdownRegen += WORD(m_pAttribute.wBaseEpRegen * CFormulaTable::m_afRate[1500][1]);
	m_pAttribute.wBaseEpBattleRegen = WORD(m_pAttribute.wBaseLpRegen * CFormulaTable::m_afRate[1600][1]);

	m_pAttribute.wBasePhysicalCriticalRate += WORD(CFormulaTable::m_afRate[1900 + pcTbldat->byClass + 1][1] + m_pAttribute.baseDex / CFormulaTable::m_afRate[1900 + pcTbldat->byClass + 1][2]);
	m_pAttribute.wBaseEnergyCriticalRate += WORD(CFormulaTable::m_afRate[2100 + pcTbldat->byClass + 1][1] + m_pAttribute.baseFoc / CFormulaTable::m_afRate[2100 + pcTbldat->byClass + 1][2]);
	m_pAttribute.wBaseCurseSuccessRate += WORD(m_pAttribute.baseFoc / CFormulaTable::m_afRate[700][1] * CFormulaTable::m_afRate[700][2]);
	m_pAttribute.wBaseCurseToleranceRate += WORD(m_pAttribute.baseDex / CFormulaTable::m_afRate[1200][1] * CFormulaTable::m_afRate[1200][2]);

	m_pAttribute.fBasePhysicalCriticalDamageRate += CFormulaTable::m_afRate[2000 + pcTbldat->byClass + 1][1] + m_pAttribute.baseDex / CFormulaTable::m_afRate[2000 + pcTbldat->byClass + 1][2];
	m_pAttribute.fBaseEnergyCriticalDamageRate += CFormulaTable::m_afRate[2200 + pcTbldat->byClass + 1][1] + m_pAttribute.baseFoc / CFormulaTable::m_afRate[2200 + pcTbldat->byClass + 1][2];

	//default equipment stats added to base
	m_pPlayerRef->GetPlayerItemContainer()->CopyBaseItemAttributesTo(m_pAttribute);
	
	//apply passive skills attributes
	m_pPlayerRef->GetSkillManager()->CopyPassiveAttributesTo(this);

	CGameServer* app = (CGameServer*)g_pApp;
	const sCHANNEL_STAT_BONUS& bonus = app->GetChannelStatBonus();
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

void CCharacterAttPC::CalculateLastAtt()
{
	Reset(); //set last to base
	
	m_pPlayerRef->GetPlayerItemContainer()->CopyItemAttributesTo(this);

	//set char title attribute
	if (m_pPlayerRef->GetCharTitleID() != INVALID_TBLIDX && m_pPlayerRef->GetWorldTblidx() != 910000)
	{
		sCHARTITLE_TBLDAT* charTitleTbldat = (sCHARTITLE_TBLDAT*)g_pTableContainer->GetCharTitleTable()->FindData(m_pPlayerRef->GetCharTitleID());
		if (charTitleTbldat)
		{
			for (BYTE i = 0; i < NTL_MAX_CHAR_TITLE_EFFECT; i++)
			{
				eSYSTEM_EFFECT_CODE effectcode = g_pTableContainer->GetSystemEffectTable()->GetEffectCodeWithTblidx(charTitleTbldat->atblSystem_Effect_Index[i]);
				if (effectcode != INVALID_SYSTEM_EFFECT_CODE)
				{
					Dbo_SetAvatarAttributeValue(this, effectcode, (float)charTitleTbldat->abySystem_Effect_Value[i], charTitleTbldat->abySystem_Effect_Type[i]);
				}
			}
		}
	}

	//buff attributes
	m_pPlayerRef->GetBuffManager()->CopyBuffAttributesTo(this);

	m_pAttribute.fLastSkillAnimationSpeedModifier += 100.f;

	//transformation attributes
	if (m_pPlayerRef->GetTransformationTbldat())
	{
		sSTATUS_TRANSFORM_TBLDAT* pTransformationTbldat = m_pPlayerRef->GetTransformationTbldat();

		m_pAttribute.lastMaxLp = (int)(m_pAttribute.lastMaxLp * pTransformationTbldat->fLP_Transform);
		m_pAttribute.wLastMaxEP = (WORD)(m_pAttribute.wLastMaxEP * pTransformationTbldat->fEP_Transform);
		m_pAttribute.wLastPhysicalOffence = (WORD)(m_pAttribute.wLastPhysicalOffence * pTransformationTbldat->fPhysical_Offence_Transform);
		m_pAttribute.wLastEnergyOffence = (WORD)(m_pAttribute.wLastEnergyOffence * pTransformationTbldat->fEnergy_Offence_Transform);
		m_pAttribute.wLastPhysicalDefence = (WORD)(m_pAttribute.wLastPhysicalDefence * pTransformationTbldat->fPhysical_Defence_Transform);
		m_pAttribute.wLastEnergyDefence = (WORD)(m_pAttribute.wLastEnergyDefence * pTransformationTbldat->fEnergy_Defence_Transform);
		m_pAttribute.fLastRunSpeed = m_pAttribute.fLastRunSpeed * pTransformationTbldat->fRun_Speed_Transform;

		if (m_pPlayerRef->GetAspectStateId() == ASPECTSTATE_KAIOKEN)
		{
			float fSpeedBonus = 3.0f;

			if (m_pPlayerRef->GetTransformGrade() == 2)
				fSpeedBonus = 5.0f;
			else if (m_pPlayerRef->GetTransformGrade() >= 3)
				fSpeedBonus = 7.0f;

			CalculateLastAttackSpeedRate(fSpeedBonus, SYSTEM_EFFECT_APPLY_TYPE_PERCENT, false);
		}
		else
		{
			CalculateLastAttackSpeedRate(5.0f, SYSTEM_EFFECT_APPLY_TYPE_PERCENT, false);
			/*
			if (pTransformationTbldat->fAttack_Speed_Transform < 1.0f)
				CalculateLastAttackSpeedRate(100.f - pTransformationTbldat->fAttack_Speed_Transform * 100.f, SYSTEM_EFFECT_APPLY_TYPE_PERCENT, false); // * 100 because the value is like 0.75 = 0.75 * 100 = 75% and then - 100 to get the real %
			else
				CalculateLastAttackSpeedRate(pTransformationTbldat->fAttack_Speed_Transform * 100.f - 100.f, SYSTEM_EFFECT_APPLY_TYPE_PERCENT, true);
			*/
		}

		if (m_pPlayerRef->GetAspectStateId() == ASPECTSTATE_SUPER_SAIYAN)
			m_pAttribute.fLastSkillAnimationSpeedModifier += 20.f; // make skill animation faster

		if (m_pPlayerRef->GetAspectStateId() == ASPECTSTATE_PURE_MAJIN)
			m_pAttribute.fLastRunSpeed *= 1.4;

		m_pAttribute.wLastAttackRate = (WORD)(m_pAttribute.wLastAttackRate * pTransformationTbldat->fAttack_Rate_Transform);
		m_pAttribute.wLastDodgeRate = (WORD)(m_pAttribute.wLastDodgeRate * pTransformationTbldat->fDodge_Rate_Transform);
		m_pAttribute.wLastBlockRate = (WORD)(m_pAttribute.wLastBlockRate * pTransformationTbldat->fBlock_Rate_Transform);
		m_pAttribute.wLastCurseSuccessRate = (WORD)(m_pAttribute.wLastCurseSuccessRate * pTransformationTbldat->fCurse_Success_Transform);
		m_pAttribute.wLastCurseToleranceRate = (WORD)(m_pAttribute.wLastCurseToleranceRate * pTransformationTbldat->fCurse_Tolerance_Transform);
		m_pAttribute.fLastAttackRange = pTransformationTbldat->fAttack_Range_Change;

		if (m_pPlayerRef->GetTransformGrade() > 1)
		{
			switch (m_pPlayerRef->GetClass())
			{
			case PC_CLASS_HUMAN_FIGHTER:
			case PC_CLASS_STREET_FIGHTER:
			case PC_CLASS_SWORD_MASTER:
			{
				m_pAttribute.wLastPhysicalCriticalRate += WORD((float)m_pAttribute.lastDex * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
				m_pAttribute.lastStr += WORD((float)m_pAttribute.lastStr * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
				m_pAttribute.wLastDodgeRate += WORD((float)m_pAttribute.baseMaxLp * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
				m_pAttribute.wLastPhysicalOffence += WORD((float)m_pAttribute.lastDex * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
				m_pAttribute.wLastEnergyOffence += WORD((float)m_pAttribute.lastDex * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
			}
			break;

			case PC_CLASS_HUMAN_MYSTIC:
			case PC_CLASS_CRANE_ROSHI:
			case PC_CLASS_TURTLE_ROSHI:
			{
				m_pAttribute.wLastEnergyCriticalRate += WORD((float)m_pAttribute.lastFoc * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
				m_pAttribute.lastSol += WORD((float)m_pAttribute.lastSol * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
				m_pAttribute.wLastAttackRate += WORD((float)m_pAttribute.baseMaxLp * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
				m_pAttribute.wLastPhysicalOffence += WORD((float)m_pAttribute.lastFoc * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
				m_pAttribute.wLastEnergyOffence += WORD((float)m_pAttribute.lastFoc * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
			}
			break;

			case PC_CLASS_NAMEK_FIGHTER:
			case PC_CLASS_DARK_WARRIOR:
			case PC_CLASS_SHADOW_KNIGHT:
			{
				m_pAttribute.wLastPhysicalDefence += WORD((float)m_pAttribute.lastEng * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
				m_pAttribute.wLastEnergyDefence += WORD((float)m_pAttribute.lastEng * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
				m_pAttribute.wLastCurseToleranceRate += WORD((float)m_pAttribute.wLastPhysicalOffence * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
				m_pAttribute.wLastPhysicalOffence += WORD((float)m_pAttribute.lastEng * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
				m_pAttribute.wLastEnergyOffence += WORD((float)m_pAttribute.lastEng * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
			}
			break;

			case PC_CLASS_NAMEK_MYSTIC:
			case PC_CLASS_POCO_SUMMONER:
			case PC_CLASS_DENDEN_HEALER:
			{
				m_pAttribute.fCoolTimeChangePercent -= float((float)m_pAttribute.wLastEnergyCriticalRate * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
				m_pAttribute.wLastMaxEP += WORD((float)m_pAttribute.wLastEnergyOffence * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
				m_pAttribute.wLastPhysicalOffence += WORD((float)m_pAttribute.lastEng * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
				m_pAttribute.wLastEnergyOffence += WORD((float)m_pAttribute.lastEng * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
			}
			break;

			case PC_CLASS_MIGHTY_MAJIN:
			case PC_CLASS_GRAND_MA:
			case PC_CLASS_ULTI_MA:
			{
				m_pAttribute.lastCon += WORD((float)m_pAttribute.lastCon * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
				m_pAttribute.wLastCurseToleranceRate += WORD((float)m_pAttribute.wLastPhysicalOffence * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
				m_pAttribute.wLastPhysicalOffence += WORD((float)m_pAttribute.lastStr * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
				m_pAttribute.wLastEnergyOffence += WORD((float)m_pAttribute.lastStr * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
			}
			break;

			case PC_CLASS_WONDER_MAJIN:
			case PC_CLASS_PLAS_MA:
			case PC_CLASS_KAR_MA:
			{
				m_pAttribute.wLastEnergyCriticalRate += WORD((float)m_pAttribute.lastFoc * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
				m_pAttribute.lastFoc += WORD((float)m_pAttribute.lastFoc * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
				m_pAttribute.wLastCurseSuccessRate += WORD((float)m_pAttribute.wLastMaxEP * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
				m_pAttribute.wLastPhysicalOffence += WORD((float)m_pAttribute.lastFoc * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
				m_pAttribute.wLastEnergyOffence += WORD((float)m_pAttribute.lastFoc * (float)m_pPlayerRef->GetTransformGrade() / 100.0f);
			}
			break;
			}
		}

	}

	//add percent values
	CalculatePercentValues();

	//vehicle -- MUST BE THE LAST
	if (m_pPlayerRef->GetVehicleTblidx() != INVALID_TBLIDX)
	{
		sVEHICLE_TBLDAT* pVehicleTbl = (sVEHICLE_TBLDAT*)g_pTableContainer->GetVehicleTable()->FindData(m_pPlayerRef->GetVehicleTblidx());
		if (pVehicleTbl)
		{
			m_pAttribute.fLastRunSpeed = (float)pVehicleTbl->bySpeed + 10;

			if (m_pPlayerRef->GetVehicleFuelId() != INVALID_HOBJECT)
			{
				CItem* pFuel = m_pPlayerRef->GetPlayerItemContainer()->GetItem(m_pPlayerRef->GetVehicleFuelId());
				if (pFuel && pFuel->IsExpired() == false)
				{
					sITEM_TBLDAT* fuelTbldat = pFuel->GetTbldat();
					if (fuelTbldat)
					{
						if (fuelTbldat->Item_Option_Tblidx != INVALID_TBLIDX && fuelTbldat->bIsCanHaveOption)
						{
							sITEM_OPTION_TBLDAT* optionTbldat = (sITEM_OPTION_TBLDAT*)g_pTableContainer->GetItemOptionTable()->FindData(fuelTbldat->Item_Option_Tblidx);
							if (optionTbldat)
							{
								m_pAttribute.fLastRunSpeed += m_pAttribute.fLastRunSpeed * optionTbldat->nValue[0] / 100;
							}
						}
					}
				}
			}
		}
	}


	CGameServer* app = (CGameServer*)g_pApp;

	if (m_pPlayerRef->GetDragonballScramble() && app->GetGsChannel() == 0)
		m_pAttribute.lastMaxLp += 5000;

	if (m_pPlayerRef->GetGMLevel() == 10 && m_pPlayerRef->GetCharID() == 2 /*&& m_pPlayerRef->GetWorldTblidx() != 910000*/)
	{
		m_pAttribute.lastMaxLp += 15000;
		m_pAttribute.lastFoc += 1000;
		m_pAttribute.fLastEnergyCriticalDamageRate += 500;
		m_pAttribute.wLastEnergyDefence += 20000;
		m_pAttribute.wLastPhysicalDefence += 20000;
		m_pAttribute.wLastEnergyOffence += 20000;
	}

	CalcSecondWeaponOffence();
	CalculateNewItems();

	//m_pAttribute.fCoolTimeChangePercent = -100.f; // TEMP

	float fAttackRangeAdjust = (m_pPlayerRef->GetAttackType() == ATTACK_TYPE_ENERGY) ? 2.0f : 1.0f;

	m_pAttribute.fLastAttackRange -= fAttackRangeAdjust; //decrease to send to the client to fix the attack range issue

	/*
	std::vector<TBLIDX> m_buffs{ 11170707, 11170706, 11170705 };
	CBuff* buff = NULL;
	for (TBLIDX i : m_buffs)
	{
		buff = m_pPlayerRef->GetBuffManager()->FindBuff(i, 1);
		if (buff)
			break;
	}

	if (buff)
	{
		switch (buff->GetSourceTblidx())
		{
		case 11170707:
		{
			m_pPlayerRef->UpdateSizeRate(10 - (10 * 0.20));
			m_pAttribute.lastMaxLp -= m_pAttribute.lastMaxLp * 10.f / 100.f;
			m_pAttribute.wLastPhysicalOffence -= m_pAttribute.wLastPhysicalOffence * 10.f / 100.f;
			m_pAttribute.wLastEnergyOffence -= m_pAttribute.wLastEnergyOffence * 10.f / 100.f;
			m_pAttribute.wLastPhysicalDefence -= m_pAttribute.wLastPhysicalDefence * 10.f / 100.f;
			m_pAttribute.wLastEnergyDefence -= m_pAttribute.wLastEnergyDefence * 10.f / 100.f;

			m_pAttribute.fLastSkillAnimationSpeedModifier += m_pAttribute.fLastSkillAnimationSpeedModifier * 20.f / 100.f;
			m_pAttribute.wLastAttackSpeedRate -= m_pAttribute.wLastAttackSpeedRate * 20.f / 100.f;
			m_pAttribute.fLastRunSpeed += m_pAttribute.fLastRunSpeed * 20.f / 100.f;
		}
		break;
		case 11170706: 
		{
			m_pPlayerRef->UpdateSizeRate(10);
			m_pAttribute.lastMaxLp = m_pAttribute.lastMaxLp;
			m_pAttribute.wLastPhysicalOffence = m_pAttribute.wLastPhysicalOffence;
			m_pAttribute.wLastEnergyOffence = m_pAttribute.wLastEnergyOffence;
			m_pAttribute.wLastPhysicalDefence = m_pAttribute.wLastPhysicalDefence;
			m_pAttribute.wLastEnergyDefence = m_pAttribute.wLastEnergyDefence;

			m_pAttribute.fLastSkillAnimationSpeedModifier = m_pAttribute.fLastSkillAnimationSpeedModifier;
			m_pAttribute.wLastAttackSpeedRate = m_pAttribute.wLastAttackSpeedRate;
			m_pAttribute.fLastRunSpeed = m_pAttribute.fLastRunSpeed;
		}
		break;
		case 11170705:
		{
			m_pPlayerRef->UpdateSizeRate(10 + (10 * 0.20));
			m_pAttribute.lastMaxLp += m_pAttribute.lastMaxLp * 10.f / 100.f;
			m_pAttribute.wLastPhysicalOffence += m_pAttribute.wLastPhysicalOffence * 10.f / 100.f;
			m_pAttribute.wLastEnergyOffence += m_pAttribute.wLastEnergyOffence * 10.f / 100.f;
			m_pAttribute.wLastPhysicalDefence += m_pAttribute.wLastPhysicalDefence * 10.f / 100.f;
			m_pAttribute.wLastEnergyDefence += m_pAttribute.wLastEnergyDefence * 10.f / 100.f;

			m_pAttribute.fLastSkillAnimationSpeedModifier -= m_pAttribute.fLastSkillAnimationSpeedModifier * 20.f / 100.f;
			m_pAttribute.wLastAttackSpeedRate += m_pAttribute.wLastAttackSpeedRate * 20.f / 100.f;
			m_pAttribute.fLastRunSpeed -= m_pAttribute.fLastRunSpeed * 20.f / 100.f;
		}
		break;
		}
	}
	*/

	/*
	if (m_pPlayerRef->GetClass() == PC_CLASS_ULTI_MA)
	{
		m_pAttribute.fParalyzeKeepTimeDown += m_pAttribute.fParalyzeKeepTimeDown * 20.f / 100.f;
		m_pAttribute.fTerrorKeepTimeDown += m_pAttribute.fTerrorKeepTimeDown * 20.f / 100.f;
		m_pAttribute.fConfuseKeepTimeDown += m_pAttribute.fConfuseKeepTimeDown * 20.f / 100.f;
		m_pAttribute.fStoneKeepTimeDown += m_pAttribute.fStoneKeepTimeDown * 20.f / 100.f;
		m_pAttribute.fCandyKeepTimeDown += m_pAttribute.fCandyKeepTimeDown * 20.f / 100.f;
	}

	if (m_pPlayerRef->GetClass() == PC_CLASS_POCO_SUMMONER)
	{
		if (m_pPlayerRef->GetCurrentPetId() != INVALID_HOBJECT)
		{
			CSummonPet* pet = g_pObjectManager->GetSummonPet(m_pPlayerRef->GetCurrentPetId());

			m_pAttribute.lastMaxLp += pet->GetCharAtt()->GetLastMaxLP() * 20.f / 100.f;
			m_pAttribute.wLastEnergyOffence = pet->GetCharAtt()->GetLastEnergyOffence() * 20.f / 100.f;
			m_pAttribute.wLastPhysicalOffence = pet->GetCharAtt()->GetLastPhysicalOffence() * 20.f / 100.f;
			m_pAttribute.wLastEnergyDefence = pet->GetCharAtt()->GetLastEnergyDefence() * 20.f / 100.f;
			m_pAttribute.wLastPhysicalDefence = pet->GetCharAtt()->GetLastPhysicalDefence() * 20.f / 100.f;
			m_pAttribute.wLastAttackRate = pet->GetCharAtt()->GetLastAttackRate() * 20.f / 100.f;
			m_pAttribute.wLastDodgeRate = pet->GetCharAtt()->GetLastDodgeRate() * 20.f / 100.f;
		}
	}
	*/
	
	
	

	
	//rAvatarAttribute.fLastRunSpeed += rAvatarAttribute.fLastRunSpeed * ((float)commonConfig->adwValue[2]) / 100.f;

	CCharacterAtt::CalculateLastAtt();

	m_pAttribute.fLastAttackRange += fAttackRangeAdjust; //set back to original
}

void CCharacterAttPC::CalculateNewItems()
{
	CGameServer* app = (CGameServer*)g_pApp;

	enum NEW_ITEMS {
		FLY_ACCEL_SPEED_UP = 11120112,
	};

	enum BUFFS {
		EXP_MOVESPEED = 850087,
	};

	enum PETS {
		BLACK_CAT = 1541102,
		BULMA = 1381101,
		KID_CHI_CHI = 1711101,
	};
	//DBO_OBJECT_SOURCE_SKILL = 0,
	//DBO_OBJECT_SOURCE_ITEM,

	//if (m_pPlayerRef->GetBuffManager()->FindBuff((TBLIDX)FLY_ACCEL_SPEED_UP, DBO_OBJECT_SOURCE_ITEM)) ACTIVE_AIR_ACCEL

	if (m_pPlayerRef->GetSummonPet())
	{
		switch (m_pPlayerRef->GetSummonPet()->GetTblidx())
		{
		case BLACK_CAT: case BULMA: case KID_CHI_CHI: { ApplySubBuff((int)EXP_MOVESPEED); } break;
		}
	}

	if (m_pPlayerRef->GetBuffManager()->FindAnyBuff((eSYSTEM_EFFECT_CODE)ACTIVE_AIR_ACCEL)) m_pAttribute.fLastFlyAccelSpeed *= 5;
	if (m_pPlayerRef->GetBuffManager()->FindAnyBuff((eSYSTEM_EFFECT_CODE)ACTIVE_HIDE_KI)) app->SetItemDropRate(app->GetItemDropRate() + 200); else app->SetItemDropRate(app->GetOriginalItemDropRate());

}

void CCharacterAttPC::ApplySubBuff(int usebuff)
{
	TBLIDX applybuff = usebuff;
	CBuff* findbuff = m_pPlayerRef->GetBuffManager()->FindBuff(applybuff, DBO_OBJECT_SOURCE_ITEM);
	if (!findbuff)
	{
		if (sUSE_ITEM_TBLDAT* pUseItemTbldat = (sUSE_ITEM_TBLDAT*)g_pTableContainer->GetUseItemTable()->FindData(applybuff))
		{
			sBUFF_INFO buffInfo;
			buffInfo.buffIndex = INVALID_BYTE;
			buffInfo.sourceTblidx = applybuff;
			buffInfo.dwTimeRemaining = pUseItemTbldat->dwKeepTimeInMilliSecs;
			buffInfo.dwInitialDuration = pUseItemTbldat->dwKeepTimeInMilliSecs;
			buffInfo.bySourceType = DBO_OBJECT_SOURCE_ITEM;

			eSYSTEM_EFFECT_CODE effectCode[NTL_MAX_EFFECT_IN_ITEM];
			effectCode[0] = g_pTableContainer->GetSystemEffectTable()->GetEffectCodeWithTblidx(pUseItemTbldat->aSystem_Effect[0]);
			effectCode[1] = g_pTableContainer->GetSystemEffectTable()->GetEffectCodeWithTblidx(pUseItemTbldat->aSystem_Effect[1]);

			for (int x = 0; x < NTL_MAX_EFFECT_IN_ITEM; x++)
			{
				switch (effectCode[x])
				{
				case ACTIVE_EP_OVER_TIME: case ACTIVE_HEAL_OVER_TIME:
				{
					buffInfo.aBuffParameter[x].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_HOT;
					buffInfo.aBuffParameter[x].buffParameter.dwRemainTime = pUseItemTbldat->dwKeepTimeInMilliSecs;
					buffInfo.aBuffParameter[x].buffParameter.fParameter = (float)pUseItemTbldat->aSystem_Effect_Value[x];
				}
				break;
				default:
				{
					buffInfo.aBuffParameter[x].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_DEFAULT;
					buffInfo.aBuffParameter[x].buffParameter.dwRemainTime = pUseItemTbldat->dwKeepTimeInMilliSecs;
					buffInfo.aBuffParameter[x].buffParameter.fParameter = (float)pUseItemTbldat->aSystem_Effect_Value[x];
				}
				break;
				}
				WORD wTemp;
				m_pPlayerRef->GetBuffManager()->RegisterSubBuff(&buffInfo, effectCode, m_pPlayerRef->GetID(), pUseItemTbldat->byBuff_Group, wTemp, pUseItemTbldat->abySystem_Effect_Type);
			}
		}
	}
}

void CCharacterAttPC::CalculateBaseStr(float fValue, BYTE byApplyType, bool bIsPlus)
{
	WORD wCurStr = m_pAttribute.baseStr;
	
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.baseStr = UnsignedSafeIncrease<WORD>(m_pAttribute.baseStr, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			float percentvalue = NtlRound((float)m_pAttribute.baseStr * fValue / 100.0f);
			m_pAttribute.baseStr = UnsignedSafeIncrease<WORD>(m_pAttribute.baseStr, (WORD)percentvalue);
		}

		m_pAttribute.wBasePhysicalOffence += WORD(float(m_pAttribute.baseStr - wCurStr) * CFormulaTable::m_afRate[100 + m_pPlayerRef->GetClass() + 1][2]);
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.baseStr = UnsignedSafeDecrease<WORD>(m_pAttribute.baseStr, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			float percentvalue = NtlRound((float)m_pAttribute.baseStr * fValue / 100.0f);
			m_pAttribute.baseStr = UnsignedSafeDecrease<WORD>(m_pAttribute.baseStr, (WORD)percentvalue);
		}

		m_pAttribute.wBasePhysicalOffence = UnsignedSafeDecrease<WORD>(m_pAttribute.wBasePhysicalOffence, WORD((wCurStr - m_pAttribute.baseStr)  * CFormulaTable::m_afRate[100 + m_pPlayerRef->GetClass() + 1][2]));
	}
}

void CCharacterAttPC::CalculateBaseCon(float fValue, BYTE byApplyType, bool bIsPlus)
{
	WORD wCurCon = m_pAttribute.baseCon;
	WORD wCurLpRegen = m_pAttribute.wBaseLpRegen;

	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.baseCon = UnsignedSafeIncrease<WORD>(m_pAttribute.baseCon, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			float percentvalue = NtlRound((float)m_pAttribute.baseCon * fValue / 100.0f);
			m_pAttribute.baseCon = UnsignedSafeIncrease<WORD>(m_pAttribute.baseCon, (WORD)percentvalue);
		}

		m_pAttribute.baseMaxLp += int((float)(m_pAttribute.baseCon - wCurCon) * CFormulaTable::m_afRate[200 + m_pPlayerRef->GetClass() + 1][2]);
		m_pAttribute.wBaseLpRegen += WORD((float)(m_pAttribute.baseCon - wCurCon) * CFormulaTable::m_afRate[300][2]);
		m_pAttribute.wBaseLpSitdownRegen += WORD(float(m_pAttribute.wBaseLpRegen - wCurLpRegen) * CFormulaTable::m_afRate[400][1]);

		m_pAttribute.wBaseBlockRate += WORD(float(m_pAttribute.baseCon - wCurCon) * CFormulaTable::m_afRate[900][2]);
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.baseCon = UnsignedSafeDecrease<WORD>(m_pAttribute.baseCon, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			float percentvalue = NtlRound((float)m_pAttribute.baseCon * fValue / 100.0f);
			m_pAttribute.baseCon = UnsignedSafeDecrease<WORD>(m_pAttribute.baseCon, (WORD)percentvalue);
		}

		m_pAttribute.baseMaxLp = UnsignedSafeDecrease<WORD>(m_pAttribute.baseMaxLp, WORD((float)(wCurCon - m_pAttribute.baseCon) * CFormulaTable::m_afRate[200 + m_pPlayerRef->GetClass() + 1][2]));
		m_pAttribute.wBaseLpRegen = UnsignedSafeDecrease<WORD>(m_pAttribute.wBaseLpRegen, WORD((float)(wCurCon - m_pAttribute.baseCon) * CFormulaTable::m_afRate[300][2]));
		m_pAttribute.wBaseLpSitdownRegen = UnsignedSafeDecrease<WORD>(m_pAttribute.wBaseLpSitdownRegen, WORD(float(wCurLpRegen - m_pAttribute.wBaseLpRegen) * CFormulaTable::m_afRate[400][1]));

		m_pAttribute.wBaseBlockRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wBaseBlockRate, WORD((float)(wCurCon - m_pAttribute.baseCon) * CFormulaTable::m_afRate[900][2]));
	}
}

void CCharacterAttPC::CalculateBaseFoc(float fValue, BYTE byApplyType, bool bIsPlus)
{
	WORD wCurFoc = m_pAttribute.baseFoc;

	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.baseFoc = UnsignedSafeIncrease<WORD>(m_pAttribute.baseFoc, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			float percentvalue = NtlRound((float)m_pAttribute.baseFoc * fValue / 100.0f);
			m_pAttribute.baseFoc = UnsignedSafeIncrease<WORD>(m_pAttribute.baseFoc, (WORD)percentvalue);
		}

		m_pAttribute.wBaseAttackRate += (WORD)(float(m_pAttribute.baseFoc - wCurFoc) / CFormulaTable::m_afRate[600][1] * CFormulaTable::m_afRate[600][2]);
		m_pAttribute.wBaseCurseSuccessRate += (WORD)(float(m_pAttribute.baseFoc - wCurFoc) / CFormulaTable::m_afRate[700][1] * CFormulaTable::m_afRate[700][2]);
		m_pAttribute.wBaseEnergyCriticalRate += (WORD)(float(m_pAttribute.baseFoc - wCurFoc) / CFormulaTable::m_afRate[2100 + m_pPlayerRef->GetClass() + 1][2]);
		m_pAttribute.fBaseEnergyCriticalDamageRate += (WORD)(float(m_pAttribute.baseFoc - wCurFoc) / CFormulaTable::m_afRate[2200 + m_pPlayerRef->GetClass() + 1][2]);
		m_pAttribute.wBaseEnergyOffence += (WORD)(float(m_pAttribute.baseFoc - wCurFoc) * CFormulaTable::m_afRate[1100 + m_pPlayerRef->GetClass() + 1][3]);

	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.baseFoc = UnsignedSafeDecrease<WORD>(m_pAttribute.baseFoc, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			float percentvalue = NtlRound((float)m_pAttribute.baseFoc * fValue / 100.0f);
			m_pAttribute.baseFoc = UnsignedSafeDecrease<WORD>(m_pAttribute.baseFoc, (WORD)percentvalue);
		}

		m_pAttribute.wBaseAttackRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wBaseAttackRate, WORD(float(wCurFoc - m_pAttribute.baseFoc) / CFormulaTable::m_afRate[600][1] * CFormulaTable::m_afRate[600][2]));
		m_pAttribute.wBaseCurseSuccessRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wBaseCurseSuccessRate, WORD(float(wCurFoc - m_pAttribute.baseFoc) / CFormulaTable::m_afRate[700][1] * CFormulaTable::m_afRate[700][2]));
		m_pAttribute.wBaseEnergyCriticalRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wBaseEnergyCriticalRate, WORD(float(wCurFoc - m_pAttribute.baseFoc) / CFormulaTable::m_afRate[2100 + m_pPlayerRef->GetClass() + 1][2]));
		m_pAttribute.fBaseEnergyCriticalDamageRate = UnsignedSafeDecrease<float>(m_pAttribute.fBaseEnergyCriticalDamageRate, WORD(float(wCurFoc - m_pAttribute.baseFoc) / CFormulaTable::m_afRate[2200 + m_pPlayerRef->GetClass() + 1][2]));
		m_pAttribute.wBaseEnergyOffence = UnsignedSafeDecrease<WORD>(m_pAttribute.wBaseEnergyOffence, WORD(float(wCurFoc - m_pAttribute.baseFoc) * CFormulaTable::m_afRate[1100 + m_pPlayerRef->GetClass() + 1][3]));

	}
}

void CCharacterAttPC::CalculateBaseDex(float fValue, BYTE byApplyType, bool bIsPlus)
{
	WORD wCurDex = m_pAttribute.baseDex;

	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.baseDex = UnsignedSafeIncrease<WORD>(m_pAttribute.baseDex, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			float percentvalue = NtlRound((float)m_pAttribute.baseDex * fValue / 100.0f);
			m_pAttribute.baseDex = UnsignedSafeIncrease<WORD>(m_pAttribute.baseDex, (WORD)percentvalue);
		}

		m_pAttribute.wBaseDodgeRate += WORD(float(m_pAttribute.baseDex - wCurDex) / CFormulaTable::m_afRate[800][1] * CFormulaTable::m_afRate[800][2]);
		m_pAttribute.wBaseCurseToleranceRate += WORD(float(m_pAttribute.baseDex - wCurDex) / CFormulaTable::m_afRate[1200][1] * CFormulaTable::m_afRate[1200][2]);
		m_pAttribute.wBasePhysicalCriticalRate += WORD(float(m_pAttribute.baseDex - wCurDex) / CFormulaTable::m_afRate[1900 + m_pPlayerRef->GetClass() + 1][2]);
		m_pAttribute.fBasePhysicalCriticalDamageRate += float(m_pAttribute.baseDex - wCurDex) / CFormulaTable::m_afRate[2000 + m_pPlayerRef->GetClass() + 1][2];
		m_pAttribute.wBasePhysicalOffence += WORD(float(m_pAttribute.baseDex - wCurDex)  * CFormulaTable::m_afRate[100 + m_pPlayerRef->GetClass() + 1][3]);
		m_pAttribute.wBaseBlockRate += WORD(float(m_pAttribute.baseDex - wCurDex)  * CFormulaTable::m_afRate[900][2]);
		m_pAttribute.wGuardRateBase += WORD(float(m_pAttribute.baseDex - wCurDex)  * CFormulaTable::m_afRate[1000][2]);
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.baseDex = UnsignedSafeDecrease<WORD>(m_pAttribute.baseDex, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			float percentvalue = NtlRound((float)m_pAttribute.baseDex * fValue / 100.0f);
			m_pAttribute.baseDex = UnsignedSafeDecrease<WORD>(m_pAttribute.baseDex, (WORD)percentvalue);
		}

		m_pAttribute.wBaseDodgeRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wBaseDodgeRate, WORD(float(wCurDex - m_pAttribute.baseDex) / CFormulaTable::m_afRate[800][1] * CFormulaTable::m_afRate[800][2]));
		m_pAttribute.wBaseCurseToleranceRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wBaseCurseToleranceRate, WORD(float(wCurDex - m_pAttribute.baseDex) / CFormulaTable::m_afRate[1200][1] * CFormulaTable::m_afRate[1200][2]));
		m_pAttribute.wBasePhysicalCriticalRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wBasePhysicalCriticalRate, WORD(float(wCurDex - m_pAttribute.baseDex) / CFormulaTable::m_afRate[1900 + m_pPlayerRef->GetClass() + 1][2]));
		m_pAttribute.fBasePhysicalCriticalDamageRate = UnsignedSafeDecrease<float>(m_pAttribute.fBasePhysicalCriticalDamageRate, float(wCurDex - m_pAttribute.baseDex) / CFormulaTable::m_afRate[2000 + m_pPlayerRef->GetClass() + 1][2]);
		m_pAttribute.wBasePhysicalOffence = UnsignedSafeDecrease<WORD>(m_pAttribute.wBasePhysicalOffence, WORD(float(wCurDex - m_pAttribute.baseDex) / CFormulaTable::m_afRate[100 + m_pPlayerRef->GetClass() + 1][3]));
		m_pAttribute.wBaseBlockRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wBaseBlockRate, WORD(float(wCurDex - m_pAttribute.baseDex) * CFormulaTable::m_afRate[900][2]));
		m_pAttribute.wGuardRateBase = UnsignedSafeDecrease<WORD>(m_pAttribute.wGuardRateBase, WORD(float(wCurDex - m_pAttribute.baseDex) * CFormulaTable::m_afRate[1000][2]));

	}
}

void CCharacterAttPC::CalculateBaseSol(float fValue, BYTE byApplyType, bool bIsPlus)
{
	WORD wCurSol = m_pAttribute.baseSol;

	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.baseSol = UnsignedSafeIncrease<WORD>(m_pAttribute.baseSol, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			float percentvalue = NtlRound((float)m_pAttribute.baseSol * fValue / 100.0f);
			m_pAttribute.baseSol = UnsignedSafeIncrease<WORD>(m_pAttribute.baseSol, (WORD)percentvalue);
		}

		m_pAttribute.wBaseEnergyOffence += WORD(float(m_pAttribute.baseSol - wCurSol) * CFormulaTable::m_afRate[1100 + m_pPlayerRef->GetClass() + 1][2]);
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.baseSol = UnsignedSafeDecrease<WORD>(m_pAttribute.baseSol, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			float percentvalue = NtlRound((float)m_pAttribute.baseSol * fValue / 100.0f);
			m_pAttribute.baseSol = UnsignedSafeDecrease<WORD>(m_pAttribute.baseSol, (WORD)percentvalue);
		}

		m_pAttribute.wBaseEnergyOffence = UnsignedSafeDecrease<WORD>(m_pAttribute.wBaseEnergyOffence, WORD(float(wCurSol - m_pAttribute.baseSol) * CFormulaTable::m_afRate[1100 + m_pPlayerRef->GetClass() + 1][2]));
	}
}

void CCharacterAttPC::CalculateBaseEng(float fValue, BYTE byApplyType, bool bIsPlus)
{
	WORD wCurEng = m_pAttribute.baseEng;
	WORD wCurEpRegen = m_pAttribute.wBaseEpRegen;

	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.baseEng = UnsignedSafeIncrease<WORD>(m_pAttribute.baseEng, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			float percentvalue = NtlRound((float)m_pAttribute.baseEng * fValue / 100.0f);
			m_pAttribute.baseEng = UnsignedSafeIncrease<WORD>(m_pAttribute.baseEng, (WORD)percentvalue);
		}

		m_pAttribute.wBaseMaxEP += WORD(float(m_pAttribute.baseEng - wCurEng) * CFormulaTable::m_afRate[1300 + m_pPlayerRef->GetClass() + 1][2]);
		m_pAttribute.wBaseEpRegen += WORD(float(m_pAttribute.baseEng - wCurEng) * CFormulaTable::m_afRate[1400][2]);
		m_pAttribute.wBaseEpSitdownRegen += WORD(float(m_pAttribute.wBaseEpRegen - wCurEpRegen) * CFormulaTable::m_afRate[1500][1]);
		m_pAttribute.wBaseEpBattleRegen += WORD(float(m_pAttribute.wBaseEpRegen - wCurEpRegen) * CFormulaTable::m_afRate[1600][1]);
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.baseEng = UnsignedSafeDecrease<WORD>(m_pAttribute.baseEng, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			float percentvalue = NtlRound((float)m_pAttribute.baseEng * fValue / 100.0f);
			m_pAttribute.baseEng = UnsignedSafeDecrease<WORD>(m_pAttribute.baseEng, (WORD)percentvalue);
		}

		m_pAttribute.wBaseMaxEP = UnsignedSafeDecrease<WORD>(m_pAttribute.wBaseMaxEP, WORD(float(wCurEng - m_pAttribute.baseEng) * CFormulaTable::m_afRate[1300 + m_pPlayerRef->GetClass() + 1][2]));
		m_pAttribute.wBaseEpRegen = UnsignedSafeDecrease<WORD>(m_pAttribute.wBaseEpRegen, WORD(float(wCurEng - m_pAttribute.baseEng) * CFormulaTable::m_afRate[1400][2]));
		m_pAttribute.wBaseEpSitdownRegen = UnsignedSafeDecrease<WORD>(m_pAttribute.wBaseEpSitdownRegen, WORD(float(wCurEpRegen - m_pAttribute.wBaseEpRegen) * CFormulaTable::m_afRate[1500][1]));
		m_pAttribute.wBaseEpBattleRegen = UnsignedSafeDecrease<WORD>(m_pAttribute.wBaseEpBattleRegen, WORD(float(wCurEpRegen - m_pAttribute.wBaseEpRegen) * CFormulaTable::m_afRate[1600][1]));
	}
}


void CCharacterAttPC::CalculateLastStr(float fValue, BYTE byApplyType, bool bIsPlus)
{
	WORD wCurStr = m_pAttribute.lastStr;

	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.lastStr = UnsignedSafeIncrease<WORD>(m_pAttribute.lastStr, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fSTR += fValue;
		}

		m_pAttribute.wLastPhysicalOffence = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastPhysicalOffence, WORD((m_pAttribute.lastStr - wCurStr)  * CFormulaTable::m_afRate[100 + m_pPlayerRef->GetClass() + 1][2]));
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.lastStr = UnsignedSafeDecrease<WORD>(m_pAttribute.lastStr, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fSTRNegative -= fValue;
		}

		m_pAttribute.wLastPhysicalOffence = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastPhysicalOffence, WORD((wCurStr - m_pAttribute.lastStr)  * CFormulaTable::m_afRate[100 + m_pPlayerRef->GetClass() + 1][2]));
	}
}

void CCharacterAttPC::CalculateLastCon(float fValue, BYTE byApplyType, bool bIsPlus)
{
	WORD wCurCon = m_pAttribute.lastCon;
	WORD wCurLpRegen = m_pAttribute.wLastLpRegen;

	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.lastCon = UnsignedSafeIncrease<WORD>(m_pAttribute.lastCon, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fCON += fValue;
		}

		m_pAttribute.lastMaxLp += int(float(m_pAttribute.lastCon - wCurCon) * CFormulaTable::m_afRate[200 + m_pPlayerRef->GetClass() + 1][2]);
		m_pAttribute.wLastLpRegen += WORD(float(m_pAttribute.lastCon - wCurCon) * CFormulaTable::m_afRate[300][2]);
		m_pAttribute.wLastLpSitdownRegen += WORD(float(m_pAttribute.wLastLpRegen - wCurLpRegen) * CFormulaTable::m_afRate[400][1]);

		m_pAttribute.wLastBlockRate += WORD(float(m_pAttribute.lastCon - wCurCon) * CFormulaTable::m_afRate[900][2]);
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.lastCon = UnsignedSafeDecrease<WORD>(m_pAttribute.lastCon, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fCONNegative -= fValue;
		}

		m_pAttribute.lastMaxLp = UnsignedSafeDecrease<WORD>(m_pAttribute.lastMaxLp, WORD(float(wCurCon - m_pAttribute.lastCon) * CFormulaTable::m_afRate[200 + m_pPlayerRef->GetClass() + 1][2]));
		m_pAttribute.wLastLpRegen = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastLpRegen, WORD(float(wCurCon - m_pAttribute.lastCon) * CFormulaTable::m_afRate[300][2]));
		m_pAttribute.wLastLpSitdownRegen = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastLpSitdownRegen, WORD(float(wCurLpRegen - m_pAttribute.wLastLpRegen) * CFormulaTable::m_afRate[400][1]));

		m_pAttribute.wLastBlockRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastBlockRate, WORD(float(wCurCon - m_pAttribute.lastCon) * CFormulaTable::m_afRate[900][2]));
	}
}

void CCharacterAttPC::CalculateLastFoc(float fValue, BYTE byApplyType, bool bIsPlus)
{
	WORD wCurFoc = m_pAttribute.lastFoc;

	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.lastFoc = UnsignedSafeIncrease<WORD>(m_pAttribute.lastFoc, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fFOC += fValue;
		}

		m_pAttribute.wLastAttackRate += WORD(float(m_pAttribute.lastFoc - wCurFoc) / CFormulaTable::m_afRate[600][1] * CFormulaTable::m_afRate[600][2]);
		m_pAttribute.wLastCurseSuccessRate += WORD(float(m_pAttribute.lastFoc - wCurFoc) / CFormulaTable::m_afRate[700][1] * CFormulaTable::m_afRate[700][2]);
		m_pAttribute.wLastEnergyCriticalRate += WORD(float(m_pAttribute.lastFoc - wCurFoc) / CFormulaTable::m_afRate[2100 + m_pPlayerRef->GetClass() + 1][2]);
		m_pAttribute.fLastEnergyCriticalDamageRate += float(m_pAttribute.lastFoc - wCurFoc) / CFormulaTable::m_afRate[2200 + m_pPlayerRef->GetClass() + 1][2];
		m_pAttribute.wLastEnergyOffence += WORD(float(m_pAttribute.lastFoc - wCurFoc) * CFormulaTable::m_afRate[1100 + m_pPlayerRef->GetClass() + 1][3]);
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.lastFoc = UnsignedSafeDecrease<WORD>(m_pAttribute.lastFoc, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fFOCNegative -= fValue;
		}

		m_pAttribute.wLastAttackRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastAttackRate, WORD(float(wCurFoc - m_pAttribute.lastFoc) / CFormulaTable::m_afRate[600][1] * CFormulaTable::m_afRate[600][2]));
		m_pAttribute.wLastCurseSuccessRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastCurseSuccessRate, WORD(float(wCurFoc - m_pAttribute.lastFoc) / CFormulaTable::m_afRate[700][1] * CFormulaTable::m_afRate[700][2]));
		m_pAttribute.wLastEnergyCriticalRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastEnergyCriticalRate, WORD(float(wCurFoc - m_pAttribute.lastFoc) / CFormulaTable::m_afRate[2100 + m_pPlayerRef->GetClass() + 1][2]));
		m_pAttribute.fLastEnergyCriticalDamageRate = UnsignedSafeDecrease<float>(m_pAttribute.fLastEnergyCriticalDamageRate, float(wCurFoc - m_pAttribute.lastFoc) / CFormulaTable::m_afRate[2200 + m_pPlayerRef->GetClass() + 1][2]);
		m_pAttribute.wLastEnergyOffence = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastEnergyOffence, WORD(float(wCurFoc - m_pAttribute.lastFoc) * CFormulaTable::m_afRate[1100 + m_pPlayerRef->GetClass() + 1][3]));
	}
}

void CCharacterAttPC::CalculateLastDex(float fValue, BYTE byApplyType, bool bIsPlus)
{
	WORD wCurDex = m_pAttribute.lastDex;

	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.lastDex = UnsignedSafeIncrease<WORD>(m_pAttribute.lastDex, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fDEX += fValue;
		}

		m_pAttribute.wLastDodgeRate += WORD(float(m_pAttribute.lastDex - wCurDex) / CFormulaTable::m_afRate[800][1] * CFormulaTable::m_afRate[800][2]);
		m_pAttribute.wLastCurseToleranceRate += WORD(float(m_pAttribute.lastDex - wCurDex) / CFormulaTable::m_afRate[1200][1] * CFormulaTable::m_afRate[1200][2]);
		m_pAttribute.wLastPhysicalCriticalRate += WORD(float(m_pAttribute.lastDex - wCurDex) / CFormulaTable::m_afRate[1900 + m_pPlayerRef->GetClass() + 1][2]);
		m_pAttribute.fLastPhysicalCriticalDamageRate += float(m_pAttribute.lastDex - wCurDex) / CFormulaTable::m_afRate[2000 + m_pPlayerRef->GetClass() + 1][2];
		m_pAttribute.wLastPhysicalOffence += WORD(float(m_pAttribute.lastDex - wCurDex)  * CFormulaTable::m_afRate[100 + m_pPlayerRef->GetClass() + 1][3]);
		m_pAttribute.wLastBlockRate += WORD(float(m_pAttribute.lastDex - wCurDex)  * CFormulaTable::m_afRate[900][2]);
		m_pAttribute.wGuardRateLast += WORD(float(m_pAttribute.lastDex - wCurDex)  * CFormulaTable::m_afRate[1000][2]);
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.lastDex = UnsignedSafeDecrease<WORD>(m_pAttribute.lastDex, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fDEXNegative -= fValue;
		}

		m_pAttribute.wLastDodgeRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastDodgeRate, WORD(float(wCurDex - m_pAttribute.lastDex) / CFormulaTable::m_afRate[800][1] * CFormulaTable::m_afRate[800][2]));
		m_pAttribute.wLastCurseToleranceRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastCurseToleranceRate, WORD(float(wCurDex - m_pAttribute.lastDex) / CFormulaTable::m_afRate[1200][1] * CFormulaTable::m_afRate[1200][2]));
		m_pAttribute.wLastPhysicalCriticalRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastPhysicalCriticalRate, WORD(float(wCurDex - m_pAttribute.lastDex) / CFormulaTable::m_afRate[1900 + m_pPlayerRef->GetClass() + 1][2]));
		m_pAttribute.fLastPhysicalCriticalDamageRate = UnsignedSafeDecrease<float>(m_pAttribute.fLastPhysicalCriticalDamageRate, float(wCurDex - m_pAttribute.lastDex) / CFormulaTable::m_afRate[2000 + m_pPlayerRef->GetClass() + 1][2]);
		m_pAttribute.wLastPhysicalOffence = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastPhysicalOffence, WORD(float(wCurDex - m_pAttribute.lastDex) / CFormulaTable::m_afRate[100 + m_pPlayerRef->GetClass() + 1][3]));
		m_pAttribute.wLastBlockRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastBlockRate, WORD(float(wCurDex - m_pAttribute.lastDex) * CFormulaTable::m_afRate[900][2]));
		m_pAttribute.wGuardRateLast = UnsignedSafeDecrease<WORD>(m_pAttribute.wGuardRateLast, WORD(float(wCurDex - m_pAttribute.lastDex) * CFormulaTable::m_afRate[1000][2]));
	}
}

void CCharacterAttPC::CalculateLastSol(float fValue, BYTE byApplyType, bool bIsPlus)
{
	WORD wCurSol = m_pAttribute.lastSol;

	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.lastSol = UnsignedSafeIncrease<WORD>(m_pAttribute.lastSol, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fSOL += fValue;
		}

		m_pAttribute.wLastEnergyOffence += WORD(float(m_pAttribute.lastSol - wCurSol) * CFormulaTable::m_afRate[1100 + m_pPlayerRef->GetClass() + 1][2]);
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.lastSol = UnsignedSafeDecrease<WORD>(m_pAttribute.lastSol, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fSOLNegative -= fValue;
		}

		m_pAttribute.wLastEnergyOffence = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastEnergyOffence, WORD(float(wCurSol - m_pAttribute.lastSol) * CFormulaTable::m_afRate[1100 + m_pPlayerRef->GetClass() + 1][2]));
	}
}

void CCharacterAttPC::CalculateLastEng(float fValue, BYTE byApplyType, bool bIsPlus)
{
	WORD wCurEng = m_pAttribute.lastEng;
	WORD wCurEpRegen = m_pAttribute.wLastEpRegen;

	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.lastEng = UnsignedSafeIncrease<WORD>(m_pAttribute.lastEng, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fENG += fValue;
		}

		m_pAttribute.wLastMaxEP += (WORD)((float)(m_pAttribute.lastEng - wCurEng) * CFormulaTable::m_afRate[1300 + m_pPlayerRef->GetClass() + 1][2]);
		m_pAttribute.wLastEpRegen += (WORD)((float)(m_pAttribute.lastEng - wCurEng) * CFormulaTable::m_afRate[1400][2]);
		m_pAttribute.wLastEpSitdownRegen += (WORD)(float(m_pAttribute.wLastEpRegen - wCurEpRegen) * CFormulaTable::m_afRate[1500][1]);
		m_pAttribute.wLastEpBattleRegen += (WORD)((float)(m_pAttribute.wLastEpRegen - wCurEpRegen) * CFormulaTable::m_afRate[1600][1]);
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.lastEng = UnsignedSafeDecrease<WORD>(m_pAttribute.lastEng, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fENGNegative -= fValue;
		}

		m_pAttribute.wLastMaxEP = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastMaxEP, WORD(float(wCurEng - m_pAttribute.lastEng) * CFormulaTable::m_afRate[1300 + m_pPlayerRef->GetClass() + 1][2]));
		m_pAttribute.wLastEpRegen = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastEpRegen, WORD(float(wCurEng - m_pAttribute.lastEng) * CFormulaTable::m_afRate[1400][2]));
		m_pAttribute.wLastEpSitdownRegen = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastEpSitdownRegen, WORD(float(wCurEpRegen - m_pAttribute.wLastEpRegen) * CFormulaTable::m_afRate[1500][1]));
		m_pAttribute.wLastEpBattleRegen = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastEpBattleRegen, WORD(float(wCurEpRegen - m_pAttribute.wLastEpRegen) * CFormulaTable::m_afRate[1600][1]));
	}
}

void CCharacterAttPC::CalculateBattleAttribute(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CCharacterAtt::CalculateBattleAttribute(fValue, byApplyType, bIsPlus);

	/*switch (m_pPlayerRef->GetClass())
	{
		case PC_CLASS_HUMAN_FIGHTER:
		case PC_CLASS_HUMAN_MYSTIC:
		case PC_CLASS_HUMAN_ENGINEER:
		case PC_CLASS_NAMEK_FIGHTER:
		case PC_CLASS_NAMEK_MYSTIC:
		case PC_CLASS_MIGHTY_MAJIN:
		case PC_CLASS_WONDER_MAJIN:

		case PC_CLASS_GUN_MANIA:
		case PC_CLASS_MECH_MANIA:
		case PC_CLASS_DARK_WARRIOR:

		case PC_CLASS_DENDEN_HEALER:
		case PC_CLASS_POCO_SUMMONER:
		case PC_CLASS_ULTI_MA:
		case PC_CLASS_GRAND_MA:
		case PC_CLASS_PLAS_MA:
		case PC_CLASS_KAR_MA:
		{
			CCharacterAtt::CalculateBattleAttribute(fValue, byApplyType, bIsPlus);
		}
		break;

		case PC_CLASS_STREET_FIGHTER:
		case PC_CLASS_SWORD_MASTER:
		case PC_CLASS_SHADOW_KNIGHT:
		{
			CCharacterAtt::CalculateBattleAttribute(fValue * 0.5f, byApplyType, bIsPlus);
		}
		break;

		case PC_CLASS_CRANE_ROSHI:
		case PC_CLASS_TURTLE_ROSHI:
		{
			CalculateLastHonestOffence(fValue, byApplyType, bIsPlus);
			CalculateLastStrangeOffence(fValue, byApplyType, bIsPlus);
			CalculateLastWildOffence(fValue, byApplyType, bIsPlus);
			CalculateLastEleganceOffence(fValue, byApplyType, bIsPlus);
			CalculateLastFunnyOffence(fValue, byApplyType, bIsPlus);
			CalculateLastHonestDefence(fValue * 0.5f, byApplyType, bIsPlus);
			CalculateLastStrangeDefence(fValue * 0.5f, byApplyType, bIsPlus);
			CalculateLastWildDefence(fValue * 0.5f, byApplyType, bIsPlus);
			CalculateLastEleganceDefence(fValue * 0.5f, byApplyType, bIsPlus);
			CalculateLastFunnyDefence(fValue * 0.5f, byApplyType, bIsPlus);
		}
		break;
	}*/
}


void CCharacterAttPC::CalcSecondWeaponOffence()
{
	m_wSubWeaponPhysicalOffence = m_pAttribute.wLastPhysicalOffence;
	m_wSubWeaponEnergyOffence = m_pAttribute.wLastEnergyOffence;

	//remove main weapon offence
	CItem* pMainWeapon = m_pPlayerRef->GetPlayerItemContainer()->GetItem(CONTAINER_TYPE_EQUIP, EQUIP_SLOT_TYPE_HAND);
	if (pMainWeapon)
	{
		m_wSubWeaponPhysicalOffence = UnsignedSafeDecrease<WORD>(m_wSubWeaponPhysicalOffence, Dbo_GetFinalOffence(pMainWeapon->GetTbldat()->wPhysical_Offence, pMainWeapon->GetGrade()));
		m_wSubWeaponEnergyOffence = UnsignedSafeDecrease<WORD>(m_wSubWeaponEnergyOffence, Dbo_GetFinalOffence(pMainWeapon->GetTbldat()->wEnergy_Offence, pMainWeapon->GetGrade()));
	}

	//add sub weapon offence
	CItem* pWeapon = m_pPlayerRef->GetPlayerItemContainer()->GetItem(CONTAINER_TYPE_EQUIP, EQUIP_SLOT_TYPE_SUB_WEAPON);
	if (pWeapon)
	{
		m_wSubWeaponPhysicalOffence = UnsignedSafeIncrease<WORD>(m_wSubWeaponPhysicalOffence, Dbo_GetFinalOffence(pWeapon->GetTbldat()->wPhysical_Offence, pWeapon->GetGrade()));
		m_wSubWeaponEnergyOffence = UnsignedSafeIncrease<WORD>(m_wSubWeaponEnergyOffence, Dbo_GetFinalOffence(pWeapon->GetTbldat()->wEnergy_Offence, pWeapon->GetGrade()));
	}
}
