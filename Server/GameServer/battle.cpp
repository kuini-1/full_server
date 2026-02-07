#include "stdafx.h"
#include "battle.h"
#include "calcs.h"
#include "Npc.h"
#include "CPlayer.h"
#include "FormulaTable.h"
#include "PlayerItemContainer.h"
#include "SystemEffectTable.h"
#include "TableContainerManager.h"
#include "SummonPet.h"


float GetCcFocReductionLimit(eSYSTEM_EFFECT_CODE* abyEffectCode)
{
	for (int i = 0; i < NTL_MAX_EFFECT_IN_SKILL; i++)
	{
		switch (abyEffectCode[i])
		{
			case ACTIVE_SLEEP: return 40.0f;
			case ACTIVE_PARALYZE: return 30.f;
			case ACTIVE_STUN: return 15.f;
			case ACTIVE_WARP_STUN: return 25.f;
			case ACTIVE_SKILL_INABILITY: return 35.f;
			case ACTIVE_CONFUSE: return 30.f;
			case ACTIVE_TERROR: return 30.f;
			case ACTIVE_FREEZE: return 35.f;
			case ACTIVE_STONE: return 30.f;
			case ACTIVE_CANDY: return 30.f;
		}
	}

	return 40.0f;
}

bool BattleIsCrit(CCharacterAtt* pAttackerAtt, CCharacterAtt* pTargetAtt, bool bIsPhysical)
{
	float fRate = 0.0f;
	float fVar;
	float fVar2;
	float fCritRate;

	if (bIsPhysical == false)
	{
		fCritRate = (float)pAttackerAtt->GetLastEnergyCriticalRate();

		if (pAttackerAtt->GetLastFoc() < 170)
			fVar = 37.f;
		else
			fVar = 21.f;
			

		fVar2 = (float)pAttackerAtt->GetLastFoc();
	}
	else
	{
		fCritRate = (float)pAttackerAtt->GetLastPhysicalCriticalRate();

		if (pAttackerAtt->GetLastDex() < 220)
			fVar = 40.f;
		else
			fVar = 28.f;

		fVar2 = (float)pAttackerAtt->GetLastDex();
	}

	fRate = fCritRate - (fCritRate * (pTargetAtt->GetLastCriticalBlockSuccessRate() / 100.f)) + fVar2 / fVar;

	//if (0.0f >= fCritBlockRate)
	//{
	//	fRate = fCritRate;
	//}
	//else
	//{
	//	fRate = fCritRate - (fCritRate * (fCritBlockRate / 100.f));
	//}

	//NTL_PRINT(PRINT_APP, "fRate = %f, fVar = %f, fVar2 = %f, fCritRate = %f, CritBlockRate = %f", fRate, fVar, fVar2, fCritRate, pTargetAtt->GetLastCriticalBlockSuccessRate());

	if (fRate > 90.0f)
		fRate = 90.0f;

	return Dbo_CheckProbabilityF(fRate);
}

//--------------------------------------------------------------------------------------//
//		IS DODGED?
//--------------------------------------------------------------------------------------//
bool BattleIsDodge(bool bTargetPC, WORD hitrate, WORD dodge, BYTE byAttackerLv, BYTE byTargetLv)
{
	float fRate = 100.0f - (CFormulaTable::m_afRate[3700][1] * float((float)hitrate / (float)MAX(hitrate + dodge, 1)) * float(float(byAttackerLv + 1) / float(byAttackerLv + byTargetLv)) * 100.0f);

	if (fRate > 90.f)
		fRate = 90.0f;

	//if(bTargetPC)
	//	NTL_PRINT(PRINT_APP, "BattleIsDodge: dodge percent %f, hitrate %u, dodge %u, byAttackerLv %u, byTargetLv %u \n", fRate, hitrate, dodge, byAttackerLv, byTargetLv);

	return Dbo_CheckProbabilityF(fRate);
}


//--------------------------------------------------------------------------------------//
//		IS RESIST CURSE?
//--------------------------------------------------------------------------------------//
bool BattleIsResist(WORD wSuccessRate, WORD wResistRate, BYTE byAttackerLv, BYTE byTargetLv)
{
	float fRate = 100.0f - (CFormulaTable::m_afRate[3900][1] * float((float)wSuccessRate / (float)MAX(wSuccessRate + wResistRate, 1)) * float(float(byAttackerLv + 1) / float(byAttackerLv + byTargetLv)) * 100.0f);

	if (fRate > 90.f)
		fRate = 90.f;

	//NTL_PRINT(PRINT_APP, "BattleIsResist: curse resist percent %f, wSuccessRate %u, wResistRate %u, byAttackerLv %u, byTargetLv %u \n", fRate, wSuccessRate, wResistRate, byAttackerLv, byTargetLv);

	return Dbo_CheckProbabilityF(fRate);
}

bool BattleIsBlock(WORD wDefenceRate, BYTE byAttackerLv, BYTE byTargetLv)
{
	//float fRate = ((float)wDefenceRate - ((float)byTargetLv / ((float)byAttackerLv * 0.13f)) + 0.02f) / 3.5f;
	float fRate = ((float)wDefenceRate * 2.f + (byTargetLv - byAttackerLv)) / 200.f;
	if (fRate > 20.f)
		fRate = 20.f;

	//NTL_PRINT(PRINT_APP, "fRate:%f, wDefenceRate:%u, byAttackerLv:%u, byTargetLv:%u", fRate, wDefenceRate, byAttackerLv, byTargetLv);

	return Dbo_CheckProbabilityF(fRate);
}

//--------------------------------------------------------------------------------------//
//		IS IN RANGE?
//--------------------------------------------------------------------------------------//
bool BattleIsInRange(CCharacterAtt* pAttackerAtt, CCharacterAtt* pTargetAtt)
{
	
	return false;
	
}


//-----------------------------------------------------------------------------------------------------------//
//											CALCULATE SKILL DAMAGE
//-----------------------------------------------------------------------------------------------------------//

