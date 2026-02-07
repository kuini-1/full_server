#pragma once

#include "NtlPacketCommon.h"
#include "NtlSkill.h"

enum eOPCODE_MW
{
	MW_OPCODE_BEGIN = 18000,

	WM_GET_PLAYER_COUNT_RES = MW_OPCODE_BEGIN,
	WM_GET_CHANNEL_STATUS_RES,
	WM_GET_ONLINE_PLAYERS_RES,
	WM_ADMIN_ACTION_RES,
	WM_GET_MASTER_UPTIME_RES,
	WM_GET_MASTER_TIME_RES,
	WM_GET_MASTER_CONFIG_RES,
	WM_GET_SERVER_COUNTS_RES,
	WM_GET_SESSION_COUNTS_RES,
	WM_GET_PLAYER_COUNTS_RES,
	WM_GET_GAME_FARM_LIST_RES,
	WM_GET_GAME_CHANNEL_LIST_RES,
	WM_GET_AUTH_SERVER_LIST_RES,
	WM_GET_CHAR_SERVER_LIST_RES,
	WM_GET_CHAT_SERVER_LIST_RES,
	WM_GET_GAME_SERVER_LIST_RES,
	WM_GET_QUERY_SERVER_LIST_RES,
	WM_GET_SERVER_LIST_ALL_RES,
	WM_GET_ONLINE_COUNT_BY_CHANNEL_RES,
	WM_GET_ONLINE_COUNT_BY_FARM_RES,
	WM_GET_ONLINE_ACCOUNT_LIST_RES,
	WM_GET_WEB_REQUEST_STATS_RES,
	WM_GET_SERVER_LOAD_SUMMARY_RES,
	WM_GET_CHANNEL_VISIBILITY_LIST_RES,
	WM_GET_SCRAMBLE_CHANNEL_LIST_RES,
	WM_GET_BONUS_STATE_RES,

	MW_OPCODE_DUMMY,
	MW_OPCODE_END = MW_OPCODE_DUMMY - 1
};


