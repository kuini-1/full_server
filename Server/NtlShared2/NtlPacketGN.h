//***********************************************************************************
//
//	File		:	NtlPacketGN.h
//
//	Begin		:	2006-05-12
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Hyun Woo, Koo   ( zeroera@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************

#pragma once

#include "NtlPacketCommon.h"

#include "NtlPacketGU.h"


//------------------------------------------------------------------
//	Protocol enumeration
//------------------------------------------------------------------
enum eOPCODE_GN
{
	GN_OPCODE_BEGIN = 36000,

	GN_GAME_DATA_RES = GN_OPCODE_BEGIN, // ????????? ???? ????? ????
	GN_CHAR_SPAWN_REQ, // ??????? ???? ??û : ex) ???? ?????? ????????? ????? ????? ???? ??û

	GN_WORLD_CREATED_NFY, // ???? ???? ???
	GN_WORLD_DESTROYED_NFY, // ???? ???? ???
	GN_OBJECT_CREATED_NFY, // ??????? ???? ???
	GN_OBJECT_DESTROYED_NFY, // ??????? ???? ???
	GN_OBJECT_WORLD_ENTERED, // ??????? ???? ???? ???
	GN_OBJECT_WORLD_LEAVED, // ??????? ???? ???? ???

	GN_PREPARE_ENTER_GAME,	// ??????? ?????? ??û???? ????
	GN_ENTER_GAME_RES, // ??????? ???? ????
	GN_LEAVE_GAME_RES, // ???? ???? ??û
	GN_ENTER_WORLD_RES, // ??????? ???? ???? ????
	GN_LEAVE_WORLD_RES, // ??????? ???? ???? ??û

	GN_CHAR_MOVE, // ???
	GN_CHAR_DEST_MOVE, // ?????????
	GN_CHAR_CHANGE_HEADING, // ???? ???
	GN_CHAR_FOLLOW_MOVE, // ??????
	GN_CHAR_JUMP, // ????
	GN_CHAR_JUMP_END, // ???? ??
	GN_CHAR_FIGHTMODE, // ??????
	GN_CHAR_MOVE_SYNC, // ??? ????? ( PUSH?? ???? ??? ??? ???? ??? )
	GN_CHAR_CHANGE_DIRECTION_ON_FLOATING, // ????? ????/falling ???? ????? ????

	GN_CHAR_ACTION_ATTACK, // ??? ???
	GN_CHAR_ACTION_SKILL, // ??? ???
	GN_CHAR_ACTION_ITEM, // ?????? ??? ???
	GN_CHAR_ACTION_HTB, // HTB
	GN_CHAR_TARGET_SELECT, // ??? ???? ( Game -> NPC )
	GN_CHAR_TARGET_FACING, // ??? ???? ( Game -> NPC )
	GN_CHAR_TARGET_LOST_NFY,
	GN_CHAR_KEEP_AGGRO, // ????
	GN_CHAR_CHANGE_AGGRO, // Aggro ???? ????
	GN_CHAR_ERASE_AGGRO_AROUND_ME, // ??? ?????? NPC/Mob???? ????? aggro ?????? ????
	GN_CHAR_START_ESCORT_REQ, // ??????? ????
	GN_CHAR_STOP_ESCORT_NFY, // ??????? ???? ( ????? ???? )

	GN_BUFF_REGISTERED, // ???? ????
	GN_BUFF_DROPPED, // ???? ???
	GN_EFFECT_AFFECTED, // DoT, HoT ??

	GN_UPDATE_CHAR_STATE, // ??????? ???? ???????
	GN_UPDATE_CHAR_ASPECT_STATE, // ??????? ??? ???? ???????
	GN_UPDATE_CHAR_CONDITION, // ??????? 
	GN_UPDATE_CHAR_LEVEL, // ??????? ???? ???????
	GN_UPDATE_CHAR_LP, // LP ???????
	GN_UPDATE_CHAR_EP, // EP ???????
	GN_UPDATE_CHAR_LP_EP, // LP/EP ???????
	GN_UPDATE_CHAR_SPEED, // ????? ???????
	GN_UPDATE_CHAR_SCALE, // ??????? scale ???????

	GN_SERVER_COMMAND, // ???? ????
	GN_TS_SERVER_EVENT, // TS ???? ????
	GN_TQS_SERVER_EVENT,

	GN_BOT_SKILL_RES, // ??? ??? [2/12/2008 SGpro]

	GN_SKILL_TARGET_LIST_REQ,
	GN_SKILL_COOL_TIME_STARTED_NFY,			// ????? cool time?? ?????????? ???
	GN_SKILL_CONFUSE_TARGET_NFY,		// Confuse ?????? ???? ???? ??? ???
	GN_SKILL_TERROR_COURSE_CHANGE_NFY,		// Terror ?????? ?? ???? ??? ???

	GN_CHAR_TELEPORT_RES, // [8/4/2008 SGpro]

	GN_WPS_SERVER_EVENT,		// deprecate
	GN_SEND_TQS_EVENT,
	GN_SEND_WPS_EVENT,

	GN_MONSTER_TRANSFORMED_NFY,

	GN_SCRIPT_ITEM_USE,				// ????? ???? ???????? ?????

	GN_CHANGE_SPS_SCENE_REQ,

	GN_WORLD_ADD_BOT_NFY,

	GN_PET_ATTACK_TARGET_NFY,		// ???? ???? ????

	GN_CHAR_TARGET_CHANGED_NFY,		// ??? ???? ???

	GN_UPDATE_CHAR_MAX_LP,
	GN_UPDATE_CHAR_MAX_EP,

	GN_OPCODE_DUMMY,
	GN_OPCODE_END = GN_OPCODE_DUMMY - 1
};


//------------------------------------------------------------------
//
//------------------------------------------------------------------
const char * NtlGetPacketName_GN(WORD wOpCode);
//------------------------------------------------------------------


//------------------------------------------------------------------
//	Define protocol structure
//------------------------------------------------------------------
#pragma pack(1)

//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_GAME_DATA_RES )
	WORD			wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_CHAR_SPAWN_REQ )
	BYTE			bySpawnType; // mob, npc ?
	TBLIDX			tblidx;
	WORLDID			worldID;
	sVECTOR3		vLoc;
	sVECTOR3		vDir;
	BYTE			byMoveType;
	TBLIDX			pathTblidx;
	float			fPathPointRange;
	float			fMoveRange;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_WORLD_CREATED_NFY )
	sWORLD_INFO		sWorldInfo;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_WORLD_DESTROYED_NFY )
	WORLDID			worldID;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_OBJECT_CREATED_NFY )
	HOBJECT			handle;
	WORLDID			worldID;
	sOBJECT_INFO	sObjectInfo;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_OBJECT_DESTROYED_NFY )
	HOBJECT			handle;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_OBJECT_WORLD_ENTERED )
	HOBJECT			handle;
	WORLDID			worldID;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_OBJECT_WORLD_LEAVED )
	HOBJECT			handle;
	WORLDID			worldID;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_PREPARE_ENTER_GAME )
	BYTE				objType; // eOBJTYPE
	union
	{
		sNPC_DATA			sNpcData; // npc
		sMOB_DATA			sMobData; // mob
		sPET_DATA			sPetData;
	};
