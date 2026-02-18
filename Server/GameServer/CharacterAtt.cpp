#include "stdafx.h"
#include "NtlBitFlagManager.h"
#include "NtlPacketGU.h"
#include "GameServer.h"
#include "char.h"
#include "SystemEffectTable.h"

#include "calcs.h"



CCharacterAtt::CCharacterAtt()
{
	Init();
}

CCharacterAtt::~CCharacterAtt()
{
}


bool CCharacterAtt::Create(CCharacter* pChar)
{
	m_pOwnerRef = pChar;
	return true;
}


void CCharacterAtt::Init()
{
	m_pOwnerRef = NULL;
	memset(&m_pAttribute, 0, sizeof(sAVATAR_ATTRIBUTE));
	m_abyBattle_Attribute[0] = m_abyBattle_Attribute[1] = m_abyBattle_Attribute[2] = BATTLE_ATTRIBUTE_NONE;

	m_wSubWeaponPhysicalOffence = 0;
	m_wSubWeaponEnergyOffence = 0;
}


void CCharacterAtt::Reset() //set all last to base
{
	m_pAttribute.lastStr = m_pAttribute.baseStr;
	m_pAttribute.lastCon = m_pAttribute.baseCon;
	m_pAttribute.lastFoc = m_pAttribute.baseFoc;
	m_pAttribute.lastDex = m_pAttribute.baseDex;
	m_pAttribute.lastSol = m_pAttribute.baseSol;
	m_pAttribute.lastEng = m_pAttribute.baseEng;
	m_pAttribute.lastMaxLp = m_pAttribute.baseMaxLp;
	m_pAttribute.wLastMaxEP = m_pAttribute.wBaseMaxEP;
	m_pAttribute.lastMaxAp = m_pAttribute.baseMaxAp;
	m_pAttribute.wLastMaxRP = m_pAttribute.wBaseMaxRP;
	m_pAttribute.wLastLpRegen = m_pAttribute.wBaseLpRegen;
	m_pAttribute.wLastLpSitdownRegen = m_pAttribute.wBaseLpSitdownRegen;
	m_pAttribute.wLastLpBattleRegen = m_pAttribute.wBaseLpBattleRegen;
	m_pAttribute.wLastEpRegen = m_pAttribute.wBaseEpRegen;
	m_pAttribute.wLastEpSitdownRegen = m_pAttribute.wBaseEpSitdownRegen;
	m_pAttribute.wLastEpBattleRegen = m_pAttribute.wBaseEpBattleRegen;
	m_pAttribute.wLastApRegen = m_pAttribute.wBaseApRegen;
	m_pAttribute.wLastApSitdownRegen = m_pAttribute.wBaseApSitdownRegen;
	m_pAttribute.wLastApBattleRegen = m_pAttribute.wBaseApBattleRegen;
	m_pAttribute.wLastApDegen = m_pAttribute.wBaseApDegen;
	m_pAttribute.wBaseApBattleDegen = m_pAttribute.wLastApBattleDegen;
	m_pAttribute.wLastRpRegen = m_pAttribute.wBaseRpRegen;
	m_pAttribute.wLastPhysicalOffence = m_pAttribute.wBasePhysicalOffence;
	m_pAttribute.wLastPhysicalDefence = m_pAttribute.wBasePhysicalDefence;
	m_pAttribute.wLastEnergyOffence = m_pAttribute.wBaseEnergyOffence;
	m_pAttribute.wLastEnergyDefence = m_pAttribute.wBaseEnergyDefence;
	m_pAttribute.wLastAttackRate = m_pAttribute.wBaseAttackRate;
	m_pAttribute.wLastDodgeRate = m_pAttribute.wBaseDodgeRate;
	m_pAttribute.wLastBlockRate = m_pAttribute.wBaseBlockRate;
	m_pAttribute.wLastBlockDamageRate = m_pAttribute.wBaseBlockDamageRate;
	m_pAttribute.wLastCurseSuccessRate = m_pAttribute.wBaseCurseSuccessRate;
	m_pAttribute.wLastCurseToleranceRate = m_pAttribute.wBaseCurseToleranceRate;
	m_pAttribute.wLastPhysicalCriticalRate = m_pAttribute.wBasePhysicalCriticalRate;
	m_pAttribute.wLastEnergyCriticalRate = m_pAttribute.wBaseEnergyCriticalRate;
	m_pAttribute.fLastPhysicalCriticalDamageRate = m_pAttribute.fBasePhysicalCriticalDamageRate;
	m_pAttribute.fLastEnergyCriticalDamageRate = m_pAttribute.fBaseEnergyCriticalDamageRate;

	m_pAttribute.fLastFlySpeed = m_pAttribute.fBaseFlySpeed;
	m_pAttribute.fLastFlyDashSpeed = m_pAttribute.fBaseFlyDashSpeed;
	m_pAttribute.fLastFlyAccelSpeed = m_pAttribute.fBaseFlyAccelSpeed;

	m_pAttribute.wLastAttackSpeedRate = m_pAttribute.wBaseAttackSpeedRate;
	m_pAttribute.fLastAttackRange = m_pAttribute.fBaseAttackRange;
	m_pAttribute.wStomachacheDefenceLast = m_pAttribute.wStomachacheDefenceBase;
	m_pAttribute.wPoisonDefenceLast = m_pAttribute.wPoisonDefenceBase;
	m_pAttribute.wBleedDefenceLast = m_pAttribute.wBleedDefenceBase;
	m_pAttribute.wBurnDefenceLast = m_pAttribute.wBurnDefenceBase;
	m_pAttribute.fLastMindCurseImmunity = m_pAttribute.fBaseMindCurseImmunity;
	m_pAttribute.fLastBodyCurseImmunity = m_pAttribute.fBaseBodyCurseImmunity;
	m_pAttribute.fLastChangeCurseImmunity = m_pAttribute.fBaseChangeCurseImmunity;
	m_pAttribute.fLastSkillAnimationSpeedModifier = m_pAttribute.fBaseSkillAnimationSpeedModifier;
	m_pAttribute.dwLastWeightLimit = m_pAttribute.dwBaseWeightLimit;
	m_pAttribute.wGuardRateLast = m_pAttribute.wGuardRateBase;
}


