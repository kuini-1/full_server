#include "stdafx.h"
#include "GameServer.h"
#include "mg_opcodes.h"
#include "NtlPacketGM.h"
#include "SubNeighborServerInfoManager.h"
#include "GameProcessor.h"
#include "ObjectManager.h"
#include "ItemManager.h"
#include "CPlayer.h"
#include "GameObject.h"
#include "Monster.h"
#include "NtlPacketGT.h"
#include "NtlPacketGU.h"
#include "NtlPacketUG.h"
#include "NtlResultCode.h"
#include "gm.h"
#include "TableContainerManager.h"
#include "ExpTable.h"
#include "SystemEffectTable.h"
#include "UseItemTable.h"
#include "ItemTable.h"
#include "PortalTable.h"
#include "WorldTable.h"
#include "World.h"
#include "NtlItem.h"
#include "NtlPacketGQ.h"
#include "GameMain.h"
#include <time.h>

#include "PacketEventObj.h"



int CMasterServerSession::OnConnect()
{
	NTL_PRINT(PRINT_APP,"CONNECTED TO MASTER SERVER");
	CGameServer * app = (CGameServer*) NtlSfxGetApp();
	app->SetMasterServerSession(this);


	//send server data to master server
	CNtlPacket packet(sizeof(sGM_NOTIFY_SERVER_BEGIN));
	sGM_NOTIFY_SERVER_BEGIN * res = (sGM_NOTIFY_SERVER_BEGIN *)packet.GetPacketData();
	res->wOpCode = GM_NOTIFY_SERVER_BEGIN;

	//server info
	res->serverInfo.bIsOn = true;
	res->serverInfo.byRunningState = DBO_SERVER_RUNNING_STATE_RUNNING;
	res->serverInfo.byServerIndex = app->m_config.byChannel;
	res->serverInfo.serverFarmId = app->m_config.byServerID;
	res->serverInfo.byServerChannelIndex = app->m_config.byChannel;
	res->serverInfo.dwLoad = 0;
	res->serverInfo.dwMaxLoad = DWORD((float)app->m_config.nMaxConnection * 0.95f); //set max connections to 95% limit
	res->serverInfo.wPortForClient = app->m_config.wClientAcceptPort;
	snprintf(res->serverInfo.achPublicAddress, NTL_MAX_LENGTH_OF_IP + 1, "%s", app->m_config.strPublicClientAcceptAddr.c_str());
	res->serverInfo.byGameServerRole = DBO_GAME_SERVER_ROLE_GENERAL;
	res->serverInfo.byServerType = NTL_SERVER_TYPE_GAME;

	//server farm indo
	res->gameServerFarmInfo.serverFarmId = app->m_config.byServerID;
	wcscpy_s(res->gameServerFarmInfo.wszGameServerFarmName, NTL_MAX_SIZE_SERVER_FARM_NAME_UNICODE + 1, s2ws(app->m_config.ServerName.c_str()).c_str());
	res->gameServerFarmInfo.byServerStatus = DBO_SERVER_STATUS_UP;
	res->gameServerFarmInfo.dwMaxLoad = res->serverInfo.dwMaxLoad;
	res->gameServerFarmInfo.dwLoad = 0;

	//channel info
	res->gameServerChannelInfo.serverFarmId = app->m_config.byServerID;
	res->gameServerChannelInfo.byServerChannelIndex = app->m_config.byChannel;
	res->gameServerChannelInfo.bIsVisible = (app->m_config.byChannel == DOJO_CHANNEL_INDEX) ? false : true;
	res->gameServerChannelInfo.byServerStatus = DBO_SERVER_STATUS_UP;
	res->gameServerChannelInfo.dwMaxLoad = res->serverInfo.dwMaxLoad;
	res->gameServerChannelInfo.dwLoad = 0;
	res->gameServerChannelInfo.bIsScrambleChannel = false;
	wcscpy_s(res->gameServerChannelInfo.sChannelBuff.wszServerChannelName, NTL_MAX_SIZE_SERVER_CHANNEL_NAME_UNICODE + 1, s2ws(app->m_config.ChannelName.c_str()).c_str());
	snprintf(res->gameServerChannelInfo.sChannelBuff.szServerChannelName, NTL_MAX_SIZE_SERVER_CHANNEL_NAME_UNICODE + 1, "%s", app->m_config.ChannelName.c_str());

	g_pServerInfoManager->RefreshServerFarmInfo(&res->gameServerFarmInfo); //add server farm info
	g_pServerInfoManager->RefreshServerChannelInfo(&res->gameServerChannelInfo); //add server channel info

	packet.SetPacketLen( sizeof(sGM_NOTIFY_SERVER_BEGIN) );
	app->Send(GetHandle(), &packet);


	return CNtlSession::OnConnect();
}

void CMasterServerSession::OnClose()
{
	CGameServer * app = (CGameServer*)NtlSfxGetApp();

	NTL_PRINT(PRINT_APP,"DISCONNECTED FROM MASTER SERVER. Player Online: %d", g_pObjectManager->GetPlayerCount());
	ERR_LOG(LOG_SYSTEM, "DISCONNECTED FROM MASTER SERVER. Player Online: %d", g_pObjectManager->GetPlayerCount());

	g_pGameProcessor->OnEvent_ServerShutdown();
}


int CMasterServerSession::OnDispatch(CNtlPacket * pPacket)
{
	sNTLPACKETHEADER * pHeader = (sNTLPACKETHEADER *)pPacket->GetPacketData();

	//printf("pHeader->wOpCode %d \n", pHeader->wOpCode);

	OpcodeHandler<CMasterServerSession> const* opHandle = mg_opcodeTable->LookupOpcode(pHeader->wOpCode);
	if (opHandle)
	{
		if (opHandle->packetProcessing == PROCESS_INPLACE)
			(this->*opHandle->handler)(pPacket);
		else
			g_pGameProcessor->PostClientPacketEvent(new TPacketEventObj<CMasterServerSession>(this, opHandle->handler, GetHandle(), pPacket, GetUniqueHandle()));
	}
	else
	{
		return CNtlSession::OnDispatch(pPacket);
	}
	

	return NTL_SUCCESS;
}

void CMasterServerSession::RecvWebOnlinePlayersReq(CNtlPacket * pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_ONLINE_PLAYERS_REQ* req = (sMG_WEB_ONLINE_PLAYERS_REQ*)pPacket->GetPacketData();

	std::vector<CPlayer*> players;
	const boost::unordered_map<CHARACTERID, CPlayer*>& playerMap = g_pObjectManager->GetPlayerMap();

	for (boost::unordered_map<CHARACTERID, CPlayer*>::const_iterator it = playerMap.begin(); it != playerMap.end(); it++)
	{
		CPlayer* player = it->second;
		if (player && player->IsInitialized() && player->GetClientSession())
		{
			players.push_back(player);
		}
	}

	const DWORD totalCount = (DWORD)players.size();
	size_t index = 0;

	while (index < players.size() || (players.empty() && index == 0))
	{
		CNtlPacket packet(sizeof(sGM_WEB_ONLINE_PLAYERS_RES));
		sGM_WEB_ONLINE_PLAYERS_RES* res = (sGM_WEB_ONLINE_PLAYERS_RES*)packet.GetPacketData();
		res->wOpCode = GM_WEB_ONLINE_PLAYERS_RES;
		res->requestId = req->requestId;
		res->serverFarmId = app->GetGsServerId();
		res->serverChannelId = app->GetGsChannel();
		res->totalCount = totalCount;
		res->count = 0;

		for (; index < players.size() && res->count < NTL_MAX_WEB_ONLINE_PLAYERS; index++)
		{
			CPlayer* player = players[index];
			res->players[res->count].accountId = player->GetAccountID();
			res->players[res->count].charId = player->GetCharID();
			NTL_SAFE_WCSNCPY_SIZEINPUT(
				res->players[res->count].awchCharName,
				NTL_MAX_SIZE_CHAR_NAME + 1,
				player->GetCharName(),
				NTL_MAX_SIZE_CHAR_NAME);
			res->players[res->count].serverFarmId = app->GetGsServerId();
			res->players[res->count].serverChannelId = app->GetGsChannel();
			res->count++;
		}

		res->bIsLast = (index >= players.size());
		packet.SetPacketLen(sizeof(sGM_WEB_ONLINE_PLAYERS_RES));
		app->Send(GetHandle(), &packet);

		if (players.empty())
			break;
	}
}