END_PROTOCOL()
//------------------------------------------------------------------
/* Named union with default ctor so MSVC can default-construct; Linux allows non-trivial members in named unions. */
union sGN_ENTER_GAME_RES_data
{
	struct { sNPC_PROFILE sNpcProfile; sCHARSTATE sNpcState; } npc;
	struct { sMOB_PROFILE sMobProfile; sCHARSTATE sMobState; } mob;
	struct { sPET_PROFILE sPetProfile; sCHARSTATE sPetState; } pet;
	sGN_ENTER_GAME_RES_data() { new (&npc) decltype(npc)(); }
	~sGN_ENTER_GAME_RES_data() { npc.~decltype(npc)(); }
};
BEGIN_PROTOCOL( GN_ENTER_GAME_RES )
	HOBJECT			handle;
	BYTE			objType;
	sGN_ENTER_GAME_RES_data	data;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_LEAVE_GAME_RES )
	HOBJECT			handle;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_ENTER_WORLD_RES )
	HOBJECT				handle;
	WORLDID				worldID;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_LEAVE_WORLD_RES )

END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL_IDENTITY( GN_CHAR_MOVE, GU_CHAR_MOVE )

END_PROTOCOL_IDENTITY()
//------------------------------------------------------------------
BEGIN_PROTOCOL_IDENTITY( GN_CHAR_DEST_MOVE, GU_CHAR_DEST_MOVE )

END_PROTOCOL_IDENTITY()
//------------------------------------------------------------------
BEGIN_PROTOCOL_IDENTITY( GN_CHAR_CHANGE_HEADING, GU_CHAR_CHANGE_HEADING )

END_PROTOCOL_IDENTITY()
//------------------------------------------------------------------
BEGIN_PROTOCOL_IDENTITY( GN_CHAR_FOLLOW_MOVE, GU_CHAR_FOLLOW_MOVE )

END_PROTOCOL_IDENTITY()
//------------------------------------------------------------------
BEGIN_PROTOCOL_IDENTITY( GN_CHAR_JUMP, GU_CHAR_JUMP )

