#pragma once

enum eATTRIBUTE_TO_UPDATE
{
	ATTRIBUTE_TO_UPDATE_STR_BASE,
	ATTRIBUTE_TO_UPDATE_STR_LAST,
	ATTRIBUTE_TO_UPDATE_CON_BASE,
	ATTRIBUTE_TO_UPDATE_CON_LAST,
	ATTRIBUTE_TO_UPDATE_FOC_BASE,
	ATTRIBUTE_TO_UPDATE_FOC_LAST,
	ATTRIBUTE_TO_UPDATE_DEX_BASE,
	ATTRIBUTE_TO_UPDATE_DEX_LAST,
	ATTRIBUTE_TO_UPDATE_SOL_BASE,
	ATTRIBUTE_TO_UPDATE_SOL_LAST,
	ATTRIBUTE_TO_UPDATE_ENG_BASE,
	ATTRIBUTE_TO_UPDATE_ENG_LAST,

	ATTRIBUTE_TO_UPDATE_MAX_LP_BASE,
	ATTRIBUTE_TO_UPDATE_MAX_LP_LAST,
	ATTRIBUTE_TO_UPDATE_MAX_EP_BASE,
	ATTRIBUTE_TO_UPDATE_MAX_EP_LAST,
	ATTRIBUTE_TO_UPDATE_MAX_AP_BASE,//new
	ATTRIBUTE_TO_UPDATE_MAX_AP_LAST,//new
	ATTRIBUTE_TO_UPDATE_MAX_RP_BASE,
	ATTRIBUTE_TO_UPDATE_MAX_RP_LAST,

	ATTRIBUTE_TO_UPDATE_LP_REGENERATION_BASE,
	ATTRIBUTE_TO_UPDATE_LP_REGENERATION_LAST,
	ATTRIBUTE_TO_UPDATE_LP_SITDOWN_REGENERATION_BASE,
	ATTRIBUTE_TO_UPDATE_LP_SITDOWN_REGENERATION_LAST,
	ATTRIBUTE_TO_UPDATE_LP_BATTLE_REGENERATION_BASE,
	ATTRIBUTE_TO_UPDATE_LP_BATTLE_REGENERATION_LAST,

	ATTRIBUTE_TO_UPDATE_EP_REGENERATION_BASE,
	ATTRIBUTE_TO_UPDATE_EP_REGENERATION_LAST,
	ATTRIBUTE_TO_UPDATE_EP_SITDOWN_REGENERATION_BASE,
	ATTRIBUTE_TO_UPDATE_EP_SITDOWN_REGENERATION_LAST,
	ATTRIBUTE_TO_UPDATE_EP_BATTLE_REGENERATION_BASE,
	ATTRIBUTE_TO_UPDATE_EP_BATTLE_REGENERATION_LAST,

	ATTRIBUTE_TO_UPDATE_AP_REGENERATION_BASE,//new
	ATTRIBUTE_TO_UPDATE_AP_REGENERATION_LAST,//new
	ATTRIBUTE_TO_UPDATE_AP_SITDOWN_REGENERATION_BASE,//new
	ATTRIBUTE_TO_UPDATE_AP_SITDOWN_REGENERATION_LAST,//new
	ATTRIBUTE_TO_UPDATE_AP_BATTLE_REGENERATION_BASE,//new
	ATTRIBUTE_TO_UPDATE_AP_BATTLE_REGENERATION_LAST,//new
	ATTRIBUTE_TO_UPDATE_AP_DEGENERATION_BASE,//new
	ATTRIBUTE_TO_UPDATE_AP_DEGENERATION_LAST,//new
	ATTRIBUTE_TO_UPDATE_AP_BATTLE_DEGENERATION_BASE,//new
	ATTRIBUTE_TO_UPDATE_AP_BATTLE_DEGENERATION_LAST,//new

	ATTRIBUTE_TO_UPDATE_RP_CHARGE_SPEED_BASE,
	ATTRIBUTE_TO_UPDATE_RP_CHARGE_SPEED_LAST,
	ATTRIBUTE_TO_UPDATE_RP_DIMIMUTION,

	ATTRIBUTE_TO_UPDATE_PHYSICAL_OFFENCE_BASE,
	ATTRIBUTE_TO_UPDATE_PHYSICAL_OFFENCE_LAST,
	ATTRIBUTE_TO_UPDATE_PHYSICAL_DEFENCE_BASE,
	ATTRIBUTE_TO_UPDATE_PHYSICAL_DEFENCE_LAST,