void CMasterServerSession::RecvWebSendNoticeReq(CNtlPacket * pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_SEND_NOTICE_REQ* req = (sMG_WEB_SEND_NOTICE_REQ*)pPacket->GetPacketData();

	if (!app->GetChatServerSession())
	{
		CNtlPacket packetRes(sizeof(sGM_WEB_SEND_NOTICE_RES));
		sGM_WEB_SEND_NOTICE_RES* resWeb = (sGM_WEB_SEND_NOTICE_RES*)packetRes.GetPacketData();
		resWeb->wOpCode = GM_WEB_SEND_NOTICE_RES;
		resWeb->requestId = req->requestId;
		resWeb->wResultCode = GAME_FAIL;
		packetRes.SetPacketLen(sizeof(sGM_WEB_SEND_NOTICE_RES));
		app->Send(GetHandle(), &packetRes);
		return;
	}

	CNtlPacket packet(sizeof(sGT_SYSTEM_DISPLAY_TEXT));
	sGT_SYSTEM_DISPLAY_TEXT* res = (sGT_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GT_SYSTEM_DISPLAY_TEXT;
	res->serverChannelId = req->bIsChannel ? req->serverChannelId : INVALID_SERVERCHANNELID;
	res->byDisplayType = SERVER_TEXT_SYSTEM;
	res->wszGmCharName[0] = L'\0';
	NTL_SAFE_WCSNCPY_SIZEINPUT(res->wszMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE_UNICODE + 1, req->awchMessage, req->wMessageLength);
	packet.SetPacketLen(sizeof(sGT_SYSTEM_DISPLAY_TEXT));
	app->SendTo(app->GetChatServerSession(), &packet);

	CNtlPacket packetRes(sizeof(sGM_WEB_SEND_NOTICE_RES));
	sGM_WEB_SEND_NOTICE_RES* resWeb = (sGM_WEB_SEND_NOTICE_RES*)packetRes.GetPacketData();
	resWeb->wOpCode = GM_WEB_SEND_NOTICE_RES;
	resWeb->requestId = req->requestId;
	resWeb->wResultCode = GAME_SUCCESS;
	packetRes.SetPacketLen(sizeof(sGM_WEB_SEND_NOTICE_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebKickPlayerReq(CNtlPacket * pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_KICK_PLAYER_REQ* req = (sMG_WEB_KICK_PLAYER_REQ*)pPacket->GetPacketData();

	CPlayer* target = NULL;
	if (req->byTargetType == 0)
		target = g_pObjectManager->FindByAccount((ACCOUNTID)req->qwTargetId);
	else
		target = g_pObjectManager->FindByChar((CHARACTERID)req->qwTargetId);

	WORD result = GAME_FAIL;
	if (target && target->GetClientSession())
	{
		target->GetClientSession()->Disconnect(false);
		result = GAME_SUCCESS;
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_KICK_PLAYER_RES));
	sGM_WEB_KICK_PLAYER_RES* res = (sGM_WEB_KICK_PLAYER_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_KICK_PLAYER_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_KICK_PLAYER_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebGiveItemReq(CNtlPacket * pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_GIVE_ITEM_REQ* req = (sMG_WEB_GIVE_ITEM_REQ*)pPacket->GetPacketData();

	CPlayer* target = g_pObjectManager->FindByChar(req->charId);
	WORD result = GAME_FAIL;

	if (target && target->IsInitialized() && target->GetPlayerItemContainer()->CountEmptyInventory() >= 1)
	{
		BYTE count = req->byCount == 0 ? 1 : req->byCount;
		g_pItemManager->CreateItem(target, req->itemTblidx, count, INVALID_BYTE, INVALID_BYTE, true);
		result = GAME_SUCCESS;
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_GIVE_ITEM_RES));
	sGM_WEB_GIVE_ITEM_RES* res = (sGM_WEB_GIVE_ITEM_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_GIVE_ITEM_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_GIVE_ITEM_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebGiveItemAllReq(CNtlPacket * pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_GIVE_ITEM_ALL_REQ* req = (sMG_WEB_GIVE_ITEM_ALL_REQ*)pPacket->GetPacketData();

	DWORD affected = 0;
	BYTE count = req->byCount == 0 ? 1 : req->byCount;

	const boost::unordered_map<CHARACTERID, CPlayer*>& playerMap = g_pObjectManager->GetPlayerMap();
	for (boost::unordered_map<CHARACTERID, CPlayer*>::const_iterator it = playerMap.begin(); it != playerMap.end(); it++)
	{
		CPlayer* player = it->second;
		if (player && player->IsInitialized() && player->GetClientSession())
		{
			if (player->GetPlayerItemContainer()->CountEmptyInventory() >= 1)
			{
				g_pItemManager->CreateItem(player, req->itemTblidx, count, INVALID_BYTE, INVALID_BYTE, true);
				affected++;
			}
		}
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_GIVE_ITEM_ALL_RES));
	sGM_WEB_GIVE_ITEM_ALL_RES* res = (sGM_WEB_GIVE_ITEM_ALL_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_GIVE_ITEM_ALL_RES;
	res->requestId = req->requestId;
	res->wResultCode = GAME_SUCCESS;
	res->dwAffectedCount = affected;
	packetRes.SetPacketLen(sizeof(sGM_WEB_GIVE_ITEM_ALL_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebExecGmCommandReq(CNtlPacket * pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_EXEC_GM_COMMAND_REQ* req = (sMG_WEB_EXEC_GM_COMMAND_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	CPlayer* target = g_pObjectManager->FindByChar(req->charId);
	if (target && target->IsInitialized())
	{
		sUG_SERVER_COMMAND cmd;
		cmd.wOpCode = UG_SERVER_COMMAND;
		WORD commandLength = req->wCommandLen;
		if (commandLength > NTL_MAX_LENGTH_OF_CHAT_MESSAGE_UNICODE)
			commandLength = NTL_MAX_LENGTH_OF_CHAT_MESSAGE_UNICODE;
		cmd.wMessageLengthInUnicode = commandLength;
		NTL_SAFE_WCSNCPY_SIZEINPUT(cmd.awchCommand, NTL_MAX_LENGTH_OF_CHAT_MESSAGE_UNICODE + 1, req->awchCommand, commandLength);
		gm_read_command(&cmd, target);
		result = GAME_SUCCESS;
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_EXEC_GM_COMMAND_RES));
	sGM_WEB_EXEC_GM_COMMAND_RES* res = (sGM_WEB_EXEC_GM_COMMAND_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_EXEC_GM_COMMAND_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_EXEC_GM_COMMAND_RES));
	app->Send(GetHandle(), &packetRes);
}

static DWORD ClampBuffDurationSeconds(DWORD seconds)
{
	if (seconds == 0)
		return 0;
	const DWORD maxSeconds = 60 * 60 * 24 * 7;
	return seconds > maxSeconds ? maxSeconds : seconds;
}

static bool ApplySkillBuffToCharacter(CCharacter* target, TBLIDX skillTblidx, DWORD durationSeconds, bool overrideValues, const float* values)
{
	if (!target || !target->IsInitialized())
		return false;

	sSKILL_TBLDAT* pSkillTbldat = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData(skillTblidx);
	if (!pSkillTbldat)
		return false;

	sDBO_BUFF_PARAMETER aBuffParameter[NTL_MAX_EFFECT_IN_SKILL];
	eSYSTEM_EFFECT_CODE aeEffectCode[NTL_MAX_EFFECT_IN_SKILL];
	ZeroMemory(aBuffParameter, sizeof(aBuffParameter));
	ZeroMemory(aeEffectCode, sizeof(aeEffectCode));

	DWORD dwKeepTime = pSkillTbldat->dwKeepTimeInMilliSecs;
	DWORD clampedSeconds = ClampBuffDurationSeconds(durationSeconds);
	if (clampedSeconds > 0)
		dwKeepTime = clampedSeconds * 1000;
	if (dwKeepTime == 0)
		dwKeepTime = 3600 * 1000;

	for (int i = 0; i < NTL_MAX_EFFECT_IN_SKILL; i++)
	{
		float effectValue = (float)pSkillTbldat->aSkill_Effect_Value[i];
		if (overrideValues)
			effectValue = values[i];

		aBuffParameter[i].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_DEFAULT;
		aBuffParameter[i].buffParameter.fParameter = effectValue;
		aBuffParameter[i].buffParameter.dwRemainValue = (DWORD)effectValue;

		aeEffectCode[i] = g_pTableContainer->GetSystemEffectTable()->GetEffectCodeWithTblidx(pSkillTbldat->skill_Effect[i]);

		if (aeEffectCode[i] == ACTIVE_HEAL_OVER_TIME || aeEffectCode[i] == ACTIVE_EP_OVER_TIME)
		{
			aBuffParameter[i].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_HOT;
			aBuffParameter[i].buffParameter.dwRemainTime = dwKeepTime;
		}
		else if (aeEffectCode[i] == ACTIVE_BLEED || aeEffectCode[i] == ACTIVE_POISON || aeEffectCode[i] == ACTIVE_STOMACHACHE || aeEffectCode[i] == ACTIVE_BURN)
		{
			aBuffParameter[i].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_DOT;
			aBuffParameter[i].buffParameter.dwRemainTime = dwKeepTime;
		}
	}

	eBUFF_TYPE buffType = BUFF_TYPE_BLESS;
	if (Dbo_IsForCurseBuff(pSkillTbldat->bySkill_Active_Type) || Dbo_IsCurseType(pSkillTbldat->bySkill_Active_Type))
		buffType = BUFF_TYPE_CURSE;

	return target->GetBuffManager()->RegisterBuff(dwKeepTime, aeEffectCode, aBuffParameter, INVALID_HOBJECT, buffType, pSkillTbldat);
}

static bool FindItemTblidxByUseItemTblidx(TBLIDX useItemTblidx, TBLIDX& outItemTblidx)
{
	CItemTable* itemTable = g_pTableContainer->GetItemTable();
	if (!itemTable)
		return false;

	for (CTable::TABLEIT iter = itemTable->Begin(); iter != itemTable->End(); ++iter)
	{
		sITEM_TBLDAT* itemTbl = (sITEM_TBLDAT*)iter->second;
		if (itemTbl && itemTbl->Use_Item_Tblidx == useItemTblidx)
		{
			outItemTblidx = itemTbl->tblidx;
			return true;
		}
	}

	return false;
}

static bool ApplyUseItemBuffToPlayer(CPlayer* target, TBLIDX useItemTblidx, DWORD durationSeconds, bool overrideValues, const float* values)
{
	if (!target || !target->IsInitialized())
		return false;

	sUSE_ITEM_TBLDAT* pUseItemTbldat = (sUSE_ITEM_TBLDAT*)g_pTableContainer->GetUseItemTable()->FindData(useItemTblidx);
	if (!pUseItemTbldat)
		return false;

	TBLIDX itemTblidx = INVALID_TBLIDX;
	if (!FindItemTblidxByUseItemTblidx(useItemTblidx, itemTblidx))
		return false;

	sBUFF_INFO buffInfo;
	ZeroMemory(&buffInfo, sizeof(buffInfo));
	buffInfo.buffIndex = INVALID_BYTE;
	buffInfo.sourceTblidx = itemTblidx;
	buffInfo.bySourceType = DBO_OBJECT_SOURCE_ITEM;

	DWORD dwKeepTime = pUseItemTbldat->dwKeepTimeInMilliSecs;
	DWORD clampedSeconds = ClampBuffDurationSeconds(durationSeconds);
	if (clampedSeconds > 0)
		dwKeepTime = clampedSeconds * 1000;
	if (dwKeepTime == 0)
		dwKeepTime = 3600 * 1000;

	buffInfo.dwTimeRemaining = dwKeepTime;
	buffInfo.dwInitialDuration = dwKeepTime;

	eSYSTEM_EFFECT_CODE effectCode[NTL_MAX_EFFECT_IN_ITEM];
	ZeroMemory(effectCode, sizeof(effectCode));
	for (int i = 0; i < NTL_MAX_EFFECT_IN_ITEM; i++)
	{
		float effectValue = (float)pUseItemTbldat->aSystem_Effect_Value[i];
		if (overrideValues)
			effectValue = values[i];

		effectCode[i] = g_pTableContainer->GetSystemEffectTable()->GetEffectCodeWithTblidx(pUseItemTbldat->aSystem_Effect[i]);
		if (effectCode[i] == INVALID_SYSTEM_EFFECT_CODE)
			continue;

		if (effectCode[i] == ACTIVE_HEAL_OVER_TIME || effectCode[i] == ACTIVE_EP_OVER_TIME)
		{
			buffInfo.aBuffParameter[i].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_HOT;
			buffInfo.aBuffParameter[i].buffParameter.dwRemainTime = dwKeepTime;
			buffInfo.aBuffParameter[i].buffParameter.fParameter = effectValue;
		}
		else if (effectCode[i] == ACTIVE_BLEED || effectCode[i] == ACTIVE_POISON || effectCode[i] == ACTIVE_STOMACHACHE || effectCode[i] == ACTIVE_BURN)
		{
			buffInfo.aBuffParameter[i].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_DOT;
			buffInfo.aBuffParameter[i].buffParameter.dwRemainTime = dwKeepTime;
			buffInfo.aBuffParameter[i].buffParameter.fParameter = effectValue;
		}
		else
		{
			buffInfo.aBuffParameter[i].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_DEFAULT;
			buffInfo.aBuffParameter[i].buffParameter.fParameter = effectValue;
			buffInfo.aBuffParameter[i].buffParameter.dwRemainValue = (DWORD)effectValue;
		}
	}

	WORD resultCode = GAME_FAIL;
	return target->GetBuffManager()->RegisterSubBuff(&buffInfo, effectCode, target->GetID(), pUseItemTbldat->byBuff_Group, resultCode, pUseItemTbldat->abySystem_Effect_Type);
}

void CMasterServerSession::RecvWebApplyBuffReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_APPLY_BUFF_REQ* req = (sMG_WEB_APPLY_BUFF_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	CPlayer* target = g_pObjectManager->FindByChar(req->charId);
	if (ApplySkillBuffToCharacter(target, req->skillTblidx, req->durationSeconds, req->bOverrideValues, req->aEffectValues))
		result = GAME_SUCCESS;

	CNtlPacket packetRes(sizeof(sGM_WEB_APPLY_BUFF_RES));
	sGM_WEB_APPLY_BUFF_RES* res = (sGM_WEB_APPLY_BUFF_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_APPLY_BUFF_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_APPLY_BUFF_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebRemoveBuffSkillReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_REMOVE_BUFF_SKILL_REQ* req = (sMG_WEB_REMOVE_BUFF_SKILL_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	CPlayer* target = g_pObjectManager->FindByChar(req->charId);
	if (target && target->IsInitialized())
	{
		if (CBuff* buff = target->GetBuffManager()->FindBuff(req->skillTblidx, DBO_OBJECT_SOURCE_SKILL))
		{
			target->GetBuffManager()->RemoveBuff(buff->GetBuffIndex(), buff->GetBuffType(), CBuff::BUFF_REMOVAL_REASON_BY_ITSELF);
			result = GAME_SUCCESS;
		}
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_REMOVE_BUFF_SKILL_RES));
	sGM_WEB_REMOVE_BUFF_SKILL_RES* res = (sGM_WEB_REMOVE_BUFF_SKILL_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_REMOVE_BUFF_SKILL_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_REMOVE_BUFF_SKILL_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebRemoveBuffEffectReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_REMOVE_BUFF_EFFECT_REQ* req = (sMG_WEB_REMOVE_BUFF_EFFECT_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	CPlayer* target = g_pObjectManager->FindByChar(req->charId);
	if (target && target->IsInitialized())
	{
		if (CBuff* buff = target->GetBuffManager()->FindAnyBuff((eSYSTEM_EFFECT_CODE)req->effectCode))
		{
			target->GetBuffManager()->RemoveBuff(buff->GetBuffIndex(), buff->GetBuffType(), CBuff::BUFF_REMOVAL_REASON_BY_ITSELF);
			result = GAME_SUCCESS;
		}
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_REMOVE_BUFF_EFFECT_RES));
	sGM_WEB_REMOVE_BUFF_EFFECT_RES* res = (sGM_WEB_REMOVE_BUFF_EFFECT_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_REMOVE_BUFF_EFFECT_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_REMOVE_BUFF_EFFECT_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebClearBuffsReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_CLEAR_BUFFS_REQ* req = (sMG_WEB_CLEAR_BUFFS_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	CPlayer* target = g_pObjectManager->FindByChar(req->charId);
	if (target && target->IsInitialized())
	{
		target->GetBuffManager()->RemoveAllBuff();
		result = GAME_SUCCESS;
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_CLEAR_BUFFS_RES));
	sGM_WEB_CLEAR_BUFFS_RES* res = (sGM_WEB_CLEAR_BUFFS_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_CLEAR_BUFFS_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_CLEAR_BUFFS_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebHealFullReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_HEAL_FULL_REQ* req = (sMG_WEB_HEAL_FULL_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	CPlayer* target = g_pObjectManager->FindByChar(req->charId);
	if (target && target->IsInitialized())
	{
		target->UpdateCurLpEp(target->GetLastMaxLP(), target->GetLastMaxEP(), true, false);
		result = GAME_SUCCESS;
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_HEAL_FULL_RES));
	sGM_WEB_HEAL_FULL_RES* res = (sGM_WEB_HEAL_FULL_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_HEAL_FULL_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_HEAL_FULL_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebAddZeniReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_ADD_ZENI_REQ* req = (sMG_WEB_ADD_ZENI_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	CPlayer* target = g_pObjectManager->FindByChar(req->charId);
	if (target && target->IsInitialized())
	{
		BYTE changeType = req->bAdd ? ZENNY_CHANGE_TYPE_CHEAT_INC : ZENNY_CHANGE_TYPE_CHEAT_DEC;
		target->UpdateZeni(changeType, req->amount, true);
		result = GAME_SUCCESS;
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_ADD_ZENI_RES));
	sGM_WEB_ADD_ZENI_RES* res = (sGM_WEB_ADD_ZENI_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_ADD_ZENI_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_ADD_ZENI_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebGiveItemCustomReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_GIVE_ITEM_CUSTOM_REQ* req = (sMG_WEB_GIVE_ITEM_CUSTOM_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	CPlayer* target = g_pObjectManager->FindByChar(req->charId);
	if (target && target->IsInitialized())
	{
		sITEM_TBLDAT* itemtbl = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(req->itemTblidx);
		if (itemtbl && target->GetPlayerItemContainer()->CountEmptyInventory() >= 1)
		{
			BYTE count = req->byCount == 0 ? 1 : req->byCount;
			if (itemtbl->byMax_Stack <= 1 && count > 1)
				count = 1;
			else if (itemtbl->byMax_Stack > 1 && count > itemtbl->byMax_Stack)
				count = itemtbl->byMax_Stack;

			std::pair<BYTE, BYTE> inv = target->GetPlayerItemContainer()->GetEmptyInventory();
			if (inv.first != INVALID_BYTE && inv.second != INVALID_BYTE)
			{
				sITEM_DATA data;
				data.Init();
				data.itemNo = itemtbl->tblidx;
				data.byStackcount = count;
				data.byPlace = inv.first;
				data.byPosition = inv.second;
				data.byRank = (req->byRank != INVALID_BYTE) ? req->byRank : itemtbl->byRank;
				data.byGrade = (req->byGrade != INVALID_BYTE) ? req->byGrade : ITEM_GRADE_LEVEL_0;
				data.byCurrentDurability = (req->byDurability != INVALID_BYTE) ? req->byDurability : itemtbl->byDurability;
				data.byBattleAttribute = (req->byBattleAttribute != INVALID_BYTE) ? req->byBattleAttribute : itemtbl->byBattle_Attribute;
				data.byRestrictState = (req->byRestrictState != INVALID_BYTE) ? req->byRestrictState : GetDefaultRestrictState(itemtbl->byRestrictType, itemtbl->byItem_Type, true);
				data.bNeedToIdentify = (req->byNeedIdentify != 0);
				data.byDurationType = (req->byDurationType != INVALID_BYTE) ? req->byDurationType : itemtbl->byDurationType;

				if (req->durationSeconds > 0)
				{
					data.byDurationType = eDURATIONTYPE_FLATSUM;
					data.nUseStartTime = time(0);
					data.nUseEndTime = data.nUseStartTime + req->durationSeconds;
				}

				data.sOptionSet.Init();
				if (req->bUseDefaultOptions)
				{
					CItem::GenerateOptionSet(req->bEnchantAble, itemtbl, &data);
				}
				else
				{
					memcpy(data.sOptionSet.aOptionTblidx, req->aOptionTblidx, sizeof(data.sOptionSet.aOptionTblidx));
					memcpy(data.sOptionSet.aRandomOption, req->aRandomOption, sizeof(data.sOptionSet.aRandomOption));
				}

				if (req->awchMaker[0] != L'\0')
					NTL_SAFE_WCSCPY(data.awchMaker, req->awchMaker);

				if (g_pItemManager->CreateItem(target, &data, true))
					result = GAME_SUCCESS;
			}
		}
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_GIVE_ITEM_CUSTOM_RES));
	sGM_WEB_GIVE_ITEM_CUSTOM_RES* res = (sGM_WEB_GIVE_ITEM_CUSTOM_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_GIVE_ITEM_CUSTOM_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_GIVE_ITEM_CUSTOM_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebApplyBuffSkillAllReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_APPLY_BUFF_SKILL_ALL_REQ* req = (sMG_WEB_APPLY_BUFF_SKILL_ALL_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	DWORD affected = 0;
	const boost::unordered_map<CHARACTERID, CPlayer*>& playerMap = g_pObjectManager->GetPlayerMap();

	for (boost::unordered_map<CHARACTERID, CPlayer*>::const_iterator it = playerMap.begin(); it != playerMap.end(); it++)
	{
		CPlayer* target = it->second;
		if (ApplySkillBuffToCharacter(target, req->skillTblidx, req->durationSeconds, req->bOverrideValues, req->aEffectValues))
		{
			affected++;
			result = GAME_SUCCESS;
		}
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_APPLY_BUFF_SKILL_ALL_RES));
	sGM_WEB_APPLY_BUFF_SKILL_ALL_RES* res = (sGM_WEB_APPLY_BUFF_SKILL_ALL_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_APPLY_BUFF_SKILL_ALL_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	res->dwAffectedCount = affected;
	packetRes.SetPacketLen(sizeof(sGM_WEB_APPLY_BUFF_SKILL_ALL_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebApplyBuffItemReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_APPLY_BUFF_ITEM_REQ* req = (sMG_WEB_APPLY_BUFF_ITEM_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	CPlayer* target = g_pObjectManager->FindByChar(req->charId);
	if (ApplyUseItemBuffToPlayer(target, req->useItemTblidx, req->durationSeconds, req->bOverrideValues, req->aEffectValues))
		result = GAME_SUCCESS;

	CNtlPacket packetRes(sizeof(sGM_WEB_APPLY_BUFF_ITEM_RES));
	sGM_WEB_APPLY_BUFF_ITEM_RES* res = (sGM_WEB_APPLY_BUFF_ITEM_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_APPLY_BUFF_ITEM_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_APPLY_BUFF_ITEM_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebApplyBuffItemAllReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_APPLY_BUFF_ITEM_ALL_REQ* req = (sMG_WEB_APPLY_BUFF_ITEM_ALL_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	DWORD affected = 0;
	const boost::unordered_map<CHARACTERID, CPlayer*>& playerMap = g_pObjectManager->GetPlayerMap();

	for (boost::unordered_map<CHARACTERID, CPlayer*>::const_iterator it = playerMap.begin(); it != playerMap.end(); it++)
	{
		CPlayer* target = it->second;
		if (ApplyUseItemBuffToPlayer(target, req->useItemTblidx, req->durationSeconds, req->bOverrideValues, req->aEffectValues))
		{
			affected++;
			result = GAME_SUCCESS;
		}
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_APPLY_BUFF_ITEM_ALL_RES));
	sGM_WEB_APPLY_BUFF_ITEM_ALL_RES* res = (sGM_WEB_APPLY_BUFF_ITEM_ALL_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_APPLY_BUFF_ITEM_ALL_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	res->dwAffectedCount = affected;
	packetRes.SetPacketLen(sizeof(sGM_WEB_APPLY_BUFF_ITEM_ALL_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebRemoveBuffSkillAllReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_REMOVE_BUFF_SKILL_ALL_REQ* req = (sMG_WEB_REMOVE_BUFF_SKILL_ALL_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	DWORD affected = 0;
	const boost::unordered_map<CHARACTERID, CPlayer*>& playerMap = g_pObjectManager->GetPlayerMap();

	for (boost::unordered_map<CHARACTERID, CPlayer*>::const_iterator it = playerMap.begin(); it != playerMap.end(); it++)
	{
		CPlayer* target = it->second;
		if (target && target->IsInitialized())
		{
			if (CBuff* buff = target->GetBuffManager()->FindBuff(req->skillTblidx, DBO_OBJECT_SOURCE_SKILL))
			{
				target->GetBuffManager()->RemoveBuff(buff->GetBuffIndex(), buff->GetBuffType(), CBuff::BUFF_REMOVAL_REASON_BY_ITSELF);
				affected++;
				result = GAME_SUCCESS;
			}
		}
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_REMOVE_BUFF_SKILL_ALL_RES));
	sGM_WEB_REMOVE_BUFF_SKILL_ALL_RES* res = (sGM_WEB_REMOVE_BUFF_SKILL_ALL_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_REMOVE_BUFF_SKILL_ALL_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	res->dwAffectedCount = affected;
	packetRes.SetPacketLen(sizeof(sGM_WEB_REMOVE_BUFF_SKILL_ALL_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebRemoveBuffEffectAllReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_REMOVE_BUFF_EFFECT_ALL_REQ* req = (sMG_WEB_REMOVE_BUFF_EFFECT_ALL_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	DWORD affected = 0;
	const boost::unordered_map<CHARACTERID, CPlayer*>& playerMap = g_pObjectManager->GetPlayerMap();

	for (boost::unordered_map<CHARACTERID, CPlayer*>::const_iterator it = playerMap.begin(); it != playerMap.end(); it++)
	{
		CPlayer* target = it->second;
		if (target && target->IsInitialized())
		{
			if (CBuff* buff = target->GetBuffManager()->FindAnyBuff((eSYSTEM_EFFECT_CODE)req->effectCode))
			{
				target->GetBuffManager()->RemoveBuff(buff->GetBuffIndex(), buff->GetBuffType(), CBuff::BUFF_REMOVAL_REASON_BY_ITSELF);
				affected++;
				result = GAME_SUCCESS;
			}
		}
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_REMOVE_BUFF_EFFECT_ALL_RES));
	sGM_WEB_REMOVE_BUFF_EFFECT_ALL_RES* res = (sGM_WEB_REMOVE_BUFF_EFFECT_ALL_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_REMOVE_BUFF_EFFECT_ALL_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	res->dwAffectedCount = affected;
	packetRes.SetPacketLen(sizeof(sGM_WEB_REMOVE_BUFF_EFFECT_ALL_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebClearBuffsAllReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_CLEAR_BUFFS_ALL_REQ* req = (sMG_WEB_CLEAR_BUFFS_ALL_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	DWORD affected = 0;
	const boost::unordered_map<CHARACTERID, CPlayer*>& playerMap = g_pObjectManager->GetPlayerMap();

	for (boost::unordered_map<CHARACTERID, CPlayer*>::const_iterator it = playerMap.begin(); it != playerMap.end(); it++)
	{
		CPlayer* target = it->second;
		if (target && target->IsInitialized())
		{
			target->GetBuffManager()->RemoveAllBuff();
			affected++;
			result = GAME_SUCCESS;
		}
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_CLEAR_BUFFS_ALL_RES));
	sGM_WEB_CLEAR_BUFFS_ALL_RES* res = (sGM_WEB_CLEAR_BUFFS_ALL_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_CLEAR_BUFFS_ALL_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	res->dwAffectedCount = affected;
	packetRes.SetPacketLen(sizeof(sGM_WEB_CLEAR_BUFFS_ALL_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebSetLevelReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_SET_LEVEL_REQ* req = (sMG_WEB_SET_LEVEL_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	CPlayer* target = g_pObjectManager->FindByChar(req->charId);
	if (target && target->IsInitialized())
	{
		sEXP_TBLDAT* ExpData = (sEXP_TBLDAT*)g_pTableContainer->GetExpTable()->FindData(req->byLevel);
		if (ExpData)
		{
			RwUInt32 curlv = target->GetLevel();

			CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_LEVEL));
			sGU_UPDATE_CHAR_LEVEL* res = (sGU_UPDATE_CHAR_LEVEL*)packet.GetPacketData();
			res->wOpCode = GU_UPDATE_CHAR_LEVEL;
			res->byCurLevel = req->byLevel;
			res->byPrevLevel = curlv;
			res->dwMaxExpInThisLevel = ExpData->dwNeed_Exp;
			res->handle = target->GetID();
			packet.SetPacketLen(sizeof(sGU_UPDATE_CHAR_LEVEL));
			target->Broadcast(&packet);

			target->SetLevel(req->byLevel);
			target->UpdateCharSP((target->GetLevel() - 1) + target->GetSkillPointsBought());
			target->GetCharAtt()->CalculateAll();
			target->UpdateCurLpEp(target->GetLastMaxLP(), target->GetLastMaxEP(), true, false);
			target->UpdateMaxRpBalls();

			if (app->GetChatServerSession())
				app->GetChatServerSession()->SendUpdatePcLevel(target);

			if (target->GetPartyID() != INVALID_PARTYID && target->GetParty())
				target->GetParty()->UpdateMemberLevel(target);

			CNtlPacket packetQry(sizeof(sGQ_PC_UPDATE_LEVEL_REQ));
			sGQ_PC_UPDATE_LEVEL_REQ* resQry = (sGQ_PC_UPDATE_LEVEL_REQ*)packetQry.GetPacketData();
			resQry->wOpCode = GQ_PC_UPDATE_LEVEL_REQ;
			resQry->handle = target->GetID();
			resQry->charId = target->GetCharID();
			resQry->dwEXP = 0;
			resQry->byLevel = target->GetLevel();
			resQry->dwSP = target->GetSkillPoints();
			packetQry.SetPacketLen(sizeof(sGQ_PC_UPDATE_LEVEL_REQ));
			app->SendTo(app->GetQueryServerSession(), &packetQry);

			result = GAME_SUCCESS;
		}
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_SET_LEVEL_RES));
	sGM_WEB_SET_LEVEL_RES* res = (sGM_WEB_SET_LEVEL_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_SET_LEVEL_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_SET_LEVEL_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebSetClassReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_SET_CLASS_REQ* req = (sMG_WEB_SET_CLASS_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	CPlayer* target = g_pObjectManager->FindByChar(req->charId);
	if (target && target->IsInitialized())
	{
		target->UpdateClass(req->byClass);
		result = GAME_SUCCESS;
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_SET_CLASS_RES));
	sGM_WEB_SET_CLASS_RES* res = (sGM_WEB_SET_CLASS_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_SET_CLASS_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_SET_CLASS_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebKillPlayerReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_KILL_PLAYER_REQ* req = (sMG_WEB_KILL_PLAYER_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	CPlayer* target = g_pObjectManager->FindByChar(req->charId);
	if (target && target->IsInitialized())
	{
		target->Faint(target, FAINT_REASON_COMMAND);
		result = GAME_SUCCESS;
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_KILL_PLAYER_RES));
	sGM_WEB_KILL_PLAYER_RES* res = (sGM_WEB_KILL_PLAYER_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_KILL_PLAYER_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_KILL_PLAYER_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebTeleportPortalReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_TELEPORT_PORTAL_REQ* req = (sMG_WEB_TELEPORT_PORTAL_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	CPlayer* target = g_pObjectManager->FindByChar(req->charId);
	if (target && target->IsInitialized())
	{
		sPORTAL_TBLDAT* pPortalTblData = (sPORTAL_TBLDAT*)g_pTableContainer->GetPortalTable()->FindData(req->portalTblidx);
		if (pPortalTblData)
		{
			target->StartTeleport(pPortalTblData->vLoc, pPortalTblData->vDir, pPortalTblData->worldId, TELEPORT_TYPE_COMMAND);
			result = GAME_SUCCESS;
		}
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_TELEPORT_PORTAL_RES));
	sGM_WEB_TELEPORT_PORTAL_RES* res = (sGM_WEB_TELEPORT_PORTAL_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_TELEPORT_PORTAL_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_TELEPORT_PORTAL_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebTeleportWorldReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_TELEPORT_WORLD_REQ* req = (sMG_WEB_TELEPORT_WORLD_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	CPlayer* target = g_pObjectManager->FindByChar(req->charId);
	if (target && target->IsInitialized())
	{
		sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData(req->worldId);
		if (pWorldTbldat)
		{
			if (CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld(req->worldId))
			{
				target->StartTeleport(pWorld->GetTbldat()->vDefaultLoc, target->GetCurDir(), req->worldId, TELEPORT_TYPE_COMMAND);
				pWorld->AddScriptToPlayer(target);
				result = GAME_SUCCESS;
			}
			else
			{
				CWorld* pWorld2 = app->GetGameMain()->GetWorldManager()->CreateWorld(pWorldTbldat);
				if (pWorld2)
				{
					target->StartTeleport(pWorld2->GetTbldat()->vStart1Loc, target->GetCurDir(), pWorld2->GetID(), TELEPORT_TYPE_COMMAND);
					pWorld2->AddScriptToPlayer(target);
					result = GAME_SUCCESS;
				}
			}
		}
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_TELEPORT_WORLD_RES));
	sGM_WEB_TELEPORT_WORLD_RES* res = (sGM_WEB_TELEPORT_WORLD_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_TELEPORT_WORLD_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_TELEPORT_WORLD_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebTeleportCoordsReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_TELEPORT_COORDS_REQ* req = (sMG_WEB_TELEPORT_COORDS_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	CPlayer* target = g_pObjectManager->FindByChar(req->charId);
	if (target && target->IsInitialized())
	{
		CNtlVector vLoc;
		vLoc.x = req->x;
		vLoc.y = req->y;
		vLoc.z = req->z;
		
		// Use current world ID if provided worldId is invalid or 0
		WORLDID worldId = req->worldId;
		if (worldId == INVALID_WORLDID || worldId == 0)
			worldId = target->GetWorldID();
		
		target->StartTeleport(vLoc, target->GetCurDir(), worldId, TELEPORT_TYPE_COMMAND);
		result = GAME_SUCCESS;
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_TELEPORT_COORDS_RES));
	sGM_WEB_TELEPORT_COORDS_RES* res = (sGM_WEB_TELEPORT_COORDS_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_TELEPORT_COORDS_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_TELEPORT_COORDS_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebMutePlayerReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_MUTE_PLAYER_REQ* req = (sMG_WEB_MUTE_PLAYER_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	CPlayer* target = g_pObjectManager->FindByChar(req->charId);
	if (target && target->IsInitialized() && app->GetChatServerSession())
	{
		WORD reasonLength = req->wReasonLength;
		if (reasonLength > NTL_MAX_LENGTH_OF_MAIL_MESSAGE)
			reasonLength = NTL_MAX_LENGTH_OF_MAIL_MESSAGE;

		CNtlPacket packet(sizeof(sGT_UPDATE_PUNISH));
		sGT_UPDATE_PUNISH* res = (sGT_UPDATE_PUNISH*)packet.GetPacketData();
		res->wOpCode = GT_UPDATE_PUNISH;
		res->accountId = 0;
		res->dwDurationInMinute = req->durationMinutes;
		NTL_SAFE_WCSCPY(res->awchGmCharName, L"WEB_ADMIN");
		NTL_SAFE_WCSNCPY_SIZEINPUT(res->awchCharName, NTL_MAX_SIZE_CHAR_NAME + 1, target->GetCharName(), NTL_MAX_SIZE_CHAR_NAME);
		NTL_SAFE_WCSNCPY_SIZEINPUT(res->wchReason, NTL_MAX_LENGTH_OF_MAIL_MESSAGE + 1, req->awchReason, reasonLength);
		packet.SetPacketLen(sizeof(sGT_UPDATE_PUNISH));
		app->SendTo(app->GetChatServerSession(), &packet);
		result = GAME_SUCCESS;
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_MUTE_PLAYER_RES));
	sGM_WEB_MUTE_PLAYER_RES* res = (sGM_WEB_MUTE_PLAYER_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_MUTE_PLAYER_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_MUTE_PLAYER_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebUnmutePlayerReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_UNMUTE_PLAYER_REQ* req = (sMG_WEB_UNMUTE_PLAYER_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	CPlayer* target = g_pObjectManager->FindByChar(req->charId);
	if (target && target->IsInitialized() && app->GetChatServerSession())
	{
		CNtlPacket packet(sizeof(sGT_UPDATE_PUNISH));
		sGT_UPDATE_PUNISH* res = (sGT_UPDATE_PUNISH*)packet.GetPacketData();
		res->wOpCode = GT_UPDATE_PUNISH;
		res->accountId = 0;
		res->dwDurationInMinute = 0;
		NTL_SAFE_WCSNCPY_SIZEINPUT(res->awchCharName, NTL_MAX_SIZE_CHAR_NAME + 1, target->GetCharName(), NTL_MAX_SIZE_CHAR_NAME);
		packet.SetPacketLen(sizeof(sGT_UPDATE_PUNISH));
		app->SendTo(app->GetChatServerSession(), &packet);
		result = GAME_SUCCESS;
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_UNMUTE_PLAYER_RES));
	sGM_WEB_UNMUTE_PLAYER_RES* res = (sGM_WEB_UNMUTE_PLAYER_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_UNMUTE_PLAYER_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_UNMUTE_PLAYER_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebToggleExpReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_TOGGLE_EXP_REQ* req = (sMG_WEB_TOGGLE_EXP_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	CPlayer* target = g_pObjectManager->FindByChar(req->charId);
	if (target && target->IsInitialized())
	{
		target->SetExpReceiveDisabled(!req->bEnable);

		const WCHAR* msg = req->bEnable ? L"Receive EXP has been enabled" : L"Receive EXP has been disabled";
		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
		res->wMessageLengthInUnicode = wcslen(msg);
		res->byDisplayType = SERVER_TEXT_SYSTEM;
		NTL_SAFE_WCSCPY(res->awchMessage, msg);
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		target->SendPacket(&packet);

		result = GAME_SUCCESS;
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_TOGGLE_EXP_RES));
	sGM_WEB_TOGGLE_EXP_RES* res = (sGM_WEB_TOGGLE_EXP_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_TOGGLE_EXP_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_TOGGLE_EXP_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebResetExpReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_RESET_EXP_REQ* req = (sMG_WEB_RESET_EXP_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	CPlayer* target = g_pObjectManager->FindByChar(req->charId);
	if (target && target->IsInitialized())
	{
		CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_EXP));
		sGU_UPDATE_CHAR_EXP* res = (sGU_UPDATE_CHAR_EXP*)packet.GetPacketData();
		res->handle = target->GetID();
		res->wOpCode = GU_UPDATE_CHAR_EXP;
		res->dwCurExp = 0;
		res->dwAcquisitionExp = 0;
		res->dwIncreasedExp = 0;
		res->dwBonusExp = 0;
		packet.SetPacketLen(sizeof(sGU_UPDATE_CHAR_EXP));
		target->SendPacket(&packet);

		target->SetExp(0);
		result = GAME_SUCCESS;
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_RESET_EXP_RES));
	sGM_WEB_RESET_EXP_RES* res = (sGM_WEB_RESET_EXP_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_RESET_EXP_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_RESET_EXP_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebLearnSkillReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_LEARN_SKILL_REQ* req = (sMG_WEB_LEARN_SKILL_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	CPlayer* target = g_pObjectManager->FindByChar(req->charId);
	if (target && target->IsInitialized() && target->GetSkillManager())
	{
		WORD wTemp = 0;
		target->GetSkillManager()->LearnSkill(req->skillTblidx, wTemp, false);
		result = GAME_SUCCESS;
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_LEARN_SKILL_RES));
	sGM_WEB_LEARN_SKILL_RES* res = (sGM_WEB_LEARN_SKILL_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_LEARN_SKILL_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_LEARN_SKILL_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebAddTitleReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_ADD_TITLE_REQ* req = (sMG_WEB_ADD_TITLE_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	CPlayer* target = g_pObjectManager->FindByChar(req->charId);
	if (target && target->IsInitialized())
	{
		TBLIDX titleIdx = req->titleTblidx;
		if ((titleIdx - 1) / NTL_MAX_CHAR_TITLE_FLAG_COUNT < NTL_MAX_CHAR_TITLE_COUNT_IN_FLAG)
		{
			if (target->CheckCharTitle(titleIdx - 1) == false)
				target->AddCharTitle(titleIdx - 1);
			result = GAME_SUCCESS;
		}
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_ADD_TITLE_RES));
	sGM_WEB_ADD_TITLE_RES* res = (sGM_WEB_ADD_TITLE_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_ADD_TITLE_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_ADD_TITLE_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebRemoveTitleReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_REMOVE_TITLE_REQ* req = (sMG_WEB_REMOVE_TITLE_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	CPlayer* target = g_pObjectManager->FindByChar(req->charId);
	if (target && target->IsInitialized())
	{
		TBLIDX titleIdx = req->titleTblidx;
		if ((titleIdx - 1) / NTL_MAX_CHAR_TITLE_FLAG_COUNT < NTL_MAX_CHAR_TITLE_COUNT_IN_FLAG)
		{
			if (target->CheckCharTitle(titleIdx - 1) == true)
				target->DelCharTitle(titleIdx - 1);
			result = GAME_SUCCESS;
		}
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_REMOVE_TITLE_RES));
	sGM_WEB_REMOVE_TITLE_RES* res = (sGM_WEB_REMOVE_TITLE_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_REMOVE_TITLE_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_REMOVE_TITLE_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebSetSoloExpBonusReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_SET_SOLO_EXP_BONUS_REQ* req = (sMG_WEB_SET_SOLO_EXP_BONUS_REQ*)pPacket->GetPacketData();

	app->SetSoloExpBonus(req->bonusPercent);

	CNtlPacket packetRes(sizeof(sGM_WEB_SET_SOLO_EXP_BONUS_RES));
	sGM_WEB_SET_SOLO_EXP_BONUS_RES* res = (sGM_WEB_SET_SOLO_EXP_BONUS_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_SET_SOLO_EXP_BONUS_RES;
	res->requestId = req->requestId;
	res->wResultCode = GAME_SUCCESS;
	packetRes.SetPacketLen(sizeof(sGM_WEB_SET_SOLO_EXP_BONUS_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebSetPartyExpBonusReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_SET_PARTY_EXP_BONUS_REQ* req = (sMG_WEB_SET_PARTY_EXP_BONUS_REQ*)pPacket->GetPacketData();

	app->SetPartyExpBonus(req->bonusPercent);

	CNtlPacket packetRes(sizeof(sGM_WEB_SET_PARTY_EXP_BONUS_RES));
	sGM_WEB_SET_PARTY_EXP_BONUS_RES* res = (sGM_WEB_SET_PARTY_EXP_BONUS_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_SET_PARTY_EXP_BONUS_RES;
	res->requestId = req->requestId;
	res->wResultCode = GAME_SUCCESS;
	packetRes.SetPacketLen(sizeof(sGM_WEB_SET_PARTY_EXP_BONUS_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebSetQuestExpBonusReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_SET_QUEST_EXP_BONUS_REQ* req = (sMG_WEB_SET_QUEST_EXP_BONUS_REQ*)pPacket->GetPacketData();

	app->SetQuestExpBonus(req->bonusPercent);

	CNtlPacket packetRes(sizeof(sGM_WEB_SET_QUEST_EXP_BONUS_RES));
	sGM_WEB_SET_QUEST_EXP_BONUS_RES* res = (sGM_WEB_SET_QUEST_EXP_BONUS_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_SET_QUEST_EXP_BONUS_RES;
	res->requestId = req->requestId;
	res->wResultCode = GAME_SUCCESS;
	packetRes.SetPacketLen(sizeof(sGM_WEB_SET_QUEST_EXP_BONUS_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebSetCraftExpBonusReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_SET_CRAFT_EXP_BONUS_REQ* req = (sMG_WEB_SET_CRAFT_EXP_BONUS_REQ*)pPacket->GetPacketData();

	app->SetCraftExpBonus(req->bonusPercent);

	CNtlPacket packetRes(sizeof(sGM_WEB_SET_CRAFT_EXP_BONUS_RES));
	sGM_WEB_SET_CRAFT_EXP_BONUS_RES* res = (sGM_WEB_SET_CRAFT_EXP_BONUS_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_SET_CRAFT_EXP_BONUS_RES;
	res->requestId = req->requestId;
	res->wResultCode = GAME_SUCCESS;
	packetRes.SetPacketLen(sizeof(sGM_WEB_SET_CRAFT_EXP_BONUS_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebSetZeniDropBonusReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_SET_ZENI_DROP_BONUS_REQ* req = (sMG_WEB_SET_ZENI_DROP_BONUS_REQ*)pPacket->GetPacketData();

	app->SetZeniDropBonus(req->bonusPercent);

	CNtlPacket packetRes(sizeof(sGM_WEB_SET_ZENI_DROP_BONUS_RES));
	sGM_WEB_SET_ZENI_DROP_BONUS_RES* res = (sGM_WEB_SET_ZENI_DROP_BONUS_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_SET_ZENI_DROP_BONUS_RES;
	res->requestId = req->requestId;
	res->wResultCode = GAME_SUCCESS;
	packetRes.SetPacketLen(sizeof(sGM_WEB_SET_ZENI_DROP_BONUS_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebSetQuestMoneyBonusReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_SET_QUEST_MONEY_BONUS_REQ* req = (sMG_WEB_SET_QUEST_MONEY_BONUS_REQ*)pPacket->GetPacketData();

	app->SetQuestMoneyBonus(req->bonusPercent);

	CNtlPacket packetRes(sizeof(sGM_WEB_SET_QUEST_MONEY_BONUS_RES));
	sGM_WEB_SET_QUEST_MONEY_BONUS_RES* res = (sGM_WEB_SET_QUEST_MONEY_BONUS_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_SET_QUEST_MONEY_BONUS_RES;
	res->requestId = req->requestId;
	res->wResultCode = GAME_SUCCESS;
	packetRes.SetPacketLen(sizeof(sGM_WEB_SET_QUEST_MONEY_BONUS_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebSetUpgradeRateBonusReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_SET_UPGRADE_RATE_BONUS_REQ* req = (sMG_WEB_SET_UPGRADE_RATE_BONUS_REQ*)pPacket->GetPacketData();

	app->SetUpgradeRateBonus(req->bonusPercent);

	CNtlPacket packetRes(sizeof(sGM_WEB_SET_UPGRADE_RATE_BONUS_RES));
	sGM_WEB_SET_UPGRADE_RATE_BONUS_RES* res = (sGM_WEB_SET_UPGRADE_RATE_BONUS_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_SET_UPGRADE_RATE_BONUS_RES;
	res->requestId = req->requestId;
	res->wResultCode = GAME_SUCCESS;
	packetRes.SetPacketLen(sizeof(sGM_WEB_SET_UPGRADE_RATE_BONUS_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebSetMonsterAggressiveReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_SET_MONSTER_AGGRESSIVE_REQ* req = (sMG_WEB_SET_MONSTER_AGGRESSIVE_REQ*)pPacket->GetPacketData();

	app->SetMonsterAggressive(req->bEnable);

	const boost::unordered_map<HOBJECT, CGameObject*>& objMap = g_pObjectManager->GetObjectMap();
	for (boost::unordered_map<HOBJECT, CGameObject*>::const_iterator it = objMap.begin(); it != objMap.end(); ++it)
	{
		CGameObject* obj = it->second;
		if (!obj || !obj->IsMonster())
			continue;

		CMonster* mob = (CMonster*)obj;
		if (!mob->IsInitialized())
			continue;

		mob->GetBotController()->ChangeControlState_Enter();
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_SET_MONSTER_AGGRESSIVE_RES));
	sGM_WEB_SET_MONSTER_AGGRESSIVE_RES* res = (sGM_WEB_SET_MONSTER_AGGRESSIVE_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_SET_MONSTER_AGGRESSIVE_RES;
	res->requestId = req->requestId;
	res->wResultCode = GAME_SUCCESS;
	packetRes.SetPacketLen(sizeof(sGM_WEB_SET_MONSTER_AGGRESSIVE_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebSetKillDebuffReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_SET_KILL_DEBUFF_REQ* req = (sMG_WEB_SET_KILL_DEBUFF_REQ*)pPacket->GetPacketData();

	sKILL_DEBUFF_CONFIG config;
	config.bEnable = req->bEnable;
	config.skillTblidx = req->skillTblidx;
	config.durationSeconds = req->durationSeconds;
	config.bOverrideValues = req->bOverrideValues;
	memcpy(config.aEffectValues, req->aEffectValues, sizeof(config.aEffectValues));
	app->SetKillDebuffConfig(config);

	CNtlPacket packetRes(sizeof(sGM_WEB_SET_KILL_DEBUFF_RES));
	sGM_WEB_SET_KILL_DEBUFF_RES* res = (sGM_WEB_SET_KILL_DEBUFF_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_SET_KILL_DEBUFF_RES;
	res->requestId = req->requestId;
	res->wResultCode = GAME_SUCCESS;
	packetRes.SetPacketLen(sizeof(sGM_WEB_SET_KILL_DEBUFF_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebResetSkillCooldownReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_RESET_SKILL_COOLDOWN_REQ* req = (sMG_WEB_RESET_SKILL_COOLDOWN_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	CPlayer* target = g_pObjectManager->FindByChar(req->charId);
	if (target && target->IsInitialized())
	{
		if (target->GetSkillManager())
		{
			target->GetSkillManager()->ResetAllCooldowns();
			target->SendCurrentSkillInfo();
		}
		if (target->GetHtbSkillManager())
		{
			target->GetHtbSkillManager()->ResetAllCooldowns();
			target->SendCurrentHtbSkillInfo();
		}
		result = GAME_SUCCESS;
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_RESET_SKILL_COOLDOWN_RES));
	sGM_WEB_RESET_SKILL_COOLDOWN_RES* res = (sGM_WEB_RESET_SKILL_COOLDOWN_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_RESET_SKILL_COOLDOWN_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	packetRes.SetPacketLen(sizeof(sGM_WEB_RESET_SKILL_COOLDOWN_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebResetSkillCooldownAllReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_RESET_SKILL_COOLDOWN_ALL_REQ* req = (sMG_WEB_RESET_SKILL_COOLDOWN_ALL_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	DWORD affected = 0;
	const boost::unordered_map<CHARACTERID, CPlayer*>& playerMap = g_pObjectManager->GetPlayerMap();

	for (boost::unordered_map<CHARACTERID, CPlayer*>::const_iterator it = playerMap.begin(); it != playerMap.end(); ++it)
	{
		CPlayer* target = it->second;
		if (!target || !target->IsInitialized())
			continue;

		if (target->GetSkillManager())
		{
			target->GetSkillManager()->ResetAllCooldowns();
			target->SendCurrentSkillInfo();
		}
		if (target->GetHtbSkillManager())
		{
			target->GetHtbSkillManager()->ResetAllCooldowns();
			target->SendCurrentHtbSkillInfo();
		}
		affected++;
		result = GAME_SUCCESS;
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_RESET_SKILL_COOLDOWN_ALL_RES));
	sGM_WEB_RESET_SKILL_COOLDOWN_ALL_RES* res = (sGM_WEB_RESET_SKILL_COOLDOWN_ALL_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_RESET_SKILL_COOLDOWN_ALL_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	res->dwAffectedCount = affected;
	packetRes.SetPacketLen(sizeof(sGM_WEB_RESET_SKILL_COOLDOWN_ALL_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebSetChannelStatBonusReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_SET_CHANNEL_STAT_BONUS_REQ* req = (sMG_WEB_SET_CHANNEL_STAT_BONUS_REQ*)pPacket->GetPacketData();

	if (req->channelId == app->GetGsChannel())
	{
		sCHANNEL_STAT_BONUS bonus;
		bonus.fMaxLpPercent = req->fMaxLpPercent;
		bonus.fMaxEpPercent = req->fMaxEpPercent;
		bonus.fPhysicalOffencePercent = req->fPhysicalOffencePercent;
		bonus.fEnergyOffencePercent = req->fEnergyOffencePercent;
		bonus.fPhysicalDefencePercent = req->fPhysicalDefencePercent;
		bonus.fEnergyDefencePercent = req->fEnergyDefencePercent;
		bonus.fAttackRatePercent = req->fAttackRatePercent;
		bonus.fDodgeRatePercent = req->fDodgeRatePercent;
		app->SetChannelStatBonus(bonus);

		const boost::unordered_map<CHARACTERID, CPlayer*>& playerMap = g_pObjectManager->GetPlayerMap();
		for (boost::unordered_map<CHARACTERID, CPlayer*>::const_iterator it = playerMap.begin(); it != playerMap.end(); ++it)
		{
			CPlayer* target = it->second;
			if (!target || !target->IsInitialized())
				continue;

			target->GetCharAtt()->CalculateAll();
			target->UpdateCurLpEp(target->GetLastMaxLP(), target->GetLastMaxEP(), true, false);
		}
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_SET_CHANNEL_STAT_BONUS_RES));
	sGM_WEB_SET_CHANNEL_STAT_BONUS_RES* res = (sGM_WEB_SET_CHANNEL_STAT_BONUS_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_SET_CHANNEL_STAT_BONUS_RES;
	res->requestId = req->requestId;
	res->wResultCode = GAME_SUCCESS;
	packetRes.SetPacketLen(sizeof(sGM_WEB_SET_CHANNEL_STAT_BONUS_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebSetMonsterStatBonusReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_SET_MONSTER_STAT_BONUS_REQ* req = (sMG_WEB_SET_MONSTER_STAT_BONUS_REQ*)pPacket->GetPacketData();

	sMONSTER_STAT_BONUS bonus;
	bonus.fMaxLpPercent = req->fMaxLpPercent;
	bonus.fMaxEpPercent = req->fMaxEpPercent;
	bonus.fPhysicalOffencePercent = req->fPhysicalOffencePercent;
	bonus.fEnergyOffencePercent = req->fEnergyOffencePercent;
	bonus.fPhysicalDefencePercent = req->fPhysicalDefencePercent;
	bonus.fEnergyDefencePercent = req->fEnergyDefencePercent;
	bonus.fAttackRatePercent = req->fAttackRatePercent;
	bonus.fDodgeRatePercent = req->fDodgeRatePercent;
	app->SetMonsterStatBonus(req->byWorldRuleType, bonus);

	const boost::unordered_map<HOBJECT, CGameObject*>& objMap = g_pObjectManager->GetObjectMap();
	for (boost::unordered_map<HOBJECT, CGameObject*>::const_iterator it = objMap.begin(); it != objMap.end(); ++it)
	{
		CGameObject* obj = it->second;
		if (!obj || !obj->IsMonster())
			continue;

		CMonster* mob = (CMonster*)obj;
		if (!mob->IsInitialized())
			continue;

		mob->GetCharAtt()->CalculateAll();
		mob->UpdateCurLpEp(mob->GetLastMaxLP(), mob->GetLastMaxEP(), true, false);
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_SET_MONSTER_STAT_BONUS_RES));
	sGM_WEB_SET_MONSTER_STAT_BONUS_RES* res = (sGM_WEB_SET_MONSTER_STAT_BONUS_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_SET_MONSTER_STAT_BONUS_RES;
	res->requestId = req->requestId;
	res->wResultCode = GAME_SUCCESS;
	packetRes.SetPacketLen(sizeof(sGM_WEB_SET_MONSTER_STAT_BONUS_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebApplyMonsterBuffSkillAllReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_APPLY_MONSTER_BUFF_SKILL_ALL_REQ* req = (sMG_WEB_APPLY_MONSTER_BUFF_SKILL_ALL_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	DWORD affected = 0;
	const boost::unordered_map<HOBJECT, CGameObject*>& objMap = g_pObjectManager->GetObjectMap();

	for (boost::unordered_map<HOBJECT, CGameObject*>::const_iterator it = objMap.begin(); it != objMap.end(); ++it)
	{
		CGameObject* obj = it->second;
		if (!obj || !obj->IsMonster())
			continue;

		CMonster* mob = (CMonster*)obj;
		if (!mob->IsInitialized())
			continue;

		if (ApplySkillBuffToCharacter(mob, req->skillTblidx, req->durationSeconds, req->bOverrideValues, req->aEffectValues))
		{
			affected++;
			result = GAME_SUCCESS;
		}
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_APPLY_MONSTER_BUFF_SKILL_ALL_RES));
	sGM_WEB_APPLY_MONSTER_BUFF_SKILL_ALL_RES* res = (sGM_WEB_APPLY_MONSTER_BUFF_SKILL_ALL_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_APPLY_MONSTER_BUFF_SKILL_ALL_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	res->dwAffectedCount = affected;
	packetRes.SetPacketLen(sizeof(sGM_WEB_APPLY_MONSTER_BUFF_SKILL_ALL_RES));
	app->Send(GetHandle(), &packetRes);
}

void CMasterServerSession::RecvWebClearMonsterBuffsAllReq(CNtlPacket* pPacket)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();
	sMG_WEB_CLEAR_MONSTER_BUFFS_ALL_REQ* req = (sMG_WEB_CLEAR_MONSTER_BUFFS_ALL_REQ*)pPacket->GetPacketData();

	WORD result = GAME_FAIL;
	DWORD affected = 0;
	const boost::unordered_map<HOBJECT, CGameObject*>& objMap = g_pObjectManager->GetObjectMap();

	for (boost::unordered_map<HOBJECT, CGameObject*>::const_iterator it = objMap.begin(); it != objMap.end(); ++it)
	{
		CGameObject* obj = it->second;
		if (!obj || !obj->IsMonster())
			continue;

		CMonster* mob = (CMonster*)obj;
		if (!mob->IsInitialized())
			continue;

		mob->GetBuffManager()->RemoveAllBuff();
		affected++;
		result = GAME_SUCCESS;
	}

	CNtlPacket packetRes(sizeof(sGM_WEB_CLEAR_MONSTER_BUFFS_ALL_RES));
	sGM_WEB_CLEAR_MONSTER_BUFFS_ALL_RES* res = (sGM_WEB_CLEAR_MONSTER_BUFFS_ALL_RES*)packetRes.GetPacketData();
	res->wOpCode = GM_WEB_CLEAR_MONSTER_BUFFS_ALL_RES;
	res->requestId = req->requestId;
	res->wResultCode = result;
	res->dwAffectedCount = affected;
	packetRes.SetPacketLen(sizeof(sGM_WEB_CLEAR_MONSTER_BUFFS_ALL_RES));
	app->Send(GetHandle(), &packetRes);
}