END_PROTOCOL_IDENTITY()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GN_CHAR_JUMP_END)
	HOBJECT				handle;
	BYTE				byMoveDirection;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GN_CHAR_MOVE_SYNC)
	HOBJECT				handle;
	sVECTOR3			vCurLoc;
	sVECTOR3			vCurDir;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL_IDENTITY( GN_CHAR_CHANGE_DIRECTION_ON_FLOATING, GU_CHAR_CHANGE_DIRECTION_ON_FLOATING )

END_PROTOCOL_IDENTITY()
//------------------------------------------------------------------
BEGIN_PROTOCOL_IDENTITY( GN_CHAR_FIGHTMODE, GU_CHAR_FIGHTMODE )

END_PROTOCOL_IDENTITY()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_CHAR_ACTION_ATTACK )
	HOBJECT			handle;
	HOBJECT			hSubject;
	HOBJECT			hTarget;
	BYTE			byAttackResult;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_CHAR_ACTION_SKILL )
	HOBJECT				handle;
	WORD				wResultCode;
	DWORD				dwLpEpEventId;
	TBLIDX				skillId;
	BYTE				byRpBonusType;		// eDBO_RP_BONUS_TYPE
	HOBJECT				hAppointedTarget;
	bool				bIsSkillHarmful;
	BYTE				bySkillResultCount;
	sSKILL_RESULT		aSkillResult[NTL_MAX_NUMBER_OF_SKILL_TARGET];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_CHAR_ACTION_ITEM )
	HOBJECT				handle;
	WORD				wResultCode;
	TBLIDX				itemTblidx;
	BYTE				bySkillResultCount;
	sSKILL_RESULT		aSkillResult[NTL_MAX_NUMBER_OF_SKILL_TARGET];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_CHAR_TARGET_SELECT )
	HOBJECT				handle;
	HOBJECT				hTarget;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_CHAR_TARGET_FACING )
	HOBJECT				handle;
	HOBJECT				hTarget;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL_IDENTITY( GN_CHAR_TARGET_LOST_NFY, GU_CHAR_TARGET_LOST_NFY )

END_PROTOCOL_IDENTITY()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_CHAR_ACTION_HTB )
	HOBJECT				hSubject;
	HOBJECT				hTarget;
	WORD				wResultValue;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_CHAR_KEEP_AGGRO )
	HOBJECT				hSubject;
	HOBJECT				hProvoker;
	WORD				wAggroValue;
	eSYSTEM_EFFECT_CODE effectCode; // ??? [4/1/2008 SGpro]
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_CHAR_CHANGE_AGGRO )
	HOBJECT				hSubject;
	HOBJECT				hProvoker;
	BYTE				byAggroChangeType;		// eDBO_AGGRO_CHANGE_TYPE
	WORD				wValue;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_CHAR_ERASE_AGGRO_AROUND_ME )
	HOBJECT				hSubject;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_CHAR_START_ESCORT_REQ )
	HOBJECT				hTarget;
	HOBJECT				hSource;
	BYTE				byEscortType;
	bool				bShared;
	PARTYID				partyID;
	BYTE				byTriggerType;
	NTL_TS_T_ID			tid;
	sTSM_SERIAL			sTSMSerial;		// TSM?? ??????? ???? ???
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_CHAR_STOP_ESCORT_NFY )
	HOBJECT				hTarget;
	HOBJECT				hSource;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_EFFECT_AFFECTED )
	HOBJECT				handle;
	HOBJECT				hCaster;
	WORD				wEffectCode;
	WORD				wResultCode;
	float				fActualArgument1;
	float				fActualArgument2;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_BUFF_REGISTERED )
	HOBJECT				hHandle;
	HOBJECT				hCaster;
	BYTE				bySourceType;
	TBLIDX				tblidx;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL_IDENTITY( GN_BUFF_DROPPED, GU_BUFF_DROPPED )

END_PROTOCOL_IDENTITY()
//------------------------------------------------------------------
BEGIN_PROTOCOL_IDENTITY( GN_UPDATE_CHAR_STATE, GU_UPDATE_CHAR_STATE )

END_PROTOCOL_IDENTITY()
//------------------------------------------------------------------
BEGIN_PROTOCOL_IDENTITY( GN_UPDATE_CHAR_ASPECT_STATE, GU_UPDATE_CHAR_ASPECT_STATE )

END_PROTOCOL_IDENTITY()
//------------------------------------------------------------------
BEGIN_PROTOCOL_IDENTITY( GN_UPDATE_CHAR_CONDITION, GU_UPDATE_CHAR_CONDITION )

