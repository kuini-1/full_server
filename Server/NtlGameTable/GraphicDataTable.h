//***********************************************************************************
//
//	File		:	GraphicDataTable.h
//
//	Begin		:	2006-05-29
//
//	Copyright	:	ⓒ NTL-Inc Co., Ltd
//
//	Author		:	Hong Ho Dong   ( battery@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************
#pragma once
#include "NtlVector.h"

// Shared
enum EGraphicAnimCommon
{
	ATTACK_ANIMATION_SET_NUM	= 6,

	ATTACK_ANIMATION_START		= 1000,
	ATTACK_ANIMATION_DEFAULT    = ATTACK_ANIMATION_START,
	ATTACK_ANIMATION_GLOVE		= ATTACK_ANIMATION_DEFAULT + ATTACK_ANIMATION_SET_NUM,
	ATTACK_ANIMATION_GUN		= ATTACK_ANIMATION_GLOVE + ATTACK_ANIMATION_SET_NUM,
	ATTACK_ANIMATION_STAFF		= ATTACK_ANIMATION_GUN + ATTACK_ANIMATION_SET_NUM,
	ATTACK_ANIMATION_DUAL_GUN	= ATTACK_ANIMATION_STAFF + ATTACK_ANIMATION_SET_NUM,
	ATTACK_ANIMATION_END        = ATTACK_ANIMATION_DUAL_GUN + ATTACK_ANIMATION_SET_NUM,

	SKILL_ANIMATION_START       = 2000,
	SKILL_ANIMATION_END			= SKILL_ANIMATION_START + 200,

    HTB_ANIMATION_START         = 3000,
    HTB_ANIMATION_END           = HTB_ANIMATION_START + 50,
};

// Social
enum ESocialAnimationList
{
    SOCIAL_ANIMATION_START = 1500,
	SOC_AGREE = SOCIAL_ANIMATION_START,
    SOC_NO,
    SOC_BYE,
    SOC_HAPPY,
    SOC_LAUGH,
    SOC_SAD,
    SOC_CLAP_HANDS,
    SOC_POINT,
    SOC_RUSH,
    SOC_YEAH,
    SOC_AMUSE,
    SOC_HI,
    SOC_PAFUPAFU,
    SOC_COURTESY,
    SOC_POKE,
	SOC_CONFUSION,
	SOCIAL_ANIMATION_END = SOC_CONFUSION,
};