//--------------------------------------------------------------------------------------//
//		
//--------------------------------------------------------------------------------------//
void CalcSkillDamage(CCharacterObject* pCaster, CCharacterObject* victim, sSKILL_TBLDAT* skilltbl, BYTE byEffectNr, float fBaseSkillDmg, float& resultvalue, BYTE& rAttackResult, int& rfReflectDmg, sDBO_LP_EP_RECOVERED* pLpEpRecover, bool bIncreaseDmg/* = false*/, bool bAttackFromBehindBonus/* = false*/)
{
	float  fFinalDamage = 0.0f, min_damage = 0.0f, max_damage = 0.0f, fAttackerPower = 0.0f, fTargetDefensePower = 0.0f, fCritDmgRate = 0.0f;

	/* INFO:
		- At "value" damage, we do not add weapon offence. See https://youtu.be/cj5E1dOIYfk?t=155 as proof. With weapon offence added we deal much more dmg. Without we deal exact the same damage. State needs to be figured out.
		- State calculation is / 1.5 instead of 2.0. Watch https://youtu.be/JlBDzAmoNTk?t=317 for more info.
	*/

	CCharacterAtt* pCasterAtt = pCaster->GetCharAtt();

	//printf("skilltbl->bySkill_Type %u, skilltbl->bySkill_Effect_Type[byEffectNr] %u \n", skilltbl->bySkill_Type, skilltbl->bySkill_Effect_Type[byEffectNr]);
	if (skilltbl->bySkill_Effect_Type[byEffectNr] == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
	{
		if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_PHYSICAL)
		{
			fAttackerPower = fBaseSkillDmg;

			fTargetDefensePower = (float)victim->GetCharAtt()->GetLastPhysicalDefence();
		}
		else if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_ENERGY)
		{
			fAttackerPower = fBaseSkillDmg;

			fTargetDefensePower = (float)victim->GetCharAtt()->GetLastEnergyDefence();
		}
		else if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_STATE)
		{
			if (pCaster->IsPC() && skilltbl->byRequire_Epuip_Slot_Type == EQUIP_SLOT_TYPE_SUB_WEAPON)
				fAttackerPower = fBaseSkillDmg + float(pCasterAtt->GetSubWeaponPhysicalOffence() + pCasterAtt->GetSubWeaponEnergyOffence() / 1.5f);
			else
				fAttackerPower = fBaseSkillDmg + float(pCasterAtt->GetLastPhysicalOffence() + pCasterAtt->GetSubWeaponEnergyOffence() / 1.5f);

			fTargetDefensePower = float(victim->GetCharAtt()->GetLastPhysicalDefence() + victim->GetCharAtt()->GetLastEnergyOffence() / 1.5f);
		}
	}
	else if (skilltbl->bySkill_Effect_Type[byEffectNr] == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
	{
		if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_PHYSICAL)
		{
			if (pCaster->IsPC() && skilltbl->byRequire_Epuip_Slot_Type == EQUIP_SLOT_TYPE_SUB_WEAPON)
				fAttackerPower = (float)pCasterAtt->GetSubWeaponPhysicalOffence() + (((float)pCasterAtt->GetSubWeaponPhysicalOffence() * fBaseSkillDmg) / 100.f);
			else
				fAttackerPower = (float)pCasterAtt->GetLastPhysicalOffence() + (((float)pCasterAtt->GetLastPhysicalOffence() * fBaseSkillDmg) / 100.f);

			fTargetDefensePower = (float)victim->GetCharAtt()->GetLastPhysicalDefence();
		}
		else if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_ENERGY)
		{
			if (pCaster->IsPC() && skilltbl->byRequire_Epuip_Slot_Type == EQUIP_SLOT_TYPE_SUB_WEAPON)
				fAttackerPower = (float)pCasterAtt->GetSubWeaponEnergyOffence() + (((float)pCasterAtt->GetSubWeaponEnergyOffence() * fBaseSkillDmg) / 100.f);
			else
				fAttackerPower = (float)pCasterAtt->GetLastEnergyOffence() + (((float)pCasterAtt->GetLastEnergyOffence() * fBaseSkillDmg) / 100.f);

			fTargetDefensePower = (float)victim->GetCharAtt()->GetLastEnergyDefence();
		}
		else if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_STATE)
		{
			float fStateOffence = 0.f;
			if (pCaster->IsPC() && skilltbl->byRequire_Epuip_Slot_Type == EQUIP_SLOT_TYPE_SUB_WEAPON)
				fStateOffence = ((float)pCasterAtt->GetSubWeaponPhysicalOffence() + (float)pCasterAtt->GetSubWeaponEnergyOffence()) / 1.5f;
			else
				fStateOffence = ((float)pCasterAtt->GetLastPhysicalOffence() + (float)pCasterAtt->GetLastEnergyOffence()) / 1.5f;

			fAttackerPower = (fStateOffence * fBaseSkillDmg) / 100.f;
			fTargetDefensePower = (float)victim->GetCharAtt()->GetLastPhysicalDefence() + (float)victim->GetCharAtt()->GetLastEnergyDefence() / 1.5f;
		}
	}

	float fAttributeBonusRate = 0.0f;
	
	//add item attribute
	if (pCaster->IsPC() && skilltbl->byRequire_Epuip_Slot_Type == EQUIP_SLOT_TYPE_SUB_WEAPON)
		fAttributeBonusRate = GetAttributeBonusRate(victim->IsPC(), true, 0, victim->GetCharAtt()->GetBattleAttributeDefence(), pCasterAtt->GetBattleAttributeOffenceSub(), pCasterAtt->GetAvatarAttribute(), victim->GetCharAtt()->GetAvatarAttribute());
	else
		fAttributeBonusRate = GetAttributeBonusRate(victim->IsPC(), false, pCasterAtt->GetBattleAttributeOffence(), victim->GetCharAtt()->GetBattleAttributeDefence(), 0, pCasterAtt->GetAvatarAttribute(), victim->GetCharAtt()->GetAvatarAttribute());


	float fDmg0 = fAttackerPower * (1.0f - (fTargetDefensePower / (fTargetDefensePower + (float)pCaster->GetLevel() * CFormulaTable::m_afRate[3100][1])));
	float fDmg1 = fDmg0 * (CFormulaTable::m_afRate[3200][1] + fAttributeBonusRate);

	min_damage = fDmg1 * (CFormulaTable::m_afRate[3500][1] + ((float)pCaster->GetLevel() * CFormulaTable::m_afRate[3500][2]));
	max_damage = fDmg1 * (CFormulaTable::m_afRate[3500][3] - ((float)pCaster->GetLevel() * CFormulaTable::m_afRate[3500][4]));

	fFinalDamage = RandomRangeF(min_damage, max_damage);

	if (pCaster->IsPC() && victim->IsMonster())
	{
		//if (victim->GetTbldat()->dwBasic_LP >= 20000)
		if (pCaster->GetCurWorld()->GetIdx() == 199000) fFinalDamage *= 0.5; 
		else fFinalDamage *= 2;
	}
	if (victim->IsPC() && pCaster->IsMonster())
	{
		if (victim->GetCurWorld()->GetIdx() == 199000)
		{
			switch (victim->GetTblidx())
			{
				case 37122101: case 33281101: fFinalDamage *= 6; break;
				default: fFinalDamage *= 10; break;
			}
		}
	}

	/*
	float casterDmgBonus = 0.0f;
	float victimDmgReduction = 0.0f;
	float scaleDmg = 0.0f;

	if (pCaster->IsPC())
	{
		CPlayer* pPlayer = g_pObjectManager->GetPC(pCaster->GetID());

		for (BYTE ITEM_POS = EQUIP_SLOT_TYPE_HAND; ITEM_POS < EQUIP_SLOT_TYPE_SUB_WEAPON + 1; ITEM_POS++)
		{
			CItem* item = pPlayer->GetPlayerItemContainer()->GetItem(CONTAINER_TYPE_EQUIP, ITEM_POS);
			if (item)
			{
				switch (ITEM_POS)
				{
				case EQUIP_SLOT_TYPE_HAND:
				{
					switch (item->GetTblidx())
					{
					case 0000: { casterDmgBonus += 10.0f; } break;
					}
				}
				break;
				case EQUIP_SLOT_TYPE_SUB_WEAPON:
				{
					switch (item->GetTblidx())
					{
					case 0000: { casterDmgBonus += 10.0f; } break;
					}
				}
				break;
				}
			}
		}
	}

	if (victim->IsPC())
	{
		CPlayer* pPlayer = g_pObjectManager->GetPC(victim->GetID());

		for (BYTE ITEM_POS = EQUIP_SLOT_TYPE_JACKET; ITEM_POS < EQUIP_SLOT_TYPE_BOOTS + 1; ITEM_POS++)
		{
			CItem* item = pPlayer->GetPlayerItemContainer()->GetItem(CONTAINER_TYPE_EQUIP, ITEM_POS);
			if (item)
			{
				switch (ITEM_POS)
				{
				case EQUIP_SLOT_TYPE_JACKET:
				{
					switch (item->GetTblidx())
					{
					case 0000: { victimDmgReduction += 10.0f; } break;
					}
				}
				break;
				case EQUIP_SLOT_TYPE_PANTS:
				{
					switch (item->GetTblidx())
					{
					case 0000: { victimDmgReduction += 10.0f; } break;
					}
				}
				break;
				case EQUIP_SLOT_TYPE_BOOTS:
				{
					switch (item->GetTblidx())
					{
					case 0000: { victimDmgReduction += 10.0f; } break;
					}
				}
				break;
				}
			}
		}
	}

	scaleDmg = casterDmgBonus - victimDmgReduction;

	fFinalDamage += scaleDmg;
	*/



	//printf("fDmg0 %f fDmg1 %f fAttributeBonusRate %f, min_damage %f, max_damage %f\n", fDmg0, fDmg1, fAttributeBonusRate, min_damage, max_damage);
	resultvalue = (fFinalDamage <= 1.f) ? 1.f : fFinalDamage;

	/*
	if (victim->IsPC() && pCaster->IsPC())
	{
		CPlayer* cVictim = g_pObjectManager->GetPC(victim->GetID());
		CPlayer* cCaster = g_pObjectManager->GetPC(pCaster->GetID());

		if (cVictim->GetClass() == PC_CLASS_ULTI_MA)
		{
			sSKILL_TBLDAT* pTempSkillTbldat;
			if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_ENERGY)
				pTempSkillTbldat = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData(2385);
			else
				pTempSkillTbldat = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData(20031);

			sDBO_BUFF_PARAMETER aBuffParameter[NTL_MAX_EFFECT_IN_SKILL];
			eSYSTEM_EFFECT_CODE aeEffectCode[NTL_MAX_EFFECT_IN_SKILL];
			aeEffectCode[0] = g_pTableContainer->GetSystemEffectTable()->GetEffectCodeWithTblidx(pTempSkillTbldat->skill_Effect[0]);
			aeEffectCode[1] = INVALID_SYSTEM_EFFECT_CODE;

			aBuffParameter[0].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_DEFAULT;
			aBuffParameter[0].buffParameter.fParameter = 0;
			aBuffParameter[0].buffParameter.dwRemainValue = 0;

			DWORD dwDurationInMs = 10 * 1000;

			if (!cVictim->GetBuffManager()->FindBuff(pTempSkillTbldat->tblidx, 0))
				cVictim->GetBuffManager()->RegisterBuff(dwDurationInMs, aeEffectCode, aBuffParameter, INVALID_HOBJECT, BUFF_TYPE_BLESS, pTempSkillTbldat);
		}

		if (cVictim->GetClass() == PC_CLASS_POCO_SUMMONER)
		{
			if (cVictim->GetCurrentPetId() != INVALID_HOBJECT)
			{
				if (cVictim->GetCurLpInPercent() <= 20.f)
				{
					CSummonPet* pet = NULL;
					pet = g_pObjectManager->GetSummonPet(cVictim->GetCurrentPetId());
					if (pet) pet->Despawn();

					int heal = (int)cVictim->GetLastMaxLP() * 33.f / 100.f;
					cVictim->UpdateCurLP(heal, true, false);

					sSKILL_TBLDAT* pTempSkillTbldat = NULL;
					pTempSkillTbldat = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData(2385);

					if (pTempSkillTbldat)
					{
						sDBO_BUFF_PARAMETER aBuffParameter[NTL_MAX_EFFECT_IN_SKILL];
						eSYSTEM_EFFECT_CODE aeEffectCode[NTL_MAX_EFFECT_IN_SKILL];
						aeEffectCode[0] = g_pTableContainer->GetSystemEffectTable()->GetEffectCodeWithTblidx(pTempSkillTbldat->skill_Effect[0]);
						aeEffectCode[1] = INVALID_SYSTEM_EFFECT_CODE;

						aBuffParameter[0].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_DEFAULT;
						aBuffParameter[0].buffParameter.fParameter = 0;
						aBuffParameter[0].buffParameter.dwRemainValue = 0;

						DWORD dwDurationInMs = 2 * 1000;

						if (!cVictim->GetBuffManager()->FindBuff(pTempSkillTbldat->tblidx, 0))
							cVictim->GetBuffManager()->RegisterBuff(dwDurationInMs, aeEffectCode, aBuffParameter, INVALID_HOBJECT, BUFF_TYPE_BLESS, pTempSkillTbldat);
					}
				}
			}
		}

		if (cVictim->GetClass() == PC_CLASS_CRANE_ROSHI)
		{
			sSKILL_TBLDAT* pTempSkillTbldat;
			pTempSkillTbldat = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData(2385);

			sDBO_BUFF_PARAMETER aBuffParameter[NTL_MAX_EFFECT_IN_SKILL];
			eSYSTEM_EFFECT_CODE aeEffectCode[NTL_MAX_EFFECT_IN_SKILL];
			aeEffectCode[0] = g_pTableContainer->GetSystemEffectTable()->GetEffectCodeWithTblidx(pTempSkillTbldat->skill_Effect[0]);
			aeEffectCode[1] = INVALID_SYSTEM_EFFECT_CODE;

			aBuffParameter[0].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_DOT;
			aBuffParameter[0].buffParameter.dwRemainValue = 0;

			DWORD dwDurationInMs = 3 * 1000;

			if (!cVictim->GetBuffManager()->FindBuff(pTempSkillTbldat->tblidx, 0))
				cVictim->GetBuffManager()->RegisterBuff(dwDurationInMs, aeEffectCode, aBuffParameter, INVALID_HOBJECT, BUFF_TYPE_BLESS, pTempSkillTbldat);
		}
	}
	*/
	
	

	//---------------//

	//check if crit success and add damage
	if (rAttackResult == BATTLE_ATTACK_RESULT_CRITICAL_HIT)		//only add crit dmg once
	{
		if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_PHYSICAL) //check if physical dmg
		{
			fCritDmgRate = pCasterAtt->GetLastPhysicalCriticalDamageRate();
		}
		else if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_ENERGY) //check if energy dmg
		{
			fCritDmgRate = pCasterAtt->GetLastEnergyCriticalDamageRate();
		}
		else if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_STATE)
		{
			fCritDmgRate = (pCasterAtt->GetLastPhysicalCriticalDamageRate() + pCasterAtt->GetLastEnergyCriticalDamageRate()) / 2.f;
		}

		resultvalue += ((resultvalue * fCritDmgRate) / 100.f);

		if (bIncreaseDmg)
		{
			float fBonus = 0.0f;
			if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_PHYSICAL) //check if physical dmg
			{
				fBonus = ((float)pCasterAtt->GetLastStr() * 0.07f) + ((float)pCasterAtt->GetLastDex() * 0.07f);

				if (fBonus > 50.f)
					fBonus = 50.f;
			}
			else if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_ENERGY) //check if energy dmg
			{
				fBonus = ((float)pCasterAtt->GetLastSol() * 0.10f) + ((float)pCasterAtt->GetLastFoc() * 0.10f);

				if (fBonus > 80.f)
					fBonus = 80.f;
			}
			//NTL_PRINT(PRINT_APP, "resultvalue: %f, fBonusRate %f", resultvalue, fBonus);
			resultvalue += ((resultvalue * fBonus) / 100.f);
			//NTL_PRINT(PRINT_APP, "new resultvalue: %f", resultvalue);
		}
	}

	if (bAttackFromBehindBonus)
	{
		float fBonus = 0.0f;
		if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_PHYSICAL) //check if physical dmg
		{
			fBonus = ((float)pCasterAtt->GetLastStr() * 0.01f) + ((float)pCasterAtt->GetLastDex() * 0.01f);

			if (fBonus > 50.f)
				fBonus = 50.f;
		}
		else if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_ENERGY) //check if energy dmg
		{
			fBonus = ((float)pCasterAtt->GetLastSol() * 0.07f) + ((float)pCasterAtt->GetLastFoc() * 0.07f);

			if (fBonus > 50.f)
				fBonus = 50.f;
		}
	//	NTL_PRINT(PRINT_APP, "back: resultvalue: %f, fBonusRate %f", resultvalue, fBonus);
		resultvalue += ((resultvalue * fBonus) / 100.f);
	//	NTL_PRINT(PRINT_APP, "back: new resultvalue: %f", resultvalue);
	}

	//---------------//
	//reflect dmg
	rfReflectDmg += (int)GetSkillReflectDamage(resultvalue, skilltbl->bySkill_Type, victim->GetCharAtt()->GetLastPhysicalReflection(), victim->GetCharAtt()->GetLastEnergyReflection());

	//---------------//
	if (pLpEpRecover)
	{
		//lp ep recover
		pLpEpRecover->targetLpRecoveredWhenHit = (int)(victim->GetCharAtt()->GetLastLpRecoveryWhenHit() + (resultvalue * victim->GetCharAtt()->GetLastLpRecoveryWhenHitInPercent() / 100.0f));
		if (pLpEpRecover->targetLpRecoveredWhenHit > 0)
			pLpEpRecover->bIsLpRecoveredWhenHit = true;

		//printf("resultvalue %f rLpEpRecover.targetLpRecoveredWhenHit %u, GetLastLpRecoveryWhenHitInPercent %f, GetLastLpRecoveryWhenHit %u, %f \n", 
		//	resultvalue, rLpEpRecover.targetLpRecoveredWhenHit, victim->GetCharAtt()->GetLastLpRecoveryWhenHitInPercent(), victim->GetCharAtt()->GetLastLpRecoveryWhenHit(), resultvalue * victim->GetCharAtt()->GetLastLpRecoveryWhenHitInPercent() / 100.0f);

		pLpEpRecover->dwTargetEpRecoveredWhenHit = (DWORD)(victim->GetCharAtt()->GetLastEpRecoveryWhenHit() + (resultvalue * victim->GetCharAtt()->GetLastEpRecoveryWhenHitInPercent() / 100.0f));
		if (pLpEpRecover->dwTargetEpRecoveredWhenHit > 0)
			pLpEpRecover->bIsEpRecoveredWhenHit = true;
	}

	//ERR_LOG(LOG_USER,"Attacker Lv: %d Victim Lv %d Victim Obj Type %d CalcSkillDamage: %f \n", ch->GetLevel(), victim->GetLevel(), victim->GetObjType(), resultvalue);
}