END_PROTOCOL_IDENTITY()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GN_UPDATE_CHAR_LEVEL)
	HOBJECT			hSubject;
	BYTE			byLevel;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GN_UPDATE_CHAR_LP)
	HOBJECT			handle;
	WORD			wCurLP;
	WORD			wMaxLP;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GN_UPDATE_CHAR_EP)
	HOBJECT			handle;
	WORD			wCurEP;
	WORD			wMaxEP;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GN_UPDATE_CHAR_LP_EP)
	HOBJECT			hSubject;
	WORD			wCurLp;
	WORD			wMaxLp;
	WORD			wCurEp;
	WORD			wMaxEp;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL_IDENTITY( GN_UPDATE_CHAR_SPEED, GU_UPDATE_CHAR_SPEED )

END_PROTOCOL_IDENTITY()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GN_UPDATE_CHAR_SCALE)
	HOBJECT			hSubject;
	float			fScale;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GN_SERVER_COMMAND)
	HOBJECT			hSubject;
	WORD			wCommandLen;
	CHAR			achCommand[NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GN_TS_SERVER_EVENT)
	HOBJECT			hSubject;
	HOBJECT			hTarget;
	BYTE			byTriggerType;
	NTL_TS_EVENT_ID	teid;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GN_TQS_SERVER_EVENT)
	HOBJECT			hTarget;
	DWORD			eventID;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GN_BOT_SKILL_RES)
	HOBJECT			handle;
	WORD			wResultCode;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GN_SKILL_TARGET_LIST_REQ)
	HOBJECT			hSubject;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL_IDENTITY( GN_SKILL_COOL_TIME_STARTED_NFY, GU_SKILL_COOL_TIME_STARTED_NFY )

END_PROTOCOL_IDENTITY()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GN_SKILL_CONFUSE_TARGET_NFY)
	HOBJECT			hSubject;
	HOBJECT			hTarget;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GN_SKILL_TERROR_COURSE_CHANGE_NFY)
	HOBJECT			hSubject;
	HOBJECT			hAttacker;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_CHAR_TELEPORT_RES ) // [8/4/2008 SGpro]
	HOBJECT			handle;
	WORD			wResultCode;
	sVECTOR3		vNewLoc;
	sVECTOR3		vNewDir;
	// NPC?? Bot?? ??? World?? ??????? ??´?.
	// ????? sWORLD_INFO?? Game->Npc?? ?????? ??´?[8/5/2008 SGpro]
	// sWORLD_INFO				sWorldInfo;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GN_WPS_SERVER_EVENT)
	BYTE			byEvtType;			// eWPS_EVENT_TYPE
	HOBJECT			hTarget;			// ???? ????
	BYTE			byObjType;			// eOBJTYPE
	TBLIDX			targetTblidx;
	DWORD			eventID;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GN_SEND_TQS_EVENT)
	BYTE			byEventType;		// eSSD_SCRIPT_TYPE : SSD_SCRIPT_TYPE_SPS | SSD_SCRIPT_TYPE_AIS ?? ???
	WORLDID			worldId;
	HOBJECT			hTarget;
	BYTE			byTargetType;
	TBLIDX			targetTblidx;
	DWORD			eventId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GN_SEND_WPS_EVENT)
	BYTE			byEventType;	// eSSD_SCRIPT_TYPE : SSD_SCRIPT_TYPE_SPS | SSD_SCRIPT_TYPE_AIS ?? ???
	HOBJECT			hTarget;
	DWORD			eventId;

	// ??? ???? : ??? ???? ???
	WORLDID			worldId;
	TBLIDX			wpsTblidx;
	BYTE			byTargetType;
	TBLIDX			targetTblidx;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GN_MONSTER_TRANSFORMED_NFY)
	HOBJECT			hSubject;
	TBLIDX			newTblidx;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GN_SCRIPT_ITEM_USE)
	HOBJECT				hTarget;			// target object
	HOBJECT				hPc;
	TBLIDX				itemTblidx;
	BYTE				byScriptType;		// eSSD_SCRIPT_TYPE : sps or sps
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_CHANGE_SPS_SCENE_REQ )
	HOBJECT				hTarget;
	TBLIDX				targetTblidx;
	DWORD				dwPlayScript;
	DWORD				dwPlayScene;
	WORLDID				worldId;
	TBLIDX				wpsTblidx;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_WORLD_ADD_BOT_NFY )
	WORLDID			worldID;
	BYTE			byObjectType;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_PET_ATTACK_TARGET_NFY )
	HOBJECT				hSubject;
	HOBJECT				hTarget;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GN_CHAR_TARGET_CHANGED_NFY )
	HOBJECT				hSubject;
	HOBJECT				hTarget;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GN_UPDATE_CHAR_MAX_LP)
	HOBJECT				hSubject;
	WORD				wMaxLp;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GN_UPDATE_CHAR_MAX_EP)
	HOBJECT				hSubject;
	WORD				wMaxEp;
END_PROTOCOL()
//------------------------------------------------------------------

#pragma pack()