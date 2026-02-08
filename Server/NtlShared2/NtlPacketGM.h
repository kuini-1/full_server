#pragma once

#include "NtlPacketCommon.h"
#include "NtlCSArchitecture.h"
#include "NtlStatistics.h"


enum eOPCODE_GM
{
	GM_OPCODE_BEGIN = 13000,

	GM_NOTIFY_SERVER_BEGIN = GM_OPCODE_BEGIN,
	GM_HEARTBEAT,
	GM_REPORT_LOAD,
	GM_REPORT_CONNECTED_USER_INFO,

	GM_REPORT_MONSTER_STATISTICS_BEGIN,
	GM_REPORT_MONSTER_STATISTICS,
	GM_REPORT_MONSTER_STATISTICS_SENDING_PAUSED_NFY,
	GM_REPORT_MONSTER_STATISTICS_END,

	GM_USERCOUNT_RES,
	GM_SERVER_CONTROL_TURN_OFF_ALL_NFY,
	GM_LOGIN_REQ,
	GM_LOGOUT_REQ,
	GM_MOVE_REQ,
	GM_KICKOUT_RES,
	GM_ON_PLAYER_INFO,

	GM_PING_RES,

	GM_PLAYER_SWITCH_CHANNEL_REQ,
	GM_CHAR_SERVER_TELEPORT_REQ,
	GM_PLAYER_EXIT_TO_CHAR_REQ,

	GM_SERVER_SHUT_DOWN,

	GM_DRAGONBALL_SCRAMBLE_SEASON_STATE_NFY,
	GM_WEB_ONLINE_PLAYERS_RES,
	GM_WEB_SEND_NOTICE_RES,
	GM_WEB_KICK_PLAYER_RES,
	GM_WEB_GIVE_ITEM_RES,
	GM_WEB_GIVE_ITEM_ALL_RES,
	GM_WEB_EXEC_GM_COMMAND_RES,
	GM_WEB_APPLY_BUFF_RES,
	GM_WEB_REMOVE_BUFF_SKILL_RES,
	GM_WEB_REMOVE_BUFF_EFFECT_RES,
	GM_WEB_CLEAR_BUFFS_RES,
	GM_WEB_HEAL_FULL_RES,
	GM_WEB_ADD_ZENI_RES,
	GM_WEB_GIVE_ITEM_CUSTOM_RES,
	GM_WEB_APPLY_BUFF_SKILL_ALL_RES,
	GM_WEB_APPLY_BUFF_ITEM_RES,
	GM_WEB_APPLY_BUFF_ITEM_ALL_RES,
	GM_WEB_REMOVE_BUFF_SKILL_ALL_RES,
	GM_WEB_REMOVE_BUFF_EFFECT_ALL_RES,
	GM_WEB_CLEAR_BUFFS_ALL_RES,
	GM_WEB_SET_LEVEL_RES,
	GM_WEB_SET_CLASS_RES,
	GM_WEB_KILL_PLAYER_RES,
	GM_WEB_TELEPORT_PORTAL_RES,
	GM_WEB_TELEPORT_WORLD_RES,
	GM_WEB_TELEPORT_COORDS_RES,
	GM_WEB_MUTE_PLAYER_RES,
	GM_WEB_UNMUTE_PLAYER_RES,
	GM_WEB_TOGGLE_EXP_RES,
	GM_WEB_RESET_EXP_RES,
	GM_WEB_LEARN_SKILL_RES,
	GM_WEB_ADD_TITLE_RES,
	GM_WEB_REMOVE_TITLE_RES,
	GM_WEB_SET_SOLO_EXP_BONUS_RES,
	GM_WEB_SET_PARTY_EXP_BONUS_RES,
	GM_WEB_SET_QUEST_EXP_BONUS_RES,
	GM_WEB_SET_CRAFT_EXP_BONUS_RES,
	GM_WEB_SET_ZENI_DROP_BONUS_RES,
	GM_WEB_SET_QUEST_MONEY_BONUS_RES,
	GM_WEB_SET_UPGRADE_RATE_BONUS_RES,
	GM_WEB_SET_MONSTER_AGGRESSIVE_RES,
	GM_WEB_SET_MONSTER_STAT_BONUS_RES,
	GM_WEB_APPLY_MONSTER_BUFF_SKILL_ALL_RES,
	GM_WEB_CLEAR_MONSTER_BUFFS_ALL_RES,
	GM_WEB_SET_KILL_DEBUFF_RES,
	GM_WEB_RESET_SKILL_COOLDOWN_RES,
	GM_WEB_RESET_SKILL_COOLDOWN_ALL_RES,
	GM_WEB_SET_CHANNEL_STAT_BONUS_RES,

