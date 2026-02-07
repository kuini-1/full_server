#ifndef __INC_DBOG_CHARATTRIBUTE_BASE_H__
#define __INC_DBOG_CHARATTRIBUTE_BASE_H__


class CCharacter;
#include "NtlAvatar.h"

class CCharacterAtt
{

public:
	CCharacterAtt();
	virtual	~CCharacterAtt();

public:

	sAVATAR_ATTRIBUTE*		GetAvatarAttributePointer() { return &m_pAttribute; }
	sAVATAR_ATTRIBUTE&		GetAvatarAttribute() { return m_pAttribute; }

public:

	bool					Create(CCharacter* pChar);

	void					Init();
	virtual void			Reset();

//CALCULATION

	virtual void	CalculateAll();
	virtual void	CalculateBaseAtt() = 0;
	virtual void	CalculateLastAtt();

	void			CalculatePercentValues();

public:

	void	SetSettingMaxLP(float nSetting) { m_fSettingMaxLP = nSetting; }
	void	SetSettingPhysicalDefence(float nSetting) { m_fSettingPhysicalDefence = nSetting; }
	void	SetSettingEnergyDefence(float nSetting) { m_fSettingEnergyDefence = nSetting; }
	void	SetSettingPhysicalOffence(float nSetting) { m_fSettingPhysicalOffence = nSetting; }
	void	SetSettingEnergyOffence(float nSetting) { m_fSettingEnergyOffence = nSetting; }