void CCharacterAtt::CalculateAll()
{
	//set backup
	m_fRunSpeedBackup = m_pAttribute.fLastRunSpeed;
	m_wAttackSpeedBackup = m_pAttribute.wLastAttackSpeedRate;
	m_fCurLpPercent = m_pOwnerRef->GetCurLpInPercent();
	m_fCurEpPercent = m_pOwnerRef->GetCurEpInPercent();
	m_fCurApPercent = m_pOwnerRef->GetCurApInPercent();

	//set to zero to avoid bug that stats keep increasing instead
	memset(&m_percentValue, 0, sizeof(m_percentValue));

	//set to zero to avoid bug that stats keep increasing instead
	memset(&m_pAttribute, 0, sizeof(sAVATAR_ATTRIBUTE));

	//calculate the base stats
	CalculateBaseAtt();

	//calculate the last stats
	CalculateLastAtt();


	//check if our LP changed
	if (m_fCurLpPercent > 0.0f && m_fCurLpPercent < 101.0f && m_fCurLpPercent != m_pOwnerRef->GetCurLpInPercent())
	{
		if (m_pOwnerRef->IsMonster())
		{
			//m_pAttribute.lastMaxLp += m_pAttribute.lastMaxLp * m_fSettingMaxLP / 100.f;
			//m_pAttribute.wLastPhysicalDefence += m_pAttribute.wLastPhysicalDefence * m_fSettingPhysicalDefence / 100.f;
			//m_pAttribute.wLastEnergyDefence += m_pAttribute.wLastEnergyDefence * m_fSettingEnergyDefence / 100.f;
			//m_pAttribute.wLastPhysicalOffence += m_pAttribute.wLastPhysicalOffence * m_fSettingPhysicalOffence / 100.f;
			//m_pAttribute.wLastEnergyOffence += m_pAttribute.wLastEnergyOffence * m_fSettingEnergyOffence / 100.f;
		}

		m_pOwnerRef->SetCurLP(int((float)m_pAttribute.lastMaxLp * m_fCurLpPercent / 100.f));

		CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_LP));
		sGU_UPDATE_CHAR_LP * res = (sGU_UPDATE_CHAR_LP *)packet.GetPacketData();
		res->wOpCode = GU_UPDATE_CHAR_LP;
		res->handle = m_pOwnerRef->GetID();
		res->dwLpEpEventId = INVALID_DWORD;
		res->curLp = m_pOwnerRef->GetCurLP();
		res->maxLp = m_pAttribute.lastMaxLp;
		m_pOwnerRef->Broadcast(&packet);
	}

	//check if our EP changed
	if (m_fCurEpPercent > 0.0f && m_fCurEpPercent < 101.0f && m_fCurEpPercent != m_pOwnerRef->GetCurEpInPercent())
	{
		m_pOwnerRef->SetCurEP(WORD((float)m_pAttribute.wLastMaxEP * m_fCurEpPercent / 100.f));

		CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_EP));
		sGU_UPDATE_CHAR_EP * res = (sGU_UPDATE_CHAR_EP *)packet.GetPacketData();
		res->wOpCode = GU_UPDATE_CHAR_EP;
		res->handle = m_pOwnerRef->GetID();
		res->dwLpEpEventId = INVALID_DWORD;
		res->wCurEP = m_pOwnerRef->GetCurEP();
		res->wMaxEP = m_pAttribute.wLastMaxEP;
		m_pOwnerRef->Broadcast(&packet);
	}

	//check if our AP changed
	if (m_fCurApPercent > 0.0f && m_fCurApPercent < 101.0f && m_fCurApPercent != m_pOwnerRef->GetCurApInPercent())
	{
		m_pOwnerRef->SetCurAP(int((float)m_pAttribute.lastMaxAp * m_fCurApPercent / 100.f));

		CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_AP));
		sGU_UPDATE_CHAR_AP * res = (sGU_UPDATE_CHAR_AP *)packet.GetPacketData();
		res->wOpCode = GU_UPDATE_CHAR_AP;
		res->handle = m_pOwnerRef->GetID();
		res->curAP = m_pOwnerRef->GetCurAP();
		res->maxAP = m_pAttribute.lastMaxAp;
		m_pOwnerRef->SendPacket(&packet);
	}
}

void CCharacterAtt::CalculateLastAtt()
{
	sAVATAR_ATTRIBUTE_LINK newavt = CNtlAvatar::GetInstance()->ConvertAVATAR_ATTRIBUTE(&m_pAttribute);
	CNtlBitFlagManager flagMgr;

	if (false == flagMgr.Create(&m_pAttribute, ATTRIBUTE_TO_UPDATE_COUNT))
	{
		ERR_LOG(LOG_USER, "CalculateLastAtt: flagMgr.Create == FALSE.");
		return;
	}

	for (DWORD dwIndex = ATTRIBUTE_TO_UPDATE_FIRST; dwIndex < ATTRIBUTE_TO_UPDATE_COUNT; dwIndex++)
		flagMgr.Set(dwIndex);

	DWORD datasize = 0;

	CNtlPacket packet(sizeof(sGU_AVATAR_ATTRIBUTE_UPDATE));
	sGU_AVATAR_ATTRIBUTE_UPDATE * res = (sGU_AVATAR_ATTRIBUTE_UPDATE *)packet.GetPacketData();
	res->wOpCode = GU_AVATAR_ATTRIBUTE_UPDATE;
	res->hHandle = m_pOwnerRef->GetID();
	res->byAttributeTotalCount = ATTRIBUTE_TO_UPDATE_COUNT;

	if (CNtlAvatar::GetInstance()->SaveAvatarAttribute(&flagMgr, &newavt, res->abyFlexibleField, &datasize) == false)
	{
		ERR_LOG(LOG_USER, "CalculateLastAtt: save avatar attribute failed ");
		return;
	}

	packet.SetPacketLen(sizeof(sGU_AVATAR_ATTRIBUTE_UPDATE));
	m_pOwnerRef->Broadcast(&packet);


	if (m_fRunSpeedBackup != m_pAttribute.fLastRunSpeed)
	{
		m_pOwnerRef->UpdateMoveSpeed(m_pAttribute.fBaseRunSpeed, m_pAttribute.fLastRunSpeed);
	}

	if (m_wAttackSpeedBackup != m_pAttribute.wLastAttackSpeedRate)
	{
		m_pOwnerRef->UpdateAttackSpeed(m_pAttribute.wLastAttackSpeedRate);
	}
}

