#include "stdafx.h"
#include "NtlBitFlag.h"
#include "NtlAvatar.h"

#include "NtlBitFlagManager.h"

CNtlAvatar::CNtlAvatar(void)
{
	Init();
}

CNtlAvatar::~CNtlAvatar(void)
{
}

void CNtlAvatar::Init()
{
	InitializeAttributeLink();
}

void CNtlAvatar::InitializeAttributeLink()
{
}

CNtlAvatar* CNtlAvatar::GetInstance()
{
	static CNtlAvatar avatar;
	return &avatar;
}

bool CNtlAvatar::UpdateAvatarAttribute(BYTE byAttributeTotalCount, void* pvRawData, sAVATAR_ATTRIBUTE* pAttributeData)
{
	BYTE* pbyCurrentPosition = (BYTE*)pvRawData;
	BYTE* pbyAttributeData = (BYTE*)pAttributeData;

	CNtlBitFlagManager changedFlag;
	if (false == changedFlag.Create(pvRawData, byAttributeTotalCount))
	{
		return false;
	}

	pbyCurrentPosition += changedFlag.GetBytesUsed();

	for (BYTE byIndex = ATTRIBUTE_TO_UPDATE_FIRST ; byIndex <= ATTRIBUTE_TO_UPDATE_LAST ; byIndex++)
	{
		if (false != changedFlag.IsSet(byIndex))
		{
			DWORD dwDataSize = m_attributeLogic[byIndex].pCopyAttributeFunction(pbyCurrentPosition, pbyAttributeData + m_attributeLogic[byIndex].dwFieldOffset);
			if (0 == dwDataSize)
			{
				return false;
			}
			else
			{
				pbyCurrentPosition += dwDataSize;
			}
		}
	}

	return true;
}

bool CNtlAvatar::SaveAvatarAttribute(CNtlBitFlagManager* pChangedFlag, sAVATAR_ATTRIBUTE_LINK* pAttributeDataLink, void* pvBuffer, DWORD* pwdDataSize)
{
	BYTE* pbyBuffer = (BYTE*)pvBuffer;
	BYTE* pbyAttributeDataLink = (BYTE*)pAttributeDataLink;

	*pwdDataSize = 0;

	::CopyMemory(pbyBuffer, pChangedFlag->GetRawData(), pChangedFlag->GetBytesUsed());

	pbyBuffer += pChangedFlag->GetBytesUsed();
	*pwdDataSize += pChangedFlag->GetBytesUsed();

	for (BYTE byIndex = ATTRIBUTE_TO_UPDATE_FIRST; byIndex <= ATTRIBUTE_TO_UPDATE_LAST; byIndex++)
	{
		if (false != pChangedFlag->IsSet(byIndex))
		{
			void* pvAttributeFieldLink = *((void**)(pbyAttributeDataLink + byIndex * sizeof(void*)));
			if (NULL == pvAttributeFieldLink)
			{
				continue;
			}

			DWORD dwDataSize = m_attributeLogic[byIndex].pCopyAttributeFunction(pvAttributeFieldLink, pbyBuffer);
			if (0 == dwDataSize)
			{
				return false;
			}
			else
			{
				pbyBuffer += dwDataSize;
				*pwdDataSize += dwDataSize;
			}
		}
	}

	return true;
}

bool CNtlAvatar::SaveAvatarAttribute(CNtlBitFlagManager* pChangedFlag, sAVATAR_ATTRIBUTE* pAttributeData, void* pvBuffer, DWORD* pwdDataSize)
{
	BYTE* pbyBuffer = (BYTE*)pvBuffer;
	BYTE* pbyAttributeData = (BYTE*)pAttributeData;

	*pwdDataSize = 0;

	::CopyMemory(pbyBuffer, pChangedFlag->GetRawData(), pChangedFlag->GetBytesUsed());

	pbyBuffer += pChangedFlag->GetBytesUsed();
	*pwdDataSize += pChangedFlag->GetBytesUsed();

	for (BYTE byIndex = ATTRIBUTE_TO_UPDATE_FIRST; byIndex <= ATTRIBUTE_TO_UPDATE_LAST; byIndex++)
	{
		if (false != pChangedFlag->IsSet(byIndex))
		{
			void* pvAttributeFieldLink = *((void**)(pbyAttributeData + byIndex * sizeof(void*)));
			if (NULL == pvAttributeFieldLink)
			{
				continue;
			}

			DWORD dwDataSize = m_attributeLogic[byIndex].pCopyAttributeFunction(pvAttributeFieldLink, pbyBuffer);
			if (0 == dwDataSize)
			{
				return false;
			}
			else
			{
				pbyBuffer += dwDataSize;
				*pwdDataSize += dwDataSize;
			}
		}
	}

	return true;
}