//Battle
enum EBattleAnimationList
{
	BATTLE_ANIMATION_START = 0x1F4,
	BTL_DEF_FP_LOOP = 0x1F4,
	BTL_DEF_FP_HURT = 0x1F5,
	BTL_DEF_FP_DOT_HURT = 0x1F6,
	BTL_DEF_FP_IDLE = 0x1F7,
	BTL_DEF_BLOCK_DEF = 0x1F8,
	BTL_DEF_BLOCK_START = 0x1F9,
	BTL_DEF_BLOCK_END = 0x1FA,
	BTL_DEF_DODGE = 0x1FB,
	BTL_DEF_KD_FLYING = 0x1FC,
	BTL_DEF_KD_LANDING = 0x1FD,
	BTL_DEF_KD_HURT = 0x1FE,
	BTL_DEF_KD_STAND_UP = 0x1FF,
	BTL_DEF_STUN = 0x200,
	BTL_DEF_COUNTER = 0x201,
	BTL_DEF_GUARD_CRUSH = 0x202,
	BTL_DEF_HTB_HOMING_UP = 0x203,
	BTL_DEF_HTB_HOMING_IDLE = 0x204,
	BTL_DEF_HTB_HOMING_DOWN = 0x205,
	BTL_DEF_HTB_HOMING_LANDING = 0x206,
	BTL_DEF_HTB_TOSS_UP = 0x207,
	BTL_DEF_HTB_TOSS_IDLE = 0x208,
	BTL_DEF_HTB_TOSS_HURT = 0x209,
	BTL_DEF_HTB_FALL_DOWN = 0x20A,
	BTL_DEF_HTB_FALL_DOWN_LANDING = 0x20B,
	BTL_DEF_HTB_FALL_DOWN_HURT = 0x20C,
	BTL_GUN_FP_LOOP = 0x20D,
	BTL_GUN_FP_HURT = 0x20E,
	BTL_GUN_FP_DOT_HURT = 0x20F,
	BTL_GUN_FP_IDLE = 0x210,
	BTL_GUN_BLOCK_DEF = 0x211,
	BTL_GUN_BLOCK_START = 0x212,
	BTL_GUN_BLOCK_END = 0x213,
	BTL_GUN_DODGE = 0x214,
	BTL_GUN_KD_FLYING = 0x215,
	BTL_GUN_KD_LANDING = 0x216,
	BTL_GUN_KD_HURT = 0x217,
	BTL_GUN_KD_STAND_UP = 0x218,
	BTL_GUN_COUNTER = 0x219,
	BTL_GUN_GUARD_CRUSH = 0x21A,
	BTL_STAFF_FP_LOOP = 0x21B,
	BTL_STAFF_FP_HURT = 0x21C,
	BTL_STAFF_FP_DOT_HURT = 0x21D,
	BTL_STAFF_FP_IDLE = 0x21E,
	BTL_STAFF_BLOCK_DEF = 0x21F,
	BTL_STAFF_BLOCK_START = 0x220,
	BTL_STAFF_BLOCK_END = 0x221,
	BTL_STAFF_DODGE = 0x222,
	BTL_STAFF_KD_FLYING = 0x223,
	BTL_STAFF_KD_LANDING = 0x224,
	BTL_STAFF_KD_HURT = 0x225,
	BTL_STAFF_KD_STAND_UP = 0x226,
	BTL_STAFF_COUNTER = 0x227,
	BTL_STAFF_GUARD_CRUSH = 0x228,
	BTL_DGUN_FP_LOOP = 0x229,
	BTL_DGUN_FP_HURT = 0x22A,
	BTL_DGUN_FP_DOT_HURT = 0x22B,
	BTL_DGUN_FP_IDLE = 0x22C,
	BTL_DGUN_BLOCK_DEF = 0x22D,
	BTL_DGUN_BLOCK_START = 0x22E,
	BTL_DGUN_BLOCK_END = 0x22F,
	BTL_DGUN_DODGE = 0x230,
	BTL_DGUN_KD_FLYING = 0x231,
	BTL_DGUN_KD_LANDING = 0x232,
	BTL_DGUN_KD_HURT = 0x233,
	BTL_DGUN_KD_STAND_UP = 0x234,
	BTL_DGUN_COUNTER = 0x235,
	BTL_DGUN_GUARD_CRUSH = 0x236,
	BATTLE_ANIMATION_END = 0x236,
};