//--------------------------------------------------------------------------------------//
//		
//--------------------------------------------------------------------------------------//
void CalcSpecialSkillDamage(CCharacterObject* pCaster, CCharacterObject* victim, sSKILL_TBLDAT* skilltbl, BYTE byEffectNr, float fBaseSkillDmg, float& resultvalue, BYTE& rAttackResult, int& rfReflectDmg, sDBO_LP_EP_RECOVERED& rLpEpRecover)
{
	float  fFinalDamage = 0.0f, min_damage = 0.0f, max_damage = 0.0f, fAttackerPower = 0.0f, fTargetDefensePower = 0.0f;
	float fCritDmgRate = 0.0f;

	if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_PHYSICAL)
	{
		if (pCaster->IsPC() && skilltbl->byRequire_Epuip_Slot_Type == EQUIP_SLOT_TYPE_SUB_WEAPON)
			fAttackerPower = ((float)pCaster->GetCharAtt()->GetSubWeaponPhysicalOffence() * (fBaseSkillDmg / 2.f)) / 100.f;
		else
			fAttackerPower = ((float)pCaster->GetCharAtt()->GetLastPhysicalOffence() * (fBaseSkillDmg / 2.f)) / 100.f;

		fTargetDefensePower = (float)victim->GetCharAtt()->GetLastPhysicalDefence();
	}
	else if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_ENERGY)
	{
		if (pCaster->IsPC() && skilltbl->byRequire_Epuip_Slot_Type == EQUIP_SLOT_TYPE_SUB_WEAPON)
			fAttackerPower = ((float)pCaster->GetCharAtt()->GetSubWeaponEnergyOffence() * (fBaseSkillDmg / 2.f)) / 100.f;
		else
			fAttackerPower = ((float)pCaster->GetCharAtt()->GetLastEnergyOffence() * (fBaseSkillDmg / 2.f)) / 100.f;

		fTargetDefensePower = (float)victim->GetCharAtt()->GetLastEnergyDefence();
	}
	else if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_STATE)
	{
		float fStateOffence = 0.f;
		if (pCaster->IsPC() && skilltbl->byRequire_Epuip_Slot_Type == EQUIP_SLOT_TYPE_SUB_WEAPON)
			fStateOffence = ((float)pCaster->GetCharAtt()->GetSubWeaponPhysicalOffence() + (float)pCaster->GetCharAtt()->GetSubWeaponEnergyOffence()) / 2.f;
		else
			fStateOffence = ((float)pCaster->GetCharAtt()->GetLastPhysicalOffence() + (float)pCaster->GetCharAtt()->GetLastEnergyOffence()) / 2.f;

		fAttackerPower = (fStateOffence * fBaseSkillDmg) / 100.f;
		fTargetDefensePower = (float)victim->GetCharAtt()->GetLastPhysicalDefence() + (float)victim->GetCharAtt()->GetLastEnergyDefence() / 2.f;
	}

	float fAttributeBonusRate = 0.0f;

	//add item attribute
	if (pCaster->IsPC() && skilltbl->byRequire_Epuip_Slot_Type == EQUIP_SLOT_TYPE_SUB_WEAPON)
		fAttributeBonusRate = GetAttributeBonusRate(victim->IsPC(), true, 0, victim->GetCharAtt()->GetBattleAttributeDefence(), pCaster->GetCharAtt()->GetBattleAttributeOffenceSub(), pCaster->GetCharAtt()->GetAvatarAttribute(), victim->GetCharAtt()->GetAvatarAttribute());
	else
		fAttributeBonusRate = GetAttributeBonusRate(victim->IsPC(), false, pCaster->GetCharAtt()->GetBattleAttributeOffence(), victim->GetCharAtt()->GetBattleAttributeDefence(), 0, pCaster->GetCharAtt()->GetAvatarAttribute(), victim->GetCharAtt()->GetAvatarAttribute());

	float fDmg0 = fAttackerPower * (1.f - (fTargetDefensePower / (fTargetDefensePower + (float)pCaster->GetLevel() * 15.f)));
	float fDmg1 = fDmg0 * (CFormulaTable::m_afRate[3200][1] + fAttributeBonusRate);

	min_damage = fDmg1 * (CFormulaTable::m_afRate[3500][1] + ((float)pCaster->GetLevel() * CFormulaTable::m_afRate[3500][2]));
	max_damage = fDmg1 * (CFormulaTable::m_afRate[3500][3] - ((float)pCaster->GetLevel() * CFormulaTable::m_afRate[3500][4]));

	fFinalDamage = RandomRangeF(min_damage, max_damage);

	if (pCaster->IsPC() && victim->IsMonster())
	{
		//if (victim->GetTbldat()->dwBasic_LP >= 20000)
		if (pCaster->GetCurWorld()->GetIdx() == 199000) fFinalDamage *= 0.5;
		else fFinalDamage *= 2;
	}
	if (victim->IsPC() && pCaster->IsMonster())
	{
		if (victim->GetCurWorld()->GetIdx() == 199000)
		{
			switch (victim->GetTblidx())
			{
			case 37122101: case 33531901: case 33281101: case 33431101: fFinalDamage *= 6; break;
			default: fFinalDamage *= 10; break;
			}
		}
	}

	resultvalue = (fFinalDamage <= 1.f) ? 1.f : fFinalDamage;

	//---------------//

	//check if crit success and add damage
	if (rAttackResult == BATTLE_ATTACK_RESULT_CRITICAL_HIT)		//only add crit dmg once
	{
		if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_PHYSICAL) //check if physical dmg
		{
			fCritDmgRate = pCaster->GetCharAtt()->GetLastPhysicalCriticalDamageRate();
		}
		else if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_ENERGY) //check if energy dmg
		{
			fCritDmgRate = pCaster->GetCharAtt()->GetLastEnergyCriticalDamageRate();
		}
		else if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_STATE)
		{
			fCritDmgRate = (pCaster->GetCharAtt()->GetLastPhysicalCriticalDamageRate() + pCaster->GetCharAtt()->GetLastEnergyCriticalDamageRate()) / 2.f;
		}

		resultvalue += resultvalue * fCritDmgRate / 100.f;
	}

	//---------------//
	//reflect dmg
	rfReflectDmg += (int)GetSkillReflectDamage(resultvalue, skilltbl->bySkill_Type, victim->GetCharAtt()->GetLastPhysicalReflection(), victim->GetCharAtt()->GetLastEnergyReflection());

	//---------------//
	//lp ep recover
	rLpEpRecover.targetLpRecoveredWhenHit += (int)(victim->GetCharAtt()->GetLastLpRecoveryWhenHit() + (resultvalue * victim->GetCharAtt()->GetLastLpRecoveryWhenHitInPercent() / 100.0f));
	if (rLpEpRecover.targetLpRecoveredWhenHit > 0)
		rLpEpRecover.bIsLpRecoveredWhenHit = true;

	rLpEpRecover.dwTargetEpRecoveredWhenHit += (DWORD)(victim->GetCharAtt()->GetLastEpRecoveryWhenHit() + (resultvalue * victim->GetCharAtt()->GetLastEpRecoveryWhenHitInPercent() / 100.0f));
	if (rLpEpRecover.dwTargetEpRecoveredWhenHit > 0)
		rLpEpRecover.bIsEpRecoveredWhenHit = true;

	//ERR_LOG(LOG_USER,"Attacker Lv: %d Victim Lv %d Victim Obj Type %d CalcSkillDamage: %f \n", ch->GetLevel(), victim->GetLevel(), victim->GetObjType(), resultvalue);
}