#pragma pack(1)
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_PLAYER_COUNT_RES)
int				chPlayerCount;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_CHANNEL_STATUS_RES)
bool				auth;
bool				chat;
bool				channel0;
bool				channel1;
bool				channel2;
bool				channel3;
bool				channel4;
bool				channel5;
bool				channel6;
bool				channel7;
bool				channel8;
bool				channel9;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_ONLINE_PLAYERS_RES)
	DWORD					requestId;
	DWORD					totalCount;
	WORD					count;
	sWEB_ONLINE_PLAYER		players[NTL_MAX_WEB_ONLINE_PLAYERS];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_ADMIN_ACTION_RES)
	DWORD					requestId;
	BYTE					byAction;
	WORD					wResultCode;
	DWORD					dwAffectedCount;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_MASTER_UPTIME_RES)
	DWORD					requestId;
	DWORD					uptimeSeconds;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_MASTER_TIME_RES)
	DWORD					requestId;
	DWORD					unixTime;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_MASTER_CONFIG_RES)
	DWORD					requestId;
	BYTE					serverAcceptLimit;
	DWORD					serverPlayerLimit;
	char					authAddress[NTL_MAX_LENGTH_OF_IP + 1];
	WORD					authPort;
	char					charAddress[NTL_MAX_LENGTH_OF_IP + 1];
	WORD					charPort;
	char					chatAddress[NTL_MAX_LENGTH_OF_IP + 1];
	WORD					chatPort;
	char					gameAddress[NTL_MAX_LENGTH_OF_IP + 1];
	WORD					gamePort;
	char					webAddress[NTL_MAX_LENGTH_OF_IP + 1];
	WORD					webPort;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_SERVER_COUNTS_RES)
	DWORD					requestId;
	WORD					authCount;
	WORD					charCount;
	WORD					chatCount;
	WORD					gameCount;
	WORD					queryCount;
	WORD					communityCount;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_SESSION_COUNTS_RES)
	DWORD					requestId;
	WORD					authSessions;
	WORD					charSessions;
	WORD					chatSessions;
	WORD					gameSessions;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_PLAYER_COUNTS_RES)
	DWORD					requestId;
	DWORD					charPlayers;
	DWORD					gamePlayers;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_GAME_FARM_LIST_RES)
	DWORD					requestId;
	WORD					count;
	sWEB_GAME_FARM_INFO		farms[NTL_MAX_WEB_FARM_LIST];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_GAME_CHANNEL_LIST_RES)
	DWORD					requestId;
	WORD					count;
	sWEB_GAME_CHANNEL_INFO	channels[NTL_MAX_WEB_CHANNEL_LIST];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_AUTH_SERVER_LIST_RES)
	DWORD					requestId;
	WORD					count;
	sWEB_SERVER_ENTRY		servers[NTL_MAX_WEB_SERVER_LIST];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_CHAR_SERVER_LIST_RES)
	DWORD					requestId;
	WORD					count;
	sWEB_SERVER_ENTRY		servers[NTL_MAX_WEB_SERVER_LIST];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_CHAT_SERVER_LIST_RES)
	DWORD					requestId;
	WORD					count;
	sWEB_SERVER_ENTRY		servers[NTL_MAX_WEB_SERVER_LIST];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_GAME_SERVER_LIST_RES)
	DWORD					requestId;
	WORD					count;
	sWEB_SERVER_ENTRY		servers[NTL_MAX_WEB_SERVER_LIST];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_QUERY_SERVER_LIST_RES)
	DWORD					requestId;
	WORD					count;
	sWEB_SERVER_ENTRY		servers[NTL_MAX_WEB_SERVER_LIST];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_SERVER_LIST_ALL_RES)
	DWORD					requestId;
	WORD					count;
	sWEB_SERVER_ENTRY		servers[NTL_MAX_WEB_SERVER_LIST];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_ONLINE_COUNT_BY_CHANNEL_RES)
	DWORD					requestId;
	WORD					count;
	sWEB_CHANNEL_COUNT		channels[NTL_MAX_WEB_CHANNEL_COUNT_LIST];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_ONLINE_COUNT_BY_FARM_RES)
	DWORD					requestId;
	WORD					count;
	sWEB_FARM_COUNT			farms[NTL_MAX_WEB_FARM_COUNT_LIST];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_ONLINE_ACCOUNT_LIST_RES)
	DWORD					requestId;
	DWORD					totalCount;
	WORD					count;
	sWEB_ONLINE_ACCOUNT		accounts[NTL_MAX_WEB_ACCOUNT_LIST];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_WEB_REQUEST_STATS_RES)
	DWORD					requestId;
	DWORD					onlineRequests;
	DWORD					actionRequests;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_SERVER_LOAD_SUMMARY_RES)
	DWORD					requestId;
	DWORD					totalLoad;
	DWORD					totalMaxLoad;
	WORD					farmCount;
	WORD					channelCount;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_CHANNEL_VISIBILITY_LIST_RES)
	DWORD					requestId;
	WORD					count;
	sWEB_CHANNEL_FLAG		channels[NTL_MAX_WEB_CHANNEL_FLAG_LIST];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_SCRAMBLE_CHANNEL_LIST_RES)
	DWORD					requestId;
	WORD					count;
	sWEB_CHANNEL_FLAG		channels[NTL_MAX_WEB_CHANNEL_FLAG_LIST];
END_PROTOCOL()
//------------------------------------------------------------------
struct sWEB_CHANNEL_STAT_BONUS
{
	SERVERCHANNELID	channelId;
	float			fMaxLpPercent;
	float			fMaxEpPercent;
	float			fPhysicalOffencePercent;
	float			fEnergyOffencePercent;
	float			fPhysicalDefencePercent;
	float			fEnergyDefencePercent;
	float			fAttackRatePercent;
	float			fDodgeRatePercent;
};
BEGIN_PROTOCOL(WM_GET_BONUS_STATE_RES)
	DWORD			requestId;
	int				soloExpBonus;
	int				partyExpBonus;
	int				questExpBonus;
	int				craftExpBonus;
	int				zeniDropBonus;
	int				questMoneyBonus;
	int				upgradeRateBonus;
	bool			bMonsterAggressive;
	float			monsterMaxLpPercent;
	float			monsterMaxEpPercent;
	float			monsterPhysicalOffencePercent;
	float			monsterEnergyOffencePercent;
	float			monsterPhysicalDefencePercent;
	float			monsterEnergyDefencePercent;
	float			monsterAttackRatePercent;
	float			monsterDodgeRatePercent;
	bool			bKillDebuffEnabled;
	TBLIDX			killDebuffSkillTblidx;
	DWORD			killDebuffDurationSeconds;
	bool			killDebuffOverrideValues;
	float			killDebuffEffectValues[NTL_MAX_EFFECT_IN_SKILL];
	WORD			channelBonusCount;
	sWEB_CHANNEL_STAT_BONUS	channelBonuses[NTL_MAX_WEB_CHANNEL_LIST];
END_PROTOCOL()
//------------------------------------------------------------------
#pragma pack()