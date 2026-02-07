#pragma once

#include "NtlPacketCommon.h"
#include "NtlItem.h"
#include "NtlMail.h"

enum eOPCODE_WM
{
	WM_OPCODE_BEGIN = 17000,

	WM_GET_PLAYER_COUNT = WM_OPCODE_BEGIN,
	WM_GET_CHANNEL_STATUS,
	WM_SEND_CHAR_ITEM,
	WM_GET_ONLINE_PLAYERS,
	WM_SEND_NOTICE,
	WM_KICK_PLAYER,
	WM_GIVE_ITEM,
	WM_GIVE_ITEM_ALL,
	WM_EXEC_GM_COMMAND,
	WM_APPLY_BUFF,
	WM_REMOVE_BUFF_SKILL,
	WM_REMOVE_BUFF_EFFECT,
	WM_CLEAR_BUFFS,
	WM_HEAL_FULL,
	WM_ADD_ZENI,
	WM_GIVE_ITEM_CUSTOM,
	WM_GET_MASTER_UPTIME,
	WM_GET_MASTER_TIME,
	WM_GET_MASTER_CONFIG,
	WM_GET_SERVER_COUNTS,
	WM_GET_SESSION_COUNTS,
	WM_GET_PLAYER_COUNTS,
	WM_GET_GAME_FARM_LIST,
	WM_GET_GAME_CHANNEL_LIST,
	WM_GET_AUTH_SERVER_LIST,
	WM_GET_CHAR_SERVER_LIST,
	WM_GET_CHAT_SERVER_LIST,
	WM_GET_GAME_SERVER_LIST,
	WM_GET_QUERY_SERVER_LIST,
	WM_GET_SERVER_LIST_ALL,
	WM_GET_ONLINE_COUNT_BY_CHANNEL,
	WM_GET_ONLINE_COUNT_BY_FARM,
	WM_GET_ONLINE_ACCOUNT_LIST,
	WM_GET_WEB_REQUEST_STATS,
	WM_GET_SERVER_LOAD_SUMMARY,
	WM_GET_CHANNEL_VISIBILITY_LIST,
	WM_GET_SCRAMBLE_CHANNEL_LIST,
	WM_APPLY_BUFF_SKILL,
	WM_APPLY_BUFF_SKILL_ALL,
	WM_APPLY_BUFF_ITEM,
	WM_APPLY_BUFF_ITEM_ALL,
	WM_REMOVE_BUFF_SKILL_ALL,
	WM_REMOVE_BUFF_EFFECT_ALL,
	WM_CLEAR_BUFFS_ALL,
	WM_SET_LEVEL,
	WM_SET_CLASS,
	WM_KILL_PLAYER,
	WM_TELEPORT_PORTAL,
	WM_TELEPORT_WORLD,
	WM_TELEPORT_COORDS,
	WM_MUTE_PLAYER,
	WM_UNMUTE_PLAYER,
	WM_TOGGLE_EXP,
	WM_RESET_EXP,
	WM_LEARN_SKILL,
	WM_ADD_TITLE,
	WM_REMOVE_TITLE,
	WM_SET_SOLO_EXP_BONUS,
	WM_SET_PARTY_EXP_BONUS,
	WM_SET_QUEST_EXP_BONUS,
	WM_SET_CRAFT_EXP_BONUS,
	WM_SET_ZENI_DROP_BONUS,
	WM_SET_QUEST_MONEY_BONUS,
	WM_SET_UPGRADE_RATE_BONUS,
	WM_SET_MONSTER_AGGRESSIVE,
	WM_SET_MONSTER_STAT_BONUS,
	WM_APPLY_MONSTER_BUFF_SKILL_ALL,
	WM_CLEAR_MONSTER_BUFFS_ALL,
	WM_SET_KILL_DEBUFF,
	WM_RESET_SKILL_COOLDOWN,
	WM_RESET_SKILL_COOLDOWN_ALL,
	WM_SET_CHANNEL_STAT_BONUS,
	WM_GET_BONUS_STATE,

	WM_OPCODE_DUMMY,
	WM_OPCODE_END = WM_OPCODE_DUMMY - 1
};