void CalcSkillDotDamage(CCharacterObject * pCaster, CCharacterObject * victim, sSKILL_TBLDAT * skilltbl, BYTE byEffectNr, WORD wDefence, float fBaseSkillDmg, float fBonusDmg, float & resultvalue, BYTE rAttackResult, BYTE byEffectCode)
{
	float  fFinalDamage = 0.0f, min_damage = 0.0f, max_damage = 0.0f, fAttackerPower = 0.0f, fCritDmgRate = 0.0f;
	float fTargetDefensePower = (float)wDefence;

	if (skilltbl->bySkill_Effect_Type[byEffectNr] == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
	{
		fAttackerPower = fBaseSkillDmg;
		fFinalDamage = fAttackerPower * (1.f - (fTargetDefensePower / (fTargetDefensePower + (float)pCaster->GetLevel() * 25.f)));
	}
	else if (skilltbl->bySkill_Effect_Type[byEffectNr] == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
	{
		if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_PHYSICAL)
		{
			if (pCaster->IsPC() && skilltbl->byRequire_Epuip_Slot_Type == EQUIP_SLOT_TYPE_SUB_WEAPON)
				fAttackerPower = ((float)pCaster->GetCharAtt()->GetSubWeaponPhysicalOffence() * fBaseSkillDmg) / 100.f;
			else
				fAttackerPower = ((float)pCaster->GetCharAtt()->GetLastPhysicalOffence() * fBaseSkillDmg) / 100.f;

			fTargetDefensePower += (float)victim->GetCharAtt()->GetLastPhysicalDefence();
		}
		else if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_ENERGY)
		{
			if (pCaster->IsPC() && skilltbl->byRequire_Epuip_Slot_Type == EQUIP_SLOT_TYPE_SUB_WEAPON)
				fAttackerPower = ((float)pCaster->GetCharAtt()->GetSubWeaponEnergyOffence() * fBaseSkillDmg) / 100.f;
			else
				fAttackerPower = ((float)pCaster->GetCharAtt()->GetLastEnergyOffence() * fBaseSkillDmg) / 100.f;

			fTargetDefensePower += (float)victim->GetCharAtt()->GetLastEnergyDefence();
		}
		else if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_STATE)
		{
			float fStateOffence;

			if (pCaster->IsPC() && skilltbl->byRequire_Epuip_Slot_Type == EQUIP_SLOT_TYPE_SUB_WEAPON)
				fStateOffence = ((float)pCaster->GetCharAtt()->GetSubWeaponPhysicalOffence() + (float)pCaster->GetCharAtt()->GetSubWeaponEnergyOffence()) / 2.f;
			else
				fStateOffence = ((float)pCaster->GetCharAtt()->GetLastPhysicalOffence() + (float)pCaster->GetCharAtt()->GetLastEnergyOffence()) / 2.f;

			fAttackerPower = (fStateOffence * fBaseSkillDmg) / 100.f;
			fTargetDefensePower += ((float)victim->GetCharAtt()->GetLastPhysicalDefence() + (float)victim->GetCharAtt()->GetLastEnergyDefence()) / 2.f;
		}

		fFinalDamage = fAttackerPower * (1.f - (fTargetDefensePower / (fTargetDefensePower + (float)pCaster->GetLevel() * 35.f)));

		//add item attribute
		float fAttributeBonusRate = 0.f;
		if (pCaster->IsPC() && skilltbl->byRequire_Epuip_Slot_Type == EQUIP_SLOT_TYPE_SUB_WEAPON)
			fAttributeBonusRate = GetAttributeBonusRate(victim->IsPC(), true, 0, victim->GetCharAtt()->GetBattleAttributeDefence(), pCaster->GetCharAtt()->GetBattleAttributeOffenceSub(), pCaster->GetCharAtt()->GetAvatarAttribute(), victim->GetCharAtt()->GetAvatarAttribute());
		else
			fAttributeBonusRate = GetAttributeBonusRate(victim->IsPC(), false, pCaster->GetCharAtt()->GetBattleAttributeOffence(), victim->GetCharAtt()->GetBattleAttributeDefence(), 0, pCaster->GetCharAtt()->GetAvatarAttribute(), victim->GetCharAtt()->GetAvatarAttribute());

		fFinalDamage += fFinalDamage * fAttributeBonusRate;
	}

	fFinalDamage += fBonusDmg;

	if (victim->IsPC())
	{
		if(byEffectCode == ACTIVE_BLEED || ACTIVE_BURN || wDefence < 1)
			fFinalDamage -= (float)wDefence / 2.0f;
		else
			fFinalDamage -= (float)wDefence / 2.0f;
	}

	resultvalue = (fFinalDamage <= 1.f) ? 1.f : fFinalDamage;

	//---------------//

	//check if crit success and add damage
	if (rAttackResult == BATTLE_ATTACK_RESULT_CRITICAL_HIT)		//only add crit dmg once
	{
		if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_PHYSICAL) //check if physical dmg
		{
			fCritDmgRate = pCaster->GetCharAtt()->GetLastPhysicalCriticalDamageRate() / 2.f;
		}
		else if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_ENERGY) //check if energy dmg
		{
			fCritDmgRate = pCaster->GetCharAtt()->GetLastEnergyCriticalDamageRate() / 2.f;
		}
		else if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_STATE)
		{
			fCritDmgRate = (pCaster->GetCharAtt()->GetLastPhysicalCriticalDamageRate() + pCaster->GetCharAtt()->GetLastEnergyCriticalDamageRate()) / 4.f;
		}

		resultvalue += resultvalue * (fCritDmgRate / 100.0f);
	}
}