//Common Animation
enum ECommonAnimationList
{
	COMMON_ANIMATION_START = 0x1,
	NML_SPAWN = 0x1,
	NML_IDLE_LOOP = 0x2,
	NML_IDLE_01 = 0x3,
	NML_IDLE_02 = 0x4,
	NML_IDLE_HURT = 0x5,
	NML_IDLE_DOT_HURT = 0x6,
	SIT_DOWN = 0x7,
	SIT_IDLE = 0x8,
	SIT_UP = 0x9,
	SIT_HURT = 0xA,
	JUMP_START = 0xB,
	JUMP_HOVER = 0xC,
	JUMP_LANDING_SHORT = 0xD,
	JUMP_LANDING_LONG = 0xE,
	WALK_FRONT = 0xF,
	RUN_FRONT = 0x10,
	IDLE_RUN_FRONT = 0x11,
	RUN_BACK = 0x12,
	IDLE_RUN_BACK = 0x13,
	TURN_LEFT = 0x14,
	TURN_RIGHT = 0x15,
	SWIM_IDLE = 0x16,
	SWIM_FRONT = 0x17,
	SWIM_BACK = 0x18,
	SWIM_IDLE_HURT = 0x19,
	SWIM_FAINTING = 0x1A,
	HOVER_IDLE = 0x1B,
	HOVER_FRONT = 0x1C,
	HOVER_IDLE_HURT = 0x1D,
	HOVER_UP = 0x1E,
	HOVER_DOWN = 0x1F,
	HOVER_LANDING = 0x20,
	FAINTING = 0x21,
	FAINTING_STAND_UP = 0x22,
	NML_DASH_FRONT = 0x23,
	NML_DASH_FRONT_LANDING = 0x24,
	NML_DASH_BACK = 0x25,
	NML_DASH_BACK_LANDING = 0x26,
	NML_DASH_LEFT = 0x27,
	NML_DASH_LEFT_LANDING = 0x28,
	NML_DASH_RIGHT = 0x29,
	NML_DASH_RIGHT_LANDING = 0x2A,
	NML_CHARGE_START = 0x2B,
	NML_CHARGE_LOOP = 0x2C,
	NML_CHARGE_END = 0x2D,
	NML_RIDE = 0x2E,
	NML_SCOUTER_ON_OFF = 0x2F,
	NML_SCOUTER_BURST = 0x30,
	NML_SKILL_ABILITY = 0x31,
	NML_TRANSFORM_SEQUELA = 0x32,
	NML_USE_HOIPOI_CAPSULE = 0x33,
	NML_STAFF_IDLE_LOOP = 0x34,
	NML_STAFF_IDLE_01 = 0x35,
	NML_STAFF_IDLE_02 = 0x36,
	NML_STAFF_FRONT_RUN = 0x37,
	NML_STAFF_IDLE_RUN_FRONT = 0x38,
	NML_STAFF_BACK_RUN = 0x39,
	NML_STAFF_LEFT_TURN = 0x3A,
	NML_STAFF_RIGHT_TURN = 0x3B,
	NML_STAFF_SIT_DOWN = 0x3C,
	NML_STAFF_SIT_IDLE = 0x3D,
	NML_STAFF_SIT_UP = 0x3E,
	NML_DGUN_IDLE_LOOP = 0x3F,
	NML_DGUN_IDLE_01 = 0x40,
	NML_DGUN_IDLE_02 = 0x41,
	NML_DGUN_FRONT_RUN = 0x42,
	NML_DGUN_IDLE_RUN_FRONT = 0x43,
	NML_DGUN_SIT_DOWN = 0x44,
	NML_DGUN_SIT_IDLE = 0x45,
	NML_DGUN_SIT_UP = 0x46,
	NML_RETURN_START = 0x47,
	NML_RETURN_CASTING = 0x48,
	NML_RETURN_END = 0x49,
	NML_COMMON_ANIM_01 = 0x4A,
	NML_BATTLE_MOVE_FRONT = 0x4B,
	NML_BATTLE_MOVE_BACK = 0x4C,
	NML_BATTLE_MOVE_RIGHT = 0x4D,
	NML_BATTLE_MOVE_LEFT = 0x4E,
	NML_MOVE_LEFT = 0x4F,
	NML_MOVE_RIGHT = 0x50,
	FLYING_MOVE_FRONT = 0x56,
	FLYING_MOVE_BACK = 0x57,
	FLYING_STAND_UP = 0x58,
	FLYING_STAND_DOWN = 0x59,
	FLYING_STAND_IDLE_1 = 0x5A,
	FLYING_JUMP_HIGH = 0x5B,
	FLYING_POSE_1 = 0x5C,
	FLYING_POSE_2 = 0x5D,
	FLYING_POSE_3 = 0x5E,
	COMMON_ANIMATION_END_CURRENT = 0x5F,
	COMMON_ANIMATION_END = 0x64,
};