	ATTRIBUTE_TO_UPDATE_ENERGY_OFFENCE_BASE,
	ATTRIBUTE_TO_UPDATE_ENERGY_OFFENCE_LAST,
	ATTRIBUTE_TO_UPDATE_ENERGY_DEFENCE_BASE,
	ATTRIBUTE_TO_UPDATE_ENERGY_DEFENCE_LAST,

	ATTRIBUTE_TO_UPDATE_ATTACK_RATE_BASE,
	ATTRIBUTE_TO_UPDATE_ATTACK_RATE_LAST,
	ATTRIBUTE_TO_UPDATE_DODGE_RATE_BASE,
	ATTRIBUTE_TO_UPDATE_DODGE_RATE_LAST,

	ATTRIBUTE_TO_UPDATE_BLOCK_RATE_BASE,
	ATTRIBUTE_TO_UPDATE_BLOCK_RATE_LAST,
	ATTRIBUTE_TO_UPDATE_BLOCK_DAMAGE_RATE_BASE,//new
	ATTRIBUTE_TO_UPDATE_BLOCK_DAMAGE_RATE_LAST,//new

	ATTRIBUTE_TO_UPDATE_CURSE_SUCCESS_BASE,
	ATTRIBUTE_TO_UPDATE_CURSE_SUCCESS_LAST,
	ATTRIBUTE_TO_UPDATE_CURSE_TOLERANCE_BASE,
	ATTRIBUTE_TO_UPDATE_CURSE_TOLERANCE_LAST,

	ATTRIBUTE_TO_UPDATE_PHYSICAL_CRITICAL_BASE,
	ATTRIBUTE_TO_UPDATE_PHYSICAL_CRITICAL_LAST,
	ATTRIBUTE_TO_UPDATE_ENERGY_CRITICAL_BASE,
	ATTRIBUTE_TO_UPDATE_ENERGY_CRITICAL_LAST,

	ATTRIBUTE_TO_UPDATE_PHYSICAL_CRITICAL_DAMAGE_RATE_BASE,//new
	ATTRIBUTE_TO_UPDATE_PHYSICAL_CRITICAL_DAMAGE_RATE_LAST,//new
	ATTRIBUTE_TO_UPDATE_ENERGY_CRITICAL_DAMAGE_RATE_BASE,//new
	ATTRIBUTE_TO_UPDATE_ENERGY_CRITICAL_DAMAGE_RATE_LAST, //new

	ATTRIBUTE_TO_UPDATE_RUN_SPEED_BASE,//updated
	ATTRIBUTE_TO_UPDATE_RUN_SPEED_LAST,//new
	ATTRIBUTE_TO_UPDATE_FLY_SPEED_BASE,//new
	ATTRIBUTE_TO_UPDATE_FLY_SPEED_LAST,//new
	ATTRIBUTE_TO_UPDATE_FLY_DASH_SPEED_BASE,//new
	ATTRIBUTE_TO_UPDATE_FLY_DASH_SPEED_LAST,//new
	ATTRIBUTE_TO_UPDATE_FLY_ACCEL_SPEED_BASE,//new
	ATTRIBUTE_TO_UPDATE_FLY_ACCEL_SPEED_LAST,//new

	ATTRIBUTE_TO_UPDATE_ATTACK_SPEED_RATE_BASE,
	ATTRIBUTE_TO_UPDATE_ATTACK_SPEED_RATE_LAST,
	ATTRIBUTE_TO_UPDATE_ATTACK_RANGE_BASE,
	ATTRIBUTE_TO_UPDATE_ATTACK_RANGE_LAST,

	ATTRIBUTE_TO_UPDATE_CASTING_TIME_CHANGE_PERCENT,
	ATTRIBUTE_TO_UPDATE_COOL_TIME_CHANGE_PERCENT,
	ATTRIBUTE_TO_UPDATE_KEEP_TIME_CHANGE_PERCENT,
	ATTRIBUTE_TO_UPDATE_DOT_TIME_CHANGE_ABSOLUTE,
	ATTRIBUTE_TO_UPDATE_REQUIRED_EP_CHANGE_PERCENT,