void CalcLifeStealDamage(CCharacterObject* pCaster, CCharacterObject* victim, sSKILL_TBLDAT* skilltbl, BYTE byEffectNr, float fBaseSkillDmg, float& resultvalue)
{
	float  fFinalDamage = 0.0f, fAttackerPower = 0.0f, fTargetDefensePower = 0.0f;

	if (skilltbl->bySkill_Type == NTL_SKILL_TYPE_ENERGY)
	{
		if (pCaster->IsPC() && skilltbl->byRequire_Epuip_Slot_Type == EQUIP_SLOT_TYPE_SUB_WEAPON)
			fAttackerPower = fBaseSkillDmg + (float)pCaster->GetCharAtt()->GetSubWeaponEnergyOffence();
		else
			fAttackerPower = fBaseSkillDmg + (float)pCaster->GetCharAtt()->GetLastEnergyOffence();

		fTargetDefensePower = (float)victim->GetCharAtt()->GetLastEnergyDefence();
	}
	else
	{
		if (pCaster->IsPC() && skilltbl->byRequire_Epuip_Slot_Type == EQUIP_SLOT_TYPE_SUB_WEAPON)
			fAttackerPower = fBaseSkillDmg + (float)pCaster->GetCharAtt()->GetSubWeaponPhysicalOffence();
		else
			fAttackerPower = fBaseSkillDmg + (float)pCaster->GetCharAtt()->GetLastPhysicalOffence();

		fTargetDefensePower = (float)victim->GetCharAtt()->GetLastPhysicalDefence();
	}

	fFinalDamage = fAttackerPower * (1.f - (fTargetDefensePower / (fTargetDefensePower + (float)pCaster->GetLevel() * 35.f)));

	resultvalue = (fFinalDamage <= 1.f) ? 1.f : fFinalDamage;

	//---------------//
}