// 변신 상태의 애니메이션 리스트
enum ETransformAnimationList
{   
	TRANSFORM_ANIMATION_START = 0x1194,
	TRANS_IDLE = 0x1194,
	TRANS_RUN_FRONT = 0x1195,
	TRANS_RUN_BACK = 0x1196,
	TRANS_ATK_DEF_1 = 0x1197,
	TRANS_ATK_DEF_2 = 0x1198,
	TRANS_SPAWN = 0x1199,
	TRANSFORM_ANIMATION_END = 0x1199,
};

// PC의 Trigger Animation List
enum ETriggerAnimationList
{
    PC_TRIGGER_ANIMATION_START = 4000,
    PC_TRIGGER_ANIMATION_END = PC_TRIGGER_ANIMATION_START + 100,
};

// PC가 비클을 탈때의 애니메이션 리스트
enum EPCVehicleSRPAnimationList
{
	VEHICLE_SRP1_ANIMATION_START = 0x1388,
	VEHICLE_SRP1_IDLE = 0x1388,
	VEHICLE_SRP1_RUN = 0x1389,
	VEHICLE_SRP1_START = 0x138A,
	VEHICLE_SRP1_STOP = 0x138B,
	VEHICLE_SRP1_JUMP = 0x138C,
	VEHICLE_SRP1_TURN_LEFT = 0x138D,
	VEHICLE_SRP1_TURN_RIGHT = 0x138E,
	VEHICLE_SRP1_LANDING = 0x138F,
	VEHICLE_SRP1_ANIMATION_END = 0x138F,
	VEHICLE_SRP2_ANIMATION_START = 0x13EC,
	VEHICLE_SRP2_IDLE = 0x13EC,
	VEHICLE_SRP2_RUN = 0x13ED,
	VEHICLE_SRP2_START = 0x13EE,
	VEHICLE_SRP2_STOP = 0x13EF,
	VEHICLE_SRP2_JUMP = 0x13F0,
	VEHICLE_SRP2_TURN_LEFT = 0x13F1,
	VEHICLE_SRP2_TURN_RIGHT = 0x13F2,
	VEHICLE_SRP2_LANDING = 0x13F3,
	VEHICLE_SRP2_ANIMATION_END = 0x13F3,
};

/// SubWeapon의 Animation List
enum EItemAnimationList
{
	ITEM_ANIMATION_START = 1,
	ITEM_IDLE = 1,
	ITEM_ATTACK,

	ITEM_ANIMATION_END = ITEM_ATTACK,
};

/// Trigger Object의 Animation List
enum EObjectAnimationList
{
	OBJECT_ANIMATION_START = 1,	
	OBJECT_ANIMATION_END = 20,
};

/// 탈것 애니메이션 리스트
enum EVehicleAnimationList
{
    VEHICLE_ANIMATION_START = 1,
    VEHICLE_IDLE = VEHICLE_ANIMATION_START,
    VEHICLE_RUN,
    VEHICLE_START,
    VEHICLE_STOP,
    VEHICLE_JUMP,
    VEHICLE_TURN_LEFT,
    VEHICLE_TURN_RIGHT,
    VEHICLE_LANDING,
    VEHICLE_ANIMATION_END,
};

/// 아이템 사용 애니메이션 리스트
enum EItemUseAnimationList
{
    ITEM_USE_ANIMATION_START = 6000,
    ITEM_USE_ANIMATION_END = 6050,
};

