#pragma once

#include "NtlSfx.h"
#include "NtlSharedType.h"
#include "SessionFactory.h"
#include "SubNeighborServerInfoManager.h"
#include "NtlPacketGM.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct sSERVERCONFIG
{
	BYTE			ServerAcceptLimit;

	CNtlString		strAuthServerAcceptIP;
	WORD			wAuthServerAcceptPort;
	CNtlString		strCharServerAcceptIP;
	WORD			wCharServerAcceptPort;
	CNtlString		strChatServerAcceptIP;
	WORD			wChatServerAcceptPort;
	CNtlString		strGameServerAcceptIP;
	WORD			wGameServerAcceptPort;

	int				ServerPlayerLimit;

	//web admin
	CNtlString		strWebServerAcceptIP;
	WORD			wWebServerAcceptPort;
};



class CMasterServer : public CNtlServerApp
{

public:
	struct sWebOnlineListRequest
	{
		HSESSION webHandle;
		DWORD requestId;
		DWORD page;
		DWORD pageSize;
		DWORD expectedServers;
		DWORD finishedServers;
		std::unordered_set<DWORD> finishedServerKeys;
		std::vector<sWEB_ONLINE_PLAYER> players;
	};

	struct sWebActionRequest
	{
		HSESSION webHandle;
		DWORD requestId;
		BYTE action;
		DWORD expectedServers;
		DWORD finishedServers;
		DWORD affectedCount;
		bool anySuccess;
	};

	struct sChannelStatBonus
	{
		SERVERCHANNELID channelId;
		float fMaxLpPercent;
		float fMaxEpPercent;
		float fPhysicalOffencePercent;
		float fEnergyOffencePercent;
		float fPhysicalDefencePercent;
		float fEnergyDefencePercent;
		float fAttackRatePercent;
		float fDodgeRatePercent;
	};

	struct sAdminBonusState
	{
		int soloExpBonus;
		int partyExpBonus;
		int questExpBonus;
		int craftExpBonus;
		int zeniDropBonus;
		int questMoneyBonus;
		int upgradeRateBonus;
		bool monsterAggressive;
		float monsterMaxLpPercent;
		float monsterMaxEpPercent;
		float monsterPhysicalOffencePercent;
		float monsterEnergyOffencePercent;
		float monsterPhysicalDefencePercent;
		float monsterEnergyDefencePercent;
		float monsterAttackRatePercent;
		float monsterDodgeRatePercent;
		bool killDebuffEnabled;
		TBLIDX killDebuffSkillTblidx;
		DWORD killDebuffDurationSeconds;
		bool killDebuffOverrideValues;
		float killDebuffEffectValues[NTL_MAX_EFFECT_IN_SKILL];
		std::unordered_map<SERVERCHANNELID, sChannelStatBonus> channelBonuses;
	};

public:

	virtual int				OnInitApp();
	virtual int				OnAppStart();
	virtual int				OnCreate();
	virtual void			OnDestroy();


	virtual int		OnConfiguration(const char * lpszConfigFile)
	{
		CNtlIniFile file;

		int rc = file.Create( lpszConfigFile );
		if( NTL_SUCCESS != rc )
		{
			return rc;
		}
		if( !file.Read("Master Server", "ServerAcceptLimit",  m_config.ServerAcceptLimit) )
		{
			return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
		}


		//AUTH SERVER
		if( !file.Read("Auth Accept", "Address", m_config.strAuthServerAcceptIP) )
		{
			return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
		}
		if( !file.Read("Auth Accept", "Port",  m_config.wAuthServerAcceptPort) )
		{
			return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
		}

		//CHAR SERVER
		if( !file.Read("Char Accept", "Address", m_config.strCharServerAcceptIP) )
		{
			return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
		}
		if( !file.Read("Char Accept", "Port",  m_config.wCharServerAcceptPort) )
		{
			return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
		}

		//CHAT SERVER
		if( !file.Read("Chat Accept", "Address", m_config.strChatServerAcceptIP) )
		{
			return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
		}
		if( !file.Read("Chat Accept", "Port",  m_config.wChatServerAcceptPort) )
		{
			return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
		}

		//GAME SERVER
		if( !file.Read("Game Accept", "Address", m_config.strGameServerAcceptIP) )
		{
			return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
		}
		if( !file.Read("Game Accept", "Port",  m_config.wGameServerAcceptPort) )
		{
			return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
		}

		// WEB SERVER
		if( !file.Read("WEB Accept", "Address", m_config.strWebServerAcceptIP) )
		{
			return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
		}
		if (!file.Read("WEB Accept", "Port", m_config.wWebServerAcceptPort))
		{
			return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
		}

		return NTL_SUCCESS;
	}


	virtual void	Run();

	virtual BOOL	OnCommandInput(std::string& sCmd);


public:

	CNtlLog  					m_log;

