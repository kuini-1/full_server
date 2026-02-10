//-----------------------------------------------------------------------------------
//		Master Server
//-----------------------------------------------------------------------------------

#include "stdafx.h"
#include "MasterServer.h"
#include "NtlPacketMW.h"
#include "NtlResultCode.h"
#include <time.h>

int	CMasterServer::OnInitApp()
{
	m_startTick = GetTickCount();
	m_startTime = time(NULL);
	m_nMaxSessionCount = m_config.ServerAcceptLimit;
	m_adminBonusState.soloExpBonus = 0;
	m_adminBonusState.partyExpBonus = 0;
	m_adminBonusState.questExpBonus = 0;
	m_adminBonusState.craftExpBonus = 0;
	m_adminBonusState.zeniDropBonus = 0;
	m_adminBonusState.questMoneyBonus = 0;
	m_adminBonusState.upgradeRateBonus = 0;
	m_adminBonusState.monsterAggressive = false;
	m_adminBonusState.monsterMaxLpPercent = 0.0f;
	m_adminBonusState.monsterMaxEpPercent = 0.0f;
	m_adminBonusState.monsterPhysicalOffencePercent = 0.0f;
	m_adminBonusState.monsterEnergyOffencePercent = 0.0f;
	m_adminBonusState.monsterPhysicalDefencePercent = 0.0f;
	m_adminBonusState.monsterEnergyDefencePercent = 0.0f;
	m_adminBonusState.monsterAttackRatePercent = 0.0f;
	m_adminBonusState.monsterDodgeRatePercent = 0.0f;
	m_adminBonusState.killDebuffEnabled = false;
	m_adminBonusState.killDebuffSkillTblidx = INVALID_TBLIDX;
	m_adminBonusState.killDebuffDurationSeconds = 0;
	m_adminBonusState.killDebuffOverrideValues = false;
	for (int i = 0; i < NTL_MAX_EFFECT_IN_SKILL; i++)
		m_adminBonusState.killDebuffEffectValues[i] = 0.0f;
	m_adminBonusState.channelBonuses.clear();

	NTL_PRINT(PRINT_APP, "Init Timed-Event Manager");
	EventMgr* m_pEventMgr = new EventMgr;
	UNREFERENCED_PARAMETER(m_pEventMgr);

	NTL_PRINT(PRINT_APP,"INIT SERVER MANAGER");
	CSubNeighborServerInfoManager* server_manager = new CSubNeighborServerInfoManager;
	UNREFERENCED_PARAMETER(server_manager);
	g_pSrvMgr->Create(NTL_SERVER_TYPE_MASTER);

	m_pSessionFactory =  new CMasterSessionFactory;
	if( NULL == m_pSessionFactory )
	{
		return NTL_ERR_SYS_MEMORY_ALLOC_FAIL;
	}

	return NTL_SUCCESS;
}

static DWORD MakeServerKey(SERVERFARMID farmId, SERVERCHANNELID channelId)
{
	return (DWORD(farmId) << 16) | DWORD(channelId);
}

void CMasterServer::RegisterOnlineListRequest(HSESSION webHandle, DWORD requestId, DWORD page, DWORD pageSize)
{
	sWebOnlineListRequest request;
	request.webHandle = webHandle;
	request.requestId = requestId;
	request.page = page;
	request.pageSize = pageSize;
	if (request.pageSize == 0 || request.pageSize > NTL_MAX_WEB_ONLINE_PLAYERS)
		request.pageSize = NTL_MAX_WEB_ONLINE_PLAYERS;
	request.expectedServers = (DWORD)g_pSrvMgr->GetGameServerSessionCount();
	request.finishedServers = 0;
	request.finishedServerKeys.clear();
	request.players.clear();

	m_webOnlineRequests[requestId] = request;
}