bool CNtlAvatar::FillAvatarAttribute(sAVATAR_ATTRIBUTE_LINK* pAttributeDataLink, sAVATAR_ATTRIBUTE* pAttributeData)
{
	BYTE* pbyAttributeDataLink = (BYTE*)pAttributeDataLink;
	BYTE* pbyAttributeData = (BYTE*)pAttributeData;

	for (BYTE byIndex = ATTRIBUTE_TO_UPDATE_FIRST; byIndex <= ATTRIBUTE_TO_UPDATE_LAST; byIndex++)
	{
		void* pvAttributeFieldLink = *((void**)(pbyAttributeDataLink + byIndex * sizeof(void*)));
		if (NULL == pvAttributeFieldLink)
		{
			continue;
		}

		DWORD dwDataSize = m_attributeLogic[byIndex].pCopyAttributeFunction(
			pvAttributeFieldLink,
			pbyAttributeData + m_attributeLogic[byIndex].dwFieldOffset);
		if (0 == dwDataSize)
		{
			return false;
		}
	}

	return true;
}

#ifndef ATTRIBUTE_LOGIC_DEFINE
#define ATTRIBUTE_LOGIC_DEFINE(field_name, type)					\
	{																\
		PtrToUlong(&(((sAVATAR_ATTRIBUTE*)NULL)->field_name)),		\
		CopyValueByType_##type										\
	}
#endif