//--------------------------------------------------------------------------------------//
//		CALCULATE NORMAL ATTACK DAMAGE
//--------------------------------------------------------------------------------------//
float CalcMeleeDamage(CCharacter* pkAttacker, CCharacter* pkVictim)
{
	float  fFinalDamage = 0.0f, min_damage = 0.0f, max_damage = 0.0f, fAttackerPower = 0.0f, fTargetDefensePower = 0.0f;
	
	if (pkAttacker->GetAttackType() == BATTLE_ATTACK_TYPE_ENERGY)
	{
		fAttackerPower = (float)pkAttacker->GetCharAtt()->GetLastEnergyOffence();
		fTargetDefensePower = (float)pkVictim->GetCharAtt()->GetLastEnergyDefence();
	}
	else
	{
		fAttackerPower = (float)pkAttacker->GetCharAtt()->GetLastPhysicalOffence();
		fTargetDefensePower = (float)pkVictim->GetCharAtt()->GetLastPhysicalDefence();
	}

	float fAttributeBonusRate = GetAttributeBonusRate(pkVictim->IsPC(), false, pkAttacker->GetCharAtt()->GetBattleAttributeOffence(), pkVictim->GetCharAtt()->GetBattleAttributeDefence(), 0, pkAttacker->GetCharAtt()->GetAvatarAttribute(), pkVictim->GetCharAtt()->GetAvatarAttribute());

	float fDmg0 = fAttackerPower * (1.f - (fTargetDefensePower / (fTargetDefensePower + (float)pkAttacker->GetLevel() * CFormulaTable::m_afRate[3100][1])));
	float fDmg1 = fDmg0 * (CFormulaTable::m_afRate[3200][1] + fAttributeBonusRate);

	min_damage = fDmg1 * (CFormulaTable::m_afRate[3500][1] + ((float)pkAttacker->GetLevel() * CFormulaTable::m_afRate[3500][2]));
	max_damage = fDmg1 * (CFormulaTable::m_afRate[3500][3] - ((float)pkAttacker->GetLevel() * CFormulaTable::m_afRate[3500][4]));

	fFinalDamage = RandomRangeF(min_damage, max_damage);

	if (pkAttacker->IsPC() && pkVictim->IsMonster())
	{
		if (pkAttacker->GetCurWorld()->GetIdx() == 199000) fFinalDamage *= 0.5;
	}
	if (pkVictim->IsPC() && pkAttacker->IsMonster())
	{
		if (pkVictim->GetCurWorld()->GetIdx() == 199000)
		{
			switch (pkVictim->GetTblidx())
			{
			case 37122101: case 33531901: case 33281101: case 33431101: fFinalDamage *= 6; break;
			default: fFinalDamage *= 10; break;
			}
		}
	}

	if (pkAttacker->IsPC())
	{
		if (fFinalDamage <= 1.f) fFinalDamage = 1.f;
	}
	else
	{
		if (fFinalDamage <= 10.f) fFinalDamage = 10.f;
	}

	//if (pkAttacker->IsPC())
	//	printf("fAttackerPower %f, fTargetDefensePower %f, min_damage %f, max_damage %f, fFinalDamage %f, fAttributeBonusRate %f, fDmg0 %f, fDmg1 %f\n", fAttackerPower, fTargetDefensePower, min_damage, max_damage, fFinalDamage, fAttributeBonusRate, fDmg0, fDmg1);

	return fFinalDamage;
}