void CMasterServer::HandleOnlineListResponse(const sGM_WEB_ONLINE_PLAYERS_RES* res)
{
	std::unordered_map<DWORD, sWebOnlineListRequest>::iterator it = m_webOnlineRequests.find(res->requestId);
	if (it == m_webOnlineRequests.end())
		return;

	sWebOnlineListRequest& request = it->second;
	for (WORD i = 0; i < res->count; i++)
	{
		request.players.push_back(res->players[i]);
	}

	if (res->bIsLast)
	{
		const DWORD serverKey = MakeServerKey(res->serverFarmId, res->serverChannelId);
		if (request.finishedServerKeys.insert(serverKey).second)
			request.finishedServers++;
	}

	if (request.expectedServers == 0 || request.finishedServers >= request.expectedServers)
	{
		const DWORD total = (DWORD)request.players.size();
		const DWORD pageSize = request.pageSize == 0 ? NTL_MAX_WEB_ONLINE_PLAYERS : request.pageSize;
		const DWORD offset = request.page * pageSize;

		CNtlPacket packet(sizeof(sWM_GET_ONLINE_PLAYERS_RES));
		sWM_GET_ONLINE_PLAYERS_RES* resWeb = (sWM_GET_ONLINE_PLAYERS_RES*)packet.GetPacketData();
		resWeb->wOpCode = WM_GET_ONLINE_PLAYERS_RES;
		resWeb->requestId = request.requestId;
		resWeb->totalCount = total;
		resWeb->count = 0;

		for (DWORD i = offset; i < total && resWeb->count < NTL_MAX_WEB_ONLINE_PLAYERS; i++)
		{
			resWeb->players[resWeb->count++] = request.players[i];
		}

		packet.SetPacketLen(sizeof(sWM_GET_ONLINE_PLAYERS_RES));
		Send(request.webHandle, &packet);
		m_webOnlineRequests.erase(it);
	}
}

void CMasterServer::RegisterActionRequest(HSESSION webHandle, DWORD requestId, BYTE action, DWORD expectedServers)
{
	sWebActionRequest request;
	request.webHandle = webHandle;
	request.requestId = requestId;
	request.action = action;
	request.expectedServers = expectedServers;
	request.finishedServers = 0;
	request.affectedCount = 0;
	request.anySuccess = false;

	m_webActionRequests[requestId] = request;
}

static void SendActionResponse(CMasterServer* app, const CMasterServer::sWebActionRequest& request)
{
	CNtlPacket packet(sizeof(sWM_ADMIN_ACTION_RES));
	sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packet.GetPacketData();
	res->wOpCode = WM_ADMIN_ACTION_RES;
	res->requestId = request.requestId;
	res->byAction = request.action;
	res->wResultCode = request.anySuccess ? GAME_SUCCESS : GAME_FAIL;
	res->dwAffectedCount = request.affectedCount;
	packet.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
	app->Send(request.webHandle, &packet);
}