	ATTRIBUTE_TO_UPDATE_HONEST_OFFENCE,
	ATTRIBUTE_TO_UPDATE_HONEST_DEFENCE,
	ATTRIBUTE_TO_UPDATE_STRANGE_OFFENCE,
	ATTRIBUTE_TO_UPDATE_STRANGE_DEFENCE,
	ATTRIBUTE_TO_UPDATE_WILD_OFFENCE,
	ATTRIBUTE_TO_UPDATE_WILD_DEFENCE,
	ATTRIBUTE_TO_UPDATE_ELEGANCE_OFFENCE,
	ATTRIBUTE_TO_UPDATE_ELEGANCE_DEFENCE,
	ATTRIBUTE_TO_UPDATE_FUNNY_OFFENCE,
	ATTRIBUTE_TO_UPDATE_FUNNY_DEFENCE,

	ATTRIBUTE_TO_UPDATE_PHYSICAL_REFLECTION,//new
	ATTRIBUTE_TO_UPDATE_ENERGY_REFLECTION,

	ATTRIBUTE_TO_UPDATE_PARALYZE_TOLERANCE_RATE,
	ATTRIBUTE_TO_UPDATE_TERROR_TOLERANCE_RATE,
	ATTRIBUTE_TO_UPDATE_CONFUSE_TOLERANCE_RATE,
	ATTRIBUTE_TO_UPDATE_STONE_TOLERANCE_RATE,
	ATTRIBUTE_TO_UPDATE_CANDY_TOLERANCE_RATE,

	ATTRIBUTE_TO_UPDATE_PARALYZE_KEEP_TIME_DOWN,
	ATTRIBUTE_TO_UPDATE_TERROR_KEEP_TIME_DOWN,
	ATTRIBUTE_TO_UPDATE_CONFUSE_KEEP_TIME_DOWN,
	ATTRIBUTE_TO_UPDATE_STONE_KEEP_TIME_DOWN,
	ATTRIBUTE_TO_UPDATE_CANDY_KEEP_TIME_DOWN,
	ATTRIBUTE_TO_UPDATE_BLEEDING_KEEP_TIME_DOWN,
	ATTRIBUTE_TO_UPDATE_POISON_KEEP_TIME_DOWN,
	ATTRIBUTE_TO_UPDATE_STOMACHACHE_KEEP_TIME_DOWN,

	ATTRIBUTE_TO_UPDATE_CRITICAL_BLOCK_SUCCESS_RATE,

	ATTRIBUTE_TO_UPDATE_LP_RECOVERY_WHEN_HIT,//new
	ATTRIBUTE_TO_UPDATE_LP_RECOVERY_WHEN_HIT_IN_PERCENT,//new
	ATTRIBUTE_TO_UPDATE_EP_RECOVERY_WHEN_HIT,//new
	ATTRIBUTE_TO_UPDATE_EP_RECOVERY_WHEN_HIT_IN_PERCENT,//new

	ATTRIBUTE_TO_UPDATE_STOMACHACHE_DEFENCE_BASE,//new
	ATTRIBUTE_TO_UPDATE_STOMACHACHE_DEFENCE_LAST,//new
	ATTRIBUTE_TO_UPDATE_POISON_DEFENCE_BASE,//new
	ATTRIBUTE_TO_UPDATE_POISON_DEFENCE_LAST,//new
	ATTRIBUTE_TO_UPDATE_BLEED_DEFENCE_BASE,//new
	ATTRIBUTE_TO_UPDATE_BLEED_DEFENCE_LAST,//new
	ATTRIBUTE_TO_UPDATE_BURN_DEFENCE_BASE,//new
	ATTRIBUTE_TO_UPDATE_BURN_DEFENCE_LAST,//new
	ATTRIBUTE_TO_UPDATE_MIND_CURSE_IMMUNITY_BASE,//new
	ATTRIBUTE_TO_UPDATE_MIND_CURSE_IMMUNITY_LAST,//new
	ATTRIBUTE_TO_UPDATE_BODY_CURSE_IMMUNITY_BASE,//new
	ATTRIBUTE_TO_UPDATE_BODY_CURSE_IMMUNITY_LAST,//new
	ATTRIBUTE_TO_UPDATE_CHANGE_CURSE_IMMUNITY_BASE,//new
	ATTRIBUTE_TO_UPDATE_CHANGE_CURSE_IMMUNITY_LAST,//new
	ATTRIBUTE_TO_UPDATE_SKILL_ANIMATION_SPEED_MODIFIER_BASE,//new
	ATTRIBUTE_TO_UPDATE_SKILL_ANIMATION_SPEED_MODIFIER_LAST,//new
	ATTRIBUTE_TO_UPDATE_WEIGHT_LIMIT_BASE,//new
	ATTRIBUTE_TO_UPDATE_WEIGHT_LIMIT_LAST,//new
	ATTRIBUTE_TO_UPDATE_SKILL_AGGRO_BONUS,//new
	ATTRIBUTE_TO_UPDATE_SKILL_AGGRO_BONUS_IN_PERCENT,//new