//Animaiton Event
enum EAnimEventType
{
	EVENT_ANIM_NONE,							///< Default
	EVENT_ANIM_END,								///< Animation Play가 끝났을 때
	EVENT_ANIM_HIT,								///< Hit Event
    EVENT_ANIM_VISUAL_EFFECT,					///< Visual Effect가 나올 때 (Loop Effect의 경우 Animation이 바뀔 경우 소멸된다.)
    EVENT_ANIM_VISUAL_SOUND,					///< Sound를 Play를 해야 할 경우
    EVENT_ANIM_FOOT_STEP,						///< Character가 발이 땅에 땋았을 경우
    EVENT_ANIM_LINK_EFFECT,						///< Character가 Create가 될 때 부터 붙는 Effect (Loop Effect만 가능)
    EVENT_ANIM_WEIGHT_TIME,						///< Character간의 전투에서 특정 시간 동안 Time을 빠르게 또는 늦게 Play 할경우(HTB에서만 사용이 된다.)
    EVENT_ANIM_TRACE_EFFECT,                    ///< 궤적 이펙트 이벤트
    EVENT_ANIM_SUB_WEAPON,                      ///< SubWeapon 활성화 이벤트
    EVENT_ANIM_POST_EFFECT,						///< PostEffect Event
	EVENT_ANIM_SUMMON_PET,						///< SummonPet Effect
    EVENT_ANIM_TMQ,                             ///< TMQ Event
    EVENT_ANIM_ALPHA,                           ///< Alpha Fade Event
    EVENT_ANIM_EXPLOSION,                       ///< TMQ 폭발 이벤트
    EVENT_ANIM_DIRECT,                          ///< 연출용 이벤트
    EVENT_ANIM_COLOR_CHANGE,                    ///< 컬러 체인지 이벤트
    EVENT_ANIM_STRETCH,                         ///< 본을 늘리는 이벤트
    EVENT_ANIM_TRIGGER,                         ///< 스킬등에서 사용되는 트리거 이벤트
    EVENT_ANIM_SKILL_CANCEL,                    ///< 스킬 캔슬 타이밍 이벤트
};

enum ETargetBehavior
{
	//Chain Attack
	TARGET_BEHAVIOR_NONE,						///< Default
	TARGET_BEHAVIOR_PUSH,						///< Target이 뒤로 밀린다.
	TARGET_BEHAVIOR_KNOCK_DOWN,					///< Target이 뒤로 날라 간다.
	
	//HTB
	TARGET_BEHAVIOR_TOSS,						///< Target을 공중으로 올린다.
	TARGET_BEHAVIOR_FALL_DOWN,					///< Target을 바닥으로 내려 친다.
};



struct SEventAnim
{
	float fTime;
	EAnimEventType eEventID;
};

struct SEventLinkEffect : public SEventAnim
{
	char		chEffectName[31 + 1];
	char		chBoneName[31 + 1];
	sVECTOR3	vOffsetPos;
	BOOL		bAttachBone;
};

struct SEventAnimEnd : public SEventAnim
{
	unsigned int uiAnimKey;
};


enum EModuleSkillAnimationList
{
	MODULE_SKILL_ANIMATION_START = 7000,
	MODULE_SKILL_ANIMATION_END = 7400,
};

enum EAttackType
{
	ATTACK_TYPE_PHYSICAL = 0x0,
	ATTACK_TYPE_ENERGY = 0x1,
};

enum EHandType
{
	HAND_TYPE_LEFT = 0x0,
	HAND_TYPE_RIGHT = 0x1,
};

enum EBoneType
{
	BONE_CHARACTER = 0x0,
	BONE_WEAPON = 0x1,
	BONE_SUB_WEAPON = 0x2,
};

enum ESubWeaponActiveFlag
{
	SUB_WEAPON_ACTIVE = 0x0,
	SUB_WEAPON_DEACTIVE = 0x1,
};

enum EPostEffectTypeFlag
{
	POST_EFFECT_TARGET_TYPE_SELF = 0x0,
	POST_EFFECT_TARGET_TYPE_TARGET = 0x1,
};

enum ETargetEffectType
{
	TARGET_EFFECT_TYPE_NONE = 0x0,
	TARGET_EFFECT_TYPE_SIDE = 0x1,
	TARGET_EFFECT_TYPE_FRONT = 0x2,
};