	void						RegisterOnlineListRequest(HSESSION webHandle, DWORD requestId, DWORD page, DWORD pageSize);
	void						HandleOnlineListResponse(const sGM_WEB_ONLINE_PLAYERS_RES* res);
	void						RegisterActionRequest(HSESSION webHandle, DWORD requestId, BYTE action, DWORD expectedServers);
	void						HandleActionResponse(const sGM_WEB_SEND_NOTICE_RES* res);
	void						HandleActionResponse(const sGM_WEB_KICK_PLAYER_RES* res);
	void						HandleActionResponse(const sGM_WEB_GIVE_ITEM_RES* res);
	void						HandleActionResponse(const sGM_WEB_GIVE_ITEM_ALL_RES* res);
	void						HandleActionResponse(const sGM_WEB_EXEC_GM_COMMAND_RES* res);
	void						HandleActionResponse(const sGM_WEB_APPLY_BUFF_RES* res);
	void						HandleActionResponse(const sGM_WEB_REMOVE_BUFF_SKILL_RES* res);
	void						HandleActionResponse(const sGM_WEB_REMOVE_BUFF_EFFECT_RES* res);
	void						HandleActionResponse(const sGM_WEB_CLEAR_BUFFS_RES* res);
	void						HandleActionResponse(const sGM_WEB_HEAL_FULL_RES* res);
	void						HandleActionResponse(const sGM_WEB_ADD_ZENI_RES* res);
	void						HandleActionResponse(const sGM_WEB_GIVE_ITEM_CUSTOM_RES* res);
	void						HandleActionResponse(const sGM_WEB_APPLY_BUFF_SKILL_ALL_RES* res);
	void						HandleActionResponse(const sGM_WEB_APPLY_BUFF_ITEM_RES* res);
	void						HandleActionResponse(const sGM_WEB_APPLY_BUFF_ITEM_ALL_RES* res);
	void						HandleActionResponse(const sGM_WEB_REMOVE_BUFF_SKILL_ALL_RES* res);
	void						HandleActionResponse(const sGM_WEB_REMOVE_BUFF_EFFECT_ALL_RES* res);
	void						HandleActionResponse(const sGM_WEB_CLEAR_BUFFS_ALL_RES* res);
	void						HandleActionResponse(const sGM_WEB_SET_LEVEL_RES* res);
	void						HandleActionResponse(const sGM_WEB_SET_CLASS_RES* res);
	void						HandleActionResponse(const sGM_WEB_KILL_PLAYER_RES* res);
	void						HandleActionResponse(const sGM_WEB_TELEPORT_PORTAL_RES* res);
	void						HandleActionResponse(const sGM_WEB_TELEPORT_WORLD_RES* res);
	void						HandleActionResponse(const sGM_WEB_TELEPORT_COORDS_RES* res);
	void						HandleActionResponse(const sGM_WEB_MUTE_PLAYER_RES* res);
	void						HandleActionResponse(const sGM_WEB_UNMUTE_PLAYER_RES* res);
	void						HandleActionResponse(const sGM_WEB_TOGGLE_EXP_RES* res);
	void						HandleActionResponse(const sGM_WEB_RESET_EXP_RES* res);
	void						HandleActionResponse(const sGM_WEB_LEARN_SKILL_RES* res);
	void						HandleActionResponse(const sGM_WEB_ADD_TITLE_RES* res);
	void						HandleActionResponse(const sGM_WEB_REMOVE_TITLE_RES* res);
	void						HandleActionResponse(const sGM_WEB_SET_SOLO_EXP_BONUS_RES* res);
	void						HandleActionResponse(const sGM_WEB_SET_PARTY_EXP_BONUS_RES* res);
	void						HandleActionResponse(const sGM_WEB_SET_QUEST_EXP_BONUS_RES* res);
	void						HandleActionResponse(const sGM_WEB_SET_CRAFT_EXP_BONUS_RES* res);
	void						HandleActionResponse(const sGM_WEB_SET_ZENI_DROP_BONUS_RES* res);
	void						HandleActionResponse(const sGM_WEB_SET_QUEST_MONEY_BONUS_RES* res);
	void						HandleActionResponse(const sGM_WEB_SET_UPGRADE_RATE_BONUS_RES* res);
	void						HandleActionResponse(const sGM_WEB_SET_MONSTER_AGGRESSIVE_RES* res);
	void						HandleActionResponse(const sGM_WEB_SET_MONSTER_STAT_BONUS_RES* res);
	void						HandleActionResponse(const sGM_WEB_APPLY_MONSTER_BUFF_SKILL_ALL_RES* res);
	void						HandleActionResponse(const sGM_WEB_CLEAR_MONSTER_BUFFS_ALL_RES* res);
	void						HandleActionResponse(const sGM_WEB_SET_KILL_DEBUFF_RES* res);
	void						HandleActionResponse(const sGM_WEB_RESET_SKILL_COOLDOWN_RES* res);
	void						HandleActionResponse(const sGM_WEB_RESET_SKILL_COOLDOWN_ALL_RES* res);
	void						HandleActionResponse(const sGM_WEB_SET_CHANNEL_STAT_BONUS_RES* res);

private:

	CNtlAcceptor				m_AuthServerAcceptor;
	CNtlAcceptor				m_CharServerAcceptor;
	CNtlAcceptor				m_ChatServerAcceptor;
	CNtlAcceptor				m_GameServerAcceptor;
	CNtlAcceptor				m_WebServerAcceptor;

public:

	sSERVERCONFIG				m_config;
	std::unordered_map<DWORD, sWebOnlineListRequest> m_webOnlineRequests;
	std::unordered_map<DWORD, sWebActionRequest> m_webActionRequests;
	sAdminBonusState			m_adminBonusState;
	DWORD						m_startTick;
	time_t						m_startTime;


};