	ATTRIBUTE_TO_UPDATE_DIRECT_HEAL_POWER_BONUS,//new
	ATTRIBUTE_TO_UPDATE_DIRECT_HEAL_POWER_BONUS_IN_PERCENT,//new
	ATTRIBUTE_TO_UPDATE_HOT_POWER_BONUS,//new
	ATTRIBUTE_TO_UPDATE_HOT_POWER_BONUS_IN_PERCENT,//new
	ATTRIBUTE_TO_UPDATE_DOT_VALUE_CHANGE_PERCENT,//new
	ATTRIBUTE_TO_UPDATE_PHYSICAL_CRITICAL_DEFENCE_RATE,//new
	ATTRIBUTE_TO_UPDATE_ENERGY_CRITICAL_DEFENCE_RATE,//new

	ATTRIBUTE_TO_UPDATE_GUARD_RATE_BASE,
	ATTRIBUTE_TO_UPDATE_GUARD_RATE_LAST,//new

	ATTRIBUTE_TO_UPDATE_SKILL_DAMAGE_BLOCK_MODE_SUCCESS_RATE_LAST,
	ATTRIBUTE_TO_UPDATE_CURSE_BLOCK_MODE_SUCCESS_RATE_LAST,
	ATTRIBUTE_TO_UPDATE_KNOCKDOWN_BLOCK_MODE_SUCCESS_RATE_LAST,
	ATTRIBUTE_TO_UPDATE_HTB_BLOCK_MODE_SUCCESS_RATE_LAST,

	ATTRIBUTE_TO_UPDATE_ITEM_UPGRADE_RATE_UP,
	ATTRIBUTE_TO_UPDATE_ITEM_BREAK_RATE_DOWN,
	ATTRIBUTE_TO_UPDATE_EXP_BOOSTER,
	ATTRIBUTE_TO_UPDATE_QUEST_DROP_UP,

	ATTRIBUTE_TO_UPDATE_COUNT,
	ATTRIBUTE_TO_UPDATE_UNKNOWN	= 0xFF,

	ATTRIBUTE_TO_UPDATE_FIRST = ATTRIBUTE_TO_UPDATE_STR_BASE,
	ATTRIBUTE_TO_UPDATE_LAST = ATTRIBUTE_TO_UPDATE_COUNT - 1,
};

#pragma pack(push, 1)

struct sAVATAR_ATTRIBUTE
{
	WORD baseStr; //absolute
	WORD lastStr; //absolute
	WORD baseCon; //absolute
	WORD lastCon; //absolute
	WORD baseFoc; //absolute
	WORD lastFoc; //absolute
	WORD baseDex; //absolute
	WORD lastDex; //absolute
	WORD baseSol; //absolute
	WORD lastSol; //absolute
	WORD baseEng; //absolute
	WORD lastEng; //absolute

	int baseMaxLp; //absolute
	int lastMaxLp; //absolute
	WORD wBaseMaxEP; //absolute
	WORD wLastMaxEP; //absolute
	int baseMaxAp; //absolute
	int lastMaxAp; //absolute
	WORD wBaseMaxRP; //absolute
	WORD wLastMaxRP; //absolute

	WORD wBaseLpRegen; //absolute
	WORD wLastLpRegen; //absolute
	WORD wBaseLpSitdownRegen; //absolute
	WORD wLastLpSitdownRegen; //absolute
	WORD wBaseLpBattleRegen; //absolute
	WORD wLastLpBattleRegen; //absolute

	WORD wBaseEpRegen; //absolute
	WORD wLastEpRegen; //absolute
	WORD wBaseEpSitdownRegen; //absolute
	WORD wLastEpSitdownRegen; //absolute
	WORD wBaseEpBattleRegen; //absolute
	WORD wLastEpBattleRegen; //absolute