	GM_OPCODE_DUMMY,
	GM_OPCODE_END = GM_OPCODE_DUMMY - 1
};


//------------------------------------------------------------------
//
//------------------------------------------------------------------
const char * NtlGetPacketName_GM(WORD wOpCode);
//------------------------------------------------------------------


#pragma pack(1)

//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_NOTIFY_SERVER_BEGIN)
sDBO_SERVER_INFO				serverInfo;
sDBO_GAME_SERVER_FARM_INFO		gameServerFarmInfo;
sDBO_GAME_SERVER_CHANNEL_INFO	gameServerChannelInfo;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_HEARTBEAT)
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_REPORT_LOAD)
	DWORD		dwMaxLoad;
	DWORD		dwLoad;
	DWORD		dwProcessUsage;			// Process Usage
	DWORD		dwSystemUsage;			// System Usage
	DWORD		dwMemoryUsage;			// Memory Usage
	DWORD		dwPing;					// by itzrnb 08/05/2009
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_REPORT_CONNECTED_USER_INFO)
	sDBO_SERVER_CHANNEL_CONNECTED_USER_INFO		connectedUserInfo;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_REPORT_MONSTER_STATISTICS_BEGIN)
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_REPORT_MONSTER_STATISTICS)
	BYTE										byCount;
	sDBO_MONSTER_STATISTICS						aMonsterStatistics[DBO_MAX_MONSTER_STATISTICS_COUNT_PER_PACKET];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_REPORT_MONSTER_STATISTICS_SENDING_PAUSED_NFY)
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_REPORT_MONSTER_STATISTICS_END)
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GM_USERCOUNT_RES )
	ACCOUNTID				managerID;		  // GM ID
	DWORD					dwConnectingCurrent;
	DWORD					dwConnectingMax;
	DWORD					dwPlayingCurrent;
	DWORD					dwPlayingMax;
	DWORD					dwPlayerCurrent;
	DWORD					dwPlayerMax;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GM_SERVER_CONTROL_TURN_OFF_ALL_NFY )
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GM_LOGIN_REQ )
	ACCOUNTID				accountId;
	BYTE					abyAuthKey[NTL_MAX_SIZE_AUTH_KEY];	// ?????
	HOBJECT					handle;
	SERVERFARMID			serverId;
	SERVERCHANNELID			serverChannelId;
	CHARACTERID				charId;
	WCHAR					awchCharName[NTL_MAX_SIZE_CHAR_NAME + 1];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GM_LOGOUT_REQ )
	ACCOUNTID				accountId;
	SERVERFARMID			serverId;
	SERVERCHANNELID			serverChannelId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GM_MOVE_REQ )
	ACCOUNTID				accountId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GM_KICKOUT_RES )
	ACCOUNTID	accountId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_ON_PLAYER_INFO)
	WORD		wCount;			
	ACCOUNTID	aAccountId[NTL_MAX_COUNT_ON_PLAY_USER];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_PING_RES)
	DWORD			dwTick;
	SERVERFARMID	serverId;
	SERVERCHANNELID	serverChannelId;
END_PROTOCOL()
//------------------------------------------------------------------

//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_PLAYER_SWITCH_CHANNEL_REQ)
	ACCOUNTID		accountId;
	SERVERFARMID	serverId;
	SERVERCHANNELID	serverChannelId;
	SERVERCHANNELID	destServerChannelId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_CHAR_SERVER_TELEPORT_REQ)