CNtlAvatar::sATTRIBUTE_LOGIC CNtlAvatar::m_attributeLogic[ATTRIBUTE_TO_UPDATE_COUNT] =
{
	ATTRIBUTE_LOGIC_DEFINE(baseStr, WORD),
	ATTRIBUTE_LOGIC_DEFINE(lastStr, WORD),
	ATTRIBUTE_LOGIC_DEFINE(baseCon, WORD),
	ATTRIBUTE_LOGIC_DEFINE(lastCon, WORD),
	ATTRIBUTE_LOGIC_DEFINE(baseFoc, WORD),
	ATTRIBUTE_LOGIC_DEFINE(lastFoc, WORD),
	ATTRIBUTE_LOGIC_DEFINE(baseDex, WORD),
	ATTRIBUTE_LOGIC_DEFINE(lastDex, WORD),
	ATTRIBUTE_LOGIC_DEFINE(baseSol, WORD),
	ATTRIBUTE_LOGIC_DEFINE(lastSol, WORD),
	ATTRIBUTE_LOGIC_DEFINE(baseEng, WORD),
	ATTRIBUTE_LOGIC_DEFINE(lastEng, WORD),
	ATTRIBUTE_LOGIC_DEFINE(baseMaxLp, int),
	ATTRIBUTE_LOGIC_DEFINE(lastMaxLp, int),
	ATTRIBUTE_LOGIC_DEFINE(wBaseMaxEP, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastMaxEP, WORD),
	ATTRIBUTE_LOGIC_DEFINE(baseMaxAp, int),
	ATTRIBUTE_LOGIC_DEFINE(lastMaxAp, int),
	ATTRIBUTE_LOGIC_DEFINE(wBaseMaxRP, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastMaxRP, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBaseLpRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastLpRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBaseLpSitdownRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastLpSitdownRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBaseLpBattleRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastLpBattleRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBaseEpRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastEpRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBaseEpSitdownRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastEpSitdownRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBaseEpBattleRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastEpBattleRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBaseApRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastApRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBaseApSitdownRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastApSitdownRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBaseApBattleRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastApBattleRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBaseApDegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastApDegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBaseApBattleDegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastApBattleDegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBaseRpRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastRpRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastRpDimimutionRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBasePhysicalOffence, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastPhysicalOffence, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBasePhysicalDefence, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastPhysicalDefence, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBaseEnergyOffence, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastEnergyOffence, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBaseEnergyDefence, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastEnergyDefence, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBaseAttackRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastAttackRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBaseDodgeRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastDodgeRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBaseBlockRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastBlockRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBaseBlockDamageRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastBlockDamageRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBaseCurseSuccessRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastCurseSuccessRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBaseCurseToleranceRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastCurseToleranceRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBasePhysicalCriticalRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastPhysicalCriticalRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBaseEnergyCriticalRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastEnergyCriticalRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE(fBasePhysicalCriticalDamageRate, float),
	ATTRIBUTE_LOGIC_DEFINE(fLastPhysicalCriticalDamageRate, float),
	ATTRIBUTE_LOGIC_DEFINE(fBaseEnergyCriticalDamageRate, float),
	ATTRIBUTE_LOGIC_DEFINE(fLastEnergyCriticalDamageRate, float),
	ATTRIBUTE_LOGIC_DEFINE(fBaseRunSpeed, float),
	ATTRIBUTE_LOGIC_DEFINE(fLastRunSpeed, float),
	ATTRIBUTE_LOGIC_DEFINE(fBaseFlySpeed, float),
	ATTRIBUTE_LOGIC_DEFINE(fLastFlySpeed, float),
	ATTRIBUTE_LOGIC_DEFINE(fBaseFlyDashSpeed, float),
	ATTRIBUTE_LOGIC_DEFINE(fLastFlyDashSpeed, float),
	ATTRIBUTE_LOGIC_DEFINE(fBaseFlyAccelSpeed, float),
	ATTRIBUTE_LOGIC_DEFINE(fLastFlyAccelSpeed, float),
	ATTRIBUTE_LOGIC_DEFINE(wBaseAttackSpeedRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wLastAttackSpeedRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE(fBaseAttackRange, float),
	ATTRIBUTE_LOGIC_DEFINE(fLastAttackRange, float),
	ATTRIBUTE_LOGIC_DEFINE(fCastingTimeChangePercent, float),
	ATTRIBUTE_LOGIC_DEFINE(fCoolTimeChangePercent, float),
	ATTRIBUTE_LOGIC_DEFINE(fKeepTimeChangePercent, float),
	ATTRIBUTE_LOGIC_DEFINE(fDotTimeChangeAbsolute, float),
	ATTRIBUTE_LOGIC_DEFINE(fRequiredEpChangePercent, float),
	ATTRIBUTE_LOGIC_DEFINE(fHonestOffence, float),
	ATTRIBUTE_LOGIC_DEFINE(fHonestDefence, float),
	ATTRIBUTE_LOGIC_DEFINE(fStrangeOffence, float),
	ATTRIBUTE_LOGIC_DEFINE(fStrangeDefence, float),
	ATTRIBUTE_LOGIC_DEFINE(fWildOffence, float),
	ATTRIBUTE_LOGIC_DEFINE(fWildDefence, float),
	ATTRIBUTE_LOGIC_DEFINE(fEleganceOffence, float),
	ATTRIBUTE_LOGIC_DEFINE(fEleganceDefence, float),
	ATTRIBUTE_LOGIC_DEFINE(fFunnyOffence, float),
	ATTRIBUTE_LOGIC_DEFINE(fFunnyDefence, float),
	ATTRIBUTE_LOGIC_DEFINE(fPhysicalReflection, float),
	ATTRIBUTE_LOGIC_DEFINE(fEnergyReflection, float),
	ATTRIBUTE_LOGIC_DEFINE(wParalyzeToleranceRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wTerrorToleranceRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wConfuseToleranceRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wStoneToleranceRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wCandyToleranceRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE(fParalyzeKeepTimeDown, float),
	ATTRIBUTE_LOGIC_DEFINE(fTerrorKeepTimeDown, float),
	ATTRIBUTE_LOGIC_DEFINE(fConfuseKeepTimeDown, float),
	ATTRIBUTE_LOGIC_DEFINE(fStoneKeepTimeDown, float),
	ATTRIBUTE_LOGIC_DEFINE(fCandyKeepTimeDown, float),
	ATTRIBUTE_LOGIC_DEFINE(fBleedingKeepTimeDown, float),
	ATTRIBUTE_LOGIC_DEFINE(fPoisonKeepTimeDown, float),
	ATTRIBUTE_LOGIC_DEFINE(fStomachacheKeepTimeDown, float),
	ATTRIBUTE_LOGIC_DEFINE(fCriticalBlockSuccessRate, float),
	ATTRIBUTE_LOGIC_DEFINE(dwLpRecoveryWhenHit, DWORD),
	ATTRIBUTE_LOGIC_DEFINE(fLpRecoveryWhenHitInPercent, float),
	ATTRIBUTE_LOGIC_DEFINE(dwEpRecoveryWhenHit, DWORD),
	ATTRIBUTE_LOGIC_DEFINE(fEpRecoveryWhenHitInPercent, float),
	ATTRIBUTE_LOGIC_DEFINE(wStomachacheDefenceBase, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wStomachacheDefenceLast, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wPoisonDefenceBase, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wPoisonDefenceLast, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBleedDefenceBase, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBleedDefenceLast, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBurnDefenceBase, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wBurnDefenceLast, WORD),
	ATTRIBUTE_LOGIC_DEFINE(fBaseMindCurseImmunity, float),
	ATTRIBUTE_LOGIC_DEFINE(fLastMindCurseImmunity, float),
	ATTRIBUTE_LOGIC_DEFINE(fBaseBodyCurseImmunity, float),
	ATTRIBUTE_LOGIC_DEFINE(fLastBodyCurseImmunity, float),
	ATTRIBUTE_LOGIC_DEFINE(fBaseChangeCurseImmunity, float),
	ATTRIBUTE_LOGIC_DEFINE(fLastChangeCurseImmunity, float),
	ATTRIBUTE_LOGIC_DEFINE(fBaseSkillAnimationSpeedModifier, float),
	ATTRIBUTE_LOGIC_DEFINE(fLastSkillAnimationSpeedModifier, float),
	ATTRIBUTE_LOGIC_DEFINE(dwBaseWeightLimit, DWORD),
	ATTRIBUTE_LOGIC_DEFINE(dwLastWeightLimit, DWORD),
	ATTRIBUTE_LOGIC_DEFINE(fSkillAggroBonus, float),
	ATTRIBUTE_LOGIC_DEFINE(fSkillAggroBonusInPercent, float),
	ATTRIBUTE_LOGIC_DEFINE(fDirectHealPowerBonus, float),
	ATTRIBUTE_LOGIC_DEFINE(fDirectHealPowerBonusInPercent, float),
	ATTRIBUTE_LOGIC_DEFINE(fHotPowerBonus, float),
	ATTRIBUTE_LOGIC_DEFINE(fHotPowerBonusInPercent, float),
	ATTRIBUTE_LOGIC_DEFINE(fDotValueChangePercent, float),
	ATTRIBUTE_LOGIC_DEFINE(fPhysicalCriticalDefenceRate, float),
	ATTRIBUTE_LOGIC_DEFINE(fEnergyCriticalDefenceRate, float),
	ATTRIBUTE_LOGIC_DEFINE(wGuardRateBase, WORD),
	ATTRIBUTE_LOGIC_DEFINE(wGuardRateLast, WORD),
	ATTRIBUTE_LOGIC_DEFINE(fSkillDamageBlockModeSuccessRate, float),
	ATTRIBUTE_LOGIC_DEFINE(fCurseBlockModeSuccessRate, float),
	ATTRIBUTE_LOGIC_DEFINE(fKnockdownBlockModeSuccessRate, float),
	ATTRIBUTE_LOGIC_DEFINE(fHtbBlockModeSuccessRate, float),
	ATTRIBUTE_LOGIC_DEFINE(fItemUpgradeBonusRate, float),
	ATTRIBUTE_LOGIC_DEFINE(fItemUpgradeBreakBonusRate, float),
	ATTRIBUTE_LOGIC_DEFINE(byExpBooster, BYTE),
	ATTRIBUTE_LOGIC_DEFINE(byQuestDropRate, BYTE),
};

#undef ATTRIBUTE_LOGIC_DEFINE

DWORD CNtlAvatar::CopyValueByType_BYTE(void* pvValue, void* pvBuffer)
{
	BYTE* pbyBuffer = (BYTE*)pvBuffer;
	*pbyBuffer = *((BYTE*)pvValue);
	return sizeof(BYTE);
}
DWORD CNtlAvatar::CopyValueByType_WORD(void* pvValue, void* pvBuffer)
{
	WORD* pbyBuffer = (WORD*)pvBuffer;
	*pbyBuffer = *((WORD*)pvValue);
	return sizeof(WORD);
}
DWORD CNtlAvatar::CopyValueByType_float(void* pvValue, void* pvBuffer)
{
	float* pbyBuffer = (float*)pvBuffer;
	*pbyBuffer = *((float*)pvValue);
	return sizeof(float);
}
DWORD CNtlAvatar::CopyValueByType_DWORD(void* pvValue, void* pvBuffer)
{
	DWORD* pbyBuffer = (DWORD*)pvBuffer;
	*pbyBuffer = *((DWORD*)pvValue);
	return sizeof(DWORD);
}
DWORD CNtlAvatar::CopyValueByType_int(void* pvValue, void* pvBuffer)
{
	int* pbyBuffer = (int*)pvBuffer;
	*pbyBuffer = *((int*)pvValue);
	return sizeof(int);
}



bool CNtlAvatar::FillAvatarAttributeNew(sAVATAR_ATTRIBUTE* pAttributeData, sAVATAR_ATTRIBUTE_LINK* pAttributeDataLink)
{
	BYTE* pbyAttributeDataLink = (BYTE*)pAttributeDataLink;
	BYTE* pbyAttributeData = (BYTE*)pAttributeData;

	for (BYTE byIndex = ATTRIBUTE_TO_UPDATE_FIRST ; byIndex <= ATTRIBUTE_TO_UPDATE_LAST ; byIndex++)
	{
		void* pvAttributeFieldLink = ((void*)(pbyAttributeData + byIndex * sizeof(void*)));
		if (NULL == pvAttributeFieldLink)
		{
			continue;
		}

		DWORD dwDataSize = m_attributeLogicNew[byIndex].pCopyAttributeFunction(
																			pvAttributeFieldLink,
																			pbyAttributeDataLink + m_attributeLogic[byIndex].dwFieldOffset);
		if (0 == dwDataSize)
		{
			return false;
		}
	}

	return true;
}



#ifndef ATTRIBUTE_LOGIC_DEFINE_NEW
#define ATTRIBUTE_LOGIC_DEFINE_NEW(field_name, type)						\
	{																		\
		PtrToUlong(&(((sAVATAR_ATTRIBUTE_LINK*)NULL)->field_name)),			\
		CopyValueByTypeNew_##type											\
	}
#endif

CNtlAvatar::sATTRIBUTE_LOGIC CNtlAvatar::m_attributeLogicNew[ATTRIBUTE_TO_UPDATE_COUNT] =
{
	ATTRIBUTE_LOGIC_DEFINE_NEW(pbaseStr, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(plastStr, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pbaseCon, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(plastCon, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pbaseFoc, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(plastFoc, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pbaseDex, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(plastDex, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pbaseSol, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(plastSol, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pbaseEng, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(plastEng, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pbaseMaxLp, int),
	ATTRIBUTE_LOGIC_DEFINE_NEW(plastMaxLp, int),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBaseMaxEP, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastMaxEP, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pbaseMaxAp, int),
	ATTRIBUTE_LOGIC_DEFINE_NEW(plastMaxAp, int),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBaseMaxRP, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastMaxRP, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBaseLpRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastLpRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBaseLpSitdownRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastLpSitdownRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBaseLpBattleRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastLpBattleRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBaseEpRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastEpRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBaseEpSitdownRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastEpSitdownRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBaseEpBattleRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastEpBattleRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBaseApRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastApRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBaseApSitdownRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastApSitdownRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBaseApBattleRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastApBattleRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBaseApDegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastApDegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBaseApBattleDegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastApBattleDegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBaseRpRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastRpRegen, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastRpDimimutionRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBasePhysicalOffence, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastPhysicalOffence, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBasePhysicalDefence, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastPhysicalDefence, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBaseEnergyOffence, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastEnergyOffence, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBaseEnergyDefence, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastEnergyDefence, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBaseAttackRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastAttackRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBaseDodgeRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastDodgeRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBaseBlockRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastBlockRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBaseBlockDamageRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastBlockDamageRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBaseCurseSuccessRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastCurseSuccessRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBaseCurseToleranceRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastCurseToleranceRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBasePhysicalCriticalRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastPhysicalCriticalRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBaseEnergyCriticalRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastEnergyCriticalRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfBasePhysicalCriticalDamageRate, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfLastPhysicalCriticalDamageRate, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfBaseEnergyCriticalDamageRate, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfLastEnergyCriticalDamageRate, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfBaseRunSpeed, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfLastRunSpeed, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfBaseFlySpeed, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfLastFlySpeed, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfBaseFlyDashSpeed, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfLastFlyDashSpeed, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfBaseFlyAccelSpeed, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfLastFlyAccelSpeed, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBaseAttackSpeedRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwLastAttackSpeedRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfBaseAttackRange, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfLastAttackRange, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfCastingTimeChangePercent, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfCoolTimeChangePercent, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfKeepTimeChangePercent, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfDotTimeChangeAbsolute, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfRequiredEpChangePercent, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfHonestOffence, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfHonestDefence, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfStrangeOffence, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfStrangeDefence, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfWildOffence, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfWildDefence, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfEleganceOffence, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfEleganceDefence, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfFunnyOffence, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfFunnyDefence, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfPhysicalReflection, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfEnergyReflection, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwParalyzeToleranceRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwTerrorToleranceRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwConfuseToleranceRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwStoneToleranceRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwCandyToleranceRate, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfParalyzeKeepTimeDown, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfTerrorKeepTimeDown, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfConfuseKeepTimeDown, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfStoneKeepTimeDown, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfCandyKeepTimeDown, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfBleedingKeepTimeDown, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfPoisonKeepTimeDown, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfStomachacheKeepTimeDown, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfCriticalBlockSuccessRate, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pdwLpRecoveryWhenHit, DWORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfLpRecoveryWhenHitInPercent, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pdwEpRecoveryWhenHit, DWORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfEpRecoveryWhenHitInPercent, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwStomachacheDefenceBase, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwStomachacheDefenceLast, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwPoisonDefenceBase, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwPoisonDefenceLast, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBleedDefenceBase, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBleedDefenceLast, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBurnDefenceBase, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwBurnDefenceLast, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfBaseMindCurseImmunity, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfLastMindCurseImmunity, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfBaseBodyCurseImmunity, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfLastBodyCurseImmunity, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfBaseChangeCurseImmunity, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfLastChangeCurseImmunity, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfBaseSkillAnimationSpeedModifier, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfLastSkillAnimationSpeedModifier, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pdwBaseWeightLimit, DWORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pdwLastWeightLimit, DWORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfSkillAggroBonus, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfSkillAggroBonusInPercent, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfDirectHealPowerBonus, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfDirectHealPowerBonusInPercent, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfHotPowerBonus, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfHotPowerBonusInPercent, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfDotValueChangePercent, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfPhysicalCriticalDefenceRate, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfEnergyCriticalDefenceRate, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwGuardRateBase, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pwGuardRateLast, WORD),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfSkillDamageBlockModeSuccessRate, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfCurseBlockModeSuccessRate, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfKnockdownBlockModeSuccessRate, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfHtbBlockModeSuccessRate, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfItemUpgradeBonusRate, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pfItemUpgradeBreakBonusRate, float),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pbyExpBooster, BYTE),
	ATTRIBUTE_LOGIC_DEFINE_NEW(pbyQuestDropRate, BYTE),

};

#undef ATTRIBUTE_LOGIC_DEFINE_NEW



DWORD CNtlAvatar::CopyValueByTypeNew_BYTE(void* pvValue, void* pvBuffer)
{
	BYTE* pbyBuffer = (BYTE*)pvBuffer;
	*pbyBuffer = *((BYTE*)pvValue);
	return sizeof(pvValue);
}
DWORD CNtlAvatar::CopyValueByTypeNew_WORD(void* pvValue, void* pvBuffer)
{
	WORD* pbyBuffer = (WORD*)pvBuffer;
	pbyBuffer = (WORD*)pvValue;
	return sizeof(WORD*);
}
DWORD CNtlAvatar::CopyValueByTypeNew_float(void* pvValue, void* pvBuffer)
{
	float* pbyBuffer = (float*)pvBuffer;
	pbyBuffer = (float*)pvValue;
	return sizeof(float*);
}
DWORD CNtlAvatar::CopyValueByTypeNew_DWORD(void* pvValue, void* pvBuffer)
{
	DWORD* pbyBuffer = (DWORD*)pvBuffer;
	*pbyBuffer = *((DWORD*)pvValue);
	return sizeof(DWORD);
}
DWORD CNtlAvatar::CopyValueByTypeNew_int(void* pvValue, void* pvBuffer)
{
	int* pbyBuffer = (int*)pvBuffer;
	*pbyBuffer = *((int*)pvValue);
	return sizeof(int);
}

sAVATAR_ATTRIBUTE_LINK CNtlAvatar::ConvertAVATAR_ATTRIBUTE(sAVATAR_ATTRIBUTE* avt)
{
	sAVATAR_ATTRIBUTE_LINK newavt;

	newavt.pbaseStr = &avt->baseStr;
	newavt.plastStr = &avt->lastStr;
	newavt.pbaseCon = &avt->baseCon;
	newavt.plastCon = &avt->lastCon;
	newavt.pbaseFoc = &avt->baseFoc;
	newavt.plastFoc = &avt->lastFoc;
	newavt.pbaseDex = &avt->baseDex;
	newavt.plastDex = &avt->lastDex;
	newavt.pbaseSol = &avt->baseSol;
	newavt.plastSol = &avt->lastSol;
	newavt.pbaseEng = &avt->baseEng;
	newavt.plastEng = &avt->lastEng;
	newavt.pbaseMaxLp = &avt->baseMaxLp;
	newavt.plastMaxLp = &avt->lastMaxLp;
	newavt.pwBaseMaxEP = &avt->wBaseMaxEP;
	newavt.pwLastMaxEP = &avt->wLastMaxEP;
	newavt.pbaseMaxAp = &avt->baseMaxAp;
	newavt.plastMaxAp = &avt->lastMaxAp;
	newavt.pwBaseMaxRP = &avt->wBaseMaxRP;
	newavt.pwLastMaxRP = &avt->wLastMaxRP;
	newavt.pwBaseLpRegen = &avt->wBaseLpRegen;
	newavt.pwLastLpRegen = &avt->wLastLpRegen;
	newavt.pwBaseLpSitdownRegen = &avt->wBaseLpSitdownRegen;
	newavt.pwLastLpSitdownRegen = &avt->wLastLpSitdownRegen;
	newavt.pwBaseLpBattleRegen = &avt->wBaseLpBattleRegen;
	newavt.pwLastLpBattleRegen = &avt->wLastLpBattleRegen;
	newavt.pwBaseEpRegen = &avt->wBaseEpRegen;
	newavt.pwLastEpRegen = &avt->wLastEpRegen;
	newavt.pwBaseEpSitdownRegen = &avt->wBaseEpSitdownRegen;
	newavt.pwLastEpSitdownRegen = &avt->wLastEpSitdownRegen;
	newavt.pwBaseEpBattleRegen = &avt->wBaseEpBattleRegen;
	newavt.pwLastEpBattleRegen = &avt->wLastEpBattleRegen;
	newavt.pwBaseApRegen = &avt->wBaseApRegen;
	newavt.pwLastApRegen = &avt->wLastApRegen;
	newavt.pwBaseApSitdownRegen = &avt->wBaseApSitdownRegen;
	newavt.pwLastApSitdownRegen = &avt->wLastApSitdownRegen;
	newavt.pwBaseApBattleRegen = &avt->wBaseApBattleRegen;
	newavt.pwLastApBattleRegen = &avt->wLastApBattleRegen;
	newavt.pwBaseApDegen = &avt->wBaseApDegen;
	newavt.pwLastApDegen = &avt->wLastApDegen;
	newavt.pwBaseApBattleDegen = &avt->wBaseApBattleDegen;
	newavt.pwLastApBattleDegen = &avt->wLastApBattleDegen;
	newavt.pwBaseRpRegen = &avt->wBaseRpRegen;
	newavt.pwLastRpRegen = &avt->wLastRpRegen;
	newavt.pwLastRpDimimutionRate = &avt->wLastRpDimimutionRate;
	newavt.pwBasePhysicalOffence = &avt->wBasePhysicalOffence;
	newavt.pwLastPhysicalOffence = &avt->wLastPhysicalOffence;
	newavt.pwBasePhysicalDefence = &avt->wBasePhysicalDefence;
	newavt.pwLastPhysicalDefence = &avt->wLastPhysicalDefence;
	newavt.pwBaseEnergyOffence = &avt->wBaseEnergyOffence;
	newavt.pwLastEnergyOffence = &avt->wLastEnergyOffence;
	newavt.pwBaseEnergyDefence = &avt->wBaseEnergyDefence;
	newavt.pwLastEnergyDefence = &avt->wLastEnergyDefence;
	newavt.pwBaseAttackRate = &avt->wBaseAttackRate;
	newavt.pwLastAttackRate = &avt->wLastAttackRate;
	newavt.pwBaseDodgeRate = &avt->wBaseDodgeRate;
	newavt.pwLastDodgeRate = &avt->wLastDodgeRate;
	newavt.pwBaseBlockRate = &avt->wBaseBlockRate;
	newavt.pwLastBlockRate = &avt->wLastBlockRate;
	newavt.pwBaseBlockDamageRate = &avt->wBaseBlockDamageRate;
	newavt.pwLastBlockDamageRate = &avt->wLastBlockDamageRate;
	newavt.pwBaseCurseSuccessRate = &avt->wBaseCurseSuccessRate;
	newavt.pwLastCurseSuccessRate = &avt->wLastCurseSuccessRate;
	newavt.pwBaseCurseToleranceRate = &avt->wBaseCurseToleranceRate;
	newavt.pwLastCurseToleranceRate = &avt->wLastCurseToleranceRate;
	newavt.pwBasePhysicalCriticalRate = &avt->wBasePhysicalCriticalRate;
	newavt.pwLastPhysicalCriticalRate = &avt->wLastPhysicalCriticalRate;
	newavt.pwBaseEnergyCriticalRate = &avt->wBaseEnergyCriticalRate;
	newavt.pwLastEnergyCriticalRate = &avt->wLastEnergyCriticalRate;
	newavt.pfBasePhysicalCriticalDamageRate = &avt->fBasePhysicalCriticalDamageRate;
	newavt.pfLastPhysicalCriticalDamageRate = &avt->fLastPhysicalCriticalDamageRate;
	newavt.pfBaseEnergyCriticalDamageRate = &avt->fBaseEnergyCriticalDamageRate;
	newavt.pfLastEnergyCriticalDamageRate = &avt->fLastEnergyCriticalDamageRate;
	newavt.pfBaseRunSpeed = &avt->fBaseRunSpeed;
	newavt.pfLastRunSpeed = &avt->fLastRunSpeed;
	newavt.pfBaseFlySpeed = &avt->fBaseFlySpeed;
	newavt.pfLastFlySpeed = &avt->fLastFlySpeed;
	newavt.pfBaseFlyDashSpeed = &avt->fBaseFlyDashSpeed;
	newavt.pfLastFlyDashSpeed = &avt->fLastFlyDashSpeed;
	newavt.pfBaseFlyAccelSpeed = &avt->fBaseFlyAccelSpeed;
	newavt.pfLastFlyAccelSpeed = &avt->fLastFlyAccelSpeed;
	newavt.pwBaseAttackSpeedRate = &avt->wBaseAttackSpeedRate;
	newavt.pwLastAttackSpeedRate = &avt->wLastAttackSpeedRate;
	newavt.pfBaseAttackRange = &avt->fBaseAttackRange;
	newavt.pfLastAttackRange = &avt->fLastAttackRange;
	newavt.pfCastingTimeChangePercent = &avt->fCastingTimeChangePercent;
	newavt.pfCoolTimeChangePercent = &avt->fCoolTimeChangePercent;
	newavt.pfKeepTimeChangePercent = &avt->fKeepTimeChangePercent;
	newavt.pfDotTimeChangeAbsolute = &avt->fDotTimeChangeAbsolute;
	newavt.pfRequiredEpChangePercent = &avt->fRequiredEpChangePercent;
	newavt.pfHonestOffence = &avt->fHonestOffence;
	newavt.pfHonestDefence = &avt->fHonestDefence;
	newavt.pfStrangeOffence = &avt->fStrangeOffence;
	newavt.pfStrangeDefence = &avt->fStrangeDefence;
	newavt.pfWildOffence = &avt->fWildOffence;
	newavt.pfWildDefence = &avt->fWildDefence;
	newavt.pfEleganceOffence = &avt->fEleganceOffence;
	newavt.pfEleganceDefence = &avt->fEleganceDefence;
	newavt.pfFunnyOffence = &avt->fFunnyOffence;
	newavt.pfFunnyDefence = &avt->fFunnyDefence;
	newavt.pfPhysicalReflection = &avt->fPhysicalReflection;
	newavt.pfEnergyReflection = &avt->fEnergyReflection;
	newavt.pwParalyzeToleranceRate = &avt->wParalyzeToleranceRate;
	newavt.pwTerrorToleranceRate = &avt->wTerrorToleranceRate;
	newavt.pwConfuseToleranceRate = &avt->wConfuseToleranceRate;
	newavt.pwStoneToleranceRate = &avt->wStoneToleranceRate;
	newavt.pwCandyToleranceRate = &avt->wCandyToleranceRate;
	newavt.pfParalyzeKeepTimeDown = &avt->fParalyzeKeepTimeDown;
	newavt.pfTerrorKeepTimeDown = &avt->fTerrorKeepTimeDown;
	newavt.pfConfuseKeepTimeDown = &avt->fConfuseKeepTimeDown;
	newavt.pfStoneKeepTimeDown = &avt->fStoneKeepTimeDown;
	newavt.pfCandyKeepTimeDown = &avt->fCandyKeepTimeDown;
	newavt.pfBleedingKeepTimeDown = &avt->fBleedingKeepTimeDown;
	newavt.pfPoisonKeepTimeDown = &avt->fPoisonKeepTimeDown;
	newavt.pfStomachacheKeepTimeDown = &avt->fStomachacheKeepTimeDown;
	newavt.pfCriticalBlockSuccessRate = &avt->fCriticalBlockSuccessRate;
	newavt.pdwLpRecoveryWhenHit = &avt->dwLpRecoveryWhenHit;
	newavt.pfLpRecoveryWhenHitInPercent = &avt->fLpRecoveryWhenHitInPercent;
	newavt.pdwEpRecoveryWhenHit = &avt->dwEpRecoveryWhenHit;
	newavt.pfEpRecoveryWhenHitInPercent = &avt->fEpRecoveryWhenHitInPercent;
	newavt.pwStomachacheDefenceBase = &avt->wStomachacheDefenceBase;
	newavt.pwStomachacheDefenceLast = &avt->wStomachacheDefenceLast;
	newavt.pwPoisonDefenceBase = &avt->wPoisonDefenceBase;
	newavt.pwPoisonDefenceLast = &avt->wPoisonDefenceLast;
	newavt.pwBleedDefenceBase = &avt->wBleedDefenceBase;
	newavt.pwBleedDefenceLast = &avt->wBleedDefenceLast;
	newavt.pwBurnDefenceBase = &avt->wBurnDefenceBase;
	newavt.pwBurnDefenceLast = &avt->wBurnDefenceLast;
	newavt.pfBaseMindCurseImmunity = &avt->fBaseMindCurseImmunity;
	newavt.pfLastMindCurseImmunity = &avt->fLastMindCurseImmunity;
	newavt.pfBaseBodyCurseImmunity = &avt->fBaseBodyCurseImmunity;
	newavt.pfLastBodyCurseImmunity = &avt->fLastBodyCurseImmunity;
	newavt.pfBaseChangeCurseImmunity = &avt->fBaseChangeCurseImmunity;
	newavt.pfLastChangeCurseImmunity = &avt->fLastChangeCurseImmunity;
	newavt.pfBaseSkillAnimationSpeedModifier = &avt->fBaseSkillAnimationSpeedModifier;
	newavt.pfLastSkillAnimationSpeedModifier = &avt->fLastSkillAnimationSpeedModifier;
	newavt.pdwBaseWeightLimit = &avt->dwBaseWeightLimit;
	newavt.pdwLastWeightLimit = &avt->dwLastWeightLimit;
	newavt.pfSkillAggroBonus = &avt->fSkillAggroBonus;
	newavt.pfSkillAggroBonusInPercent = &avt->fSkillAggroBonusInPercent;
	newavt.pfDirectHealPowerBonus = &avt->fDirectHealPowerBonus;
	newavt.pfDirectHealPowerBonusInPercent = &avt->fDirectHealPowerBonusInPercent;
	newavt.pfHotPowerBonus = &avt->fHotPowerBonus;
	newavt.pfHotPowerBonusInPercent = &avt->fHotPowerBonusInPercent;
	newavt.pfDotValueChangePercent = &avt->fDotValueChangePercent;
	newavt.pfPhysicalCriticalDefenceRate = &avt->fPhysicalCriticalDefenceRate;
	newavt.pfEnergyCriticalDefenceRate = &avt->fEnergyCriticalDefenceRate;
	newavt.pwGuardRateBase = &avt->wGuardRateBase;
	newavt.pwGuardRateLast = &avt->wGuardRateLast;
	newavt.pfSkillDamageBlockModeSuccessRate = &avt->fSkillDamageBlockModeSuccessRate;
	newavt.pfCurseBlockModeSuccessRate = &avt->fCurseBlockModeSuccessRate;
	newavt.pfKnockdownBlockModeSuccessRate = &avt->fKnockdownBlockModeSuccessRate;
	newavt.pfHtbBlockModeSuccessRate = &avt->fHtbBlockModeSuccessRate;
	newavt.pfItemUpgradeBonusRate = &avt->fItemUpgradeBonusRate;
	newavt.pfItemUpgradeBreakBonusRate = &avt->fItemUpgradeBreakBonusRate;
	newavt.pbyExpBooster = &avt->byExpBooster;
	newavt.pbyQuestDropRate = &avt->byQuestDropRate;

	return newavt;
}


