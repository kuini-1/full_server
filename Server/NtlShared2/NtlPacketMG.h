#pragma once

#include "NtlPacketCommon.h"
#include "NtlItem.h"
#include "NtlMail.h"
#include "NtlCSArchitecture.h"
#include "NtlSystemTool.h"

enum eOPCODE_MG
{
	MG_OPCODE_BEGIN = 12000,

	MG_HEARTBEAT = MG_OPCODE_BEGIN,
	MG_AUTH_SERVER_FARM_INFO,
	MG_CHARACTER_SERVER_FARM_INFO,
	MG_GAME_SERVER_FARM_INFO,
	MG_GAME_SERVER_CHANNEL_INFO,
	MG_SERVERS_INFO,
	MG_SERVICE_INFO,
	MG_WORLD_ASSIGNMENT_INFO_BEGIN,
	MG_WORLD_ASSIGNMENT_INFO,
	MG_WORLD_ASSIGNMENT_INFO_END,
	MG_SERVERS_INFO_END,
	MG_NOTIFY_ILLEGAL_SERVER,

	MG_SERVER_INFO_CHANGED_NFY,
	MG_AUTH_SERVER_FARM_INFO_CHANGED_NFY,
	MG_CHARACTER_SERVER_FARM_INFO_CHANGED_NFY,

	MG_GAME_SERVER_FARM_INFO_CHANGED_NFY,
	MG_GAME_SERVER_CHANNEL_INFO_CHANGED_NFY,

	MG_SERVER_USER_ALL_KICK_REQ,					// ???? ???? ??????? ???? logout
	MG_SERVER_VERSION_CHANGED_NFY, // by sooshia 07/23/2008

	MG_SERVER_TURNOFF_FOR_DEVEL_NFY,
	MG_SERVER_MAX_USER_NFY,
	MG_LOGIN_RES,
	MG_MOVE_RES,
	MG_KICKOUT_REQ,

	MG_PING_REQ, // by itzrnb 08/05/2009

	MG_REPORT_MONSTER_STATISTICS_CONTINUE_REQ,

	MG_PLAYER_SWITCH_CHANNEL_RES,
	MG_CHAR_SERVER_TELEPORT_RES,
	MG_PLAYER_EXIT_TO_CHAR_RES,

	MG_SERVERS_INFO_ADD,
	MG_WEB_ONLINE_PLAYERS_REQ,
	MG_WEB_SEND_NOTICE_REQ,
	MG_WEB_KICK_PLAYER_REQ,
	MG_WEB_GIVE_ITEM_REQ,
	MG_WEB_GIVE_ITEM_ALL_REQ,
	MG_WEB_EXEC_GM_COMMAND_REQ,
	MG_WEB_APPLY_BUFF_REQ,
	MG_WEB_REMOVE_BUFF_SKILL_REQ,
	MG_WEB_REMOVE_BUFF_EFFECT_REQ,
	MG_WEB_CLEAR_BUFFS_REQ,
	MG_WEB_HEAL_FULL_REQ,
	MG_WEB_ADD_ZENI_REQ,
	MG_WEB_GIVE_ITEM_CUSTOM_REQ,
	MG_WEB_APPLY_BUFF_SKILL_ALL_REQ,
	MG_WEB_APPLY_BUFF_ITEM_REQ,
	MG_WEB_APPLY_BUFF_ITEM_ALL_REQ,
	MG_WEB_REMOVE_BUFF_SKILL_ALL_REQ,
	MG_WEB_REMOVE_BUFF_EFFECT_ALL_REQ,
	MG_WEB_CLEAR_BUFFS_ALL_REQ,
	MG_WEB_SET_LEVEL_REQ,
	MG_WEB_SET_CLASS_REQ,
	MG_WEB_KILL_PLAYER_REQ,
	MG_WEB_TELEPORT_PORTAL_REQ,
	MG_WEB_TELEPORT_WORLD_REQ,
	MG_WEB_TELEPORT_COORDS_REQ,
	MG_WEB_MUTE_PLAYER_REQ,
	MG_WEB_UNMUTE_PLAYER_REQ,
	MG_WEB_TOGGLE_EXP_REQ,
	MG_WEB_RESET_EXP_REQ,
	MG_WEB_LEARN_SKILL_REQ,
	MG_WEB_ADD_TITLE_REQ,
	MG_WEB_REMOVE_TITLE_REQ,
	MG_WEB_SET_SOLO_EXP_BONUS_REQ,
	MG_WEB_SET_PARTY_EXP_BONUS_REQ,
	MG_WEB_SET_QUEST_EXP_BONUS_REQ,
	MG_WEB_SET_CRAFT_EXP_BONUS_REQ,
	MG_WEB_SET_ZENI_DROP_BONUS_REQ,
	MG_WEB_SET_QUEST_MONEY_BONUS_REQ,
	MG_WEB_SET_UPGRADE_RATE_BONUS_REQ,
	MG_WEB_SET_MONSTER_AGGRESSIVE_REQ,
	MG_WEB_SET_MONSTER_STAT_BONUS_REQ,
	MG_WEB_APPLY_MONSTER_BUFF_SKILL_ALL_REQ,
	MG_WEB_CLEAR_MONSTER_BUFFS_ALL_REQ,
	MG_WEB_SET_KILL_DEBUFF_REQ,
	MG_WEB_RESET_SKILL_COOLDOWN_REQ,
	MG_WEB_RESET_SKILL_COOLDOWN_ALL_REQ,
	MG_WEB_SET_CHANNEL_STAT_BONUS_REQ,