void CalcDirectHeal(CCharacterObject * pCaster, sSKILL_TBLDAT * skilltbl, BYTE byEffectNr, float & resultvalue)
{
	resultvalue = (float)skilltbl->aSkill_Effect_Value[byEffectNr];

	if (pCaster->IsPC() && skilltbl->byRequire_Epuip_Slot_Type == EQUIP_SLOT_TYPE_SUB_WEAPON)
	{
		//add weapon offence to healing power
		resultvalue += (float)pCaster->GetCharAtt()->GetSubWeaponEnergyOffence();

		// Add % heal bonus (offence * %)
		resultvalue += (float)pCaster->GetCharAtt()->GetSubWeaponEnergyOffence() * pCaster->GetCharAtt()->GetLastDirectHealPowerBonusInPercent() / 100.f;
	}
	else
	{
		//add weapon offence to healing power
		resultvalue += (float)pCaster->GetCharAtt()->GetLastEnergyOffence();

		// Add % heal bonus (offence * %)
		resultvalue += (float)pCaster->GetCharAtt()->GetLastEnergyOffence() * pCaster->GetCharAtt()->GetLastDirectHealPowerBonusInPercent() / 100.f;
	}

	// Add Static Bonus
	resultvalue += pCaster->GetCharAtt()->GetLastDirectHealPowerBonus();

	//NTL_PRINT(PRINT_APP,"resultvalue %f, GetSubWeaponEnergyOffence %u, GetLastEnergyOffence %u, GetLastDirectHealPowerBonusInPercent %f, GetLastDirectHealPowerBonus %f \n", 
	//	resultvalue, pCaster->GetCharAtt()->GetSubWeaponEnergyOffence(), pCaster->GetCharAtt()->GetLastEnergyOffence(), pCaster->GetCharAtt()->GetLastDirectHealPowerBonusInPercent(), pCaster->GetCharAtt()->GetLastDirectHealPowerBonus());
}