	WORD wBaseApRegen; //absolute
	WORD wLastApRegen; //absolute
	WORD wBaseApSitdownRegen; //absolute
	WORD wLastApSitdownRegen; //absolute
	WORD wBaseApBattleRegen; //absolute
	WORD wLastApBattleRegen; //absolute
	WORD wBaseApDegen; //absolute
	WORD wLastApDegen; //absolute
	WORD wBaseApBattleDegen; //absolute
	WORD wLastApBattleDegen; //absolute

	WORD wBaseRpRegen; //absolute
	WORD wLastRpRegen; //absolute		RP rate increase. (/ Second)
	WORD wLastRpDimimutionRate;			//RP 감소 속도.(/second)

	WORD wBasePhysicalOffence; //absolute
	WORD wLastPhysicalOffence; //absolute
	WORD wBasePhysicalDefence; //absolute
	WORD wLastPhysicalDefence; //absolute
//	WORD wBasePhysicalPiercingOffence;
//	WORD wLastPhysicalPiercingOffence;

	WORD wBaseEnergyOffence; //absolute
	WORD wLastEnergyOffence; //absolute
	WORD wBaseEnergyDefence; //absolute
	WORD wLastEnergyDefence; //absolute
//	WORD wBaseEnergyPiercingOffence;
//	WORD wLastEnergyPiercingOffence;

	WORD wBaseAttackRate; //percent
	WORD wLastAttackRate; //percent
	WORD wBaseDodgeRate; //percent
	WORD wLastDodgeRate; //percent

	WORD wBaseBlockRate; //percent
	WORD wLastBlockRate; //percent
	WORD wBaseBlockDamageRate; //percent
	WORD wLastBlockDamageRate; //percent

	WORD wBaseCurseSuccessRate; //percent
	WORD wLastCurseSuccessRate; //percent
	WORD wBaseCurseToleranceRate; //percent
	WORD wLastCurseToleranceRate; //percent

	WORD wBasePhysicalCriticalRate; //percent
	WORD wLastPhysicalCriticalRate; //percent
	WORD wBaseEnergyCriticalRate; //percent
	WORD wLastEnergyCriticalRate; //percent

	float fBasePhysicalCriticalDamageRate; //percent
	float fLastPhysicalCriticalDamageRate; //percent
	float fBaseEnergyCriticalDamageRate; //percent
	float fLastEnergyCriticalDamageRate; //percent

	float fBaseRunSpeed; //absolute
	float fLastRunSpeed; //absolute
	float fBaseFlySpeed; //absolute
	float fLastFlySpeed; //absolute
	float fBaseFlyDashSpeed; //absolute
	float fLastFlyDashSpeed; //absolute
	float fBaseFlyAccelSpeed; //absolute
	float fLastFlyAccelSpeed; //absolute

	WORD wBaseAttackSpeedRate; //absolute
	WORD wLastAttackSpeedRate; //absolute
	float fBaseAttackRange; //absolute
	float fLastAttackRange; //absolute

	float fCastingTimeChangePercent; //percent
	float fCoolTimeChangePercent; //percent
	float fKeepTimeChangePercent; //percent
	float fDotTimeChangeAbsolute; //absolute
	float fRequiredEpChangePercent; //percent
	float fHonestOffence; //absolute
	float fHonestDefence; //absolute
	float fStrangeOffence; //absolute
	float fStrangeDefence; //absolute
	float fWildOffence; //absolute
	float fWildDefence; //absolute
	float fEleganceOffence; //absolute
	float fEleganceDefence; //absolute
	float fFunnyOffence; //absolute
	float fFunnyDefence; //absolute
	float fPhysicalReflection; //absolute
	float fEnergyReflection; //absolute