	MG_OPCODE_DUMMY,
	MG_OPCODE_END = MG_OPCODE_DUMMY - 1
};


//------------------------------------------------------------------
//
//------------------------------------------------------------------
const char * NtlGetPacketName_MG(WORD wOpCode);
//------------------------------------------------------------------


#pragma pack(1)

//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_HEARTBEAT)
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_AUTH_SERVER_FARM_INFO)
sDBO_AUTH_SERVER_FARM_INFO			serverFarmInfo;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_CHARACTER_SERVER_FARM_INFO)
sDBO_CHARACTER_SERVER_FARM_INFO		serverFarmInfo;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_GAME_SERVER_FARM_INFO)
sDBO_GAME_SERVER_FARM_INFO			gameServerFarmInfo;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_GAME_SERVER_CHANNEL_INFO)
sDBO_GAME_SERVER_CHANNEL_INFO		gameServerChannelInfo;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_SERVERS_INFO)
sDBO_SERVER_INFO		serverInfo;
BYTE					byIsMyInfo;		// 0x00 : false, 0x01 : true
BYTE					byChannelCount;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_SERVICE_INFO)
BYTE								byServicePublisher;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WORLD_ASSIGNMENT_INFO_BEGIN)
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WORLD_ASSIGNMENT_INFO)
SERVERCHANNELID						serverChannelId;
SERVERINDEX							serverIndex;
BYTE								byWorldTblidxCount;
TBLIDX								aWorldTblidx[DBO_MAX_WORLD_COUNT_IN_INFO_PACKET];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WORLD_ASSIGNMENT_INFO_END)
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_SERVERS_INFO_END)
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_NOTIFY_ILLEGAL_SERVER)
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_SERVER_INFO_CHANGED_NFY)
BYTE				byServerType;
SERVERFARMID		serverFarmId;
SERVERCHANNELID		serverChannelId;
SERVERINDEX			serverIndex;
bool				bIsOn;
BYTE				byRunningState;
DWORD				dwLoad;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_AUTH_SERVER_FARM_INFO_CHANGED_NFY)
	BYTE				byServerStatus;		// eDBO_SERVER_STATUS
	DWORD				dwMaxLoad;
	DWORD				dwLoad;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_CHARACTER_SERVER_FARM_INFO_CHANGED_NFY)
