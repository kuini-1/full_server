#pragma once

#include "NtlSfx.h"
#include "NtlSharedDef.h"
#include "NtlServer.h"
#include <unordered_map>

#include "GameSessionFactory.h"


class CChatServerSession;
class CMasterServerSession;
class CQueryServerSession;
class CGameProcessor;
class CGameMain;
class CGameData;
class CActionPatternSystem;

struct sMONSTER_STAT_BONUS
{
	float fMaxLpPercent;
	float fMaxEpPercent;
	float fPhysicalOffencePercent;
	float fEnergyOffencePercent;
	float fPhysicalDefencePercent;
	float fEnergyDefencePercent;
	float fAttackRatePercent;
	float fDodgeRatePercent;
};

struct sCHANNEL_STAT_BONUS
{
	float fMaxLpPercent;
	float fMaxEpPercent;
	float fPhysicalOffencePercent;
	float fEnergyOffencePercent;
	float fPhysicalDefencePercent;
	float fEnergyDefencePercent;
	float fAttackRatePercent;
	float fDodgeRatePercent;
};

struct sKILL_DEBUFF_CONFIG
{
	bool bEnable;
	TBLIDX skillTblidx;
	DWORD durationSeconds;
	bool bOverrideValues;
	float aEffectValues[NTL_MAX_EFFECT_IN_SKILL];
};

class CGameServer : public CNtlServerApp
{

public:

	CGameServer();
	virtual ~CGameServer();

private:

	DWORD			m_dwCurTickCount;
	DWORD			m_dwLastTimePerformanceLogged;
	DWORD			m_dwLastTimeLoadReported;
	DWORD			m_dwLastTimeMemoryUseLogged;

	time_t			m_tmCurrentTime;

public:

	virtual int		OnInitApp();
	virtual int		OnAppStart();
	virtual int		OnCreate();

	virtual int		OnConfiguration(const char * lpszConfigFile);
	virtual BOOL	OnCommandInput(std::string& sCmd);

	virtual void	Run();

private:

	void			Init();
	void			Destroy();

private:

	void			DoUpdatePerformanceLog(DWORD dwNow);
	void			DoReportLoad(DWORD dwNow);
	void			DoUpdateMemoryUseLog(DWORD dwNow);

public:

	inline DWORD			GetCurTickCount() { return m_dwCurTickCount; }
	inline time_t			GetTime()			{ return m_tmCurrentTime; }

	BOOL			IsTestServer()					{ return m_config.bTestServer; }

	CNtlString		GetConfigChatServerIP()			{ return m_config.strChatServerIP; }
	WORD			GetConfigChatServerPort()		{ return m_config.wChatServerPort; }
	CNtlString		GetConfigCharServerIP()			{ return m_config.strCharServerIP; }
	WORD			GetConfigCharServerPort()		{ return m_config.wCharServerPort; }

	CNtlString		GetLogPath()					{ return m_config.strLogPath; }
	CNtlString		GetTsPath()						{ return m_config.strTsPath; }

	SERVERCHANNELID	GetGsChannel()	{ return m_config.byChannel; }
	SERVERFARMID	GetGsServerId()	{ return m_config.byServerID; }

	BYTE			GetPlayerMaxLevel()		{ return m_config.MaxLevel; }
	int				GetSoloExpRate()		{ return m_config.SoloExpRate + m_eventSoloExpBonus; }
	int				GetPartyExpRate()		{ return m_config.PartyExpRate + m_eventPartyExpBonus; }
	int				GetItemDropRate()		{ return m_config.ItemDropRate; }
	int				GetOriginalItemDropRate() { return m_config.OriginalItemDropRate; }
	int				GetZeniDropRate()		{ return m_config.ZeniDropRate + m_eventZeniDropBonus; }
	int				GetZeniBonusRate()		{ return m_config.ZeniBonusRate; }
	int				GetZeniPartyBonusRate() { return m_config.ZeniPartyBonusRate; }
	int				GetQuestMoneyRate()		{ return m_config.QuestMoneyRate + m_eventQuestMoneyBonus; }
	int				GetQuestExpRate()		{ return m_config.QuestExpRate + m_eventQuestExpBonus; }
	int				GetCraftExpRate() { return m_config.CraftExpRate + m_eventCraftExpBonus; }

	void			SetItemDropRate(int value) { m_config.ItemDropRate = value; }
	void			SetSoloExpBonus(int value) { m_eventSoloExpBonus = value; }
	void			SetPartyExpBonus(int value) { m_eventPartyExpBonus = value; }
	void			SetQuestExpBonus(int value) { m_eventQuestExpBonus = value; }
	void			SetCraftExpBonus(int value) { m_eventCraftExpBonus = value; }
	void			SetZeniDropBonus(int value) { m_eventZeniDropBonus = value; }
	void			SetQuestMoneyBonus(int value) { m_eventQuestMoneyBonus = value; }
	void			SetUpgradeRateBonus(int value) { m_eventUpgradeRateBonus = value; }
	int				GetUpgradeRateBonus() { return m_eventUpgradeRateBonus; }