void CalcHealOverTime(CCharacterObject * pCaster, sSKILL_TBLDAT * skilltbl, BYTE byEffectNr, float & resultvalue)
{
	resultvalue = (float)skilltbl->aSkill_Effect_Value[byEffectNr];

	if (pCaster->IsPC() && skilltbl->byRequire_Epuip_Slot_Type == EQUIP_SLOT_TYPE_SUB_WEAPON)
	{
		// Add % heal bonus (offence * %)
		resultvalue += (float)pCaster->GetCharAtt()->GetSubWeaponEnergyOffence() * pCaster->GetCharAtt()->GetLastHotPowerBonusInPercent() / 100.f;
	}
	else
	{
		// Add % heal bonus (offence * %)
		resultvalue += (float)pCaster->GetCharAtt()->GetLastEnergyOffence() * pCaster->GetCharAtt()->GetLastHotPowerBonusInPercent() / 100.f;
	}

	// Add Static Bonus
	resultvalue += pCaster->GetCharAtt()->GetLastHotPowerBonus();
}


//--------------------------------------------------------------------------------------//
//		INCREASES THE AGGRO FROM MONSTER WHICH ARE ATTACKING pTARGET (USED WHEN PCASTER HEAL PTARGET)
//--------------------------------------------------------------------------------------//
void IncreaseTargetEnemyAggro(CCharacter * pCaster, CCharacter * pTarget, DWORD dwDefaultAggro)
{
	int nAgro = (int)(dwDefaultAggro + pCaster->GetCharAtt()->GetLastSkillAggroBonus());
	nAgro += (int)((float)nAgro * pCaster->GetCharAtt()->GetLastSkillAggroBonusInPercent() / 100.f);

	CTargetListManager::AGGROPOINT_MAP::iterator it = pTarget->GetTargetListManager()->AggroBegin();
	CTargetListManager::AGGROPOINT_MAP::iterator itEnd = pTarget->GetTargetListManager()->AggroEnd();

	int nLoopCount = 0;

	while (it != itEnd)
	{
		++nLoopCount;
		if (nLoopCount > 5000)
		{
			ERR_LOG(LOG_GENERAL, "INFINITE LOOP FOUND");
		}

		CCharacter* pAttacker = g_pObjectManager->GetChar(it->first);
		if (pAttacker && pAttacker->IsInitialized())
		{
			if (pAttacker->IsNPC() || pAttacker->IsMonster())
			{
				pAttacker->ChangeAggro(pCaster->GetID(), DBO_AGGRO_CHANGE_TYPE_INCREASE, (DWORD)nAgro);
			}
		}

		++it;
	}
}

float GetReflectDamage(float fDmg, BYTE byAttackType, float fPhysicalReflect, float fEnergyReflect)
{
	if (byAttackType == BATTLE_ATTACK_TYPE_PHYSICAL)
	{
		return fDmg * fPhysicalReflect / 100.0f;
	}
	else if (byAttackType == BATTLE_ATTACK_TYPE_ENERGY)
	{
		return fDmg * fEnergyReflect / 100.0f;
	}

	return 0.0f;
}

float GetSkillReflectDamage(float fDmg, BYTE bySkillType, float fPhysicalReflect, float fEnergyReflect)
{
	if (bySkillType == NTL_SKILL_TYPE_PHYSICAL)
	{
		return fDmg * fPhysicalReflect / 100.0f;
	}
	else if (bySkillType == NTL_SKILL_TYPE_ENERGY)
	{
		return fDmg * fEnergyReflect / 100.0f;
	}

	return 0.0f;
}

float GetAttributeBonusRate(bool bIsPc, bool bSubWeapon, BYTE byOffence, BYTE byDefence, BYTE bySubOffence, sAVATAR_ATTRIBUTE& sOffenceAttribute, sAVATAR_ATTRIBUTE& sDefenceAttribute)
{
	float fAttributeBonusRate = 0.0f;
	BYTE byAttrOffence;

	if (bSubWeapon)
	{
		byAttrOffence = bySubOffence;
		fAttributeBonusRate = NtlGetBattleAttributeBonusRate(bySubOffence, byDefence);
	}
	else
	{
		byAttrOffence = byOffence;
		fAttributeBonusRate = NtlGetBattleAttributeBonusRate(byOffence, byDefence);
	}

	switch (byAttrOffence)
	{
		case BATTLE_ATTRIBUTE_HONEST: fAttributeBonusRate += sOffenceAttribute.fHonestOffence - sDefenceAttribute.fHonestDefence; break;
		case BATTLE_ATTRIBUTE_STRANGE: fAttributeBonusRate += sOffenceAttribute.fStrangeOffence - sDefenceAttribute.fStrangeDefence; break;
		case BATTLE_ATTRIBUTE_WILD: fAttributeBonusRate += sOffenceAttribute.fWildOffence - sDefenceAttribute.fWildDefence; break;
		case BATTLE_ATTRIBUTE_ELEGANCE: fAttributeBonusRate += sOffenceAttribute.fEleganceOffence - sDefenceAttribute.fEleganceDefence; break;
		case BATTLE_ATTRIBUTE_FUNNY: fAttributeBonusRate += sOffenceAttribute.fFunnyOffence - sDefenceAttribute.fFunnyDefence; break;

		default:
		{
			switch (byDefence)
			{
				case BATTLE_ATTRIBUTE_HONEST: fAttributeBonusRate -= sDefenceAttribute.fHonestDefence; break;
				case BATTLE_ATTRIBUTE_STRANGE: fAttributeBonusRate -= sDefenceAttribute.fStrangeDefence; break;
				case BATTLE_ATTRIBUTE_WILD: fAttributeBonusRate -= sDefenceAttribute.fWildDefence; break;
				case BATTLE_ATTRIBUTE_ELEGANCE: fAttributeBonusRate -= sDefenceAttribute.fEleganceDefence; break;
				case BATTLE_ATTRIBUTE_FUNNY: fAttributeBonusRate -= sDefenceAttribute.fFunnyDefence; break;

				default: break;
			}
		}
		break;
	}

	//printf("fAttributeBonusRate %f, byAttrOffence %f, byDefence %f \n", fAttributeBonusRate, byAttrOffence, byDefence);
	return fAttributeBonusRate / 100.f;
}