BYTE				byRunningState;
DWORD				dwMaxLoad;
DWORD				dwLoad;
SERVERINDEX			serverIndex;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_GAME_SERVER_FARM_INFO_CHANGED_NFY)
	SERVERFARMID		serverFarmId;

	BYTE				byServerStatus;		// eDBO_SERVER_STATUS
	DWORD				dwMaxLoad;
	DWORD				dwLoad;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_GAME_SERVER_CHANNEL_INFO_CHANGED_NFY)
SERVERCHANNELID		byServerChannelIndex;
SERVERFARMID		serverFarmId;
BYTE				byServerStatus;		// eDBO_SERVER_STATUS
DWORD				dwMaxLoad;
DWORD				dwLoad;
bool				bIsScrambleChannel;
_CHANNEL_BUFF		sChannelBuff;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( MG_SERVER_USER_ALL_KICK_REQ )
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_SERVER_VERSION_CHANGED_NFY) // by sooshia 07/23/2008
	WORD				wLVersion;
	WORD				wRVersion;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_SERVER_TURNOFF_FOR_DEVEL_NFY)
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_SERVER_MAX_USER_NFY)
	DWORD				dwMaxUser;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( MG_LOGIN_RES )
	WORD					wResultCode;
	ACCOUNTID				accountId;
	BYTE					abyAuthKey[NTL_MAX_SIZE_AUTH_KEY];	// ?????
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( MG_MOVE_RES )
	WORD					wResultCode;
	ACCOUNTID				accountId;
	BYTE					abyAuthKey[NTL_MAX_SIZE_AUTH_KEY];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( MG_KICKOUT_REQ )
	ACCOUNTID			accountId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( MG_PING_REQ )
	DWORD				dwTick;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( MG_REPORT_MONSTER_STATISTICS_CONTINUE_REQ )
END_PROTOCOL()
//------------------------------------------------------------------