	void			SetMonsterAggressive(bool enable) { m_bMonsterAggressive = enable; }
	bool			IsMonsterAggressive() { return m_bMonsterAggressive; }
	void			SetMonsterStatBonus(BYTE worldRuleType, const sMONSTER_STAT_BONUS& bonus) { m_monsterStatBonusByWorldType[worldRuleType] = bonus; }
	const sMONSTER_STAT_BONUS& GetMonsterStatBonus(BYTE worldRuleType) 
	{ 
		auto it = m_monsterStatBonusByWorldType.find(worldRuleType);
		if (it != m_monsterStatBonusByWorldType.end())
			return it->second;
		static sMONSTER_STAT_BONUS emptyBonus = {0};
		return emptyBonus;
	}
	void			SetChannelStatBonus(const sCHANNEL_STAT_BONUS& bonus) { m_channelStatBonus = bonus; }
	const sCHANNEL_STAT_BONUS& GetChannelStatBonus() { return m_channelStatBonus; }
	void			SetKillDebuffConfig(const sKILL_DEBUFF_CONFIG& config) { m_killDebuffConfig = config; }
	const sKILL_DEBUFF_CONFIG& GetKillDebuffConfig() { return m_killDebuffConfig; }

	CNtlString		GetDatabaseHost()	{	return m_config.DatabaseHost;	}
	CNtlString		GetDatabaseUser()	{	return m_config.DatabaseUser;	}
	CNtlString		GetDatabasePassword()	{	return m_config.DatabasePassword;	}
	CNtlString		GetDatabaseDatabase()	{	return m_config.Database;	}
	CNtlString		GetAccDbHost()	{ return m_config.AccDatabaseHost; }
	CNtlString		GetAccDbUser()	{ return m_config.AccDatabaseUser; }
	CNtlString		GetAccDbPass()	{ return m_config.AccDatabasePassword; }
	CNtlString		GetAccDbDatabase() { return m_config.AccDatabase; }

	inline bool		IsDojoChannel() { return m_config.byChannel == DOJO_CHANNEL_INDEX; }


	CActionPatternSystem*			GetActionPatternSystem() { return m_pActionPatternSystem; }
	CGameData*						GetGameData() { return m_pGameData; }
	CGameMain*						GetGameMain() { return m_pGameMain; }
	CGameProcessor*					GetGameProcessor() { return m_pGameProcessor; }

	CNtlLog*						GetLog() { return &m_log; }



public:
	sGAME_SERVERCONFIG			m_config;

private:

	CNtlLog  					m_log;

	int							m_eventSoloExpBonus;
	int							m_eventPartyExpBonus;
	int							m_eventQuestExpBonus;
	int							m_eventCraftExpBonus;
	int							m_eventZeniDropBonus;
	int							m_eventQuestMoneyBonus;
	int							m_eventUpgradeRateBonus;
	bool						m_bMonsterAggressive;
	std::unordered_map<BYTE, sMONSTER_STAT_BONUS>	m_monsterStatBonusByWorldType; // BYTE = world rule type (GAMERULE_*)
	sCHANNEL_STAT_BONUS			m_channelStatBonus;
	sKILL_DEBUFF_CONFIG			m_killDebuffConfig;

private:

	CChatServerSession *		m_pChatServerSession;
	CMasterServerSession *		m_pMasterServerSession;
	CQueryServerSession *		m_pQueryServerSession;

private:

	CNtlConnector				m_serverChatConnector;
	CNtlConnector				m_serverQueryConnector;
	CNtlConnector				m_serverMasterConnector;
	CNtlAcceptor				m_clientAcceptor;

private:

	CGameProcessor*				m_pGameProcessor;
	CGameMain *					m_pGameMain;
	CGameData *					m_pGameData;
	CActionPatternSystem *		m_pActionPatternSystem;

public:


	CChatServerSession* GetChatServerSession() { return m_pChatServerSession; }
	void SetChatServerSession(CChatServerSession * pServerSession)
	{
		m_pChatServerSession = pServerSession;
	}

	CQueryServerSession* GetQueryServerSession() { return m_pQueryServerSession; }
	void SetQueryServerSession(CQueryServerSession * pServerSession)
	{
		m_pQueryServerSession = pServerSession;
	}

	CNtlConnector*		GetMasterServerConnector() { return &m_serverMasterConnector; }

	CMasterServerSession* GetMasterServerSession() { return m_pMasterServerSession; }
	void SetMasterServerSession(CMasterServerSession * pServerSession)
	{
		m_pMasterServerSession = pServerSession;
	}

};