ACCOUNTID		accountId;
SERVERFARMID	serverId;
SERVERCHANNELID	serverChannelId;
SERVERCHANNELID	destServerChannelId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_PLAYER_EXIT_TO_CHAR_REQ)
	HOBJECT		hHandle;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_SERVER_SHUT_DOWN)
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_DRAGONBALL_SCRAMBLE_SEASON_STATE_NFY)
WORD			wSeasonState;
SERVERFARMID	serverFarmID;
SERVERCHANNELID	serverChannelID;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_ONLINE_PLAYERS_RES)
	DWORD					requestId;
	SERVERFARMID			serverFarmId;
	SERVERCHANNELID			serverChannelId;
	DWORD					totalCount;
	WORD					count;
	bool					bIsLast;
	sWEB_ONLINE_PLAYER		players[NTL_MAX_WEB_ONLINE_PLAYERS];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_SEND_NOTICE_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_KICK_PLAYER_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_GIVE_ITEM_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_GIVE_ITEM_ALL_RES)
	DWORD					requestId;
	WORD					wResultCode;
	DWORD					dwAffectedCount;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_EXEC_GM_COMMAND_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_APPLY_BUFF_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_REMOVE_BUFF_SKILL_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_REMOVE_BUFF_EFFECT_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_CLEAR_BUFFS_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_HEAL_FULL_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_ADD_ZENI_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_GIVE_ITEM_CUSTOM_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_APPLY_BUFF_SKILL_ALL_RES)
	DWORD					requestId;
	WORD					wResultCode;
	DWORD					dwAffectedCount;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_APPLY_BUFF_ITEM_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_APPLY_BUFF_ITEM_ALL_RES)
	DWORD					requestId;
	WORD					wResultCode;
	DWORD					dwAffectedCount;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_REMOVE_BUFF_SKILL_ALL_RES)
	DWORD					requestId;
	WORD					wResultCode;
	DWORD					dwAffectedCount;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_REMOVE_BUFF_EFFECT_ALL_RES)
	DWORD					requestId;
	WORD					wResultCode;
	DWORD					dwAffectedCount;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_CLEAR_BUFFS_ALL_RES)
	DWORD					requestId;
	WORD					wResultCode;
	DWORD					dwAffectedCount;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_SET_LEVEL_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_SET_CLASS_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_KILL_PLAYER_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_TELEPORT_PORTAL_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_TELEPORT_WORLD_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_TELEPORT_COORDS_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_MUTE_PLAYER_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_UNMUTE_PLAYER_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_TOGGLE_EXP_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_RESET_EXP_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_LEARN_SKILL_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_ADD_TITLE_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_REMOVE_TITLE_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_SET_SOLO_EXP_BONUS_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_SET_PARTY_EXP_BONUS_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_SET_QUEST_EXP_BONUS_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_SET_CRAFT_EXP_BONUS_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_SET_ZENI_DROP_BONUS_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_SET_QUEST_MONEY_BONUS_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_SET_UPGRADE_RATE_BONUS_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_SET_MONSTER_AGGRESSIVE_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_SET_MONSTER_STAT_BONUS_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_APPLY_MONSTER_BUFF_SKILL_ALL_RES)
	DWORD					requestId;
	WORD					wResultCode;
	DWORD					dwAffectedCount;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_CLEAR_MONSTER_BUFFS_ALL_RES)
	DWORD					requestId;
	WORD					wResultCode;
	DWORD					dwAffectedCount;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_SET_KILL_DEBUFF_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_RESET_SKILL_COOLDOWN_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_RESET_SKILL_COOLDOWN_ALL_RES)
	DWORD					requestId;
	WORD					wResultCode;
	DWORD					dwAffectedCount;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GM_WEB_SET_CHANNEL_STAT_BONUS_RES)
	DWORD					requestId;
	WORD					wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------

#pragma pack()