//------------------------------------------------------------------
BEGIN_PROTOCOL( MG_PLAYER_SWITCH_CHANNEL_RES )
WORD							wResultCode;
ACCOUNTID						accountId;
BYTE							abyAuthKey[NTL_MAX_SIZE_AUTH_KEY];
sDBO_SERVER_INFO				serverInfo;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_CHAR_SERVER_TELEPORT_RES)
WORD							wResultCode;
ACCOUNTID						accountId;
BYTE							abyAuthKey[NTL_MAX_SIZE_AUTH_KEY];
sDBO_SERVER_INFO				serverInfo;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( MG_PLAYER_EXIT_TO_CHAR_RES )
HOBJECT								hHandle;
BYTE								byServerInfoCount;
sDBO_CHARACTER_SERVER_FARM_INFO		charServerInfo;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_SERVERS_INFO_ADD)
sDBO_SERVER_INFO		serverInfo;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_ONLINE_PLAYERS_REQ)
	DWORD					requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_SEND_NOTICE_REQ)
	DWORD					requestId;
	bool					bIsChannel;
	SERVERCHANNELID			serverChannelId;
	WORD					wMessageLength;
	WCHAR					awchMessage[NTL_MAX_LENGTH_OF_CHAT_MESSAGE_UNICODE + 1];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_KICK_PLAYER_REQ)
	DWORD					requestId;
	BYTE					byTargetType; // 0 = accountId, 1 = charId
	QWORD					qwTargetId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_GIVE_ITEM_REQ)
	DWORD					requestId;
	CHARACTERID				charId;
	TBLIDX					itemTblidx;
	BYTE					byCount;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_GIVE_ITEM_ALL_REQ)
	DWORD					requestId;
	TBLIDX					itemTblidx;
	BYTE					byCount;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_EXEC_GM_COMMAND_REQ)
	DWORD					requestId;
	CHARACTERID				charId;
	WORD					wCommandLen;
	WCHAR					awchCommand[NTL_MAX_LENGTH_OF_CHAT_MESSAGE_UNICODE + 1];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_APPLY_BUFF_REQ)
	DWORD					requestId;
	CHARACTERID				charId;
	TBLIDX					skillTblidx;
	DWORD					durationSeconds;
	bool					bOverrideValues;
	float					aEffectValues[NTL_MAX_EFFECT_IN_SKILL];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_REMOVE_BUFF_SKILL_REQ)
	DWORD					requestId;
	CHARACTERID				charId;
	TBLIDX					skillTblidx;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_REMOVE_BUFF_EFFECT_REQ)
	DWORD					requestId;
	CHARACTERID				charId;
	WORD					effectCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_CLEAR_BUFFS_REQ)
	DWORD					requestId;
	CHARACTERID				charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_HEAL_FULL_REQ)
	DWORD					requestId;
	CHARACTERID				charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_ADD_ZENI_REQ)
	DWORD					requestId;
	CHARACTERID				charId;
	DWORD					amount;
	bool					bAdd;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_GIVE_ITEM_CUSTOM_REQ)
	DWORD					requestId;
	CHARACTERID				charId;
	TBLIDX					itemTblidx;
	BYTE					byCount;
	BYTE					byRank;
	BYTE					byGrade;
	BYTE					byDurability;
	BYTE					byBattleAttribute;
	BYTE					byRestrictState;
	BYTE					byNeedIdentify;
	BYTE					byDurationType;
	DWORD					durationSeconds;
	bool					bUseDefaultOptions;
	bool					bEnchantAble;
	TBLIDX					aOptionTblidx[NTL_MAX_OPTION_IN_ITEM];
	sITEM_RANDOM_OPTION		aRandomOption[NTL_MAX_RANDOM_OPTION_IN_ITEM];
	WCHAR					awchMaker[NTL_MAX_SIZE_CHAR_NAME + 1];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_APPLY_BUFF_SKILL_ALL_REQ)
	DWORD					requestId;
	TBLIDX					skillTblidx;
	DWORD					durationSeconds;
	bool					bOverrideValues;
	float					aEffectValues[NTL_MAX_EFFECT_IN_SKILL];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_APPLY_BUFF_ITEM_REQ)
	DWORD					requestId;
	CHARACTERID				charId;
	TBLIDX					useItemTblidx;
	DWORD					durationSeconds;
	bool					bOverrideValues;
	float					aEffectValues[NTL_MAX_EFFECT_IN_ITEM];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_APPLY_BUFF_ITEM_ALL_REQ)
	DWORD					requestId;
	TBLIDX					useItemTblidx;
	DWORD					durationSeconds;
	bool					bOverrideValues;
	float					aEffectValues[NTL_MAX_EFFECT_IN_ITEM];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_REMOVE_BUFF_SKILL_ALL_REQ)
	DWORD					requestId;
	TBLIDX					skillTblidx;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_REMOVE_BUFF_EFFECT_ALL_REQ)
	DWORD					requestId;
	WORD					effectCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_CLEAR_BUFFS_ALL_REQ)
	DWORD					requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_SET_LEVEL_REQ)
	DWORD					requestId;
	CHARACTERID				charId;
	BYTE					byLevel;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_SET_CLASS_REQ)
	DWORD					requestId;
	CHARACTERID				charId;
	BYTE					byClass;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_KILL_PLAYER_REQ)
	DWORD					requestId;
	CHARACTERID				charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_TELEPORT_PORTAL_REQ)
	DWORD					requestId;
	CHARACTERID				charId;
	TBLIDX					portalTblidx;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_TELEPORT_WORLD_REQ)
	DWORD					requestId;
	CHARACTERID				charId;
	WORLDID					worldId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_TELEPORT_COORDS_REQ)
	DWORD					requestId;
	CHARACTERID				charId;
	WORLDID					worldId;
	float					x;
	float					y;
	float					z;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_MUTE_PLAYER_REQ)
	DWORD					requestId;
	CHARACTERID				charId;
	DWORD					durationMinutes;
	WORD					wReasonLength;
	WCHAR					awchReason[NTL_MAX_LENGTH_OF_MAIL_MESSAGE + 1];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_UNMUTE_PLAYER_REQ)
	DWORD					requestId;
	CHARACTERID				charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_TOGGLE_EXP_REQ)
	DWORD					requestId;
	CHARACTERID				charId;
	bool					bEnable;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_RESET_EXP_REQ)
	DWORD					requestId;
	CHARACTERID				charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_LEARN_SKILL_REQ)
	DWORD					requestId;
	CHARACTERID				charId;
	TBLIDX					skillTblidx;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_ADD_TITLE_REQ)
	DWORD					requestId;
	CHARACTERID				charId;
	TBLIDX					titleTblidx;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_REMOVE_TITLE_REQ)
	DWORD					requestId;
	CHARACTERID				charId;
	TBLIDX					titleTblidx;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_SET_SOLO_EXP_BONUS_REQ)
	DWORD					requestId;
	int						bonusPercent;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_SET_PARTY_EXP_BONUS_REQ)
	DWORD					requestId;
	int						bonusPercent;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_SET_QUEST_EXP_BONUS_REQ)
	DWORD					requestId;
	int						bonusPercent;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_SET_CRAFT_EXP_BONUS_REQ)
	DWORD					requestId;
	int						bonusPercent;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_SET_ZENI_DROP_BONUS_REQ)
	DWORD					requestId;
	int						bonusPercent;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_SET_QUEST_MONEY_BONUS_REQ)
	DWORD					requestId;
	int						bonusPercent;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_SET_UPGRADE_RATE_BONUS_REQ)
	DWORD					requestId;
	int						bonusPercent;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_SET_MONSTER_AGGRESSIVE_REQ)
	DWORD					requestId;
	bool					bEnable;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_SET_MONSTER_STAT_BONUS_REQ)
	DWORD					requestId;
	BYTE					byWorldRuleType; // GAMERULE_NORMAL, GAMERULE_HUNT, GAMERULE_TIMEQUEST, GAMERULE_CCBATTLEDUNGEON, etc.
	float					fMaxLpPercent;
	float					fMaxEpPercent;
	float					fPhysicalOffencePercent;
	float					fEnergyOffencePercent;
	float					fPhysicalDefencePercent;
	float					fEnergyDefencePercent;
	float					fAttackRatePercent;
	float					fDodgeRatePercent;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_APPLY_MONSTER_BUFF_SKILL_ALL_REQ)
	DWORD					requestId;
	TBLIDX					skillTblidx;
	DWORD					durationSeconds;
	bool					bOverrideValues;
	float					aEffectValues[NTL_MAX_EFFECT_IN_SKILL];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_CLEAR_MONSTER_BUFFS_ALL_REQ)
	DWORD					requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_SET_KILL_DEBUFF_REQ)
	DWORD					requestId;
	bool					bEnable;
	TBLIDX					skillTblidx;
	DWORD					durationSeconds;
	bool					bOverrideValues;
	float					aEffectValues[NTL_MAX_EFFECT_IN_SKILL];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_RESET_SKILL_COOLDOWN_REQ)
	DWORD					requestId;
	CHARACTERID				charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_RESET_SKILL_COOLDOWN_ALL_REQ)
	DWORD					requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(MG_WEB_SET_CHANNEL_STAT_BONUS_REQ)
	DWORD					requestId;
	SERVERCHANNELID			channelId;
	float					fMaxLpPercent;
	float					fMaxEpPercent;
	float					fPhysicalOffencePercent;
	float					fEnergyOffencePercent;
	float					fPhysicalDefencePercent;
	float					fEnergyDefencePercent;
	float					fAttackRatePercent;
	float					fDodgeRatePercent;
END_PROTOCOL()
//------------------------------------------------------------------
#pragma pack()