	WORD wParalyzeToleranceRate;	//percent
	WORD wTerrorToleranceRate;	//percent
	WORD wConfuseToleranceRate;	//percent
	WORD wStoneToleranceRate;	//percent
	WORD wCandyToleranceRate;	//percent
	float fParalyzeKeepTimeDown;	//percent
	float fTerrorKeepTimeDown;	//percent
	float fConfuseKeepTimeDown;	//percent
	float fStoneKeepTimeDown;	//percent
	float fCandyKeepTimeDown;	//percent
	float fBleedingKeepTimeDown;	//percent
	float fPoisonKeepTimeDown;	//percent
	float fStomachacheKeepTimeDown;	//percent
	float fCriticalBlockSuccessRate;	//percent
	DWORD dwLpRecoveryWhenHit; //absolute
	float fLpRecoveryWhenHitInPercent;	//percent
	DWORD dwEpRecoveryWhenHit; //absolute
	float fEpRecoveryWhenHitInPercent;	//percent
	WORD wStomachacheDefenceBase;
	WORD wStomachacheDefenceLast;
	WORD wPoisonDefenceBase;
	WORD wPoisonDefenceLast;
	WORD wBleedDefenceBase;
	WORD wBleedDefenceLast;
	WORD wBurnDefenceBase;
	WORD wBurnDefenceLast;
	float fBaseMindCurseImmunity;
	float fLastMindCurseImmunity;
	float fBaseBodyCurseImmunity;
	float fLastBodyCurseImmunity;
	float fBaseChangeCurseImmunity;
	float fLastChangeCurseImmunity;
	float fBaseSkillAnimationSpeedModifier;
	float fLastSkillAnimationSpeedModifier;
	DWORD dwBaseWeightLimit;
	DWORD dwLastWeightLimit;
	float fSkillAggroBonus;
	float fSkillAggroBonusInPercent;
	float fDirectHealPowerBonus;
	float fDirectHealPowerBonusInPercent;
	float fHotPowerBonus;
	float fHotPowerBonusInPercent;
	float fDotValueChangePercent;
	float fPhysicalCriticalDefenceRate; //not used
	float fEnergyCriticalDefenceRate; //not used
	WORD wGuardRateBase;
	WORD wGuardRateLast;
	float fSkillDamageBlockModeSuccessRate;
	float fCurseBlockModeSuccessRate;
	float fKnockdownBlockModeSuccessRate;
	float fHtbBlockModeSuccessRate;
	float fItemUpgradeBonusRate;
	float fItemUpgradeBreakBonusRate;
	BYTE byExpBooster;
	BYTE byQuestDropRate;
};

struct sAVATAR_ATTRIBUTE_LINK
{
	WORD *pbaseStr;
	WORD *plastStr;
	WORD *pbaseCon;
	WORD *plastCon;
	WORD *pbaseFoc;
	WORD *plastFoc;
	WORD *pbaseDex;
	WORD *plastDex;
	WORD *pbaseSol;
	WORD *plastSol;
	WORD *pbaseEng;
	WORD *plastEng;

	int *pbaseMaxLp;
	int *plastMaxLp;
	WORD *pwBaseMaxEP;
	WORD *pwLastMaxEP;
	int *pbaseMaxAp;
	int *plastMaxAp;
	WORD *pwBaseMaxRP;
	WORD *pwLastMaxRP;

	WORD *pwBaseLpRegen;
	WORD *pwLastLpRegen;
	WORD *pwBaseLpSitdownRegen;
	WORD *pwLastLpSitdownRegen;
	WORD *pwBaseLpBattleRegen;
	WORD *pwLastLpBattleRegen;

	WORD *pwBaseEpRegen;
	WORD *pwLastEpRegen;
	WORD *pwBaseEpSitdownRegen;
	WORD *pwLastEpSitdownRegen;
	WORD *pwBaseEpBattleRegen;
	WORD *pwLastEpBattleRegen;

	WORD *pwBaseApRegen;
	WORD *pwLastApRegen;
	WORD *pwBaseApSitdownRegen;
	WORD *pwLastApSitdownRegen;
	WORD *pwBaseApBattleRegen;
	WORD *pwLastApBattleRegen;
	WORD *pwBaseApDegen;
	WORD *pwLastApDegen;
	WORD *pwBaseApBattleDegen;
	WORD *pwLastApBattleDegen;

	WORD *pwBaseRpRegen;
	WORD *pwLastRpRegen;
	WORD *pwLastRpDimimutionRate;

	WORD *pwBasePhysicalOffence;
	WORD *pwLastPhysicalOffence;
	WORD *pwBasePhysicalDefence;
	WORD *pwLastPhysicalDefence;
//	WORD *pwBasePhysicalPiercingOffence;
//	WORD *pwLastPhysicalPiercingOffence;

	WORD *pwBaseEnergyOffence;
	WORD *pwLastEnergyOffence;
	WORD *pwBaseEnergyDefence;
	WORD *pwLastEnergyDefence;
//	WORD *pwBaseEnergyPiercingOffence;
//	WORD *pwLastEnergyPiercingOffence;

	WORD *pwBaseAttackRate;
	WORD *pwLastAttackRate;
	WORD *pwBaseDodgeRate;
	WORD *pwLastDodgeRate;