enum EProjectileEffectType
{
	BEID_PROJ_BALL = 0x7D0,
	BEID_PROJ_BEAM = 0x7D1,
	BEID_PROJ_HISSIDAN = 0x7D2,
	BEID_PROJ_HELLZONE = 0x7D3,
	BEID_PROJ_MULTI_HISSIDAN = 0x7D4,
	BEID_PROJ_MAGARE = 0x7D5,
};

enum EAnimCinematicEventType
{
	E_ANIM_CINEMATIC_TMQ_IN = 0x0,
	E_ANIM_CINEMATIC_TMQ_OUT = 0x1,
	E_ANIM_CINEMATIC_DWC_IN = 0x2,
};

enum ENtlPLExplosionEventType
{
	EXPLOSION_EVENT_TYPE_SMALL = 0x0,
	EXPLOSION_EVENT_TYPE_NORMAL = 0x1,
	EXPLOSION_EVENT_TYPE_BIG = 0x2,
};

enum ENtlPLDirectEventType
{
	DIRECT_EVENT_TYPE_CAMERA_SHAKE = 0x0,
};

enum EFootStepType
{
	FOOT_LEFT = 0x0,
	FOOT_RIGHT = 0x1,
};

enum EFootStepMobType
{
	FOOT_TYPE_NORMAL = 0x0,
	FOOT_TYPE_LARGE = 0x1,
};

struct SHissidanData
{
	BOOL bApplyAngle;
	sVECTOR2 v2dAngle;
};

struct SMultiHissidanData
{
	int nCount;
	sVECTOR2 *pArrayAngle;
};

struct SHellZoneData
{
	sVECTOR3 vTargetEffectOffset;
	float fTargetEffectStartWaitTime;
	float fTargetEffectSpeed;
};

union UEffectTypeExtraData
{
	SHissidanData hissidanData;
	SMultiHissidanData multiHissidanData;
	SHellZoneData hellZoneData;
};

enum EColorChangeType
{
	COLOR_CHANGE_TYPE_START = 0x0,
	COLOR_CHANGE_TYPE_END = 0x1,
	COLOR_CHANGE_TYPE_ANIM = 0x2,
};

enum EStretchEventType
{
	E_STRETCH_PULLING = 0x0,
	E_STRETCH_HIT = 0x1,
};

struct RwRGBA
{
	char red;
	char green;
	char blue;
	char alpha;
};

enum RwBlendFunction
{
	rwBLENDNABLEND = 0x0,
	rwBLENDZERO = 0x1,
	rwBLENDONE = 0x2,
	rwBLENDSRCCOLOR = 0x3,
	rwBLENDINVSRCCOLOR = 0x4,
	rwBLENDSRCALPHA = 0x5,
	rwBLENDINVSRCALPHA = 0x6,
	rwBLENDDESTALPHA = 0x7,
	rwBLENDINVDESTALPHA = 0x8,
	rwBLENDDESTCOLOR = 0x9,
	rwBLENDINVDESTCOLOR = 0xA,
	rwBLENDSRCALPHASAT = 0xB,
	rwBLENDFUNCTIONFORCEENUMSIZEINT = 0x7FFFFFFF,
};

struct SEventAnimHit : public SEventAnim
{
	unsigned int uiDamage;
	BOOL bPowerEffect;
	EAttackType eAttackType;
	ETargetBehavior eTargetBehavior;
	BOOL bKB2Push;
	EHandType eHandType;
	EProjectileEffectType uiProjectileEffectType;
	EBoneType eProjectileShotType;
	int nSubWeaponFlag;
	char chBoneName[31 + 1];
	char chProjectileEffectName[31 + 1];
	float fProjectileSpeed;
	BOOL bTargetAttach;
	float fTargetHeight;
	char chTargetEffectName[31 + 1];
	ETargetEffectType eTargetEffectType;
	char chSubTargetEffect[31 + 1];
	char chTargetSoundName[31 + 1];
	int eSoundType;
	BOOL bHitSoundEcho;
	BOOL bCameraShake;
	float fCameraShakeFactor;
	float fCameraShakeMaxHeight;
	char chWordEffect[31 + 1];
	UEffectTypeExtraData uEffectTypeExtraData;
};