enum eWEB_ADMIN_ACTION
{
	WEB_ACTION_SEND_NOTICE = 1,
	WEB_ACTION_KICK_PLAYER = 2,
	WEB_ACTION_GIVE_ITEM = 3,
	WEB_ACTION_GIVE_ITEM_ALL = 4,
	WEB_ACTION_EXEC_GM_COMMAND = 5,
	WEB_ACTION_APPLY_BUFF = 6,
	WEB_ACTION_REMOVE_BUFF_SKILL = 7,
	WEB_ACTION_REMOVE_BUFF_EFFECT = 8,
	WEB_ACTION_CLEAR_BUFFS = 9,
	WEB_ACTION_HEAL_FULL = 10,
	WEB_ACTION_ADD_ZENI = 11,
	WEB_ACTION_GIVE_ITEM_CUSTOM = 12,
	WEB_ACTION_APPLY_BUFF_SKILL_ALL = 13,
	WEB_ACTION_APPLY_BUFF_ITEM = 14,
	WEB_ACTION_APPLY_BUFF_ITEM_ALL = 15,
	WEB_ACTION_REMOVE_BUFF_SKILL_ALL = 16,
	WEB_ACTION_REMOVE_BUFF_EFFECT_ALL = 17,
	WEB_ACTION_CLEAR_BUFFS_ALL = 18,
	WEB_ACTION_SET_LEVEL = 19,
	WEB_ACTION_SET_CLASS = 20,
	WEB_ACTION_KILL_PLAYER = 21,
	WEB_ACTION_TELEPORT_PORTAL = 22,
	WEB_ACTION_TELEPORT_WORLD = 23,
	WEB_ACTION_TELEPORT_COORDS = 24,
	WEB_ACTION_MUTE_PLAYER = 25,
	WEB_ACTION_UNMUTE_PLAYER = 26,
	WEB_ACTION_TOGGLE_EXP = 27,
	WEB_ACTION_RESET_EXP = 28,
	WEB_ACTION_LEARN_SKILL = 29,
	WEB_ACTION_ADD_TITLE = 30,
	WEB_ACTION_REMOVE_TITLE = 31,
	WEB_ACTION_SET_SOLO_EXP_BONUS = 32,
	WEB_ACTION_SET_PARTY_EXP_BONUS = 33,
	WEB_ACTION_SET_QUEST_EXP_BONUS = 34,
	WEB_ACTION_SET_CRAFT_EXP_BONUS = 35,
	WEB_ACTION_SET_ZENI_DROP_BONUS = 36,
	WEB_ACTION_SET_QUEST_MONEY_BONUS = 37,
	WEB_ACTION_SET_UPGRADE_RATE_BONUS = 38,
	WEB_ACTION_SET_MONSTER_AGGRESSIVE = 39,
	WEB_ACTION_SET_MONSTER_STAT_BONUS = 40,
	WEB_ACTION_APPLY_MONSTER_BUFF_SKILL_ALL = 41,
	WEB_ACTION_CLEAR_MONSTER_BUFFS_ALL = 42,
	WEB_ACTION_SET_KILL_DEBUFF = 43,
	WEB_ACTION_RESET_SKILL_COOLDOWN = 44,
	WEB_ACTION_RESET_SKILL_COOLDOWN_ALL = 45,
	WEB_ACTION_SET_CHANNEL_STAT_BONUS = 46
};

#pragma pack(1)