	WORD *pwBaseBlockRate;
	WORD *pwLastBlockRate;
	WORD *pwBaseBlockDamageRate;
	WORD *pwLastBlockDamageRate;

	WORD *pwBaseCurseSuccessRate;
	WORD *pwLastCurseSuccessRate;
	WORD *pwBaseCurseToleranceRate;
	WORD *pwLastCurseToleranceRate;

	WORD *pwBasePhysicalCriticalRate;
	WORD *pwLastPhysicalCriticalRate;
	WORD *pwBaseEnergyCriticalRate;
	WORD *pwLastEnergyCriticalRate;

	float __unaligned *pfBasePhysicalCriticalDamageRate;
	float __unaligned *pfLastPhysicalCriticalDamageRate;
	float __unaligned *pfBaseEnergyCriticalDamageRate;
	float __unaligned *pfLastEnergyCriticalDamageRate;

	float __unaligned *pfBaseRunSpeed;
	float __unaligned *pfLastRunSpeed;
	float __unaligned *pfBaseFlySpeed;
	float __unaligned *pfLastFlySpeed;
	float __unaligned *pfBaseFlyDashSpeed;
	float __unaligned *pfLastFlyDashSpeed;
	float __unaligned *pfBaseFlyAccelSpeed;
	float __unaligned *pfLastFlyAccelSpeed;

	WORD *pwBaseAttackSpeedRate;
	WORD *pwLastAttackSpeedRate;
	float __unaligned *pfBaseAttackRange;
	float __unaligned *pfLastAttackRange;

	float __unaligned *pfCastingTimeChangePercent;
	float __unaligned *pfCoolTimeChangePercent;
	float __unaligned *pfKeepTimeChangePercent;
	float __unaligned *pfDotTimeChangeAbsolute;
	float __unaligned *pfRequiredEpChangePercent;
	float __unaligned *pfHonestOffence;
	float __unaligned *pfHonestDefence;
	float __unaligned *pfStrangeOffence;
	float __unaligned *pfStrangeDefence;
	float __unaligned *pfWildOffence;
	float __unaligned *pfWildDefence;
	float __unaligned *pfEleganceOffence;
	float __unaligned *pfEleganceDefence;
	float __unaligned *pfFunnyOffence;
	float __unaligned *pfFunnyDefence;
	float __unaligned *pfPhysicalReflection;
	float __unaligned *pfEnergyReflection;

	WORD *pwParalyzeToleranceRate;
	WORD *pwTerrorToleranceRate;
	WORD *pwConfuseToleranceRate;
	WORD *pwStoneToleranceRate;
	WORD *pwCandyToleranceRate;
	float *pfParalyzeKeepTimeDown;
	float *pfTerrorKeepTimeDown;
	float *pfConfuseKeepTimeDown;
	float *pfStoneKeepTimeDown;
	float *pfCandyKeepTimeDown;
	float *pfBleedingKeepTimeDown;
	float *pfPoisonKeepTimeDown;
	float *pfStomachacheKeepTimeDown;
	float *pfCriticalBlockSuccessRate;
	DWORD *pdwLpRecoveryWhenHit;
	float *pfLpRecoveryWhenHitInPercent;
	DWORD *pdwEpRecoveryWhenHit;
	float *pfEpRecoveryWhenHitInPercent;
	WORD *pwStomachacheDefenceBase;
	WORD *pwStomachacheDefenceLast;
	WORD *pwPoisonDefenceBase;
	WORD *pwPoisonDefenceLast;
	WORD *pwBleedDefenceBase;
	WORD *pwBleedDefenceLast;
	WORD *pwBurnDefenceBase;
	WORD *pwBurnDefenceLast;
	float *pfBaseMindCurseImmunity;
	float *pfLastMindCurseImmunity;
	float *pfBaseBodyCurseImmunity;
	float *pfLastBodyCurseImmunity;
	float *pfBaseChangeCurseImmunity;
	float *pfLastChangeCurseImmunity;
	float *pfBaseSkillAnimationSpeedModifier;
	float *pfLastSkillAnimationSpeedModifier;
	DWORD *pdwBaseWeightLimit;
	DWORD *pdwLastWeightLimit;
	float *pfSkillAggroBonus;
	float *pfSkillAggroBonusInPercent;
	float *pfDirectHealPowerBonus;
	float *pfDirectHealPowerBonusInPercent;
	float *pfHotPowerBonus;
	float *pfHotPowerBonusInPercent;
	float *pfDotValueChangePercent;
	float *pfPhysicalCriticalDefenceRate;
	float *pfEnergyCriticalDefenceRate;
	WORD *pwGuardRateBase;
	WORD *pwGuardRateLast;
	float *pfSkillDamageBlockModeSuccessRate;
	float *pfCurseBlockModeSuccessRate;
	float *pfKnockdownBlockModeSuccessRate;
	float *pfHtbBlockModeSuccessRate;
	float *pfItemUpgradeBonusRate;
	float *pfItemUpgradeBreakBonusRate;
	BYTE *pbyExpBooster;
	BYTE *pbyQuestDropRate;
};