void CCharacterAtt::CalculatePercentValues()
{
	if (m_percentValue.m_fSTR != 0.f)
		CalculateLastStr(NtlRound((float)m_pAttribute.lastStr * m_percentValue.m_fSTR / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fSTRNegative != 0.f)
		CalculateLastStr(NtlRound(fabs((float)m_pAttribute.lastStr * m_percentValue.m_fSTRNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fCON != 0.f)
		CalculateLastCon(NtlRound((float)m_pAttribute.lastCon * m_percentValue.m_fCON / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fCONNegative != 0.f)
		CalculateLastCon(NtlRound(fabs((float)m_pAttribute.lastCon * m_percentValue.m_fCONNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fFOC != 0.f)
		CalculateLastFoc(NtlRound((float)m_pAttribute.lastFoc * m_percentValue.m_fFOC / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fFOCNegative != 0.f)
		CalculateLastFoc(NtlRound(fabs((float)m_pAttribute.lastFoc * m_percentValue.m_fFOCNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fDEX != 0.f)
		CalculateLastDex(NtlRound((float)m_pAttribute.lastDex * m_percentValue.m_fDEX / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fDEXNegative != 0.f)
		CalculateLastDex(NtlRound(fabs((float)m_pAttribute.lastDex * m_percentValue.m_fDEXNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fSOL != 0.f)
		CalculateLastSol(NtlRound((float)m_pAttribute.lastSol * m_percentValue.m_fSOL / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fSOLNegative != 0.f)
		CalculateLastSol(NtlRound(fabs((float)m_pAttribute.lastSol * m_percentValue.m_fSOLNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fENG != 0.f)
		CalculateLastEng(NtlRound((float)m_pAttribute.lastEng * m_percentValue.m_fENG / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fENGNegative != 0.f)
		CalculateLastEng(NtlRound(fabs((float)m_pAttribute.lastEng * m_percentValue.m_fENGNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fLP != 0.f)
		CalculateLastMaxLP(NtlRound((float)m_pAttribute.lastMaxLp * m_percentValue.m_fLP / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fLPNegative != 0.f)
		CalculateLastMaxLP(NtlRound(fabs((float)m_pAttribute.lastMaxLp * m_percentValue.m_fLPNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fEP != 0.f)
		CalculateLastMaxEP(NtlRound((float)m_pAttribute.wLastMaxEP * m_percentValue.m_fEP / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fEPNegative != 0.f)
		CalculateLastMaxEP(NtlRound(fabs((float)m_pAttribute.wLastMaxEP * m_percentValue.m_fEPNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fAP != 0.f)
		CalculateLastMaxAP(NtlRound((float)m_pAttribute.lastMaxAp * m_percentValue.m_fAP / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fAPNegative != 0.f)
		CalculateLastMaxAP(NtlRound(fabs((float)m_pAttribute.lastMaxAp * m_percentValue.m_fAPNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fRP != 0.f)
		CalculateLastMaxRP(NtlRound((float)m_pAttribute.wLastMaxRP * m_percentValue.m_fRP / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fRPNegative != 0.f)
		CalculateLastMaxRP(NtlRound(fabs((float)m_pAttribute.wLastMaxRP * m_percentValue.m_fRPNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fLpRegen != 0.f)
		CalculateLastLpRegen(NtlRound((float)m_pAttribute.wLastLpRegen * m_percentValue.m_fLpRegen / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fLpRegenNegative != 0.f)
		CalculateLastLpRegen(NtlRound(fabs((float)m_pAttribute.wLastLpRegen * m_percentValue.m_fLpRegenNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fLpSitdownRegen != 0.f)
		CalculateLastLpSitdownRegen(NtlRound((float)m_pAttribute.wLastLpSitdownRegen * m_percentValue.m_fLpSitdownRegen / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fLpSitdownRegenNegative != 0.f)
		CalculateLastLpSitdownRegen(NtlRound(fabs((float)m_pAttribute.wLastLpSitdownRegen * m_percentValue.m_fLpSitdownRegenNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fLpBattleRegen != 0.f)
		CalculateLastLpBattleRegen(NtlRound((float)m_pAttribute.wLastLpBattleRegen * m_percentValue.m_fLpBattleRegen / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fLpBattleRegenNegative != 0.f)
		CalculateLastLpBattleRegen(NtlRound(fabs((float)m_pAttribute.wLastLpBattleRegen * m_percentValue.m_fLpBattleRegenNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fEpRegen != 0.f)
		CalculateLastEpRegen(NtlRound((float)m_pAttribute.wLastEpRegen * m_percentValue.m_fEpRegen / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fEpRegenNegative != 0.f)
		CalculateLastEpRegen(NtlRound(fabs((float)m_pAttribute.wLastEpRegen * m_percentValue.m_fEpRegenNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fEpSitdownRegen != 0.f)
		CalculateLastEpSitdownRegen(NtlRound((float)m_pAttribute.wLastEpSitdownRegen * m_percentValue.m_fEpSitdownRegen / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fEpSitdownRegenNegative != 0.f)
		CalculateLastEpSitdownRegen(NtlRound(fabs((float)m_pAttribute.wLastEpSitdownRegen * m_percentValue.m_fEpSitdownRegenNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fEpBattleRegen != 0.f)
		CalculateLastEpBattleRegen(NtlRound((float)m_pAttribute.wLastEpBattleRegen * m_percentValue.m_fEpBattleRegen / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fEpBattleRegenNegative != 0.f)
		CalculateLastEpBattleRegen(NtlRound(fabs((float)m_pAttribute.wLastEpBattleRegen * m_percentValue.m_fEpBattleRegenNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fApRegen != 0.f)
		CalculateLastApRegen(NtlRound((float)m_pAttribute.wLastApRegen * m_percentValue.m_fApRegen / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fApRegenNegative != 0.f)
		CalculateLastApRegen(NtlRound(fabs((float)m_pAttribute.wLastApRegen * m_percentValue.m_fApRegenNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fApSitdownRegen != 0.f)
		CalculateLastApSitdownRegen(NtlRound((float)m_pAttribute.wLastApSitdownRegen * m_percentValue.m_fApSitdownRegen / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fApSitdownRegenNegative != 0.f)
		CalculateLastApSitdownRegen(NtlRound(fabs((float)m_pAttribute.wLastApSitdownRegen * m_percentValue.m_fApSitdownRegenNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fApBattleRegen != 0.f)
		CalculateLastApBattleRegen(NtlRound((float)m_pAttribute.wLastApBattleRegen * m_percentValue.m_fApBattleRegen / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fApBattleRegenNegative != 0.f)
		CalculateLastApBattleRegen(NtlRound(fabs((float)m_pAttribute.wLastApBattleRegen * m_percentValue.m_fApBattleRegenNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fApDegen != 0.f)
		CalculateLastApDegen(NtlRound((float)m_pAttribute.wLastApDegen * m_percentValue.m_fApDegen / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fApDegenNegative != 0.f)
		CalculateLastApDegen(NtlRound(fabs((float)m_pAttribute.wLastApDegen * m_percentValue.m_fApDegenNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fApBattleDegen != 0.f)
		CalculateLastApBattleDegen(NtlRound((float)m_pAttribute.wLastApBattleDegen * m_percentValue.m_fApBattleDegen / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fApBattleDegenNegative != 0.f)
		CalculateLastApBattleDegen(NtlRound(fabs((float)m_pAttribute.wLastApBattleDegen * m_percentValue.m_fApBattleDegenNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fRpRegen != 0.f)
		CalculateLastRpRegen(NtlRound((float)m_pAttribute.wLastRpRegen * m_percentValue.m_fRpRegen / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fRpRegenNegative != 0.f)
		CalculateLastRpRegen(NtlRound(fabs((float)m_pAttribute.wLastRpRegen * m_percentValue.m_fRpRegenNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	//if (m_percentValue.m_fRpDimimutionRate != 0.f)
	//	Calculatelastrpdim((float)m_pAttribute.wLastRpDimimutionRate * m_percentValue.m_fRpDimimutionRate / 100.f, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);

	if (m_percentValue.m_fPhysicalOffence != 0.f)
		CalculateLastPhysicalOffence(NtlRound((float)m_pAttribute.wLastPhysicalOffence * m_percentValue.m_fPhysicalOffence / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fPhysicalOffenceNegative != 0.f)
		CalculateLastPhysicalOffence(NtlRound(fabs((float)m_pAttribute.wLastPhysicalOffence * m_percentValue.m_fPhysicalOffenceNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fPhysicalDefence != 0.f)
		CalculateLastPhysicalDefence(NtlRound((float)m_pAttribute.wLastPhysicalDefence * m_percentValue.m_fPhysicalDefence / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fPhysicalDefenceNegative != 0.f)
		CalculateLastPhysicalDefence(NtlRound(fabs((float)m_pAttribute.wLastPhysicalDefence * m_percentValue.m_fPhysicalDefenceNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fEnergyOffence != 0.f)
		CalculateLastEnergyOffence(NtlRound((float)m_pAttribute.wLastEnergyOffence * m_percentValue.m_fEnergyOffence / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fEnergyOffenceNegative != 0.f)
		CalculateLastEnergyOffence(NtlRound(fabs((float)m_pAttribute.wLastEnergyOffence * m_percentValue.m_fEnergyOffenceNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fEnergyDefence != 0.f)
		CalculateLastEnergyDefence(NtlRound((float)m_pAttribute.wLastEnergyDefence * m_percentValue.m_fEnergyDefence / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fEnergyDefenceNegative != 0.f)
		CalculateLastEnergyDefence(NtlRound(fabs((float)m_pAttribute.wLastEnergyDefence * m_percentValue.m_fEnergyDefenceNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fAttackRate != 0.f)
		CalculateLastAttackRate(NtlRound((float)m_pAttribute.wLastAttackRate * m_percentValue.m_fAttackRate / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fAttackRateNegative != 0.f)
		CalculateLastAttackRate(NtlRound(fabs((float)m_pAttribute.wLastAttackRate * m_percentValue.m_fAttackRateNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fDodgeRate != 0.f)
		CalculateLastDodgeRate(NtlRound((float)m_pAttribute.wLastDodgeRate * m_percentValue.m_fDodgeRate / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fDodgeRateNegative != 0.f)
		CalculateLastDodgeRate(NtlRound(fabs((float)m_pAttribute.wLastDodgeRate * m_percentValue.m_fDodgeRateNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fBlockRate != 0.f)
		CalculateLastBlockRate(NtlRound((float)m_pAttribute.wLastBlockRate * m_percentValue.m_fBlockRate / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fBlockRateNegative != 0.f)
		CalculateLastBlockRate(NtlRound(fabs((float)m_pAttribute.wLastBlockRate * m_percentValue.m_fBlockRateNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fBlockDamageRate != 0.f)
		CalculateLastBlockDamageRate(NtlRound((float)m_pAttribute.wLastBlockDamageRate * m_percentValue.m_fBlockDamageRate / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fBlockDamageRateNegative != 0.f)
		CalculateLastBlockDamageRate(NtlRound(fabs((float)m_pAttribute.wLastBlockDamageRate * m_percentValue.m_fBlockDamageRateNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fCurseSuccessRate != 0.f)
		CalculateLastCurseSuccessRate(NtlRound((float)m_pAttribute.wLastCurseSuccessRate * m_percentValue.m_fCurseSuccessRate / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fCurseSuccessRateNegative != 0.f)
		CalculateLastCurseSuccessRate(NtlRound(fabs((float)m_pAttribute.wLastCurseSuccessRate * m_percentValue.m_fCurseSuccessRateNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fCurseToleranceRate != 0.f)
		CalculateLastCurseToleranceRate(NtlRound((float)m_pAttribute.wBaseCurseToleranceRate * m_percentValue.m_fCurseToleranceRate / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fCurseToleranceRateNegative != 0.f)
		CalculateLastCurseToleranceRate(NtlRound(fabs((float)m_pAttribute.wBaseCurseToleranceRate * m_percentValue.m_fCurseToleranceRateNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fPhysicalCriticalRate != 0.f)
		CalculateLastPhysicalCriticalRate(NtlRound((float)m_pAttribute.wLastPhysicalCriticalRate * m_percentValue.m_fPhysicalCriticalRate / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fPhysicalCriticalRateNegative != 0.f)
		CalculateLastPhysicalCriticalRate(NtlRound(fabs((float)m_pAttribute.wLastPhysicalCriticalRate * m_percentValue.m_fPhysicalCriticalRateNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fEnergyCriticalRate != 0.f)
		CalculateLastEnergyCriticalRate(NtlRound((float)m_pAttribute.wLastEnergyCriticalRate * m_percentValue.m_fEnergyCriticalRate / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fEnergyCriticalRateNegative != 0.f)
		CalculateLastEnergyCriticalRate(NtlRound(fabs((float)m_pAttribute.wLastEnergyCriticalRate * m_percentValue.m_fEnergyCriticalRateNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fPhysicalCriticalDamageRate != 0.f)
		CalculateLastPhysicalCriticalDamageRate(NtlRound((float)m_pAttribute.fLastPhysicalCriticalDamageRate * m_percentValue.m_fPhysicalCriticalDamageRate / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fPhysicalCriticalDamageRateNegative != 0.f)
		CalculateLastPhysicalCriticalDamageRate(NtlRound(fabs((float)m_pAttribute.fLastPhysicalCriticalDamageRate * m_percentValue.m_fPhysicalCriticalDamageRateNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fEnergyCriticalDamageRate != 0.f)
		CalculateLastEnergyCriticalDamageRate(NtlRound((float)m_pAttribute.fLastEnergyCriticalDamageRate * m_percentValue.m_fEnergyCriticalDamageRate / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fEnergyCriticalDamageRateNegative != 0.f)
		CalculateLastEnergyCriticalDamageRate(NtlRound(fabs((float)m_pAttribute.fLastEnergyCriticalDamageRate * m_percentValue.m_fEnergyCriticalDamageRateNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);


	//m_pAttribute.fLastRunSpeed += m_pAttribute.fLastRunSpeed * m_percentValue.m_fRunSpeed / 100.f;
	m_pAttribute.fLastFlySpeed += m_pAttribute.fLastFlySpeed * m_percentValue.m_fFlySpeed / 100.f;
	m_pAttribute.fLastFlyDashSpeed += m_pAttribute.fLastFlyDashSpeed * m_percentValue.m_fFlyDashSpeed / 100.f;
	m_pAttribute.fLastFlyAccelSpeed += m_pAttribute.fLastFlyAccelSpeed * m_percentValue.m_fFlyAccelSpeed / 100.f;

	if (m_percentValue.m_fAttackSpeedRate != 0.f)
		CalculateLastAttackSpeedRate(NtlRound((float)m_pAttribute.wLastAttackSpeedRate * m_percentValue.m_fAttackSpeedRate / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fAttackSpeedRateNegative != 0.f)
		CalculateLastAttackSpeedRate(NtlRound(fabs((float)m_pAttribute.wLastAttackSpeedRate * m_percentValue.m_fAttackSpeedRateNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fAttackRange != 0.f)
		CalculateLastAttackRange(NtlRound((float)m_pAttribute.fLastAttackRange * m_percentValue.m_fAttackRange / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fAttackRangeNegative != 0.f)
		CalculateLastAttackRange(NtlRound(fabs((float)m_pAttribute.fLastAttackRange * m_percentValue.m_fAttackRangeNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fStomachacheDef != 0.f)
		CalculateLastStomachacheDefence(NtlRound((float)m_pAttribute.wStomachacheDefenceLast * m_percentValue.m_fStomachacheDef / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fStomachacheDefNegative != 0.f)
		CalculateLastStomachacheDefence(NtlRound(fabs((float)m_pAttribute.wStomachacheDefenceLast * m_percentValue.m_fStomachacheDefNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fPoisonDef != 0.f)
		CalculateLastPoisonDefence(NtlRound((float)m_pAttribute.wPoisonDefenceLast * m_percentValue.m_fPoisonDef / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fPoisonDefNegative != 0.f)
		CalculateLastPoisonDefence(NtlRound(fabs((float)m_pAttribute.wPoisonDefenceLast * m_percentValue.m_fPoisonDefNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fBleedDef != 0.f)
		CalculateLastBleedDefence(NtlRound((float)m_pAttribute.wBleedDefenceLast * m_percentValue.m_fBleedDef / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fBleedDefNegative != 0.f)
		CalculateLastBleedDefence(NtlRound(fabs((float)m_pAttribute.wBleedDefenceLast * m_percentValue.m_fBleedDefNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	if (m_percentValue.m_fBurnDef != 0.f)
		CalculateLastBurnDefence(NtlRound((float)m_pAttribute.wBurnDefenceLast * m_percentValue.m_fBurnDef / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fBurnDefNegative != 0.f)
		CalculateLastBurnDefence(NtlRound(fabs((float)m_pAttribute.wBurnDefenceLast * m_percentValue.m_fBurnDefNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

	m_pAttribute.fLastMindCurseImmunity += m_pAttribute.fLastMindCurseImmunity * m_percentValue.m_fMindCurseImmunity / 100.f;
	m_pAttribute.fLastBodyCurseImmunity += m_pAttribute.fLastBodyCurseImmunity * m_percentValue.m_fBodyCurseImmunity / 100.f;
	m_pAttribute.fLastChangeCurseImmunity += m_pAttribute.fLastChangeCurseImmunity * m_percentValue.m_fChangeCurseImmunity / 100.f;
	m_pAttribute.fLastSkillAnimationSpeedModifier += m_pAttribute.fLastSkillAnimationSpeedModifier * m_percentValue.m_fSkillAnimationSpeedModifier / 100.f;

	if (m_percentValue.m_fGuardRate != 0.f)
		CalcAttributeW(m_pAttribute.wGuardRateLast, NtlRound((float)m_pAttribute.wGuardRateLast * m_percentValue.m_fGuardRate / 100.f), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
	if (m_percentValue.m_fGuardRateNegative != 0.f)
		CalcAttributeW(m_pAttribute.wGuardRateLast, NtlRound(fabs((float)m_pAttribute.wGuardRateLast * m_percentValue.m_fGuardRateNegative / 100.f)), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);

}


void CCharacterAtt::CalculateBaseStr(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.baseStr, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateBaseCon(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.baseCon, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateBaseFoc(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.baseFoc, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateBaseDex(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.baseDex, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateBaseSol(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.baseSol, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateBaseEng(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.baseEng, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastStr(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.lastStr, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastCon(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.lastCon, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastFoc(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.lastFoc, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastDex(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.lastDex, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastSol(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.lastSol, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastEng(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.lastEng, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateBaseMaxLP(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeN(m_pAttribute.baseMaxLp, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastMaxLP(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.lastMaxLp += (int)fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fLP += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.lastMaxLp -= (int)fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fLPNegative -= fValue;
		}

		if (m_pAttribute.lastMaxLp < 0)
			m_pAttribute.lastMaxLp = 0;
	}
}

void CCharacterAtt::CalculateBaseMaxEP(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBaseMaxEP, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastMaxEP(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastMaxEP = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastMaxEP, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fEP += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastMaxEP = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastMaxEP, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fEPNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseMaxRP(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBaseMaxRP, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastMaxRP(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastMaxRP = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastMaxRP, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fRP += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastMaxRP = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastMaxRP, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fRPNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseMaxAP(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeN(m_pAttribute.baseMaxAp, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastMaxAP(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.lastMaxAp += (int)fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fAP += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.lastMaxAp -= (int)fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fAPNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseLpRegen(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBaseLpRegen, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastLpRegen(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastLpRegen = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastLpRegen, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fLpRegen += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastLpRegen = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastLpRegen, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fLpRegenNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseLpSitdownRegen(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBaseLpSitdownRegen, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastLpSitdownRegen(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastLpSitdownRegen = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastLpSitdownRegen, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fLpSitdownRegen += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastLpSitdownRegen = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastLpSitdownRegen, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fLpSitdownRegenNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseLpBattleRegen(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBaseLpBattleRegen, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastLpBattleRegen(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastLpBattleRegen = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastLpBattleRegen, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fLpBattleRegen += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastLpBattleRegen = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastLpBattleRegen, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fLpBattleRegenNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseEpRegen(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBaseEpRegen, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastEpRegen(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastEpRegen = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastEpRegen, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fEpRegen += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastEpRegen = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastEpRegen, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fEpRegenNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseEpSitdownRegen(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBaseEpSitdownRegen, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastEpSitdownRegen(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastEpSitdownRegen = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastEpSitdownRegen, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fEpSitdownRegen += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastEpSitdownRegen = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastEpSitdownRegen, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fEpSitdownRegenNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseEpBattleRegen(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBaseEpBattleRegen, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastEpBattleRegen(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastEpBattleRegen = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastEpBattleRegen, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fEpBattleRegen += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastEpBattleRegen = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastEpBattleRegen, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fEpBattleRegenNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseApRegen(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBaseApRegen, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastApRegen(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastApRegen = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastApRegen, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fApRegen += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastApRegen = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastApRegen, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fApRegenNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseApSitdownRegen(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBaseApSitdownRegen, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastApSitdownRegen(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastApSitdownRegen = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastApSitdownRegen, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fApSitdownRegen += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastApSitdownRegen = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastApSitdownRegen, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fApSitdownRegenNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseApBattleRegen(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBaseApBattleRegen, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastApBattleRegen(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastApBattleRegen = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastApBattleRegen, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fApBattleRegen += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastApBattleRegen = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastApBattleRegen, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fApBattleRegenNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseApDegen(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBaseApDegen, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastApDegen(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastApDegen = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastApDegen, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fApDegen += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastApDegen = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastApDegen, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fApDegenNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseApBattleDegen(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBaseApBattleDegen, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastApBattleDegen(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastApBattleDegen = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastApBattleDegen, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fApBattleDegen += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastApBattleDegen = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastApBattleDegen, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fApBattleDegenNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseRpRegen(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBaseRpRegen, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastRpRegen(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastRpRegen = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastRpRegen, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fRpRegen += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastRpRegen = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastRpRegen, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fRpRegenNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBasePhysicalOffence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBasePhysicalOffence, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastPhysicalOffence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastPhysicalOffence = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastPhysicalOffence, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fPhysicalOffence += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastPhysicalOffence = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastPhysicalOffence, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fPhysicalOffenceNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseEnergyOffence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBaseEnergyOffence, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastEnergyOffence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastEnergyOffence = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastEnergyOffence, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fEnergyOffence += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastEnergyOffence = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastEnergyOffence, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fEnergyOffenceNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBasePhysicalDefence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBasePhysicalDefence, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastPhysicalDefence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastPhysicalDefence = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastPhysicalDefence, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fPhysicalDefence += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastPhysicalDefence = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastPhysicalDefence, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fPhysicalDefenceNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseEnergyDefence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBaseEnergyDefence, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastEnergyDefence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastEnergyDefence = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastEnergyDefence, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fEnergyDefence += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastEnergyDefence = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastEnergyDefence, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fEnergyDefenceNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseAttackRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBaseAttackRate, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastAttackRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastAttackRate = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastAttackRate, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fAttackRate += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastAttackRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastAttackRate, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fAttackRateNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseDodgeRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBaseDodgeRate, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastDodgeRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastDodgeRate = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastDodgeRate, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fDodgeRate += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastDodgeRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastDodgeRate, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fDodgeRateNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseBlockRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBaseBlockRate, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastBlockRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastBlockRate = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastBlockRate, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fBlockRate += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastBlockRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastBlockRate, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fBlockRateNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseBlockDamageRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBaseBlockDamageRate, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastBlockDamageRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastBlockDamageRate = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastBlockDamageRate, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fBlockDamageRate += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastBlockDamageRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastBlockDamageRate, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fBlockDamageRateNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseCurseSuccessRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBaseCurseSuccessRate, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastCurseSuccessRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastCurseSuccessRate = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastCurseSuccessRate, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fCurseSuccessRate += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastCurseSuccessRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastCurseSuccessRate, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fCurseSuccessRateNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseCurseToleranceRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBaseCurseToleranceRate, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastCurseToleranceRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastCurseToleranceRate = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastCurseToleranceRate, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fCurseToleranceRate += fValue;

			if (m_percentValue.m_fCurseToleranceRate > 60.f)
				m_percentValue.m_fCurseToleranceRate = 60.f;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastCurseToleranceRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastCurseToleranceRate, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fCurseToleranceRateNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBasePhysicalCriticalRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBasePhysicalCriticalRate, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastPhysicalCriticalRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastPhysicalCriticalRate = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastPhysicalCriticalRate, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fPhysicalCriticalRate += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastPhysicalCriticalRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastPhysicalCriticalRate, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fPhysicalCriticalRateNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseEnergyCriticalRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBaseEnergyCriticalRate, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastEnergyCriticalRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastEnergyCriticalRate = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastEnergyCriticalRate, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fEnergyCriticalRate += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastEnergyCriticalRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastEnergyCriticalRate, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fEnergyCriticalRateNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBasePhysicalCriticalDamageRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fBasePhysicalCriticalDamageRate, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastPhysicalCriticalDamageRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.fLastPhysicalCriticalDamageRate += fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fPhysicalCriticalDamageRate += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.fLastPhysicalCriticalDamageRate -= fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fPhysicalCriticalDamageRateNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseEnergyCriticalDamageRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fBaseEnergyCriticalDamageRate, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastEnergyCriticalDamageRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.fLastEnergyCriticalDamageRate += fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fEnergyCriticalDamageRate += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.fLastEnergyCriticalDamageRate -= fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fEnergyCriticalDamageRateNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseRunSpeed(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fBaseRunSpeed, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastRunSpeed(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.fLastRunSpeed += fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_pAttribute.fLastRunSpeed += m_pAttribute.fBaseRunSpeed * fValue / 100.f;
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
			m_pAttribute.fLastRunSpeed -= m_pAttribute.fBaseRunSpeed * fValue / 100.f;
		}

		if (m_pAttribute.fLastRunSpeed < 0.f)
			m_pAttribute.fLastRunSpeed = 0.f;
	}
}

void CCharacterAtt::CalculateBaseFlySpeed(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fBaseFlySpeed, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastFlySpeed(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.fLastFlySpeed += fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fFlySpeed += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.fLastFlySpeed -= fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fFlySpeedNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseFlyDashSpeed(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fBaseFlyDashSpeed, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastFlyDashSpeed(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.fLastFlyDashSpeed += fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fFlyDashSpeed += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.fLastFlyDashSpeed -= fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fFlyDashSpeedNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseFlyAccelSpeed(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fBaseFlyAccelSpeed, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastFlyAccelSpeed(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.fLastFlyAccelSpeed += fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fFlyAccelSpeed += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.fLastFlyAccelSpeed -= fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fFlyAccelSpeedNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseAttackSpeedRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBaseAttackSpeedRate, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastAttackSpeedRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	/*
		Increase = slower
		Decrease = faster
	*/

	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastAttackSpeedRate = UnsignedSafeIncrease<WORD>(m_pAttribute.wLastAttackSpeedRate, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fAttackSpeedRate += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wLastAttackSpeedRate = UnsignedSafeDecrease<WORD>(m_pAttribute.wLastAttackSpeedRate, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fAttackSpeedRateNegative -= fValue;
		}

		if(m_pAttribute.wLastAttackSpeedRate < 80)
			m_pAttribute.wLastAttackSpeedRate = 80;
	}
}

void CCharacterAtt::CalculateBaseAttackRange(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fBaseAttackRange, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastAttackRange(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.fLastAttackRange += fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fAttackRange += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.fLastAttackRange -= fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fAttackRangeNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateCastingTimeChangePercent(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fCastingTimeChangePercent, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateCoolTimeChangePercent(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fCoolTimeChangePercent, fValue, byApplyType, bIsPlus, -63.f);
}

void CCharacterAtt::CalculateKeepTimeChangePercent(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fKeepTimeChangePercent, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateDotTimeChangeAbsolute(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fDotTimeChangeAbsolute, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateRequiredEpChangePercent(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fRequiredEpChangePercent, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateBattleAttribute(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalculateLastHonestOffence(fValue, byApplyType, bIsPlus);
	CalculateLastStrangeOffence(fValue, byApplyType, bIsPlus);
	CalculateLastWildOffence(fValue, byApplyType, bIsPlus);
	CalculateLastEleganceOffence(fValue, byApplyType, bIsPlus);
	CalculateLastFunnyOffence(fValue, byApplyType, bIsPlus);
	CalculateLastHonestDefence(fValue, byApplyType, bIsPlus);
	CalculateLastStrangeDefence(fValue, byApplyType, bIsPlus);
	CalculateLastWildDefence(fValue, byApplyType, bIsPlus);
	CalculateLastEleganceDefence(fValue, byApplyType, bIsPlus);
	CalculateLastFunnyDefence(fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastHonestOffence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fHonestOffence, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastHonestDefence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fHonestDefence, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastStrangeOffence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fStrangeOffence, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastStrangeDefence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fStrangeDefence, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastWildOffence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fWildOffence, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastWildDefence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fWildDefence, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastEleganceOffence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fEleganceOffence, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastEleganceDefence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fEleganceDefence, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastFunnyOffence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fFunnyOffence, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastFunnyDefence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fFunnyDefence, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastPhysicalReflection(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fPhysicalReflection, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastEnergyReflection(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fEnergyReflection, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastParalyzeToleranceRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wParalyzeToleranceRate, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastTerrorToleranceRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wTerrorToleranceRate, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastConfuseToleranceRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wConfuseToleranceRate, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastStoneToleranceRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wStoneToleranceRate, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastCandyToleranceRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wCandyToleranceRate, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastParalyzeKeepTimeDown(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fParalyzeKeepTimeDown, fValue, byApplyType, bIsPlus, 99.9f);
}

void CCharacterAtt::CalculateLastTerrorKeepTimeDown(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fTerrorKeepTimeDown, fValue, byApplyType, bIsPlus, 99.9f);
}

void CCharacterAtt::CalculateLastConfuseKeepTimeDown(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fConfuseKeepTimeDown, fValue, byApplyType, bIsPlus, 99.9f);
}

void CCharacterAtt::CalculateLastStoneKeepTimeDown(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fStoneKeepTimeDown, fValue, byApplyType, bIsPlus, 99.9f);
}

void CCharacterAtt::CalculateLastCandyKeepTimeDown(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fCandyKeepTimeDown, fValue, byApplyType, bIsPlus, 99.9f);
}

void CCharacterAtt::CalculateLastBleedingKeepTimeDown(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fBleedingKeepTimeDown, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastPoisonKeepTimeDown(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fPoisonKeepTimeDown, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastStomachacheKeepTimeDown(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fStomachacheKeepTimeDown, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastCriticalBlockSuccessRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fCriticalBlockSuccessRate, fValue, byApplyType, bIsPlus, DBO_MAX_CRITICAL_BLOCK_SUCCESS_RATE);
}

void CCharacterAtt::CalculateLastLpRecoveryWhenHit(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeDW(m_pAttribute.dwLpRecoveryWhenHit, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastLpRecoveryWhenHitInPercent(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fLpRecoveryWhenHitInPercent, fValue, byApplyType, bIsPlus, m_pOwnerRef->IsPC() == true ? 60.f : INVALID_FLOAT);
}

void CCharacterAtt::CalculateLastEpRecoveryWhenHit(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeDW(m_pAttribute.dwEpRecoveryWhenHit, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastEpRecoveryWhenHitInPercent(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fEpRecoveryWhenHitInPercent, fValue, byApplyType, bIsPlus, m_pOwnerRef->IsPC() == true ? 60.f : INVALID_FLOAT);
}

void CCharacterAtt::CalculateBaseStomachacheDefence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wStomachacheDefenceBase, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastStomachacheDefence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wStomachacheDefenceLast = UnsignedSafeIncrease<WORD>(m_pAttribute.wStomachacheDefenceLast, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fStomachacheDef += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wStomachacheDefenceLast = UnsignedSafeDecrease<WORD>(m_pAttribute.wStomachacheDefenceLast, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fStomachacheDefNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBasePoisonDefence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wPoisonDefenceBase, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastPoisonDefence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wPoisonDefenceLast = UnsignedSafeIncrease<WORD>(m_pAttribute.wPoisonDefenceLast, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fPoisonDef += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wPoisonDefenceLast = UnsignedSafeDecrease<WORD>(m_pAttribute.wPoisonDefenceLast, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fPoisonDefNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseBleedDefence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBleedDefenceBase, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastBleedDefence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wBleedDefenceLast = UnsignedSafeIncrease<WORD>(m_pAttribute.wBleedDefenceLast, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fBleedDef += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wBleedDefenceLast = UnsignedSafeDecrease<WORD>(m_pAttribute.wBleedDefenceLast, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fBleedDefNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseBurnDefence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wBurnDefenceBase, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastBurnDefence(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wBurnDefenceLast = UnsignedSafeIncrease<WORD>(m_pAttribute.wBurnDefenceLast, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fBurnDef += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wBurnDefenceLast = UnsignedSafeDecrease<WORD>(m_pAttribute.wBurnDefenceLast, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fBurnDefNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseMindCurseImmunity(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fBaseMindCurseImmunity, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastMindCurseImmunity(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.fLastMindCurseImmunity += fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fMindCurseImmunity += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.fLastMindCurseImmunity -= fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fMindCurseImmunityNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseBodyCurseImmunity(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fBaseBodyCurseImmunity, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastBodyCurseImmunity(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.fLastBodyCurseImmunity += fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fBodyCurseImmunity += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.fLastBodyCurseImmunity -= fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fBodyCurseImmunityNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseChangeCurseImmunity(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fBaseChangeCurseImmunity, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastChangeCurseImmunity(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.fLastChangeCurseImmunity += fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fChangeCurseImmunity += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.fLastChangeCurseImmunity -= fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fChangeCurseImmunityNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateBaseSkillAnimationSpeedModifier(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fBaseSkillAnimationSpeedModifier, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastSkillAnimationSpeedModifier(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.fLastSkillAnimationSpeedModifier += fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fSkillAnimationSpeedModifier += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.fLastSkillAnimationSpeedModifier -= fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fSkillAnimationSpeedModifierNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateLastSkillAggroBonus(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fSkillAggroBonus, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastSkillAggroBonusInPercent(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fSkillAggroBonusInPercent, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastDirectHealPowerBonus(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fDirectHealPowerBonus, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastDirectHealPowerBonusInPercent(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fDirectHealPowerBonusInPercent, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastHotPowerBonus(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fHotPowerBonus, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastHotPowerBonusInPercent(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fHotPowerBonusInPercent, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastDotValueChangePercent(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fDotValueChangePercent, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastPhysicalCriticalDefenceRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fPhysicalCriticalDefenceRate, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateLastEnergyCriticalDefenceRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fEnergyCriticalDefenceRate, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateGuardRateBase(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeW(m_pAttribute.wGuardRateBase, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateGuardRateLast(float fValue, BYTE byApplyType, bool bIsPlus)
{
	if (bIsPlus == true)
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wGuardRateLast += (WORD)fValue;
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fGuardRate += fValue;
		}
	}
	else
	{
		if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
		{
			m_pAttribute.wGuardRateLast = UnsignedSafeDecrease<WORD>(m_pAttribute.wGuardRateLast, (WORD)fValue);
		}
		else if (byApplyType == SYSTEM_EFFECT_APPLY_TYPE_PERCENT)
		{
			m_percentValue.m_fGuardRateNegative -= fValue;
		}
	}
}

void CCharacterAtt::CalculateSkillDamageBlockModeSuccessRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fSkillDamageBlockModeSuccessRate, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateCurseBlockModeSuccessRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fCurseBlockModeSuccessRate, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateKnockdownBlockModeSuccessRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fKnockdownBlockModeSuccessRate, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateHtbBlockModeSuccessRate(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeF(m_pAttribute.fHtbBlockModeSuccessRate, fValue, byApplyType, bIsPlus);
}

void CCharacterAtt::CalculateExpBooster(float fValue, BYTE byApplyType, bool bIsPlus)
{
	CalcAttributeBYTE(m_pAttribute.byExpBooster, fValue, byApplyType, bIsPlus);
}

bool CCharacterAtt::HasAnyProp()
{
	float fAttributeBonusRate = 0.0f;

	fAttributeBonusRate += m_pAttribute.fHonestOffence;
	fAttributeBonusRate += m_pAttribute.fStrangeOffence;
	fAttributeBonusRate += m_pAttribute.fWildOffence;
	fAttributeBonusRate += m_pAttribute.fEleganceOffence;
	fAttributeBonusRate += m_pAttribute.fFunnyOffence;

	fAttributeBonusRate += m_pAttribute.fHonestDefence;
	fAttributeBonusRate += m_pAttribute.fStrangeDefence;
	fAttributeBonusRate += m_pAttribute.fWildDefence;
	fAttributeBonusRate += m_pAttribute.fEleganceDefence;
	fAttributeBonusRate += m_pAttribute.fFunnyDefence;

	return fAttributeBonusRate > 0.0f;
}

