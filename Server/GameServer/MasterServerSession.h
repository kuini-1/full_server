#pragma once

#include "NtlSession.h"
#include "NtlSharedType.h"

class CNtlPacket;


class CMasterServerSession : public CNtlSession
{
public:

	CMasterServerSession()
		:CNtlSession(SESSION_SERVER_CON_GAME_TO_MASTER) 
	{
	}

	~CMasterServerSession() {}

public:

	virtual int							OnConnect();
	virtual void						OnClose();
	virtual int							OnDispatch(CNtlPacket * pPacket);


public:

	void						OnInvalid(CNtlPacket * pPacket);

	void						RecvPingRes(CNtlPacket * pPacket);

	void						RecvServerInfoAdd(CNtlPacket * pPacket);

	void						RecvGameServerChannelInfo(CNtlPacket * pPacket);

	void						RecvCharServerUpdate(CNtlPacket * pPacket);
	void						RecvGameServerUpdate(CNtlPacket * pPacket);

	void						RecvUserLogin(CNtlPacket * pPacket);
	void						RecvUserMove(CNtlPacket * pPacket);
	void						RecvPlayerSwitchChannel(CNtlPacket * pPacket);
	void						RecvCharServerTeleportRes(CNtlPacket * pPacket);
	void						RecvWebOnlinePlayersReq(CNtlPacket * pPacket);
	void						RecvWebSendNoticeReq(CNtlPacket * pPacket);
	void						RecvWebKickPlayerReq(CNtlPacket * pPacket);
	void						RecvWebGiveItemReq(CNtlPacket * pPacket);
	void						RecvWebGiveItemAllReq(CNtlPacket * pPacket);
	void						RecvWebExecGmCommandReq(CNtlPacket * pPacket);
	void						RecvWebApplyBuffReq(CNtlPacket * pPacket);
	void						RecvWebRemoveBuffSkillReq(CNtlPacket * pPacket);
	void						RecvWebRemoveBuffEffectReq(CNtlPacket * pPacket);
	void						RecvWebClearBuffsReq(CNtlPacket * pPacket);
	void						RecvWebHealFullReq(CNtlPacket * pPacket);
	void						RecvWebAddZeniReq(CNtlPacket * pPacket);
	void						RecvWebGiveItemCustomReq(CNtlPacket * pPacket);
	void						RecvWebApplyBuffSkillAllReq(CNtlPacket * pPacket);
	void						RecvWebApplyBuffItemReq(CNtlPacket * pPacket);
	void						RecvWebApplyBuffItemAllReq(CNtlPacket * pPacket);
	void						RecvWebRemoveBuffSkillAllReq(CNtlPacket * pPacket);
	void						RecvWebRemoveBuffEffectAllReq(CNtlPacket * pPacket);
	void						RecvWebClearBuffsAllReq(CNtlPacket * pPacket);
	void						RecvWebSetLevelReq(CNtlPacket * pPacket);
	void						RecvWebSetClassReq(CNtlPacket * pPacket);
	void						RecvWebKillPlayerReq(CNtlPacket * pPacket);
	void						RecvWebTeleportPortalReq(CNtlPacket * pPacket);
	void						RecvWebTeleportWorldReq(CNtlPacket * pPacket);
	void						RecvWebTeleportCoordsReq(CNtlPacket * pPacket);
	void						RecvWebMutePlayerReq(CNtlPacket * pPacket);
	void						RecvWebUnmutePlayerReq(CNtlPacket * pPacket);
	void						RecvWebToggleExpReq(CNtlPacket * pPacket);
	void						RecvWebResetExpReq(CNtlPacket * pPacket);
	void						RecvWebLearnSkillReq(CNtlPacket * pPacket);
	void						RecvWebAddTitleReq(CNtlPacket * pPacket);
	void						RecvWebRemoveTitleReq(CNtlPacket * pPacket);
	void						RecvWebSetSoloExpBonusReq(CNtlPacket * pPacket);
	void						RecvWebSetPartyExpBonusReq(CNtlPacket * pPacket);
	void						RecvWebSetQuestExpBonusReq(CNtlPacket * pPacket);
	void						RecvWebSetCraftExpBonusReq(CNtlPacket * pPacket);
	void						RecvWebSetZeniDropBonusReq(CNtlPacket * pPacket);
	void						RecvWebSetQuestMoneyBonusReq(CNtlPacket * pPacket);
	void						RecvWebSetUpgradeRateBonusReq(CNtlPacket * pPacket);
	void						RecvWebSetMonsterAggressiveReq(CNtlPacket * pPacket);
	void						RecvWebSetMonsterStatBonusReq(CNtlPacket * pPacket);
	void						RecvWebApplyMonsterBuffSkillAllReq(CNtlPacket * pPacket);
	void						RecvWebClearMonsterBuffsAllReq(CNtlPacket * pPacket);
	void						RecvWebSetKillDebuffReq(CNtlPacket * pPacket);
	void						RecvWebResetSkillCooldownReq(CNtlPacket * pPacket);
	void						RecvWebResetSkillCooldownAllReq(CNtlPacket * pPacket);
	void						RecvWebSetChannelStatBonusReq(CNtlPacket * pPacket);
};