	virtual void	CalculateBaseStr(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseCon(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseFoc(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseDex(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseSol(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseEng(float fValue, BYTE byApplyType, bool bIsPlus);

	virtual void	CalculateLastStr(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastCon(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastFoc(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastDex(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastSol(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastEng(float fValue, BYTE byApplyType, bool bIsPlus);

	virtual void	CalculateBaseMaxLP(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastMaxLP(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseMaxEP(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastMaxEP(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseMaxRP(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastMaxRP(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseMaxAP(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastMaxAP(float fValue, BYTE byApplyType, bool bIsPlus);

	virtual void	CalculateBaseLpRegen(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastLpRegen(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseLpSitdownRegen(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastLpSitdownRegen(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseLpBattleRegen(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastLpBattleRegen(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseEpRegen(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastEpRegen(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseEpSitdownRegen(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastEpSitdownRegen(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseEpBattleRegen(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastEpBattleRegen(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseApRegen(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastApRegen(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseApSitdownRegen(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastApSitdownRegen(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseApBattleRegen(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastApBattleRegen(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseApDegen(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastApDegen(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseApBattleDegen(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastApBattleDegen(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseRpRegen(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastRpRegen(float fValue, BYTE byApplyType, bool bIsPlus);

	virtual void	CalculateBasePhysicalOffence(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastPhysicalOffence(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseEnergyOffence(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastEnergyOffence(float fValue, BYTE byApplyType, bool bIsPlus);

	virtual void	CalculateBasePhysicalDefence(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastPhysicalDefence(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseEnergyDefence(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastEnergyDefence(float fValue, BYTE byApplyType, bool bIsPlus);

	virtual void	CalculateBaseAttackRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastAttackRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseDodgeRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastDodgeRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseBlockRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastBlockRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseBlockDamageRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastBlockDamageRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseCurseSuccessRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastCurseSuccessRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseCurseToleranceRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastCurseToleranceRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBasePhysicalCriticalRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastPhysicalCriticalRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseEnergyCriticalRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastEnergyCriticalRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBasePhysicalCriticalDamageRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastPhysicalCriticalDamageRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseEnergyCriticalDamageRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastEnergyCriticalDamageRate(float fValue, BYTE byApplyType, bool bIsPlus);

	virtual void	CalculateBaseRunSpeed(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastRunSpeed(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseFlySpeed(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastFlySpeed(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseFlyDashSpeed(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastFlyDashSpeed(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseFlyAccelSpeed(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastFlyAccelSpeed(float fValue, BYTE byApplyType, bool bIsPlus);

	virtual void	CalculateBaseAttackSpeedRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastAttackSpeedRate(float fValue, BYTE byApplyType, bool bIsPlus);

	virtual void	CalculateBaseAttackRange(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastAttackRange(float fValue, BYTE byApplyType, bool bIsPlus);

	virtual void	CalculateCastingTimeChangePercent(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateCoolTimeChangePercent(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateKeepTimeChangePercent(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateDotTimeChangeAbsolute(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateRequiredEpChangePercent(float fValue, BYTE byApplyType, bool bIsPlus);

	virtual void	CalculateBattleAttribute(float fValue, BYTE byApplyType, bool bIsPlus);

	virtual void	CalculateLastHonestOffence(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastHonestDefence(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastStrangeOffence(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastStrangeDefence(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastWildOffence(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastWildDefence(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastEleganceOffence(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastEleganceDefence(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastFunnyOffence(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastFunnyDefence(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastPhysicalReflection(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastEnergyReflection(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastParalyzeToleranceRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastTerrorToleranceRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastConfuseToleranceRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastStoneToleranceRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastCandyToleranceRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastParalyzeKeepTimeDown(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastTerrorKeepTimeDown(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastConfuseKeepTimeDown(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastStoneKeepTimeDown(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastCandyKeepTimeDown(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastBleedingKeepTimeDown(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastPoisonKeepTimeDown(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastStomachacheKeepTimeDown(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastCriticalBlockSuccessRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastLpRecoveryWhenHit(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastLpRecoveryWhenHitInPercent(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastEpRecoveryWhenHit(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastEpRecoveryWhenHitInPercent(float fValue, BYTE byApplyType, bool bIsPlus);

	virtual void	CalculateBaseStomachacheDefence(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastStomachacheDefence(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBasePoisonDefence(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastPoisonDefence(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseBleedDefence(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastBleedDefence(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseBurnDefence(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastBurnDefence(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseMindCurseImmunity(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastMindCurseImmunity(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseBodyCurseImmunity(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastBodyCurseImmunity(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseChangeCurseImmunity(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastChangeCurseImmunity(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseSkillAnimationSpeedModifier(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastSkillAnimationSpeedModifier(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseWeightLimit(float fValue, BYTE byApplyType, bool bIsPlus) {}
	virtual void	CalculateLastWeightLimit(float fValue, BYTE byApplyType, bool bIsPlus) {}
	virtual void	CalculateBaseSkillAggroBonus(float fValue, BYTE byApplyType, bool bIsPlus) {} //stat dont exist
	virtual void	CalculateLastSkillAggroBonus(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseSkillAggroBonusInPercent(float fValue, BYTE byApplyType, bool bIsPlus) {} //stat dont exist
	virtual void	CalculateLastSkillAggroBonusInPercent(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseDirectHealPowerBonus(float fValue, BYTE byApplyType, bool bIsPlus) {} //stat dont exist
	virtual void	CalculateLastDirectHealPowerBonus(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseDirectHealPowerBonusInPercent(float fValue, BYTE byApplyType, bool bIsPlus) {} //stat dont exist
	virtual void	CalculateLastDirectHealPowerBonusInPercent(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseHotPowerBonus(float fValue, BYTE byApplyType, bool bIsPlus) {} //stat dont exist
	virtual void	CalculateLastHotPowerBonus(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateBaseHotPowerBonusInPercent(float fValue, BYTE byApplyType, bool bIsPlus) {} //stat dont exist
	virtual void	CalculateLastHotPowerBonusInPercent(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastDotValueChangePercent(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastPhysicalCriticalDefenceRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateLastEnergyCriticalDefenceRate(float fValue, BYTE byApplyType, bool bIsPlus);

	virtual void	CalculateGuardRateBase(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateGuardRateLast(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateSkillDamageBlockModeSuccessRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateCurseBlockModeSuccessRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateKnockdownBlockModeSuccessRate(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void	CalculateHtbBlockModeSuccessRate(float fValue, BYTE byApplyType, bool bIsPlus);
	//virtual void	CalculateItemUpgradeBonusRate() {}
	//virtual void	CalculateItemUpgradeBreakBonusRate() {}
	virtual void	CalculateExpBooster(float fValue, BYTE byApplyType, bool bIsPlus);
	//virtual void	CalculateQuestDropRate() {}

//GETTER

	WORD	GetBaseCurseSuccessRate() { return m_pAttribute.wBaseCurseSuccessRate; }
	float	GetBaseRunSpeed() { return m_pAttribute.fBaseRunSpeed; }
	float	GetBaseFlySpeed() { return m_pAttribute.fBaseFlySpeed; }
	float	GetBaseFlyDashSpeed() { return m_pAttribute.fBaseFlyDashSpeed; }
	float	GetBaseFlyAccelSpeed() { return m_pAttribute.fBaseFlyAccelSpeed; }

	void	SetLastStr(int nStat) { m_pAttribute.lastStr = nStat; }
	int		GetLastStr() { return m_pAttribute.lastStr; }
	int		GetBaseStr() { return m_pAttribute.baseStr; }

	void	SetLastCon(int nCon) { m_pAttribute.lastCon = nCon; }
	int		GetLastCon() { return m_pAttribute.lastCon; }
	int		GetBaseCon() { return m_pAttribute.baseCon; }

	void	SetLastFoc(int nStat) { m_pAttribute.lastFoc = nStat; }
	int		GetLastFoc() { return m_pAttribute.lastFoc; }
	int		GetBaseFoc() { return m_pAttribute.baseFoc; }

	void	SetLastDex(int nStat) { m_pAttribute.lastDex = nStat; }
	int		GetLastDex() { return m_pAttribute.lastDex; }
	int		GetBaseDex() { return m_pAttribute.baseDex; }

	void	SetLastSol(int nStat) { m_pAttribute.lastSol = nStat; }
	int		GetLastSol() { return m_pAttribute.lastSol; }
	int		GetBaseSol() { return m_pAttribute.baseSol; }

	void	SetLastEng(int nStat) { m_pAttribute.lastEng = nStat; }
	int		GetLastEng() { return m_pAttribute.lastEng; }

	int		GetLastMaxLP() { return m_pAttribute.lastMaxLp; }
	WORD	GetLastMaxEP() { return m_pAttribute.wLastMaxEP; }
	int		GetLastMaxAP() { return m_pAttribute.lastMaxAp; }
	WORD	GetLastMaxRP() { return m_pAttribute.wLastMaxRP; }

	WORD	GetLastLPRegen() { return m_pAttribute.wLastLpRegen; }
	WORD	GetLastLPSitdownRegen() { return m_pAttribute.wLastLpSitdownRegen; }
	WORD	GetLastLPBattleRegen() { return m_pAttribute.wLastLpBattleRegen; }
	WORD	GetLastEPRegen() { return m_pAttribute.wLastEpRegen; }
	WORD	GetLastEPSitdownRegen() { return m_pAttribute.wLastEpSitdownRegen; }
	WORD	GetLastEPBattleRegen() { return m_pAttribute.wLastEpBattleRegen; }
	WORD	GetLastAPRegen() { return m_pAttribute.wLastApRegen; }
	WORD	GetLastAPSitdownRegen() { return m_pAttribute.wLastApSitdownRegen; }
	WORD	GetLastAPBattleRegen() { return m_pAttribute.wLastApBattleRegen; }
	WORD	GetLastAPDegen() { return m_pAttribute.wLastApDegen; }
	WORD	GetLastAPBattleDegen() { return m_pAttribute.wLastApBattleDegen; }
	WORD	GetLastRPRegen() { return m_pAttribute.wLastRpRegen; }
	WORD	GetLastRPDimimutionRate() { return m_pAttribute.wLastRpDimimutionRate; }
	WORD	GetLastPhysicalOffence() { return m_pAttribute.wLastPhysicalOffence; }
	WORD	GetLastPhysicalDefence() { return m_pAttribute.wLastPhysicalDefence; }
	void	SetLastPhysicalDefence(WORD wDef) { m_pAttribute.wLastPhysicalDefence = wDef; }
	WORD	GetLastEnergyOffence() { return m_pAttribute.wLastEnergyOffence; }
	WORD	GetLastEnergyDefence() { return m_pAttribute.wLastEnergyDefence; }
	WORD	GetLastAttackRate() { return m_pAttribute.wLastAttackRate; }
	WORD	GetLastDodgeRate() { return m_pAttribute.wLastDodgeRate; }
	WORD	GetLastBlockRate() { return m_pAttribute.wLastBlockRate; }
	WORD	GetBaseBlockRate() { return m_pAttribute.wBaseBlockRate; }
	WORD	GetLastBlockDamageRate() { return m_pAttribute.wLastBlockDamageRate; }
	WORD	GetLastCurseSuccessRate() { return m_pAttribute.wLastCurseSuccessRate; }
	WORD	GetLastCurseToleranceRate() { return m_pAttribute.wLastCurseToleranceRate; }
	WORD	GetLastPhysicalCriticalRate() { return m_pAttribute.wLastPhysicalCriticalRate; }
	WORD	GetLastEnergyCriticalRate() { return m_pAttribute.wLastEnergyCriticalRate; }
	float	GetLastPhysicalCriticalDamageRate() { return m_pAttribute.fLastPhysicalCriticalDamageRate; }
	float	GetBasePhysicalCriticalDamageRate() { return m_pAttribute.fBasePhysicalCriticalDamageRate; }
	float	GetLastEnergyCriticalDamageRate() { return m_pAttribute.fLastEnergyCriticalDamageRate; }
	float	GetBaseEnergyCriticalDamageRate() { return m_pAttribute.fBaseEnergyCriticalDamageRate; }

	float	GetLastWalkSpeed() { return m_pAttribute.fBaseRunSpeed; }
	float	GetLastRunSpeed() { return m_pAttribute.fLastRunSpeed; }
	float	GetLastFlySpeed() { return m_pAttribute.fLastFlySpeed; }
	float	GetLastFlyDashSpeed() { return m_pAttribute.fLastFlyDashSpeed; }
	float	GetLastFlyAccelSpeed() { return m_pAttribute.fLastFlyAccelSpeed; }

	WORD	GetLastAttackSpeedRate() { return m_pAttribute.wLastAttackSpeedRate; }

	void	SetLastAttackRange(float fRange) { m_pAttribute.fLastAttackRange = fRange; }
	float	GetLastAttackRange() { return m_pAttribute.fLastAttackRange; }

	float	GetCastingTimeChangePercent() { return m_pAttribute.fCastingTimeChangePercent; }
	float	GetCoolTimeChangePercent() { return m_pAttribute.fCoolTimeChangePercent; }
	float	GetKeepTimeChangePercent() { return m_pAttribute.fKeepTimeChangePercent; }
	float	GetDotTimeChangeAbsolute() { return m_pAttribute.fDotTimeChangeAbsolute; }
	float	GetRequiredEpChangePercent() { return m_pAttribute.fRequiredEpChangePercent; }

	float	GetLastHonestOffence() { return m_pAttribute.fHonestOffence; }
	float	GetLastHonestDefence() { return m_pAttribute.fHonestDefence; }
	float	GetLastStrangeOffence() { return m_pAttribute.fStrangeOffence; }
	float	GetLastStrangeDefence() { return m_pAttribute.fStrangeDefence; }
	float	GetLastWildOffence() { return m_pAttribute.fWildOffence; }
	float	GetLastWildDefence() { return m_pAttribute.fWildDefence; }
	float	GetLastEleganceOffence() { return m_pAttribute.fEleganceOffence; }
	float	GetLastEleganceDefence() { return m_pAttribute.fEleganceDefence; }
	float	GetLastFunnyOffence() { return m_pAttribute.fFunnyOffence; }
	float	GetLastFunnyDefence() { return m_pAttribute.fFunnyDefence; }

	float	GetLastPhysicalReflection() { return m_pAttribute.fPhysicalReflection; }
	float	GetLastEnergyReflection() { return m_pAttribute.fEnergyReflection; }

	WORD	GetLastParalyzeToleranceRate() { return m_pAttribute.wParalyzeToleranceRate; }
	WORD	GetLastTerrorToleranceRate() { return m_pAttribute.wTerrorToleranceRate; }
	WORD	GetLastConfuseToleranceRate() { return m_pAttribute.wConfuseToleranceRate; }
	WORD	GetLastStoneToleranceRate() { return m_pAttribute.wStoneToleranceRate; }
	WORD	GetLastCandyToleranceRate() { return m_pAttribute.wCandyToleranceRate; }

	float	GetLastParalyzeKeepTimeDown() { return m_pAttribute.fParalyzeKeepTimeDown; }
	float	GetLastTerrorKeepTimeDown() { return m_pAttribute.fTerrorKeepTimeDown; }
	float	GetLastConfuseKeepTimeDown() { return m_pAttribute.fConfuseKeepTimeDown; }
	float	GetLastStoneKeepTimeDown() { return m_pAttribute.fStoneKeepTimeDown; }
	float	GetLastCandyKeepTimeDown() { return m_pAttribute.fCandyKeepTimeDown; }
	float	GetLastBleedingKeepTimeDown() { return m_pAttribute.fBleedingKeepTimeDown; }
	float	GetLastPoisonKeepTimeDown() { return m_pAttribute.fPoisonKeepTimeDown; }
	float	GetLastStomachacheKeepTimeDown() { return m_pAttribute.fStomachacheKeepTimeDown; }

	float	GetLastCriticalBlockSuccessRate() { return m_pAttribute.fCriticalBlockSuccessRate; }
	DWORD	GetLastLpRecoveryWhenHit() { return m_pAttribute.dwLpRecoveryWhenHit; }
	float	GetLastLpRecoveryWhenHitInPercent() { return m_pAttribute.fLpRecoveryWhenHitInPercent; }
	DWORD	GetLastEpRecoveryWhenHit() { return m_pAttribute.dwEpRecoveryWhenHit; }
	float	GetLastEpRecoveryWhenHitInPercent() { return m_pAttribute.fEpRecoveryWhenHitInPercent; }

	WORD	GetLastStomachacheDefence() { return m_pAttribute.wStomachacheDefenceLast; }
	WORD	GetLastPoisonDefence() { return m_pAttribute.wPoisonDefenceLast; }
	WORD	GetLastBleedDefence() { return m_pAttribute.wBleedDefenceLast; }
	WORD	GetLastBurnDefence() { return m_pAttribute.wBurnDefenceLast; }
	float	GetLastMindCurseImmunity() { return m_pAttribute.fLastMindCurseImmunity; }
	float	GetLastBodyCurseImmunity() { return m_pAttribute.fLastBodyCurseImmunity; }
	float	GetLastChangeCurseImmunity() { return m_pAttribute.fLastChangeCurseImmunity; }
	float	GetLastSkillAnimationSpeedModifier() { return m_pAttribute.fLastSkillAnimationSpeedModifier; }
	DWORD	GetLastWeightLimit() { return m_pAttribute.dwLastWeightLimit; }
	float	GetLastSkillAggroBonus() { return m_pAttribute.fSkillAggroBonus; }
	float	GetLastSkillAggroBonusInPercent() { return m_pAttribute.fSkillAggroBonusInPercent; }
	float	GetLastDirectHealPowerBonus() { return m_pAttribute.fDirectHealPowerBonus; }
	float	GetLastDirectHealPowerBonusInPercent() { return m_pAttribute.fDirectHealPowerBonusInPercent; }
	float	GetLastHotPowerBonus() { return m_pAttribute.fHotPowerBonus; }
	float	GetLastHotPowerBonusInPercent() { return m_pAttribute.fHotPowerBonusInPercent; }
	float	GetLastDotValueChangePercent() { return m_pAttribute.fDotValueChangePercent; }
	BYTE	GetBattleAttributeOffence() { return m_abyBattle_Attribute[0]; }
	BYTE	GetBattleAttributeOffenceSub() { return m_abyBattle_Attribute[2]; }
	BYTE	GetBattleAttributeDefence() { return m_abyBattle_Attribute[1]; }
//	float	GetLastPhysicalCriticalDefenceRate() { return m_pAttribute.fPhysicalCriticalDefenceRate; } //this is used not anymore. Use GetLastCriticalBlockSuccessRate instead
//	float	GetLastEnergyCriticalDefenceRate() { return m_pAttribute.fEnergyCriticalDefenceRate; } //this is used not anymore. Use GetLastCriticalBlockSuccessRate instead

	float	GetSkillDamageBlockModeSuccessRate() { return m_pAttribute.fSkillDamageBlockModeSuccessRate; }
	float	GetCurseBlockModeSuccessRate() { return m_pAttribute.fCurseBlockModeSuccessRate; }
	float	GetKnockDownBlockModeSuccessRate() { return m_pAttribute.fKnockdownBlockModeSuccessRate; }
	float	GetHtbKnockDownBlockModeSuccessRate() { return m_pAttribute.fHtbBlockModeSuccessRate; }

	WORD	GetLastGuardRate() { return m_pAttribute.wGuardRateLast; }
	WORD	GetBaseGuardRate() { return m_pAttribute.wGuardRateBase; }

	BYTE	GetExpBoost() { return m_pAttribute.byExpBooster; }

//SETTER
	void	SetLastMaxLP(int nLastMaxLP) { m_pAttribute.lastMaxLp = nLastMaxLP; }
	void	SetLastMaxEP(WORD wLastMaxEP) { m_pAttribute.wLastMaxEP = wLastMaxEP; }

	void	SetLastRunSpeed(float fSpeed) { m_pAttribute.fLastRunSpeed = fSpeed; }
	void	SetLastFlySpeed(float fSpeed) { m_pAttribute.fLastFlySpeed = fSpeed; }
	void	SetLastFlyDashSpeed(float fSpeed) { m_pAttribute.fLastFlyDashSpeed = fSpeed; }
	void	SetLastFlyAccelSpeed(float fSpeed) { m_pAttribute.fLastFlyAccelSpeed = fSpeed; }

	void	SetLastAttackSpeedRate(WORD wSpeed) { m_pAttribute.wLastAttackSpeedRate = wSpeed; }

	void	SetLastSkillAnimationSpeedModifier(float fSpeed) { m_pAttribute.fLastSkillAnimationSpeedModifier = fSpeed; }

	void	SetBattleAttributeOffence(BYTE byAttribute) { m_abyBattle_Attribute[0] = byAttribute; }
	void	SetBattleAttributeOffenceSub(BYTE byAttribute) { m_abyBattle_Attribute[2] = byAttribute; }
	void	SetBattleAttributeDefence(BYTE byAttribute) { m_abyBattle_Attribute[1] = byAttribute; }

	void	SetLastPhysicalOffence(WORD wAttack) { m_pAttribute.wLastPhysicalOffence = wAttack; }
	void	SetLastEnergyOffence(WORD wAttack) { m_pAttribute.wLastEnergyOffence = wAttack; }

	void	SetSubOffence(WORD wOffence) { m_wSubWeaponPhysicalOffence = wOffence; m_wSubWeaponEnergyOffence = wOffence; }
	WORD	GetSubWeaponPhysicalOffence() { return m_wSubWeaponPhysicalOffence; }
	WORD	GetSubWeaponEnergyOffence() { return m_wSubWeaponEnergyOffence; }

	bool	HasAnyProp();

protected:

	CCharacter*				m_pOwnerRef;
	sAVATAR_ATTRIBUTE		m_pAttribute;
	BYTE					m_abyBattle_Attribute[3];		//eBATTLE_ATTRIBUTE //player 2 and other 1 [0] Offence (main), [1] = defence, [2] = Offence (sub weapon)

	float					m_fRunSpeedBackup;
	WORD					m_wAttackSpeedBackup;

	WORD					m_wSubWeaponPhysicalOffence;
	WORD					m_wSubWeaponEnergyOffence;

	float					m_fCurLpPercent;
	float					m_fCurEpPercent;
	float					m_fCurApPercent;

	float					m_fSettingMaxLP;
	float					m_fSettingPhysicalDefence;
	float					m_fSettingEnergyDefence;
	float					m_fSettingPhysicalOffence;
	float					m_fSettingEnergyOffence;

	// percent values
	struct sPERCENT_VALUES
	{
		float					m_fSTR;
		float					m_fCON;
		float					m_fFOC;
		float					m_fDEX;
		float					m_fSOL;
		float					m_fENG;
		float					m_fLP;
		float					m_fEP;
		float					m_fAP;
		float					m_fRP;
		float					m_fLpRegen;
		float					m_fLpSitdownRegen;
		float					m_fLpBattleRegen;
		float					m_fEpRegen;
		float					m_fEpSitdownRegen;
		float					m_fEpBattleRegen;
		float					m_fApRegen;
		float					m_fApSitdownRegen;
		float					m_fApBattleRegen;
		float					m_fApDegen;
		float					m_fApBattleDegen;
		float					m_fRpRegen;
		float					m_fRpDimimutionRate;
		float					m_fPhysicalOffence;
		float					m_fPhysicalDefence;
		float					m_fEnergyOffence;
		float					m_fEnergyDefence;
		float					m_fAttackRate;
		float					m_fDodgeRate;
		float					m_fBlockRate;
		float					m_fBlockDamageRate;
		float					m_fCurseSuccessRate;
		float					m_fCurseToleranceRate;
		float					m_fPhysicalCriticalRate;
		float					m_fEnergyCriticalRate;
		float					m_fPhysicalCriticalDamageRate;
		float					m_fEnergyCriticalDamageRate;
		float					m_fRunSpeed;
		float					m_fFlySpeed;
		float					m_fFlyDashSpeed;
		float					m_fFlyAccelSpeed;
		float					m_fAttackSpeedRate;
		float					m_fAttackRange;
		float					m_fStomachacheDef;
		float					m_fPoisonDef;
		float					m_fBleedDef;
		float					m_fBurnDef;
		float					m_fMindCurseImmunity;
		float					m_fBodyCurseImmunity;
		float					m_fChangeCurseImmunity;
		float					m_fSkillAnimationSpeedModifier;
		float					m_fGuardRate;

		float					m_fSTRNegative;
		float					m_fCONNegative;
		float					m_fFOCNegative;
		float					m_fDEXNegative;
		float					m_fSOLNegative;
		float					m_fENGNegative;
		float					m_fLPNegative;
		float					m_fEPNegative;
		float					m_fAPNegative;
		float					m_fRPNegative;
		float					m_fLpRegenNegative;
		float					m_fLpSitdownRegenNegative;
		float					m_fLpBattleRegenNegative;
		float					m_fEpRegenNegative;
		float					m_fEpSitdownRegenNegative;
		float					m_fEpBattleRegenNegative;
		float					m_fApRegenNegative;
		float					m_fApSitdownRegenNegative;
		float					m_fApBattleRegenNegative;
		float					m_fApDegenNegative;
		float					m_fApBattleDegenNegative;
		float					m_fRpRegenNegative;
		float					m_fRpDimimutionRateNegative;
		float					m_fPhysicalOffenceNegative;
		float					m_fPhysicalDefenceNegative;
		float					m_fEnergyOffenceNegative;
		float					m_fEnergyDefenceNegative;
		float					m_fAttackRateNegative;
		float					m_fDodgeRateNegative;
		float					m_fBlockRateNegative;
		float					m_fBlockDamageRateNegative;
		float					m_fCurseSuccessRateNegative;
		float					m_fCurseToleranceRateNegative;
		float					m_fPhysicalCriticalRateNegative;
		float					m_fEnergyCriticalRateNegative;
		float					m_fPhysicalCriticalDamageRateNegative;
		float					m_fEnergyCriticalDamageRateNegative;
		float					m_fRunSpeedNegative;
		float					m_fFlySpeedNegative;
		float					m_fFlyDashSpeedNegative;
		float					m_fFlyAccelSpeedNegative;
		float					m_fAttackSpeedRateNegative;
		float					m_fAttackRangeNegative;
		float					m_fStomachacheDefNegative;
		float					m_fPoisonDefNegative;
		float					m_fBleedDefNegative;
		float					m_fBurnDefNegative;
		float					m_fMindCurseImmunityNegative;
		float					m_fBodyCurseImmunityNegative;
		float					m_fChangeCurseImmunityNegative;
		float					m_fSkillAnimationSpeedModifierNegative;
		float					m_fGuardRateNegative;
	}
	m_percentValue;
};

#endif