struct SEventSound : public SEventAnim
{
	char chSoundName[63 + 1];
	char chSoundName2[63 + 1];
	char chSoundName3[63 + 1];
	char chSoundName4[63 + 1];
	int eSoundType;
	BOOL bLoop;
	float fSoundVolume;
	float fSoundDist;
	float fSoundDecayDist;
	float fSoundPitchMin;
	float fSoundPitchMax;
};


struct SEventFootStep : public SEventAnim
{
	EFootStepType eFootStepType;
	EFootStepMobType eFootStepMobType;
};

struct SEventVisualEffect : public SEventAnim
{
	char chEffectName[31 + 1];
	EBoneType eBoneType;
	char chBoneName[31 + 1];
	RwV3d vOffSetPos;
	BOOL bAttach;
	BOOL bAttachBone;
	BOOL bApplyScale;
	BOOL bProjectileType;
	BOOL bLinkedTargetEffect;
};

struct SEventWeightTime : public SEventAnim
{
	float fLifeTime;
	float fWeightValue;
};

struct SEventTrace : public SEventAnim
{
	float fLifeTime;
	float fEdgeLifeTime;

	enum EAttachType
	{
		CHARACTER_BONE = 0x0,
		WEAPONE_BONE = 0x1,
		SUB_WEAPON_BONE = 0x2,
	}eAttachType;

	enum ETraceKind
	{
		EVENT_TRACE = 0x0,
		ITEM_TRACE = 0x1,
	}eTraceKind;

	char strStartBoneName[31 + 1];
	char strEndBoneName[31 + 1];
	char strTexture[31 + 1];
	RwV3d v3dStartBoneOffset;
	RwV3d v3dEndBoneOffset;
	float fEdgeGap;
	int nSplinePointCount;
	int nMaxEdgeCount;
	float fMaxLength;
	RwBlendFunction eSrcBlend;
	RwBlendFunction eDestBlend;
	RwRGBA colStartColor;
	RwRGBA colEndColor;
};

struct SEventSubWeapon : public SEventAnim
{
	ESubWeaponActiveFlag eSubWeaponActiveFlag;
};

struct SEventPostEffect : public SEventAnim
{
	char szPostEffectName[31 + 1];
	EPostEffectTypeFlag eTarget;
	RwV3d v3dOffset;
	BOOL bCenterFixEnable;
	float fTargetHeight;
	char szPCBoneName[31 + 1];
};

struct SEventSummonPet : public SEventAnim
{
};

struct SEventAnimCinematic : public SEventAnim
{
	EAnimCinematicEventType eAnimCinematicEventType;
};

struct SEventAlpha : public SEventAnim
{
	int nStartAlpha;
	int nDestAlpha;
	float fFadeTime;
	float fLifeTime;

	enum EAlphaEventType
	{
		E_ALPHA_EVENT_CLUMP = 0x0,
		E_ALPHA_EVENT_ATOMIC = 0x1,
	}eAlphaEventType;

	unsigned int bfAtomicList;
};

struct SEventExplosion : public SEventAnim
{
	ENtlPLExplosionEventType eType;
};

struct SEventDirect : public SEventAnim
{
	ENtlPLDirectEventType eType;
};

struct SEventColorChange : public SEventAnim
{
	EColorChangeType eType;
	RwRGBA colorEdge;
	RwRGBA colorBody;
	RwRGBA colorAdd;
};

struct SEventStretch : public SEventAnim
{
	EStretchEventType eType;
	char szBoneName[2][31 + 1];
	float fStretchSpeed;
	float fWidth;
	float fAccel;
	char szScaleBone[31 + 1];
	float fScaleSize;
	char szAxisBone[31 + 1];
	char szTargetEffect[31 + 1];
};

struct SEventTrigger : public SEventAnim
{
};

struct SEventSkillCancel : public SEventAnim
{
};
