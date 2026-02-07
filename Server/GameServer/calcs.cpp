#include "stdafx.h"
#include "calcs.h"
#include "CharacterAtt.h"




void Dbo_SetAvatarAttributeValue(CCharacterAtt * pCharAtt, eSYSTEM_EFFECT_CODE effectcode, float effectvalue, BYTE byApplyType)
{
	sAVATAR_ATTRIBUTE& avt = pCharAtt->GetAvatarAttribute();
	//NTL_PRINT(PRINT_APP,"code: %u, effect: %f, byApplyType: %u", effectcode, effectvalue, byApplyType);
	switch (effectcode)
	{
		//passive
		case PASSIVE_MAX_LP_UP:				pCharAtt->CalculateBaseMaxLP(effectvalue, byApplyType, true);	break;
		case PASSIVE_MAX_EP_UP:				pCharAtt->CalculateBaseMaxEP(effectvalue, byApplyType, true);	break;
		case PASSIVE_MAX_RP_DOWN:			pCharAtt->CalculateBaseMaxRP(effectvalue, byApplyType, false);	break;
		case PASSIVE_PHYSICAL_OFFENCE_UP: 	pCharAtt->CalculateBasePhysicalOffence(effectvalue, byApplyType, true);	break;
		case PASSIVE_ENERGY_OFFENCE_UP: 	pCharAtt->CalculateBaseEnergyOffence(effectvalue, byApplyType, true);	break;
		case PASSIVE_PHYSICAL_DEFENCE_UP: 	pCharAtt->CalculateBasePhysicalDefence(effectvalue, byApplyType, true);	break;
		case PASSIVE_ENERGY_DEFENCE_UP: 	pCharAtt->CalculateBaseEnergyDefence(effectvalue, byApplyType, true);	break;
		case PASSIVE_STR_UP:				pCharAtt->CalculateBaseStr(effectvalue, byApplyType, true);	break;
		case PASSIVE_CON_UP:				pCharAtt->CalculateBaseCon(effectvalue, byApplyType, true);	break;
		case PASSIVE_FOC_UP:				pCharAtt->CalculateBaseFoc(effectvalue, byApplyType, true);	break;
		case PASSIVE_DEX_UP:				pCharAtt->CalculateBaseDex(effectvalue, byApplyType, true);	break;
		case PASSIVE_SOL_UP:				pCharAtt->CalculateBaseSol(effectvalue, byApplyType, true);	break;
		case PASSIVE_ENG_UP:				pCharAtt->CalculateBaseEng(effectvalue, byApplyType, true); break;
		case PASSIVE_LP_REGENERATION:		pCharAtt->CalculateBaseLpRegen(effectvalue, byApplyType, true);	break;
		case PASSIVE_EP_REGENERATION:		pCharAtt->CalculateBaseEpRegen(effectvalue, byApplyType, true);	break;
		case PASSIVE_ATTACK_RATE_UP:		pCharAtt->CalculateBaseAttackRate(effectvalue, byApplyType, true);	break;
		case PASSIVE_DODGE_RATE_UP:			pCharAtt->CalculateBaseDodgeRate(effectvalue, byApplyType, true);	break;
		case PASSIVE_BLOCK_RATE_UP:			pCharAtt->CalculateBaseBlockRate(effectvalue, byApplyType, true);	break;
		case PASSIVE_CURSE_SUCCESS_UP:		pCharAtt->CalculateBaseCurseSuccessRate(effectvalue, byApplyType, true);	break;
		case PASSIVE_CURSE_TOLERANCE_UP:	pCharAtt->CalculateBaseCurseToleranceRate(effectvalue, byApplyType, true);	break;
		case PASSIVE_PHYSICAL_CRITICAL_UP:	pCharAtt->CalculateBasePhysicalCriticalRate(effectvalue, byApplyType, true);	break;
		case PASSIVE_ENERGY_CRITICAL_UP:	pCharAtt->CalculateBaseEnergyCriticalRate(effectvalue, byApplyType, true);	break;
		case PASSIVE_RP_CHARGE_SPEED:		pCharAtt->CalculateBaseRpRegen(effectvalue, byApplyType, true);	break;
		case PASSIVE_MOVE_SPEED:			pCharAtt->CalculateBaseRunSpeed(effectvalue, byApplyType, true);	break;
		case PASSIVE_ATTACK_SPEED_UP:		pCharAtt->CalculateBaseAttackSpeedRate(effectvalue, byApplyType, false);	break;
		case PASSIVE_SKILL_CASTING_TIME_DOWN:pCharAtt->CalculateCastingTimeChangePercent(effectvalue, byApplyType, false);	break;
		case PASSIVE_SKILL_COOL_TIME_DOWN:	pCharAtt->CalculateCoolTimeChangePercent(effectvalue * 0.7f, byApplyType, false);		break;
		case PASSIVE_BLOCK_MODE:			pCharAtt->CalculateGuardRateBase(effectvalue, byApplyType, true);	break;
		case PASSIVE_DOT_VALUE_UP_ALL:		pCharAtt->CalculateLastDotValueChangePercent(effectvalue, byApplyType, true);		break;
		case PASSIVE_DOT_TIME_UP_ALL:		pCharAtt->CalculateDotTimeChangeAbsolute(effectvalue, byApplyType, true);		break;
		case PASSIVE_SKILL_KEEP_TIME_UP:	pCharAtt->CalculateKeepTimeChangePercent(effectvalue, byApplyType, true);		break;
		case PASSIVE_REQUIRE_EP_DOWN:		pCharAtt->CalculateRequiredEpChangePercent(effectvalue, byApplyType, false);	break;

			//bless
		case ACTIVE_MAX_LP_UP:				pCharAtt->CalculateLastMaxLP(effectvalue, byApplyType, true);	break;
		case ACTIVE_MAX_EP_UP:				pCharAtt->CalculateLastMaxEP(effectvalue, byApplyType, true);	break;
		case ACTIVE_MAX_RP_UP:				pCharAtt->CalculateLastMaxRP(effectvalue, byApplyType, true);	break;
		case ACTIVE_PHYSICAL_OFFENCE_UP: {	pCharAtt->CalculateLastPhysicalOffence(effectvalue, byApplyType, true);	}break;
		case ACTIVE_ENERGY_OFFENCE_UP: {	pCharAtt->CalculateLastEnergyOffence(effectvalue, byApplyType, true);	}break;
		case ACTIVE_PHYSICAL_DEFENCE_UP: {	pCharAtt->CalculateLastPhysicalDefence(effectvalue, byApplyType, true);	}break;
		case ACTIVE_ENERGY_DEFENCE_UP: {	pCharAtt->CalculateLastEnergyDefence(effectvalue, byApplyType, true);	}break;
		case ACTIVE_STR_UP:					pCharAtt->CalculateLastStr(effectvalue, byApplyType, true);	break;
		case ACTIVE_CON_UP:					pCharAtt->CalculateLastCon(effectvalue, byApplyType, true);	break;
		case ACTIVE_FOC_UP:					pCharAtt->CalculateLastFoc(effectvalue, byApplyType, true);	break;
		case ACTIVE_DEX_UP:					pCharAtt->CalculateLastDex(effectvalue, byApplyType, true);	break;
		case ACTIVE_SOL_UP:					pCharAtt->CalculateLastSol(effectvalue, byApplyType, true);	break;
		case ACTIVE_ENG_UP:					pCharAtt->CalculateLastEng(effectvalue, byApplyType, true); break;
		case ACTIVE_MOVE_SPEED_UP:			pCharAtt->CalculateLastRunSpeed(effectvalue, byApplyType, true);	break;
		case ACTIVE_ATTACK_SPEED_UP:		pCharAtt->CalculateLastAttackSpeedRate(effectvalue * 0.5f, byApplyType, false); break;
		case ACTIVE_ATTACK_RATE_UP:			pCharAtt->CalculateLastAttackRate(effectvalue, byApplyType, true);	break;
		case ACTIVE_DODGE_RATE_UP:			pCharAtt->CalculateLastDodgeRate(effectvalue, byApplyType, true);	break;
		case ACTIVE_BLOCK_RATE_UP:			pCharAtt->CalculateLastBlockRate(effectvalue, byApplyType, true);	break;
		case ACTIVE_HONEST_DEFENCE_UP:		pCharAtt->CalculateLastHonestDefence(effectvalue, byApplyType, true);	break;
		case ACTIVE_STRANGE_DEFENCE_UP:		pCharAtt->CalculateLastStrangeDefence(effectvalue, byApplyType, true);	break;
		case ACTIVE_WILD_DEFENCE_UP:		pCharAtt->CalculateLastWildDefence(effectvalue, byApplyType, true);	break;
		case ACTIVE_ELEGANCE_DEFENCE_UP:	pCharAtt->CalculateLastEleganceDefence(effectvalue, byApplyType, true);	break;
		case ACTIVE_FUNNY_DEFENCE_UP:		pCharAtt->CalculateLastFunnyDefence(effectvalue, byApplyType, true);	break;
		case ACTIVE_ENERGY_REFLECTION:		pCharAtt->CalculateLastEnergyReflection(effectvalue, byApplyType, true);	break;
		case ACTIVE_LP_REGENERATION:		pCharAtt->CalculateLastLpRegen(effectvalue, byApplyType, true);	break;
		case ACTIVE_EP_REGENERATION:		pCharAtt->CalculateLastEpRegen(effectvalue, byApplyType, true);	break;
		case ACTIVE_RP_CHARGE_SPEED:		pCharAtt->CalculateLastRpRegen(effectvalue, byApplyType, true);	break;
		case ACTIVE_CURSE_SUCCESS:			pCharAtt->CalculateLastCurseSuccessRate(effectvalue, byApplyType, true);	break;
		case ACTIVE_CURSE_TOLERANCE:		pCharAtt->CalculateLastCurseToleranceRate(effectvalue, byApplyType, true);	break;
		case ACTIVE_PHYSICAL_CRITICAL:		pCharAtt->CalculateLastPhysicalCriticalRate(effectvalue, byApplyType, true);	break;
		case ACTIVE_ENERGY_CRITICAL:			pCharAtt->CalculateLastEnergyCriticalRate(effectvalue, byApplyType, true);	break;
		case ACTIVE_SKILL_CASTING_TIME_DOWN:	pCharAtt->CalculateCastingTimeChangePercent(effectvalue, byApplyType, false);	break;
		case ACTIVE_SKILL_COOL_TIME_DOWN:		pCharAtt->CalculateCoolTimeChangePercent(effectvalue * 0.7f, byApplyType, false);	break;
		case ACTIVE_PARALYZE_TOLERANCE_UP:		pCharAtt->CalculateLastParalyzeToleranceRate(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break; //add value because the value is "increase by X%. But we have 0 so it wont increase in any way"
		case ACTIVE_TERROR_TOLERANCE_UP:		pCharAtt->CalculateLastTerrorToleranceRate(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;//add value because the value is "increase by X%. But we have 0 so it wont increase in any way"
		case ACTIVE_CONFUSE_TOLERANCE_UP:		pCharAtt->CalculateLastConfuseToleranceRate(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;//add value because the value is "increase by X%. But we have 0 so it wont increase in any way"
		case ACTIVE_STONE_TOLERANCE_UP:			pCharAtt->CalculateLastStoneToleranceRate(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;//add value because the value is "increase by X%. But we have 0 so it wont increase in any way"
		case ACTIVE_CANDY_TOLERANCE_UP:			pCharAtt->CalculateLastCandyToleranceRate(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;//add value because the value is "increase by X%. But we have 0 so it wont increase in any way"
		case ACTIVE_PARALYZE_KEEPTIME_DOWN:		pCharAtt->CalculateLastParalyzeKeepTimeDown(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_TERROR_KEEPTIME_DOWN:		pCharAtt->CalculateLastTerrorKeepTimeDown(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_CONFUSE_KEEPTIME_DOWN:		pCharAtt->CalculateLastConfuseKeepTimeDown(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_STONE_KEEPTIME_DOWN:		pCharAtt->CalculateLastStoneKeepTimeDown(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_CANDY_KEEPTIME_DOWN:		pCharAtt->CalculateLastCandyKeepTimeDown(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_BLEEDING_KEEPTIME_DOWN:		pCharAtt->CalculateLastBleedingKeepTimeDown(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_POISON_KEEPTIME_DOWN:		pCharAtt->CalculateLastPoisonKeepTimeDown(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_STOMACHACHE_KEEPTIME_DOWN:	pCharAtt->CalculateLastStomachacheKeepTimeDown(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_CRITICAL_BLOCK_UP:			pCharAtt->CalculateLastCriticalBlockSuccessRate(effectvalue, byApplyType, true);	break;
		case ACTIVE_NORMAL_SKILL_BLOCK_UP:		pCharAtt->CalculateSkillDamageBlockModeSuccessRate(effectvalue, byApplyType, true);	break;
		case ACTIVE_CURSE_SKILL_BLOCK_UP:		pCharAtt->CalculateCurseBlockModeSuccessRate(effectvalue, byApplyType, true);	break;
		case ACTIVE_KNOCKDOWN_ATTACK_BLOCK_UP:	pCharAtt->CalculateKnockdownBlockModeSuccessRate(effectvalue, byApplyType, true);	break;
		case ACTIVE_HTB_SKILL_BLOCK_UP:			pCharAtt->CalculateHtbBlockModeSuccessRate(effectvalue, byApplyType, true);	break;
		case ACTIVE_SIT_DOWN_LP_REGENERATION_UP:	pCharAtt->CalculateLastLpSitdownRegen(effectvalue, byApplyType, true);	break;
		case ACTIVE_SIT_DOWN_EP_REGENERATION_UP:	pCharAtt->CalculateLastEpSitdownRegen(effectvalue, byApplyType, true);	break;
		case ACTIVE_PHYSICAL_CRITICAL_DAMAGE_UP:	pCharAtt->CalculateLastPhysicalCriticalDamageRate(effectvalue, byApplyType, true);	break;
		case ACTIVE_ENERGY_CRITICAL_DAMAGE_UP:		pCharAtt->CalculateLastEnergyCriticalDamageRate(effectvalue, byApplyType, true);	break;
		case ACTIVE_ATTACK_RANGE_UP:				pCharAtt->CalculateLastAttackRange(effectvalue, byApplyType, true);	break;

		case ACTIVE_BATTLE_ATTRIBUTE_UP:			pCharAtt->CalculateBattleAttribute(effectvalue, byApplyType, true);		break;

		case ACTIVE_PHYSICAL_REFLECTION:			pCharAtt->CalculateLastPhysicalReflection(effectvalue, byApplyType, true);	break;
		case ACTIVE_LP_RECOVERY:					pCharAtt->CalculateLastLpRecoveryWhenHit(effectvalue, byApplyType, true);	break;
		case ACTIVE_EP_RECOVERY:					pCharAtt->CalculateLastEpRecoveryWhenHit(effectvalue, byApplyType, true);	break;
		case ACTIVE_LP_RECOVERY_IN_PERCENT:			pCharAtt->CalculateLastLpRecoveryWhenHitInPercent(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_EP_RECOVERY_IN_PERCENT:			pCharAtt->CalculateLastEpRecoveryWhenHitInPercent(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_BATTLE_LP_UP:				pCharAtt->CalculateLastLpBattleRegen(effectvalue, byApplyType, true);	break;
		case ACTIVE_BATTLE_EP_UP:				pCharAtt->CalculateLastEpBattleRegen(effectvalue, byApplyType, true);	break;
		case ACTIVE_ALL_OFFENCE_UP:			pCharAtt->CalculateLastPhysicalOffence(effectvalue, byApplyType, true);	pCharAtt->CalculateLastEnergyOffence(effectvalue, byApplyType, true); break;
		case ACTIVE_ALL_DEFENCE_UP:			pCharAtt->CalculateLastPhysicalDefence(effectvalue, byApplyType, true);	pCharAtt->CalculateLastEnergyDefence(effectvalue, byApplyType, true); break;

		case ACTIVE_MIND_IMMUNITY:				pCharAtt->CalculateLastMindCurseImmunity(effectvalue, byApplyType, true);	break;
		case ACTIVE_BODY_IMMUNITY:				pCharAtt->CalculateLastBodyCurseImmunity(effectvalue, byApplyType, true);	break;
		case ACTIVE_CHANGE_IMMUNITY:			pCharAtt->CalculateLastChangeCurseImmunity(effectvalue, byApplyType, true);	break;
		case ACTIVE_SKILL_AGGRO_UP:				pCharAtt->CalculateLastSkillAggroBonus(effectvalue, byApplyType, true);	break;
		case ACTIVE_DH_POWER_UP:				pCharAtt->CalculateLastDirectHealPowerBonus(effectvalue, byApplyType, true);	break;
		case ACTIVE_HOT_POWER_UP:				pCharAtt->CalculateLastHotPowerBonus(effectvalue, byApplyType, true);	break;
		case ACTIVE_SKILL_AGGRO_UP_IN_PERCENT:	pCharAtt->CalculateLastSkillAggroBonusInPercent(effectvalue, byApplyType, true);	break;
		case ACTIVE_DH_POWER_UP_IN_PERCENT:		pCharAtt->CalculateLastDirectHealPowerBonusInPercent(effectvalue, byApplyType, true);	break;
		case ACTIVE_HOT_POWER_UP_IN_PERCENT:	pCharAtt->CalculateLastHotPowerBonusInPercent(effectvalue, byApplyType, true);	break;
		case ACTIVE_MAX_AP_UP:					pCharAtt->CalculateLastMaxAP(effectvalue, byApplyType, true);	break;
		case ACTIVE_AP_REGENERATION:			pCharAtt->CalculateLastApRegen(effectvalue * 1000, byApplyType, true);	break;

		case ACTIVE_HONEST_OFFENCE_UP:			pCharAtt->CalculateLastHonestOffence(effectvalue, byApplyType, true);	break;
		case ACTIVE_STRANGE_OFFENCE_UP:			pCharAtt->CalculateLastStrangeOffence(effectvalue, byApplyType, true);	break;
		case ACTIVE_WILD_OFFENCE_UP:			pCharAtt->CalculateLastWildOffence(effectvalue, byApplyType, true);	break;
		case ACTIVE_ELEGANCE_OFFENCE_UP:		pCharAtt->CalculateLastEleganceOffence(effectvalue, byApplyType, true);	break;
		case ACTIVE_FUNNY_OFFENCE_UP:			pCharAtt->CalculateLastFunnyOffence(effectvalue, byApplyType, true);	break;
		case ACTIVE_STOMACHACHE_DEFENCE:		pCharAtt->CalculateLastStomachacheDefence(effectvalue, byApplyType, true);	break;
		case ACTIVE_POISON_DEFENCE:				pCharAtt->CalculateLastPoisonDefence(effectvalue, byApplyType, true);	break;
		case ACTIVE_BLEED_DEFENCE:				pCharAtt->CalculateLastBleedDefence(effectvalue, byApplyType, true);	break;
		case ACTIVE_BURN_DEFENCE:				pCharAtt->CalculateLastBurnDefence(effectvalue, byApplyType, true);	break;

		case ACTIVE_VIABILITY:					pCharAtt->CalculateLastBlockRate(effectvalue, byApplyType, true);	break;

			//curse
		case ACTIVE_SKILL_AGGRO_DOWN: {	pCharAtt->CalculateLastSkillAggroBonus(effectvalue, byApplyType, false);	}break;
		case ACTIVE_SKILL_AGGRO_DOWN_IN_PERCENT: {	pCharAtt->CalculateLastSkillAggroBonusInPercent(effectvalue, byApplyType, false);	}break;
		case ACTIVE_MAX_LP_DOWN: {	pCharAtt->CalculateLastMaxLP(effectvalue, byApplyType, false);	}break;
		case ACTIVE_MAX_EP_DOWN: {	pCharAtt->CalculateLastMaxEP(effectvalue, byApplyType, false);	}break;
		case ACTIVE_MAX_RP_DOWN: {	pCharAtt->CalculateLastMaxRP(effectvalue, byApplyType, false);	}break;
		case ACTIVE_PHYSICAL_OFFENCE_DOWN: {	pCharAtt->CalculateLastPhysicalOffence(effectvalue, byApplyType, false);	}break;
		case ACTIVE_ENERGY_OFFENCE_DOWN: {	pCharAtt->CalculateLastEnergyOffence(effectvalue, byApplyType, false);	}break;
		case ACTIVE_PHYSICAL_DEFENCE_DOWN: { pCharAtt->CalculateLastPhysicalDefence(effectvalue, byApplyType, false);	}break;
		case ACTIVE_ENERGY_DEFENCE_DOWN: {	pCharAtt->CalculateLastEnergyDefence(effectvalue, byApplyType, false);	}break;
		case ACTIVE_STR_DOWN: 					pCharAtt->CalculateLastStr(effectvalue, byApplyType, false);	break;
		case ACTIVE_CON_DOWN: 					pCharAtt->CalculateLastCon(effectvalue, byApplyType, false);	break;
		case ACTIVE_FOC_DOWN:					pCharAtt->CalculateLastFoc(effectvalue, byApplyType, false);	break;
		case ACTIVE_DEX_DOWN: 					pCharAtt->CalculateLastDex(effectvalue, byApplyType, false);	break;
		case ACTIVE_SOL_DOWN: 					pCharAtt->CalculateLastSol(effectvalue, byApplyType, false);	break;
		case ACTIVE_ENG_DOWN: 					pCharAtt->CalculateLastEng(effectvalue, byApplyType, false);	break;
		case ACTIVE_MOVE_SPEED_DOWN: {		pCharAtt->CalculateLastRunSpeed(effectvalue, byApplyType, false);	}break;
		case ACTIVE_ATTACK_SPEED_DOWN:			pCharAtt->CalculateLastAttackSpeedRate(effectvalue, byApplyType, true); break;
		case ACTIVE_ATTACK_RATE_DOWN: {		pCharAtt->CalculateLastAttackRate(effectvalue, byApplyType, false);	}break;
		case ACTIVE_DODGE_RATE_DOWN: {		pCharAtt->CalculateLastDodgeRate(effectvalue, byApplyType, false);	}break;
		case ACTIVE_BLOCK_RATE_DOWN: {		pCharAtt->CalculateLastBlockRate(effectvalue, byApplyType, false);	}break;
		case ACTIVE_HONEST_DEFENCE_DOWN: {	pCharAtt->CalculateLastHonestDefence(effectvalue, byApplyType, false);	}break;
		case ACTIVE_STRANGE_DEFENCE_DOWN: {	pCharAtt->CalculateLastStrangeDefence(effectvalue, byApplyType, false);	}break;
		case ACTIVE_WILD_DEFENCE_DOWN: {		pCharAtt->CalculateLastWildDefence(effectvalue, byApplyType, false);	}break;
		case ACTIVE_ELEGANCE_DEFENCE_DOWN: {	pCharAtt->CalculateLastEleganceDefence(effectvalue, byApplyType, false);	}break;
		case ACTIVE_FUNNY_DEFENCE_DOWN: {	pCharAtt->CalculateLastFunnyDefence(effectvalue, byApplyType, false);	}break;
		case ACTIVE_SKILL_CASTING_TIME_UP: {	pCharAtt->CalculateCastingTimeChangePercent(effectvalue, byApplyType, true);	}break;
		case ACTIVE_SKILL_COOL_TIME_UP: {	pCharAtt->CalculateCoolTimeChangePercent(effectvalue * 0.7f, byApplyType, true);	}break;
		case ACTIVE_ATTACK_RANGE_DOWN: {		pCharAtt->CalculateLastAttackRange(effectvalue, byApplyType, false);	}break;

		case ACTIVE_BATTLE_ATTRIBUTE_DOWN:		pCharAtt->CalculateBattleAttribute(effectvalue, byApplyType, false);		break;

		case ACTIVE_ALL_OFFENCE_DOWN:		pCharAtt->CalculateLastPhysicalOffence(effectvalue, byApplyType, false); pCharAtt->CalculateLastEnergyOffence(effectvalue, byApplyType, false);		break;
		case ACTIVE_ALL_DEFENCE_DOWN:		pCharAtt->CalculateLastPhysicalDefence(effectvalue, byApplyType, false); pCharAtt->CalculateLastEnergyDefence(effectvalue, byApplyType, false);		break;

		case ACTIVE_EXP_BOOSTER:	pCharAtt->CalculateExpBooster(effectvalue, byApplyType, true); break;

		default: /*printf("code %u missing effect %f \n", effectcode, effectvalue); */break;
	}
}





void Dbo_SetRandomOptionValues(CCharacterAtt* pCharAtt, eSYSTEM_EFFECT_CODE effectcode, float effectvalue)
{
	sAVATAR_ATTRIBUTE& avt = pCharAtt->GetAvatarAttribute();
	//NTL_PRINT(PRINT_APP, "code %u effect %f \n", effectcode, effectvalue);
	switch (effectcode)
	{
		case ACTIVE_MAX_LP_UP:				pCharAtt->CalculateLastMaxLP(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_MAX_EP_UP:				pCharAtt->CalculateLastMaxEP(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_MAX_RP_UP:				pCharAtt->CalculateLastMaxRP(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_PHYSICAL_OFFENCE_UP: {	pCharAtt->CalculateLastPhysicalOffence(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	}break;
		case ACTIVE_ENERGY_OFFENCE_UP: {	pCharAtt->CalculateLastEnergyOffence(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	}break;
		case ACTIVE_PHYSICAL_DEFENCE_UP: {	pCharAtt->CalculateLastPhysicalDefence(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	}break;
		case ACTIVE_ENERGY_DEFENCE_UP: {	pCharAtt->CalculateLastEnergyDefence(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	}break;
		case ACTIVE_STR_UP:					pCharAtt->CalculateLastStr(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_CON_UP:					pCharAtt->CalculateLastCon(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_FOC_UP:					pCharAtt->CalculateLastFoc(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_DEX_UP:					pCharAtt->CalculateLastDex(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_SOL_UP:					pCharAtt->CalculateLastSol(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_ENG_UP:					pCharAtt->CalculateLastEng(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true); break;
		case ACTIVE_MOVE_SPEED_UP:			pCharAtt->CalculateLastRunSpeed(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_PERCENT, true);	break;
		case ACTIVE_ATTACK_SPEED_UP:		pCharAtt->CalculateLastAttackSpeedRate(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_PERCENT, false);
		case ACTIVE_ATTACK_RATE_UP:			pCharAtt->CalculateLastAttackRate(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_DODGE_RATE_UP:			pCharAtt->CalculateLastDodgeRate(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_BLOCK_RATE_UP:			pCharAtt->CalculateLastBlockRate(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_HONEST_DEFENCE_UP:		pCharAtt->CalculateLastHonestDefence(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_STRANGE_DEFENCE_UP:		pCharAtt->CalculateLastStrangeDefence(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_WILD_DEFENCE_UP:		pCharAtt->CalculateLastWildDefence(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_ELEGANCE_DEFENCE_UP:	pCharAtt->CalculateLastEleganceDefence(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_FUNNY_DEFENCE_UP:		pCharAtt->CalculateLastFunnyDefence(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_ENERGY_REFLECTION:		pCharAtt->CalculateLastEnergyReflection(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_LP_REGENERATION:		pCharAtt->CalculateLastLpRegen(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_EP_REGENERATION:		pCharAtt->CalculateLastEpRegen(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_RP_CHARGE_SPEED:		pCharAtt->CalculateLastRpRegen(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_CURSE_SUCCESS:			pCharAtt->CalculateLastCurseSuccessRate(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_CURSE_TOLERANCE:		pCharAtt->CalculateLastCurseToleranceRate(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_PHYSICAL_CRITICAL:		pCharAtt->CalculateLastPhysicalCriticalRate(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_ENERGY_CRITICAL:		pCharAtt->CalculateLastEnergyCriticalRate(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_SKILL_COOL_TIME_DOWN:		pCharAtt->CalculateCoolTimeChangePercent(effectvalue * 0.7f, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);	break;
		case ACTIVE_PARALYZE_TOLERANCE_UP:		pCharAtt->CalculateLastParalyzeToleranceRate(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;//add value because the value is "increase by X%. But we have 0 so it wont increase in any way"
		case ACTIVE_TERROR_TOLERANCE_UP:		pCharAtt->CalculateLastTerrorToleranceRate(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;//add value because the value is "increase by X%. But we have 0 so it wont increase in any way"
		case ACTIVE_CONFUSE_TOLERANCE_UP:		pCharAtt->CalculateLastConfuseToleranceRate(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;//add value because the value is "increase by X%. But we have 0 so it wont increase in any way"
		case ACTIVE_STONE_TOLERANCE_UP:			pCharAtt->CalculateLastStoneToleranceRate(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;//add value because the value is "increase by X%. But we have 0 so it wont increase in any way"
		case ACTIVE_CANDY_TOLERANCE_UP:			pCharAtt->CalculateLastCandyToleranceRate(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;//add value because the value is "increase by X%. But we have 0 so it wont increase in any way"
		case ACTIVE_PARALYZE_KEEPTIME_DOWN:		pCharAtt->CalculateLastParalyzeKeepTimeDown(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_TERROR_KEEPTIME_DOWN:		pCharAtt->CalculateLastTerrorKeepTimeDown(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_CONFUSE_KEEPTIME_DOWN:		pCharAtt->CalculateLastConfuseKeepTimeDown(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_STONE_KEEPTIME_DOWN:		pCharAtt->CalculateLastStoneKeepTimeDown(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_CANDY_KEEPTIME_DOWN:		pCharAtt->CalculateLastCandyKeepTimeDown(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_BLEEDING_KEEPTIME_DOWN:		pCharAtt->CalculateLastBleedingKeepTimeDown(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_POISON_KEEPTIME_DOWN:		pCharAtt->CalculateLastPoisonKeepTimeDown(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_STOMACHACHE_KEEPTIME_DOWN:	pCharAtt->CalculateLastStomachacheKeepTimeDown(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_CRITICAL_BLOCK_UP:			pCharAtt->CalculateLastCriticalBlockSuccessRate(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_CURSE_SKILL_BLOCK_UP:		pCharAtt->CalculateCurseBlockModeSuccessRate(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_PHYSICAL_CRITICAL_DAMAGE_UP:	pCharAtt->CalculateLastPhysicalCriticalDamageRate(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_ENERGY_CRITICAL_DAMAGE_UP:		pCharAtt->CalculateLastEnergyCriticalDamageRate(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;

		case ACTIVE_BATTLE_ATTRIBUTE_UP:			pCharAtt->CalculateBattleAttribute(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);		break;

		case ACTIVE_PHYSICAL_REFLECTION:			pCharAtt->CalculateLastPhysicalReflection(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_LP_RECOVERY:					pCharAtt->CalculateLastLpRecoveryWhenHit(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_EP_RECOVERY:					pCharAtt->CalculateLastEpRecoveryWhenHit(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_HONEST_OFFENCE_UP:				pCharAtt->CalculateLastHonestOffence(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_STRANGE_OFFENCE_UP:				pCharAtt->CalculateLastStrangeOffence(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_WILD_OFFENCE_UP:				pCharAtt->CalculateLastWildOffence(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_ELEGANCE_OFFENCE_UP:			pCharAtt->CalculateLastEleganceOffence(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_FUNNY_OFFENCE_UP:				pCharAtt->CalculateLastFunnyOffence(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_LP_RECOVERY_IN_PERCENT:			pCharAtt->CalculateLastLpRecoveryWhenHitInPercent(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_EP_RECOVERY_IN_PERCENT:			pCharAtt->CalculateLastEpRecoveryWhenHitInPercent(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_STOMACHACHE_DEFENCE:		pCharAtt->CalculateLastStomachacheDefence(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_POISON_DEFENCE:				pCharAtt->CalculateLastPoisonDefence(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_BLEED_DEFENCE:				pCharAtt->CalculateLastBleedDefence(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;
		case ACTIVE_BURN_DEFENCE:				pCharAtt->CalculateLastBurnDefence(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;

		case ACTIVE_VIABILITY:					pCharAtt->CalculateLastBlockRate(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);	break;

		case ACTIVE_MAX_RP_DOWN:				pCharAtt->CalculateLastMaxRP(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);	break;

		case ACTIVE_BATTLE_ATTRIBUTE_DOWN:			pCharAtt->CalculateBattleAttribute(effectvalue, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);		break;

		default: ERR_LOG(LOG_GENERAL, "Dbo_SetRandomOptionValues: effectcode %u not set yet \n", effectcode); break;
	}
}