//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_SEND_CHAR_ITEM)
	CHARACTERID		characterID;
	TBLIDX			item;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_ONLINE_PLAYERS)
	DWORD			requestId;
	DWORD			page;
	DWORD			pageSize;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_SEND_NOTICE)
	DWORD			requestId;
	bool			bIsChannel;
	SERVERCHANNELID	serverChannelId;
	WORD			wMessageLength;
	WCHAR			awchMessage[NTL_MAX_LENGTH_OF_CHAT_MESSAGE_UNICODE + 1];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_KICK_PLAYER)
	DWORD			requestId;
	BYTE			byTargetType; // 0 = accountId, 1 = charId
	QWORD			qwTargetId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GIVE_ITEM)
	DWORD			requestId;
	CHARACTERID		charId;
	TBLIDX			itemTblidx;
	BYTE			byCount;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GIVE_ITEM_ALL)
	DWORD			requestId;
	TBLIDX			itemTblidx;
	BYTE			byCount;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_EXEC_GM_COMMAND)
	DWORD			requestId;
	CHARACTERID		charId;
	WORD			wCommandLen;
	WCHAR			awchCommand[NTL_MAX_LENGTH_OF_CHAT_MESSAGE_UNICODE + 1];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_APPLY_BUFF)
	DWORD			requestId;
	CHARACTERID		charId;
	TBLIDX			skillTblidx;
	DWORD			durationSeconds;
	bool			bOverrideValues;
	float			aEffectValues[NTL_MAX_EFFECT_IN_SKILL];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_REMOVE_BUFF_SKILL)
	DWORD			requestId;
	CHARACTERID		charId;
	TBLIDX			skillTblidx;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_REMOVE_BUFF_EFFECT)
	DWORD			requestId;
	CHARACTERID		charId;
	WORD			effectCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_CLEAR_BUFFS)
	DWORD			requestId;
	CHARACTERID		charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_HEAL_FULL)
	DWORD			requestId;
	CHARACTERID		charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_ADD_ZENI)
	DWORD			requestId;
	CHARACTERID		charId;
	DWORD			amount;
	bool			bAdd;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GIVE_ITEM_CUSTOM)
	DWORD			requestId;
	CHARACTERID		charId;
	TBLIDX			itemTblidx;
	BYTE			byCount;
	BYTE			byRank;
	BYTE			byGrade;
	BYTE			byDurability;
	BYTE			byBattleAttribute;
	BYTE			byRestrictState;
	BYTE			byNeedIdentify;
	BYTE			byDurationType;
	DWORD			durationSeconds;
	bool			bUseDefaultOptions;
	bool			bEnchantAble;
	TBLIDX			aOptionTblidx[NTL_MAX_OPTION_IN_ITEM];
	sITEM_RANDOM_OPTION aRandomOption[NTL_MAX_RANDOM_OPTION_IN_ITEM];
	WCHAR			awchMaker[NTL_MAX_SIZE_CHAR_NAME + 1];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_APPLY_BUFF_SKILL)
	DWORD			requestId;
	CHARACTERID		charId;
	TBLIDX			skillTblidx;
	DWORD			durationSeconds;
	bool			bOverrideValues;
	float			aEffectValues[NTL_MAX_EFFECT_IN_SKILL];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_APPLY_BUFF_SKILL_ALL)
	DWORD			requestId;
	TBLIDX			skillTblidx;
	DWORD			durationSeconds;
	bool			bOverrideValues;
	float			aEffectValues[NTL_MAX_EFFECT_IN_SKILL];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_APPLY_BUFF_ITEM)
	DWORD			requestId;
	CHARACTERID		charId;
	TBLIDX			useItemTblidx;
	DWORD			durationSeconds;
	bool			bOverrideValues;
	float			aEffectValues[NTL_MAX_EFFECT_IN_ITEM];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_APPLY_BUFF_ITEM_ALL)
	DWORD			requestId;
	TBLIDX			useItemTblidx;
	DWORD			durationSeconds;
	bool			bOverrideValues;
	float			aEffectValues[NTL_MAX_EFFECT_IN_ITEM];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_REMOVE_BUFF_SKILL_ALL)
	DWORD			requestId;
	TBLIDX			skillTblidx;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_REMOVE_BUFF_EFFECT_ALL)
	DWORD			requestId;
	WORD			effectCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_CLEAR_BUFFS_ALL)
	DWORD			requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_SET_LEVEL)
	DWORD			requestId;
	CHARACTERID		charId;
	BYTE			byLevel;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_SET_CLASS)
	DWORD			requestId;
	CHARACTERID		charId;
	BYTE			byClass;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_KILL_PLAYER)
	DWORD			requestId;
	CHARACTERID		charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_TELEPORT_PORTAL)
	DWORD			requestId;
	CHARACTERID		charId;
	TBLIDX			portalTblidx;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_TELEPORT_WORLD)
	DWORD			requestId;
	CHARACTERID		charId;
	WORLDID			worldId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_TELEPORT_COORDS)
	DWORD			requestId;
	CHARACTERID		charId;
	WORLDID			worldId;
	float			x;
	float			y;
	float			z;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_MUTE_PLAYER)
	DWORD			requestId;
	CHARACTERID		charId;
	DWORD			durationMinutes;
	WORD			wReasonLength;
	WCHAR			awchReason[NTL_MAX_LENGTH_OF_MAIL_MESSAGE + 1];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_UNMUTE_PLAYER)
	DWORD			requestId;
	CHARACTERID		charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_TOGGLE_EXP)
	DWORD			requestId;
	CHARACTERID		charId;
	bool			bEnable;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_RESET_EXP)
	DWORD			requestId;
	CHARACTERID		charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_LEARN_SKILL)
	DWORD			requestId;
	CHARACTERID		charId;
	TBLIDX			skillTblidx;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_ADD_TITLE)
	DWORD			requestId;
	CHARACTERID		charId;
	TBLIDX			titleTblidx;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_REMOVE_TITLE)
	DWORD			requestId;
	CHARACTERID		charId;
	TBLIDX			titleTblidx;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_SET_SOLO_EXP_BONUS)
	DWORD			requestId;
	int				bonusPercent;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_SET_PARTY_EXP_BONUS)
	DWORD			requestId;
	int				bonusPercent;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_SET_QUEST_EXP_BONUS)
	DWORD			requestId;
	int				bonusPercent;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_SET_CRAFT_EXP_BONUS)
	DWORD			requestId;
	int				bonusPercent;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_SET_ZENI_DROP_BONUS)
	DWORD			requestId;
	int				bonusPercent;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_SET_QUEST_MONEY_BONUS)
	DWORD			requestId;
	int				bonusPercent;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_SET_UPGRADE_RATE_BONUS)
	DWORD			requestId;
	int				bonusPercent;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_SET_MONSTER_AGGRESSIVE)
	DWORD			requestId;
	bool			bEnable;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_SET_MONSTER_STAT_BONUS)
	DWORD			requestId;
	BYTE			byWorldRuleType; // GAMERULE_NORMAL, GAMERULE_HUNT, GAMERULE_TIMEQUEST, GAMERULE_CCBATTLEDUNGEON, etc.
	float			fMaxLpPercent;
	float			fMaxEpPercent;
	float			fPhysicalOffencePercent;
	float			fEnergyOffencePercent;
	float			fPhysicalDefencePercent;
	float			fEnergyDefencePercent;
	float			fAttackRatePercent;
	float			fDodgeRatePercent;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_APPLY_MONSTER_BUFF_SKILL_ALL)
	DWORD			requestId;
	TBLIDX			skillTblidx;
	DWORD			durationSeconds;
	bool			bOverrideValues;
	float			aEffectValues[NTL_MAX_EFFECT_IN_SKILL];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_CLEAR_MONSTER_BUFFS_ALL)
	DWORD			requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_SET_KILL_DEBUFF)
	DWORD			requestId;
	bool			bEnable;
	TBLIDX			skillTblidx;
	DWORD			durationSeconds;
	bool			bOverrideValues;
	float			aEffectValues[NTL_MAX_EFFECT_IN_SKILL];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_RESET_SKILL_COOLDOWN)
	DWORD			requestId;
	CHARACTERID		charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_RESET_SKILL_COOLDOWN_ALL)
	DWORD			requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_SET_CHANNEL_STAT_BONUS)
	DWORD			requestId;
	SERVERCHANNELID	channelId;
	float			fMaxLpPercent;
	float			fMaxEpPercent;
	float			fPhysicalOffencePercent;
	float			fEnergyOffencePercent;
	float			fPhysicalDefencePercent;
	float			fEnergyDefencePercent;
	float			fAttackRatePercent;
	float			fDodgeRatePercent;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_BONUS_STATE)
	DWORD			requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_MASTER_UPTIME)
	DWORD			requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_MASTER_TIME)
	DWORD			requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_MASTER_CONFIG)
	DWORD			requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_SERVER_COUNTS)
	DWORD			requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_SESSION_COUNTS)
	DWORD			requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_PLAYER_COUNTS)
	DWORD			requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_GAME_FARM_LIST)
	DWORD			requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_GAME_CHANNEL_LIST)
	DWORD			requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_AUTH_SERVER_LIST)
	DWORD			requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_CHAR_SERVER_LIST)
	DWORD			requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_CHAT_SERVER_LIST)
	DWORD			requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_GAME_SERVER_LIST)
	DWORD			requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_QUERY_SERVER_LIST)
	DWORD			requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_SERVER_LIST_ALL)
	DWORD			requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_ONLINE_COUNT_BY_CHANNEL)
	DWORD			requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_ONLINE_COUNT_BY_FARM)
	DWORD			requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_ONLINE_ACCOUNT_LIST)
	DWORD			requestId;
	DWORD			page;
	DWORD			pageSize;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_WEB_REQUEST_STATS)
	DWORD			requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_SERVER_LOAD_SUMMARY)
	DWORD			requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_CHANNEL_VISIBILITY_LIST)
	DWORD			requestId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(WM_GET_SCRAMBLE_CHANNEL_LIST)
	DWORD			requestId;
END_PROTOCOL()
//------------------------------------------------------------------
#pragma pack()