void CMasterServer::HandleActionResponse(const sGM_WEB_SEND_NOTICE_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_KICK_PLAYER_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_GIVE_ITEM_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_GIVE_ITEM_ALL_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;
	request.affectedCount += res->dwAffectedCount;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_EXEC_GM_COMMAND_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_APPLY_BUFF_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_REMOVE_BUFF_SKILL_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_REMOVE_BUFF_EFFECT_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_CLEAR_BUFFS_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_HEAL_FULL_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_ADD_ZENI_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_GIVE_ITEM_CUSTOM_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_APPLY_BUFF_SKILL_ALL_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;
	request.affectedCount += res->dwAffectedCount;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_APPLY_BUFF_ITEM_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_APPLY_BUFF_ITEM_ALL_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;
	request.affectedCount += res->dwAffectedCount;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_REMOVE_BUFF_SKILL_ALL_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;
	request.affectedCount += res->dwAffectedCount;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_REMOVE_BUFF_EFFECT_ALL_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;
	request.affectedCount += res->dwAffectedCount;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_CLEAR_BUFFS_ALL_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;
	request.affectedCount += res->dwAffectedCount;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_SET_LEVEL_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_SET_CLASS_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_KILL_PLAYER_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_TELEPORT_PORTAL_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_TELEPORT_WORLD_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_TELEPORT_COORDS_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_MUTE_PLAYER_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_UNMUTE_PLAYER_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_TOGGLE_EXP_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_RESET_EXP_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_LEARN_SKILL_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_ADD_TITLE_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_REMOVE_TITLE_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_SET_SOLO_EXP_BONUS_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_SET_PARTY_EXP_BONUS_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_SET_QUEST_EXP_BONUS_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_SET_CRAFT_EXP_BONUS_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_SET_ZENI_DROP_BONUS_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_SET_QUEST_MONEY_BONUS_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_SET_UPGRADE_RATE_BONUS_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_SET_MONSTER_AGGRESSIVE_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_SET_MONSTER_STAT_BONUS_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_APPLY_MONSTER_BUFF_SKILL_ALL_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;
	request.affectedCount += res->dwAffectedCount;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_CLEAR_MONSTER_BUFFS_ALL_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;
	request.affectedCount += res->dwAffectedCount;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_SET_KILL_DEBUFF_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_RESET_SKILL_COOLDOWN_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_RESET_SKILL_COOLDOWN_ALL_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;
	request.affectedCount += res->dwAffectedCount;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

void CMasterServer::HandleActionResponse(const sGM_WEB_SET_CHANNEL_STAT_BONUS_RES* res)
{
	std::unordered_map<DWORD, sWebActionRequest>::iterator it = m_webActionRequests.find(res->requestId);
	if (it == m_webActionRequests.end())
		return;

	sWebActionRequest& request = it->second;
	request.finishedServers++;
	if (res->wResultCode == GAME_SUCCESS)
		request.anySuccess = true;

	if (request.finishedServers >= request.expectedServers)
	{
		SendActionResponse(this, request);
		m_webActionRequests.erase(it);
	}
}

int CMasterServer::OnAppStart()
{
	g_pSrvMgr->StartEvents();

	return NTL_SUCCESS;
}

int	CMasterServer::OnCreate()
{
	int rc = NTL_SUCCESS;

	rc = m_AuthServerAcceptor.Create(m_config.strAuthServerAcceptIP.c_str(), m_config.wAuthServerAcceptPort, 1, m_config.wAuthServerAcceptPort, SESSION_SERVER_CON_AUTH_TO_MASTER, 1, 1, 1, 1);
	if( NTL_SUCCESS != rc ) 
	{
		NTL_PRINT(PRINT_SYSTEM, "CMasterServer::OnCreate - AuthServerAcceptor.Create failed: %d(%s)", rc, NtlGetErrorMessage(rc));
		return rc;
	}
	rc = m_network.Associate( &m_AuthServerAcceptor, true );
	if( NTL_SUCCESS != rc )
	{
		NTL_PRINT(PRINT_SYSTEM, "CMasterServer::OnCreate - Associate AuthServerAcceptor failed: %d(%s)", rc, NtlGetErrorMessage(rc));
		return rc;
	}
	rc = m_CharServerAcceptor.Create(m_config.strCharServerAcceptIP.c_str(), m_config.wCharServerAcceptPort, 1, m_config.wCharServerAcceptPort, SESSION_SERVER_CON_CHAR_TO_MASTER, 10, 10, 10, 10);
	if( NTL_SUCCESS != rc ) 
	{
		NTL_PRINT(PRINT_SYSTEM, "CMasterServer::OnCreate - CharServerAcceptor.Create failed: %d(%s)", rc, NtlGetErrorMessage(rc));
		return rc;
	}
	rc = m_network.Associate( &m_CharServerAcceptor, true );
	if( NTL_SUCCESS != rc )
	{
		NTL_PRINT(PRINT_SYSTEM, "CMasterServer::OnCreate - Associate CharServerAcceptor failed: %d(%s)", rc, NtlGetErrorMessage(rc));
		return rc;
	}
	rc = m_ChatServerAcceptor.Create(m_config.strChatServerAcceptIP.c_str(), m_config.wChatServerAcceptPort, 1, m_config.wChatServerAcceptPort, SESSION_SERVER_CON_CHAT_TO_MASTER, 5, 5, 2, 5);
	if( NTL_SUCCESS != rc ) 
	{
		NTL_PRINT(PRINT_SYSTEM, "CMasterServer::OnCreate - ChatServerAcceptor.Create failed: %d(%s)", rc, NtlGetErrorMessage(rc));
		return rc;
	}
	rc = m_network.Associate( &m_ChatServerAcceptor, true );
	if( NTL_SUCCESS != rc )
	{
		NTL_PRINT(PRINT_SYSTEM, "CMasterServer::OnCreate - Associate ChatServerAcceptor failed: %d(%s)", rc, NtlGetErrorMessage(rc));
		return rc;
	}
	rc = m_GameServerAcceptor.Create(m_config.strGameServerAcceptIP.c_str(), m_config.wGameServerAcceptPort, 1, m_config.wGameServerAcceptPort, SESSION_SERVER_CON_GAME_TO_MASTER, 10, 10, 10, 10);
	if( NTL_SUCCESS != rc ) 
	{
		NTL_PRINT(PRINT_SYSTEM, "CMasterServer::OnCreate - GameServerAcceptor.Create failed: %d(%s)", rc, NtlGetErrorMessage(rc));
		return rc;
	}
	rc = m_network.Associate( &m_GameServerAcceptor, true );
	if( NTL_SUCCESS != rc )
	{
		NTL_PRINT(PRINT_SYSTEM, "CMasterServer::OnCreate - Associate GameServerAcceptor failed: %d(%s)", rc, NtlGetErrorMessage(rc));
		return rc;
	}

	rc = m_WebServerAcceptor.Create(m_config.strWebServerAcceptIP.c_str(), m_config.wWebServerAcceptPort, 1, m_config.wWebServerAcceptPort, SESSION_SERVER_CON_WEB, 1, 1, 1, 1);
	if (NTL_SUCCESS != rc)
	{
		NTL_PRINT(PRINT_SYSTEM, "CMasterServer::OnCreate - WebServerAcceptor.Create failed: %d(%s)", rc, NtlGetErrorMessage(rc));
		return rc;
	}
	rc = m_network.Associate(&m_WebServerAcceptor, true);
	if (NTL_SUCCESS != rc)
	{
		NTL_PRINT(PRINT_SYSTEM, "CMasterServer::OnCreate - Associate WebServerAcceptor failed: %d(%s)", rc, NtlGetErrorMessage(rc));
		return rc;
	}

	return NTL_SUCCESS;

}

void	CMasterServer::OnDestroy()
{
}


void	CMasterServer::Run()
{
	DWORD m_dwTickCount, dwLastLoop = 0;
	DWORD m_dwLastTimeGameMainUpdated = GetTickCount();

	while (IsRunnable())
	{
		m_dwTickCount = GetTickCount();

		if (dwLastLoop && m_dwTickCount - dwLastLoop > 1000)
		{
			NTL_PRINT(PRINT_APP, "m_dwTickCount - dwLastLoop %u > 1000", m_dwTickCount - dwLastLoop);
			ERR_LOG(LOG_GENERAL, "m_dwTickCount - dwLastLoop %u > 1000", m_dwTickCount - dwLastLoop);
		}

		if (m_dwTickCount - m_dwLastTimeGameMainUpdated >= 1000) //update events every 1000 second
		{
			DWORD dwTickDiff = m_dwTickCount - m_dwLastTimeGameMainUpdated;

			g_pSrvMgr->TickProcess(dwTickDiff);

			m_dwLastTimeGameMainUpdated = m_dwTickCount;
		}

		dwLastLoop = GetTickCount();
		Wait(1);
	}
}



//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
BOOL CMasterServer::OnCommandInput(std::string& sCmd)
{
	if (sCmd == "printplayers")
	{
		g_pSrvMgr->PrintOnlinePlayers();
	}
	else if (sCmd == "playercount")
	{
		NTL_PRINT(PRINT_APP,"Players in CharServers: %u, Players in GameServers: %u", g_pSrvMgr->GetPlayerInCharServer(), g_pSrvMgr->GetPlayerInGameServer());
	}

	return TRUE;
}


int main(int argc, _TCHAR* argv[])
{
	CMasterServer app;
	CNtlFileStream traceFileStream;

	SYSTEMTIME ti;
	GetLocalTime( &ti );

#if defined(_WIN32)
	SetConsoleTitle( TEXT("DBOD MASTER") );
#endif

	int rc = app.Create(argc, argv, (argc > 1) ? argv[1] : "./config/MasterServer.ini");

	if( NTL_SUCCESS != rc )
	{
		printf("Server Application Create Fail %d(%s)\n", rc, NtlGetErrorMessage(rc) );
		Sleep(20000);
		return rc;
	}
	
	// LOG FILE
	char m_LogFile[256];
	sprintf(m_LogFile, "./logs/masterserver/log_%02u-%02u-%02u.txt", ti.wYear, ti.wMonth, ti.wDay);

	// Create log directory if it doesn't exist
#if !defined(_WIN32)
	// Try to create directories, ignore errors if they already exist
	mkdir("./logs", 0755);
	mkdir("./logs/masterserver", 0755);
#endif

	rc = traceFileStream.Create(m_LogFile);
	if (NTL_SUCCESS != rc)
	{
		NTL_PRINT(PRINT_APP, "Failed to create log file: %s (error: %d) - continuing without log file", m_LogFile, rc);
		// Don't return - continue without log file
	}
	else
	{
		NTL_PRINT(PRINT_APP, "Log file created: %s", m_LogFile);
		app.m_log.AttachLogStream(traceFileStream.GetFilePtr());
		NtlSetPrintFlag(PRINT_APP | PRINT_SYSTEM);
	}

	app.Start();
	NTL_PRINT(PRINT_APP, "MASTER SERVER STARTED");

	app.WaitCommandInput();
	app.WaitForTerminate();
	return 0;
}