#pragma pack(pop)

class CNtlBitFlagManager;

class CNtlAvatar
{
public:
	struct sATTRIBUTE_LOGIC
	{
		DWORD dwFieldOffset;
		DWORD (*pCopyAttributeFunction)(void* pvBuffer, void* pvValue);
	};

protected:
	CNtlAvatar(void);
public:
	virtual ~CNtlAvatar(void);

protected:
	void Init();

	virtual void InitializeAttributeLink();

public:
	static CNtlAvatar* GetInstance();

public:

	// Updates sAVATAR_ATTRIBUTE structure with using raw data in a packet.(This function will be used mainly on client-side.)
	// (Packet -> Meta data + sAVATAR_ATTRIBUTE)
	static bool UpdateAvatarAttribute(BYTE byAttributeTotalCount, void* pvRawData, sAVATAR_ATTRIBUTE* pAttributeData);

	// Generates raw data in a packet with using attribute data.(This function will be used mainly on server-side.)
	// (Meta data + Attribute data -> Packet)
	static bool SaveAvatarAttribute(CNtlBitFlagManager* pChangedFlag, sAVATAR_ATTRIBUTE_LINK* pAttributeDataLink, void* pvBuffer, DWORD* pwdDataSize);
	static bool SaveAvatarAttribute(CNtlBitFlagManager* pChangedFlag, sAVATAR_ATTRIBUTE* pAttributeData, void* pvBuffer, DWORD* pwdDataSize);

	// Copies attribute data into sAVATAR_ATTRIBUTE structure.(This function will be used mainly on server-side.)
	// (Attribute data -> sAVATAR_ATTRIBUTE)
	static bool FillAvatarAttribute(sAVATAR_ATTRIBUTE_LINK* pAttributeDataLink, sAVATAR_ATTRIBUTE* pAttributeData);


	//NEW SELFMADE! ABOVE NOT USED
	// convert from sAVATAR_ATTRIBUTE to sAVATAR_ATTRIBUTE_LINK
	static bool FillAvatarAttributeNew(sAVATAR_ATTRIBUTE* pAttributeData, sAVATAR_ATTRIBUTE_LINK* pAttributeDataLink);
	// Very bad way to convert sAVATAR_ATTRIBUTE to sAVATAR_ATTRIBUTE_LINK
	static sAVATAR_ATTRIBUTE_LINK ConvertAVATAR_ATTRIBUTE(sAVATAR_ATTRIBUTE* avt);

public:
	static sATTRIBUTE_LOGIC m_attributeLogic[ATTRIBUTE_TO_UPDATE_COUNT];

	// needed to convert from sAVATAR_ATTRIBUTE to sAVATAR_ATTRIBUTE_LINK
	static sATTRIBUTE_LOGIC m_attributeLogicNew[ATTRIBUTE_TO_UPDATE_COUNT];

	static DWORD CopyValueByType_BYTE(void* pvValue, void* pvBuffer);
	static DWORD CopyValueByType_WORD(void* pvValue, void* pvBuffer);
	static DWORD CopyValueByType_float(void* pvValue, void* pvBuffer);
	static DWORD CopyValueByType_DWORD(void* pvValue, void* pvBuffer);
	static DWORD CopyValueByType_int(void* pvValue, void* pvBuffer);

	static DWORD CopyValueByTypeNew_BYTE(void* pvValue, void* pvBuffer);
	static DWORD CopyValueByTypeNew_WORD(void* pvValue, void* pvBuffer);
	static DWORD CopyValueByTypeNew_float(void* pvValue, void* pvBuffer);
	static DWORD CopyValueByTypeNew_DWORD(void* pvValue, void* pvBuffer);
	static DWORD CopyValueByTypeNew_int(void* pvValue, void* pvBuffer);
};