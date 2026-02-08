#include "stdafx.h"
#include "MasterServer.h"
#include "../MasterServer/PacketHead.h"
#include "NtlPacketSYS.h"
#include "NtlResultCode.h"
#include <time.h>
#include <map>
#include <vector>
#include <unordered_map>

static void FillWebServerEntry(sWEB_SERVER_ENTRY& out, sDBO_SERVER_INFO* info, BYTE serverTypeOverride)
{
	ZeroMemory(&out, sizeof(out));
	if (!info)
		return;

	out.byServerType = serverTypeOverride;
	out.serverFarmId = info->serverFarmId;
	out.serverChannelId = info->byServerChannelIndex;
	out.serverIndex = info->byServerIndex;
	out.bIsOn = info->bIsOn;
	out.byRunningState = info->byRunningState;
	out.dwLoad = info->dwLoad;
	out.dwMaxLoad = info->dwMaxLoad;
	out.wPortForClient = info->wPortForClient;
	NTL_STRNCPY_S_FULL(out.achPublicAddress, NTL_MAX_LENGTH_OF_IP + 1, info->achPublicAddress);
}


int CWebSession::OnAccept()
{
	NTL_PRINT(PRINT_APP, "CONNECTION FROM WEB-SERVER ACCEPTED");
	return CNtlSession::OnAccept();
}


void CWebSession::OnClose()
{
	NTL_PRINT(PRINT_APP, "CONNECTION FROM WEB-SERVER CLOSED");
}


int CWebSession::OnDispatch(CNtlPacket * pPacket)
{
	CMasterServer * app = (CMasterServer*)g_pApp;
	sNTLPACKETHEADER * pHeader = (sNTLPACKETHEADER *)pPacket->GetPacketData();

	printf("Web Server | pHeader->wOpCode %d \n", pHeader->wOpCode);

	switch (pHeader->wOpCode)
	{
		case SYS_ALIVE:
		{
			ResetAliveTime();
			return NTL_SUCCESS;
		}
		break;

		case WM_GET_PLAYER_COUNT:
		{
			CNtlPacket packet(sizeof(sWM_GET_PLAYER_COUNT_RES));
			sWM_GET_PLAYER_COUNT_RES * res = (sWM_GET_PLAYER_COUNT_RES *)packet.GetPacketData();
			res->wOpCode = WM_GET_PLAYER_COUNT_RES;
			res->chPlayerCount = g_pSrvMgr->GetPlayerInGameServer();
			app->Send(GetHandle(), &packet);
		}
		break;

		case WM_GET_CHANNEL_STATUS:
		{
			CNtlPacket packet(sizeof(sWM_GET_CHANNEL_STATUS_RES));
			sWM_GET_CHANNEL_STATUS_RES * res = (sWM_GET_CHANNEL_STATUS_RES *)packet.GetPacketData();
			res->wOpCode = WM_GET_CHANNEL_STATUS_RES;

			res->auth		= DBO_SERVER_STATUS_DOWN;
			res->chat		= DBO_SERVER_STATUS_DOWN;
			res->channel0	= DBO_SERVER_STATUS_DOWN;
			res->channel1	= DBO_SERVER_STATUS_DOWN;
			res->channel2	= DBO_SERVER_STATUS_DOWN;
			res->channel3	= DBO_SERVER_STATUS_DOWN;
			res->channel4	= DBO_SERVER_STATUS_DOWN;
			res->channel5	= DBO_SERVER_STATUS_DOWN;
			res->channel6	= DBO_SERVER_STATUS_DOWN;
			res->channel7	= DBO_SERVER_STATUS_DOWN;
			res->channel8	= DBO_SERVER_STATUS_DOWN;
			res->channel9	= DBO_SERVER_STATUS_DOWN;

			sDBO_SERVER_INFO* pAuth = g_pSrvMgr->GetServerInfo(NTL_SERVER_TYPE_AUTH, 0, 0, 0);
			if (pAuth)
			{
				if (pAuth->bIsOn)
					res->auth = DBO_SERVER_STATUS_UP; 
			}

			sDBO_SERVER_INFO* pChat = g_pSrvMgr->GetServerInfo(NTL_SERVER_TYPE_COMMUNITY, 0, 0, 0);
			if (pChat)
			{
				if(pChat->bIsOn)
					res->chat = DBO_SERVER_STATUS_UP;
			}

			sSERVER_FARM_INFO* pFarm = g_pSrvMgr->GetServerFarmInfo(0);
			if (pFarm)
			{
				for (std::map<SERVERCHANNELID, sSERVER_CHANNEL_INFO*>::iterator it = pFarm->mapGameServerChannelInfo.begin(); it != pFarm->mapGameServerChannelInfo.end(); it++)
				{
					sSERVER_CHANNEL_INFO* pChannel = it->second;

					switch (pChannel->byServerChannelIndex)
					{
						case 0: res->channel0 = pChannel->byServerStatus; break;
						case 1: res->channel1 = pChannel->byServerStatus; break;
						case 2: res->channel2 = pChannel->byServerStatus; break;
						case 3: res->channel3 = pChannel->byServerStatus; break;
						case 4: res->channel4 = pChannel->byServerStatus; break;
						case 5: res->channel5 = pChannel->byServerStatus; break;
						case 6: res->channel6 = pChannel->byServerStatus; break;
						case 7: res->channel7 = pChannel->byServerStatus; break;
						case 8: res->channel8 = pChannel->byServerStatus; break;
						case 9: res->channel9 = pChannel->byServerStatus; break;
					}
					printf("Web Server | channel index %u, channel status %u \n", pChannel->byServerChannelIndex, pChannel->byServerStatus);
				}
			}

			
			
			app->Send(GetHandle(), &packet);
		}
		break;

		case WM_GET_ONLINE_PLAYERS:
		{
			sWM_GET_ONLINE_PLAYERS* req = (sWM_GET_ONLINE_PLAYERS*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterOnlineListRequest(GetHandle(), req->requestId, req->page, req->pageSize);

			if (serverCount == 0)
			{
				CNtlPacket packet(sizeof(sWM_GET_ONLINE_PLAYERS_RES));
				sWM_GET_ONLINE_PLAYERS_RES* res = (sWM_GET_ONLINE_PLAYERS_RES*)packet.GetPacketData();
				res->wOpCode = WM_GET_ONLINE_PLAYERS_RES;
				res->requestId = req->requestId;
				res->totalCount = 0;
				res->count = 0;
				packet.SetPacketLen(sizeof(sWM_GET_ONLINE_PLAYERS_RES));
				app->Send(GetHandle(), &packet);
				app->m_webOnlineRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_ONLINE_PLAYERS_REQ));
			sMG_WEB_ONLINE_PLAYERS_REQ* res = (sMG_WEB_ONLINE_PLAYERS_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_ONLINE_PLAYERS_REQ;
			res->requestId = req->requestId;
			packet.SetPacketLen(sizeof(sMG_WEB_ONLINE_PLAYERS_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_SEND_NOTICE:
		{
			sWM_SEND_NOTICE* req = (sWM_SEND_NOTICE*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();
			WORD messageLength = req->wMessageLength;
			if (messageLength > NTL_MAX_LENGTH_OF_CHAT_MESSAGE_UNICODE)
				messageLength = NTL_MAX_LENGTH_OF_CHAT_MESSAGE_UNICODE;

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_SEND_NOTICE, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_SEND_NOTICE;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_SEND_NOTICE_REQ));
			sMG_WEB_SEND_NOTICE_REQ* res = (sMG_WEB_SEND_NOTICE_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_SEND_NOTICE_REQ;
			res->requestId = req->requestId;
			res->bIsChannel = req->bIsChannel;
			res->serverChannelId = req->serverChannelId;
			res->wMessageLength = messageLength;
			NTL_SAFE_WCSNCPY_SIZEINPUT(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE_UNICODE + 1, req->awchMessage, messageLength);
			packet.SetPacketLen(sizeof(sMG_WEB_SEND_NOTICE_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_KICK_PLAYER:
		{
			sWM_KICK_PLAYER* req = (sWM_KICK_PLAYER*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_KICK_PLAYER, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_KICK_PLAYER;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_KICK_PLAYER_REQ));
			sMG_WEB_KICK_PLAYER_REQ* res = (sMG_WEB_KICK_PLAYER_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_KICK_PLAYER_REQ;
			res->requestId = req->requestId;
			res->byTargetType = req->byTargetType;
			res->qwTargetId = req->qwTargetId;
			packet.SetPacketLen(sizeof(sMG_WEB_KICK_PLAYER_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_GIVE_ITEM:
		{
			sWM_GIVE_ITEM* req = (sWM_GIVE_ITEM*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_GIVE_ITEM, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_GIVE_ITEM;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_GIVE_ITEM_REQ));
			sMG_WEB_GIVE_ITEM_REQ* res = (sMG_WEB_GIVE_ITEM_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_GIVE_ITEM_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			res->itemTblidx = req->itemTblidx;
			res->byCount = req->byCount;
			packet.SetPacketLen(sizeof(sMG_WEB_GIVE_ITEM_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_GIVE_ITEM_ALL:
		{
			sWM_GIVE_ITEM_ALL* req = (sWM_GIVE_ITEM_ALL*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_GIVE_ITEM_ALL, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_GIVE_ITEM_ALL;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_GIVE_ITEM_ALL_REQ));
			sMG_WEB_GIVE_ITEM_ALL_REQ* res = (sMG_WEB_GIVE_ITEM_ALL_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_GIVE_ITEM_ALL_REQ;
			res->requestId = req->requestId;
			res->itemTblidx = req->itemTblidx;
			res->byCount = req->byCount;
			packet.SetPacketLen(sizeof(sMG_WEB_GIVE_ITEM_ALL_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_EXEC_GM_COMMAND:
		{
			sWM_EXEC_GM_COMMAND* req = (sWM_EXEC_GM_COMMAND*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();
			WORD commandLength = req->wCommandLen;
			if (commandLength > NTL_MAX_LENGTH_OF_CHAT_MESSAGE_UNICODE)
				commandLength = NTL_MAX_LENGTH_OF_CHAT_MESSAGE_UNICODE;

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_EXEC_GM_COMMAND, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_EXEC_GM_COMMAND;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_EXEC_GM_COMMAND_REQ));
			sMG_WEB_EXEC_GM_COMMAND_REQ* res = (sMG_WEB_EXEC_GM_COMMAND_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_EXEC_GM_COMMAND_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			res->wCommandLen = commandLength;
			NTL_SAFE_WCSNCPY_SIZEINPUT(res->awchCommand, NTL_MAX_LENGTH_OF_CHAT_MESSAGE_UNICODE + 1, req->awchCommand, commandLength);
			packet.SetPacketLen(sizeof(sMG_WEB_EXEC_GM_COMMAND_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_APPLY_BUFF:
		{
			sWM_APPLY_BUFF* req = (sWM_APPLY_BUFF*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_APPLY_BUFF, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_APPLY_BUFF;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_APPLY_BUFF_REQ));
			sMG_WEB_APPLY_BUFF_REQ* res = (sMG_WEB_APPLY_BUFF_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_APPLY_BUFF_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			res->skillTblidx = req->skillTblidx;
			res->durationSeconds = req->durationSeconds;
			res->bOverrideValues = req->bOverrideValues;
			memcpy(res->aEffectValues, req->aEffectValues, sizeof(res->aEffectValues));
			packet.SetPacketLen(sizeof(sMG_WEB_APPLY_BUFF_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_REMOVE_BUFF_SKILL:
		{
			sWM_REMOVE_BUFF_SKILL* req = (sWM_REMOVE_BUFF_SKILL*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_REMOVE_BUFF_SKILL, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_REMOVE_BUFF_SKILL;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_REMOVE_BUFF_SKILL_REQ));
			sMG_WEB_REMOVE_BUFF_SKILL_REQ* res = (sMG_WEB_REMOVE_BUFF_SKILL_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_REMOVE_BUFF_SKILL_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			res->skillTblidx = req->skillTblidx;
			packet.SetPacketLen(sizeof(sMG_WEB_REMOVE_BUFF_SKILL_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_REMOVE_BUFF_EFFECT:
		{
			sWM_REMOVE_BUFF_EFFECT* req = (sWM_REMOVE_BUFF_EFFECT*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_REMOVE_BUFF_EFFECT, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_REMOVE_BUFF_EFFECT;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_REMOVE_BUFF_EFFECT_REQ));
			sMG_WEB_REMOVE_BUFF_EFFECT_REQ* res = (sMG_WEB_REMOVE_BUFF_EFFECT_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_REMOVE_BUFF_EFFECT_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			res->effectCode = req->effectCode;
			packet.SetPacketLen(sizeof(sMG_WEB_REMOVE_BUFF_EFFECT_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_CLEAR_BUFFS:
		{
			sWM_CLEAR_BUFFS* req = (sWM_CLEAR_BUFFS*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_CLEAR_BUFFS, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_CLEAR_BUFFS;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_CLEAR_BUFFS_REQ));
			sMG_WEB_CLEAR_BUFFS_REQ* res = (sMG_WEB_CLEAR_BUFFS_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_CLEAR_BUFFS_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			packet.SetPacketLen(sizeof(sMG_WEB_CLEAR_BUFFS_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_HEAL_FULL:
		{
			sWM_HEAL_FULL* req = (sWM_HEAL_FULL*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_HEAL_FULL, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_HEAL_FULL;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_HEAL_FULL_REQ));
			sMG_WEB_HEAL_FULL_REQ* res = (sMG_WEB_HEAL_FULL_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_HEAL_FULL_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			packet.SetPacketLen(sizeof(sMG_WEB_HEAL_FULL_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_ADD_ZENI:
		{
			sWM_ADD_ZENI* req = (sWM_ADD_ZENI*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_ADD_ZENI, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_ADD_ZENI;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_ADD_ZENI_REQ));
			sMG_WEB_ADD_ZENI_REQ* res = (sMG_WEB_ADD_ZENI_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_ADD_ZENI_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			res->amount = req->amount;
			res->bAdd = req->bAdd;
			packet.SetPacketLen(sizeof(sMG_WEB_ADD_ZENI_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_GIVE_ITEM_CUSTOM:
		{
			sWM_GIVE_ITEM_CUSTOM* req = (sWM_GIVE_ITEM_CUSTOM*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_GIVE_ITEM_CUSTOM, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_GIVE_ITEM_CUSTOM;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_GIVE_ITEM_CUSTOM_REQ));
			sMG_WEB_GIVE_ITEM_CUSTOM_REQ* res = (sMG_WEB_GIVE_ITEM_CUSTOM_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_GIVE_ITEM_CUSTOM_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			res->itemTblidx = req->itemTblidx;
			res->byCount = req->byCount;
			res->byRank = req->byRank;
			res->byGrade = req->byGrade;
			res->byDurability = req->byDurability;
			res->byBattleAttribute = req->byBattleAttribute;
			res->byRestrictState = req->byRestrictState;
			res->byNeedIdentify = req->byNeedIdentify;
			res->byDurationType = req->byDurationType;
			res->durationSeconds = req->durationSeconds;
			res->bUseDefaultOptions = req->bUseDefaultOptions;
			res->bEnchantAble = req->bEnchantAble;
			memcpy(res->aOptionTblidx, req->aOptionTblidx, sizeof(res->aOptionTblidx));
			memcpy(res->aRandomOption, req->aRandomOption, sizeof(res->aRandomOption));
			NTL_SAFE_WCSCPY(res->awchMaker, req->awchMaker);
			packet.SetPacketLen(sizeof(sMG_WEB_GIVE_ITEM_CUSTOM_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_APPLY_BUFF_SKILL:
		{
			sWM_APPLY_BUFF_SKILL* req = (sWM_APPLY_BUFF_SKILL*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_APPLY_BUFF, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_APPLY_BUFF;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_APPLY_BUFF_REQ));
			sMG_WEB_APPLY_BUFF_REQ* res = (sMG_WEB_APPLY_BUFF_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_APPLY_BUFF_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			res->skillTblidx = req->skillTblidx;
			res->durationSeconds = req->durationSeconds;
			res->bOverrideValues = req->bOverrideValues;
			memcpy(res->aEffectValues, req->aEffectValues, sizeof(res->aEffectValues));
			packet.SetPacketLen(sizeof(sMG_WEB_APPLY_BUFF_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_APPLY_BUFF_SKILL_ALL:
		{
			sWM_APPLY_BUFF_SKILL_ALL* req = (sWM_APPLY_BUFF_SKILL_ALL*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_APPLY_BUFF_SKILL_ALL, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_APPLY_BUFF_SKILL_ALL;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_APPLY_BUFF_SKILL_ALL_REQ));
			sMG_WEB_APPLY_BUFF_SKILL_ALL_REQ* res = (sMG_WEB_APPLY_BUFF_SKILL_ALL_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_APPLY_BUFF_SKILL_ALL_REQ;
			res->requestId = req->requestId;
			res->skillTblidx = req->skillTblidx;
			res->durationSeconds = req->durationSeconds;
			res->bOverrideValues = req->bOverrideValues;
			memcpy(res->aEffectValues, req->aEffectValues, sizeof(res->aEffectValues));
			packet.SetPacketLen(sizeof(sMG_WEB_APPLY_BUFF_SKILL_ALL_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_APPLY_BUFF_ITEM:
		{
			sWM_APPLY_BUFF_ITEM* req = (sWM_APPLY_BUFF_ITEM*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_APPLY_BUFF_ITEM, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_APPLY_BUFF_ITEM;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_APPLY_BUFF_ITEM_REQ));
			sMG_WEB_APPLY_BUFF_ITEM_REQ* res = (sMG_WEB_APPLY_BUFF_ITEM_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_APPLY_BUFF_ITEM_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			res->useItemTblidx = req->useItemTblidx;
			res->durationSeconds = req->durationSeconds;
			res->bOverrideValues = req->bOverrideValues;
			memcpy(res->aEffectValues, req->aEffectValues, sizeof(res->aEffectValues));
			packet.SetPacketLen(sizeof(sMG_WEB_APPLY_BUFF_ITEM_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_APPLY_BUFF_ITEM_ALL:
		{
			sWM_APPLY_BUFF_ITEM_ALL* req = (sWM_APPLY_BUFF_ITEM_ALL*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_APPLY_BUFF_ITEM_ALL, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_APPLY_BUFF_ITEM_ALL;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_APPLY_BUFF_ITEM_ALL_REQ));
			sMG_WEB_APPLY_BUFF_ITEM_ALL_REQ* res = (sMG_WEB_APPLY_BUFF_ITEM_ALL_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_APPLY_BUFF_ITEM_ALL_REQ;
			res->requestId = req->requestId;
			res->useItemTblidx = req->useItemTblidx;
			res->durationSeconds = req->durationSeconds;
			res->bOverrideValues = req->bOverrideValues;
			memcpy(res->aEffectValues, req->aEffectValues, sizeof(res->aEffectValues));
			packet.SetPacketLen(sizeof(sMG_WEB_APPLY_BUFF_ITEM_ALL_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_REMOVE_BUFF_SKILL_ALL:
		{
			sWM_REMOVE_BUFF_SKILL_ALL* req = (sWM_REMOVE_BUFF_SKILL_ALL*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_REMOVE_BUFF_SKILL_ALL, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_REMOVE_BUFF_SKILL_ALL;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_REMOVE_BUFF_SKILL_ALL_REQ));
			sMG_WEB_REMOVE_BUFF_SKILL_ALL_REQ* res = (sMG_WEB_REMOVE_BUFF_SKILL_ALL_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_REMOVE_BUFF_SKILL_ALL_REQ;
			res->requestId = req->requestId;
			res->skillTblidx = req->skillTblidx;
			packet.SetPacketLen(sizeof(sMG_WEB_REMOVE_BUFF_SKILL_ALL_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_REMOVE_BUFF_EFFECT_ALL:
		{
			sWM_REMOVE_BUFF_EFFECT_ALL* req = (sWM_REMOVE_BUFF_EFFECT_ALL*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_REMOVE_BUFF_EFFECT_ALL, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_REMOVE_BUFF_EFFECT_ALL;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_REMOVE_BUFF_EFFECT_ALL_REQ));
			sMG_WEB_REMOVE_BUFF_EFFECT_ALL_REQ* res = (sMG_WEB_REMOVE_BUFF_EFFECT_ALL_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_REMOVE_BUFF_EFFECT_ALL_REQ;
			res->requestId = req->requestId;
			res->effectCode = req->effectCode;
			packet.SetPacketLen(sizeof(sMG_WEB_REMOVE_BUFF_EFFECT_ALL_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_CLEAR_BUFFS_ALL:
		{
			sWM_CLEAR_BUFFS_ALL* req = (sWM_CLEAR_BUFFS_ALL*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_CLEAR_BUFFS_ALL, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_CLEAR_BUFFS_ALL;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_CLEAR_BUFFS_ALL_REQ));
			sMG_WEB_CLEAR_BUFFS_ALL_REQ* res = (sMG_WEB_CLEAR_BUFFS_ALL_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_CLEAR_BUFFS_ALL_REQ;
			res->requestId = req->requestId;
			packet.SetPacketLen(sizeof(sMG_WEB_CLEAR_BUFFS_ALL_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_SET_LEVEL:
		{
			sWM_SET_LEVEL* req = (sWM_SET_LEVEL*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_SET_LEVEL, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_SET_LEVEL;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_SET_LEVEL_REQ));
			sMG_WEB_SET_LEVEL_REQ* res = (sMG_WEB_SET_LEVEL_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_SET_LEVEL_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			res->byLevel = req->byLevel;
			packet.SetPacketLen(sizeof(sMG_WEB_SET_LEVEL_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_SET_CLASS:
		{
			sWM_SET_CLASS* req = (sWM_SET_CLASS*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_SET_CLASS, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_SET_CLASS;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_SET_CLASS_REQ));
			sMG_WEB_SET_CLASS_REQ* res = (sMG_WEB_SET_CLASS_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_SET_CLASS_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			res->byClass = req->byClass;
			packet.SetPacketLen(sizeof(sMG_WEB_SET_CLASS_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_KILL_PLAYER:
		{
			sWM_KILL_PLAYER* req = (sWM_KILL_PLAYER*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_KILL_PLAYER, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_KILL_PLAYER;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_KILL_PLAYER_REQ));
			sMG_WEB_KILL_PLAYER_REQ* res = (sMG_WEB_KILL_PLAYER_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_KILL_PLAYER_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			packet.SetPacketLen(sizeof(sMG_WEB_KILL_PLAYER_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_TELEPORT_PORTAL:
		{
			sWM_TELEPORT_PORTAL* req = (sWM_TELEPORT_PORTAL*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_TELEPORT_PORTAL, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_TELEPORT_PORTAL;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_TELEPORT_PORTAL_REQ));
			sMG_WEB_TELEPORT_PORTAL_REQ* res = (sMG_WEB_TELEPORT_PORTAL_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_TELEPORT_PORTAL_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			res->portalTblidx = req->portalTblidx;
			packet.SetPacketLen(sizeof(sMG_WEB_TELEPORT_PORTAL_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_TELEPORT_WORLD:
		{
			sWM_TELEPORT_WORLD* req = (sWM_TELEPORT_WORLD*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_TELEPORT_WORLD, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_TELEPORT_WORLD;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_TELEPORT_WORLD_REQ));
			sMG_WEB_TELEPORT_WORLD_REQ* res = (sMG_WEB_TELEPORT_WORLD_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_TELEPORT_WORLD_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			res->worldId = req->worldId;
			packet.SetPacketLen(sizeof(sMG_WEB_TELEPORT_WORLD_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_TELEPORT_COORDS:
		{
			sWM_TELEPORT_COORDS* req = (sWM_TELEPORT_COORDS*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_TELEPORT_COORDS, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_TELEPORT_COORDS;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_TELEPORT_COORDS_REQ));
			sMG_WEB_TELEPORT_COORDS_REQ* res = (sMG_WEB_TELEPORT_COORDS_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_TELEPORT_COORDS_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			res->worldId = req->worldId;
			res->x = req->x;
			res->y = req->y;
			res->z = req->z;
			packet.SetPacketLen(sizeof(sMG_WEB_TELEPORT_COORDS_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_MUTE_PLAYER:
		{
			sWM_MUTE_PLAYER* req = (sWM_MUTE_PLAYER*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();
			WORD reasonLength = req->wReasonLength;
			if (reasonLength > NTL_MAX_LENGTH_OF_MAIL_MESSAGE)
				reasonLength = NTL_MAX_LENGTH_OF_MAIL_MESSAGE;

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_MUTE_PLAYER, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_MUTE_PLAYER;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_MUTE_PLAYER_REQ));
			sMG_WEB_MUTE_PLAYER_REQ* res = (sMG_WEB_MUTE_PLAYER_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_MUTE_PLAYER_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			res->durationMinutes = req->durationMinutes;
			res->wReasonLength = reasonLength;
			NTL_SAFE_WCSNCPY_SIZEINPUT(res->awchReason, NTL_MAX_LENGTH_OF_MAIL_MESSAGE + 1, req->awchReason, reasonLength);
			packet.SetPacketLen(sizeof(sMG_WEB_MUTE_PLAYER_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_UNMUTE_PLAYER:
		{
			sWM_UNMUTE_PLAYER* req = (sWM_UNMUTE_PLAYER*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_UNMUTE_PLAYER, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_UNMUTE_PLAYER;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_UNMUTE_PLAYER_REQ));
			sMG_WEB_UNMUTE_PLAYER_REQ* res = (sMG_WEB_UNMUTE_PLAYER_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_UNMUTE_PLAYER_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			packet.SetPacketLen(sizeof(sMG_WEB_UNMUTE_PLAYER_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_TOGGLE_EXP:
		{
			sWM_TOGGLE_EXP* req = (sWM_TOGGLE_EXP*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_TOGGLE_EXP, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_TOGGLE_EXP;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_TOGGLE_EXP_REQ));
			sMG_WEB_TOGGLE_EXP_REQ* res = (sMG_WEB_TOGGLE_EXP_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_TOGGLE_EXP_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			res->bEnable = req->bEnable;
			packet.SetPacketLen(sizeof(sMG_WEB_TOGGLE_EXP_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_RESET_EXP:
		{
			sWM_RESET_EXP* req = (sWM_RESET_EXP*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_RESET_EXP, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_RESET_EXP;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_RESET_EXP_REQ));
			sMG_WEB_RESET_EXP_REQ* res = (sMG_WEB_RESET_EXP_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_RESET_EXP_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			packet.SetPacketLen(sizeof(sMG_WEB_RESET_EXP_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_LEARN_SKILL:
		{
			sWM_LEARN_SKILL* req = (sWM_LEARN_SKILL*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_LEARN_SKILL, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_LEARN_SKILL;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_LEARN_SKILL_REQ));
			sMG_WEB_LEARN_SKILL_REQ* res = (sMG_WEB_LEARN_SKILL_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_LEARN_SKILL_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			res->skillTblidx = req->skillTblidx;
			packet.SetPacketLen(sizeof(sMG_WEB_LEARN_SKILL_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_ADD_TITLE:
		{
			sWM_ADD_TITLE* req = (sWM_ADD_TITLE*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_ADD_TITLE, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_ADD_TITLE;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_ADD_TITLE_REQ));
			sMG_WEB_ADD_TITLE_REQ* res = (sMG_WEB_ADD_TITLE_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_ADD_TITLE_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			res->titleTblidx = req->titleTblidx;
			packet.SetPacketLen(sizeof(sMG_WEB_ADD_TITLE_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_REMOVE_TITLE:
		{
			sWM_REMOVE_TITLE* req = (sWM_REMOVE_TITLE*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_REMOVE_TITLE, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_REMOVE_TITLE;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_REMOVE_TITLE_REQ));
			sMG_WEB_REMOVE_TITLE_REQ* res = (sMG_WEB_REMOVE_TITLE_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_REMOVE_TITLE_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			res->titleTblidx = req->titleTblidx;
			packet.SetPacketLen(sizeof(sMG_WEB_REMOVE_TITLE_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_SET_SOLO_EXP_BONUS:
		{
			sWM_SET_SOLO_EXP_BONUS* req = (sWM_SET_SOLO_EXP_BONUS*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();
			app->m_adminBonusState.soloExpBonus = req->bonusPercent;

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_SET_SOLO_EXP_BONUS, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_SET_SOLO_EXP_BONUS;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_SET_SOLO_EXP_BONUS_REQ));
			sMG_WEB_SET_SOLO_EXP_BONUS_REQ* res = (sMG_WEB_SET_SOLO_EXP_BONUS_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_SET_SOLO_EXP_BONUS_REQ;
			res->requestId = req->requestId;
			res->bonusPercent = req->bonusPercent;
			packet.SetPacketLen(sizeof(sMG_WEB_SET_SOLO_EXP_BONUS_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_SET_PARTY_EXP_BONUS:
		{
			sWM_SET_PARTY_EXP_BONUS* req = (sWM_SET_PARTY_EXP_BONUS*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();
			app->m_adminBonusState.partyExpBonus = req->bonusPercent;

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_SET_PARTY_EXP_BONUS, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_SET_PARTY_EXP_BONUS;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_SET_PARTY_EXP_BONUS_REQ));
			sMG_WEB_SET_PARTY_EXP_BONUS_REQ* res = (sMG_WEB_SET_PARTY_EXP_BONUS_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_SET_PARTY_EXP_BONUS_REQ;
			res->requestId = req->requestId;
			res->bonusPercent = req->bonusPercent;
			packet.SetPacketLen(sizeof(sMG_WEB_SET_PARTY_EXP_BONUS_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_SET_QUEST_EXP_BONUS:
		{
			sWM_SET_QUEST_EXP_BONUS* req = (sWM_SET_QUEST_EXP_BONUS*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();
			app->m_adminBonusState.questExpBonus = req->bonusPercent;

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_SET_QUEST_EXP_BONUS, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_SET_QUEST_EXP_BONUS;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_SET_QUEST_EXP_BONUS_REQ));
			sMG_WEB_SET_QUEST_EXP_BONUS_REQ* res = (sMG_WEB_SET_QUEST_EXP_BONUS_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_SET_QUEST_EXP_BONUS_REQ;
			res->requestId = req->requestId;
			res->bonusPercent = req->bonusPercent;
			packet.SetPacketLen(sizeof(sMG_WEB_SET_QUEST_EXP_BONUS_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_SET_CRAFT_EXP_BONUS:
		{
			sWM_SET_CRAFT_EXP_BONUS* req = (sWM_SET_CRAFT_EXP_BONUS*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();
			app->m_adminBonusState.craftExpBonus = req->bonusPercent;

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_SET_CRAFT_EXP_BONUS, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_SET_CRAFT_EXP_BONUS;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_SET_CRAFT_EXP_BONUS_REQ));
			sMG_WEB_SET_CRAFT_EXP_BONUS_REQ* res = (sMG_WEB_SET_CRAFT_EXP_BONUS_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_SET_CRAFT_EXP_BONUS_REQ;
			res->requestId = req->requestId;
			res->bonusPercent = req->bonusPercent;
			packet.SetPacketLen(sizeof(sMG_WEB_SET_CRAFT_EXP_BONUS_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_SET_ZENI_DROP_BONUS:
		{
			sWM_SET_ZENI_DROP_BONUS* req = (sWM_SET_ZENI_DROP_BONUS*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();
			app->m_adminBonusState.zeniDropBonus = req->bonusPercent;

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_SET_ZENI_DROP_BONUS, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_SET_ZENI_DROP_BONUS;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_SET_ZENI_DROP_BONUS_REQ));
			sMG_WEB_SET_ZENI_DROP_BONUS_REQ* res = (sMG_WEB_SET_ZENI_DROP_BONUS_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_SET_ZENI_DROP_BONUS_REQ;
			res->requestId = req->requestId;
			res->bonusPercent = req->bonusPercent;
			packet.SetPacketLen(sizeof(sMG_WEB_SET_ZENI_DROP_BONUS_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_SET_QUEST_MONEY_BONUS:
		{
			sWM_SET_QUEST_MONEY_BONUS* req = (sWM_SET_QUEST_MONEY_BONUS*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();
			app->m_adminBonusState.questMoneyBonus = req->bonusPercent;

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_SET_QUEST_MONEY_BONUS, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_SET_QUEST_MONEY_BONUS;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_SET_QUEST_MONEY_BONUS_REQ));
			sMG_WEB_SET_QUEST_MONEY_BONUS_REQ* res = (sMG_WEB_SET_QUEST_MONEY_BONUS_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_SET_QUEST_MONEY_BONUS_REQ;
			res->requestId = req->requestId;
			res->bonusPercent = req->bonusPercent;
			packet.SetPacketLen(sizeof(sMG_WEB_SET_QUEST_MONEY_BONUS_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_SET_UPGRADE_RATE_BONUS:
		{
			sWM_SET_UPGRADE_RATE_BONUS* req = (sWM_SET_UPGRADE_RATE_BONUS*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();
			app->m_adminBonusState.upgradeRateBonus = req->bonusPercent;

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_SET_UPGRADE_RATE_BONUS, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_SET_UPGRADE_RATE_BONUS;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_SET_UPGRADE_RATE_BONUS_REQ));
			sMG_WEB_SET_UPGRADE_RATE_BONUS_REQ* res = (sMG_WEB_SET_UPGRADE_RATE_BONUS_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_SET_UPGRADE_RATE_BONUS_REQ;
			res->requestId = req->requestId;
			res->bonusPercent = req->bonusPercent;
			packet.SetPacketLen(sizeof(sMG_WEB_SET_UPGRADE_RATE_BONUS_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_SET_MONSTER_AGGRESSIVE:
		{
			sWM_SET_MONSTER_AGGRESSIVE* req = (sWM_SET_MONSTER_AGGRESSIVE*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();
			app->m_adminBonusState.monsterAggressive = req->bEnable;

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_SET_MONSTER_AGGRESSIVE, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_SET_MONSTER_AGGRESSIVE;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_SET_MONSTER_AGGRESSIVE_REQ));
			sMG_WEB_SET_MONSTER_AGGRESSIVE_REQ* res = (sMG_WEB_SET_MONSTER_AGGRESSIVE_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_SET_MONSTER_AGGRESSIVE_REQ;
			res->requestId = req->requestId;
			res->bEnable = req->bEnable;
			packet.SetPacketLen(sizeof(sMG_WEB_SET_MONSTER_AGGRESSIVE_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_SET_MONSTER_STAT_BONUS:
		{
			sWM_SET_MONSTER_STAT_BONUS* req = (sWM_SET_MONSTER_STAT_BONUS*)pPacket->GetPacketData();
			// Note: Monster stat bonus is now per-world-type, stored per GameServer
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();
			app->m_adminBonusState.monsterMaxLpPercent = req->fMaxLpPercent;
			app->m_adminBonusState.monsterMaxEpPercent = req->fMaxEpPercent;
			app->m_adminBonusState.monsterPhysicalOffencePercent = req->fPhysicalOffencePercent;
			app->m_adminBonusState.monsterEnergyOffencePercent = req->fEnergyOffencePercent;
			app->m_adminBonusState.monsterPhysicalDefencePercent = req->fPhysicalDefencePercent;
			app->m_adminBonusState.monsterEnergyDefencePercent = req->fEnergyDefencePercent;
			app->m_adminBonusState.monsterAttackRatePercent = req->fAttackRatePercent;
			app->m_adminBonusState.monsterDodgeRatePercent = req->fDodgeRatePercent;

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_SET_MONSTER_STAT_BONUS, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_SET_MONSTER_STAT_BONUS;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_SET_MONSTER_STAT_BONUS_REQ));
			sMG_WEB_SET_MONSTER_STAT_BONUS_REQ* res = (sMG_WEB_SET_MONSTER_STAT_BONUS_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_SET_MONSTER_STAT_BONUS_REQ;
			res->requestId = req->requestId;
			res->byWorldRuleType = req->byWorldRuleType;
			res->fMaxLpPercent = req->fMaxLpPercent;
			res->fMaxEpPercent = req->fMaxEpPercent;
			res->fPhysicalOffencePercent = req->fPhysicalOffencePercent;
			res->fEnergyOffencePercent = req->fEnergyOffencePercent;
			res->fPhysicalDefencePercent = req->fPhysicalDefencePercent;
			res->fEnergyDefencePercent = req->fEnergyDefencePercent;
			res->fAttackRatePercent = req->fAttackRatePercent;
			res->fDodgeRatePercent = req->fDodgeRatePercent;
			packet.SetPacketLen(sizeof(sMG_WEB_SET_MONSTER_STAT_BONUS_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_APPLY_MONSTER_BUFF_SKILL_ALL:
		{
			sWM_APPLY_MONSTER_BUFF_SKILL_ALL* req = (sWM_APPLY_MONSTER_BUFF_SKILL_ALL*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_APPLY_MONSTER_BUFF_SKILL_ALL, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_APPLY_MONSTER_BUFF_SKILL_ALL;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_APPLY_MONSTER_BUFF_SKILL_ALL_REQ));
			sMG_WEB_APPLY_MONSTER_BUFF_SKILL_ALL_REQ* res = (sMG_WEB_APPLY_MONSTER_BUFF_SKILL_ALL_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_APPLY_MONSTER_BUFF_SKILL_ALL_REQ;
			res->requestId = req->requestId;
			res->skillTblidx = req->skillTblidx;
			res->durationSeconds = req->durationSeconds;
			res->bOverrideValues = req->bOverrideValues;
			memcpy(res->aEffectValues, req->aEffectValues, sizeof(res->aEffectValues));
			packet.SetPacketLen(sizeof(sMG_WEB_APPLY_MONSTER_BUFF_SKILL_ALL_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_CLEAR_MONSTER_BUFFS_ALL:
		{
			sWM_CLEAR_MONSTER_BUFFS_ALL* req = (sWM_CLEAR_MONSTER_BUFFS_ALL*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_CLEAR_MONSTER_BUFFS_ALL, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_CLEAR_MONSTER_BUFFS_ALL;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_CLEAR_MONSTER_BUFFS_ALL_REQ));
			sMG_WEB_CLEAR_MONSTER_BUFFS_ALL_REQ* res = (sMG_WEB_CLEAR_MONSTER_BUFFS_ALL_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_CLEAR_MONSTER_BUFFS_ALL_REQ;
			res->requestId = req->requestId;
			packet.SetPacketLen(sizeof(sMG_WEB_CLEAR_MONSTER_BUFFS_ALL_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_SET_KILL_DEBUFF:
		{
			sWM_SET_KILL_DEBUFF* req = (sWM_SET_KILL_DEBUFF*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->m_adminBonusState.killDebuffEnabled = req->bEnable;
			app->m_adminBonusState.killDebuffSkillTblidx = req->skillTblidx;
			app->m_adminBonusState.killDebuffDurationSeconds = req->durationSeconds;
			app->m_adminBonusState.killDebuffOverrideValues = req->bOverrideValues;
			memcpy(app->m_adminBonusState.killDebuffEffectValues, req->aEffectValues, sizeof(app->m_adminBonusState.killDebuffEffectValues));

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_SET_KILL_DEBUFF, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_SET_KILL_DEBUFF;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_SET_KILL_DEBUFF_REQ));
			sMG_WEB_SET_KILL_DEBUFF_REQ* res = (sMG_WEB_SET_KILL_DEBUFF_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_SET_KILL_DEBUFF_REQ;
			res->requestId = req->requestId;
			res->bEnable = req->bEnable;
			res->skillTblidx = req->skillTblidx;
			res->durationSeconds = req->durationSeconds;
			res->bOverrideValues = req->bOverrideValues;
			memcpy(res->aEffectValues, req->aEffectValues, sizeof(res->aEffectValues));
			packet.SetPacketLen(sizeof(sMG_WEB_SET_KILL_DEBUFF_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_RESET_SKILL_COOLDOWN:
		{
			sWM_RESET_SKILL_COOLDOWN* req = (sWM_RESET_SKILL_COOLDOWN*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_RESET_SKILL_COOLDOWN, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_RESET_SKILL_COOLDOWN;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_RESET_SKILL_COOLDOWN_REQ));
			sMG_WEB_RESET_SKILL_COOLDOWN_REQ* res = (sMG_WEB_RESET_SKILL_COOLDOWN_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_RESET_SKILL_COOLDOWN_REQ;
			res->requestId = req->requestId;
			res->charId = req->charId;
			packet.SetPacketLen(sizeof(sMG_WEB_RESET_SKILL_COOLDOWN_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_RESET_SKILL_COOLDOWN_ALL:
		{
			sWM_RESET_SKILL_COOLDOWN_ALL* req = (sWM_RESET_SKILL_COOLDOWN_ALL*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_RESET_SKILL_COOLDOWN_ALL, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_RESET_SKILL_COOLDOWN_ALL;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_RESET_SKILL_COOLDOWN_ALL_REQ));
			sMG_WEB_RESET_SKILL_COOLDOWN_ALL_REQ* res = (sMG_WEB_RESET_SKILL_COOLDOWN_ALL_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_RESET_SKILL_COOLDOWN_ALL_REQ;
			res->requestId = req->requestId;
			packet.SetPacketLen(sizeof(sMG_WEB_RESET_SKILL_COOLDOWN_ALL_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_SET_CHANNEL_STAT_BONUS:
		{
			sWM_SET_CHANNEL_STAT_BONUS* req = (sWM_SET_CHANNEL_STAT_BONUS*)pPacket->GetPacketData();
			DWORD serverCount = (DWORD)g_pSrvMgr->GetGameServerSessionCount();

			CMasterServer::sChannelStatBonus bonus;
			bonus.channelId = req->channelId;
			bonus.fMaxLpPercent = req->fMaxLpPercent;
			bonus.fMaxEpPercent = req->fMaxEpPercent;
			bonus.fPhysicalOffencePercent = req->fPhysicalOffencePercent;
			bonus.fEnergyOffencePercent = req->fEnergyOffencePercent;
			bonus.fPhysicalDefencePercent = req->fPhysicalDefencePercent;
			bonus.fEnergyDefencePercent = req->fEnergyDefencePercent;
			bonus.fAttackRatePercent = req->fAttackRatePercent;
			bonus.fDodgeRatePercent = req->fDodgeRatePercent;
			app->m_adminBonusState.channelBonuses[req->channelId] = bonus;

			app->RegisterActionRequest(GetHandle(), req->requestId, WEB_ACTION_SET_CHANNEL_STAT_BONUS, serverCount);

			if (serverCount == 0)
			{
				CNtlPacket packetRes(sizeof(sWM_ADMIN_ACTION_RES));
				sWM_ADMIN_ACTION_RES* res = (sWM_ADMIN_ACTION_RES*)packetRes.GetPacketData();
				res->wOpCode = WM_ADMIN_ACTION_RES;
				res->requestId = req->requestId;
				res->byAction = WEB_ACTION_SET_CHANNEL_STAT_BONUS;
				res->wResultCode = GAME_FAIL;
				res->dwAffectedCount = 0;
				packetRes.SetPacketLen(sizeof(sWM_ADMIN_ACTION_RES));
				app->Send(GetHandle(), &packetRes);
				app->m_webActionRequests.erase(req->requestId);
				break;
			}

			CNtlPacket packet(sizeof(sMG_WEB_SET_CHANNEL_STAT_BONUS_REQ));
			sMG_WEB_SET_CHANNEL_STAT_BONUS_REQ* res = (sMG_WEB_SET_CHANNEL_STAT_BONUS_REQ*)packet.GetPacketData();
			res->wOpCode = MG_WEB_SET_CHANNEL_STAT_BONUS_REQ;
			res->requestId = req->requestId;
			res->channelId = req->channelId;
			res->fMaxLpPercent = req->fMaxLpPercent;
			res->fMaxEpPercent = req->fMaxEpPercent;
			res->fPhysicalOffencePercent = req->fPhysicalOffencePercent;
			res->fEnergyOffencePercent = req->fEnergyOffencePercent;
			res->fPhysicalDefencePercent = req->fPhysicalDefencePercent;
			res->fEnergyDefencePercent = req->fEnergyDefencePercent;
			res->fAttackRatePercent = req->fAttackRatePercent;
			res->fDodgeRatePercent = req->fDodgeRatePercent;
			packet.SetPacketLen(sizeof(sMG_WEB_SET_CHANNEL_STAT_BONUS_REQ));
			g_pSrvMgr->BroadcastServer(NTL_SERVER_TYPE_GAME, &packet);
		}
		break;

		case WM_GET_BONUS_STATE:
		{
			sWM_GET_BONUS_STATE* req = (sWM_GET_BONUS_STATE*)pPacket->GetPacketData();
			CNtlPacket packet(sizeof(sWM_GET_BONUS_STATE_RES));
			sWM_GET_BONUS_STATE_RES* res = (sWM_GET_BONUS_STATE_RES*)packet.GetPacketData();
			res->wOpCode = WM_GET_BONUS_STATE_RES;
			res->requestId = req->requestId;
			res->soloExpBonus = app->m_adminBonusState.soloExpBonus;
			res->partyExpBonus = app->m_adminBonusState.partyExpBonus;
			res->questExpBonus = app->m_adminBonusState.questExpBonus;
			res->craftExpBonus = app->m_adminBonusState.craftExpBonus;
			res->zeniDropBonus = app->m_adminBonusState.zeniDropBonus;
			res->questMoneyBonus = app->m_adminBonusState.questMoneyBonus;
			res->upgradeRateBonus = app->m_adminBonusState.upgradeRateBonus;
			res->bMonsterAggressive = app->m_adminBonusState.monsterAggressive;
			res->monsterMaxLpPercent = app->m_adminBonusState.monsterMaxLpPercent;
			res->monsterMaxEpPercent = app->m_adminBonusState.monsterMaxEpPercent;
			res->monsterPhysicalOffencePercent = app->m_adminBonusState.monsterPhysicalOffencePercent;
			res->monsterEnergyOffencePercent = app->m_adminBonusState.monsterEnergyOffencePercent;
			res->monsterPhysicalDefencePercent = app->m_adminBonusState.monsterPhysicalDefencePercent;
			res->monsterEnergyDefencePercent = app->m_adminBonusState.monsterEnergyDefencePercent;
			res->monsterAttackRatePercent = app->m_adminBonusState.monsterAttackRatePercent;
			res->monsterDodgeRatePercent = app->m_adminBonusState.monsterDodgeRatePercent;
			res->bKillDebuffEnabled = app->m_adminBonusState.killDebuffEnabled;
			res->killDebuffSkillTblidx = app->m_adminBonusState.killDebuffSkillTblidx;
			res->killDebuffDurationSeconds = app->m_adminBonusState.killDebuffDurationSeconds;
			res->killDebuffOverrideValues = app->m_adminBonusState.killDebuffOverrideValues;
			memcpy(res->killDebuffEffectValues, app->m_adminBonusState.killDebuffEffectValues, sizeof(res->killDebuffEffectValues));

			res->channelBonusCount = 0;
			for (std::unordered_map<SERVERCHANNELID, CMasterServer::sChannelStatBonus>::const_iterator it = app->m_adminBonusState.channelBonuses.begin();
				it != app->m_adminBonusState.channelBonuses.end() && res->channelBonusCount < NTL_MAX_WEB_CHANNEL_LIST;
				++it)
			{
				const CMasterServer::sChannelStatBonus& bonus = it->second;
				sWEB_CHANNEL_STAT_BONUS& dest = res->channelBonuses[res->channelBonusCount++];
				dest.channelId = bonus.channelId;
				dest.fMaxLpPercent = bonus.fMaxLpPercent;
				dest.fMaxEpPercent = bonus.fMaxEpPercent;
				dest.fPhysicalOffencePercent = bonus.fPhysicalOffencePercent;
				dest.fEnergyOffencePercent = bonus.fEnergyOffencePercent;
				dest.fPhysicalDefencePercent = bonus.fPhysicalDefencePercent;
				dest.fEnergyDefencePercent = bonus.fEnergyDefencePercent;
				dest.fAttackRatePercent = bonus.fAttackRatePercent;
				dest.fDodgeRatePercent = bonus.fDodgeRatePercent;
			}

			packet.SetPacketLen(sizeof(sWM_GET_BONUS_STATE_RES));
			app->Send(GetHandle(), &packet);
		}
		break;

		case WM_GET_MASTER_UPTIME:
		{
			sWM_GET_MASTER_UPTIME* req = (sWM_GET_MASTER_UPTIME*)pPacket->GetPacketData();
			CNtlPacket packet(sizeof(sWM_GET_MASTER_UPTIME_RES));
			sWM_GET_MASTER_UPTIME_RES* res = (sWM_GET_MASTER_UPTIME_RES*)packet.GetPacketData();
			res->wOpCode = WM_GET_MASTER_UPTIME_RES;
			res->requestId = req->requestId;
			res->uptimeSeconds = (GetTickCount() - app->m_startTick) / 1000;
			packet.SetPacketLen(sizeof(sWM_GET_MASTER_UPTIME_RES));
			app->Send(GetHandle(), &packet);
		}
		break;

		case WM_GET_MASTER_TIME:
		{
			sWM_GET_MASTER_TIME* req = (sWM_GET_MASTER_TIME*)pPacket->GetPacketData();
			CNtlPacket packet(sizeof(sWM_GET_MASTER_TIME_RES));
			sWM_GET_MASTER_TIME_RES* res = (sWM_GET_MASTER_TIME_RES*)packet.GetPacketData();
			res->wOpCode = WM_GET_MASTER_TIME_RES;
			res->requestId = req->requestId;
			res->unixTime = (DWORD)time(NULL);
			packet.SetPacketLen(sizeof(sWM_GET_MASTER_TIME_RES));
			app->Send(GetHandle(), &packet);
		}
		break;

		case WM_GET_MASTER_CONFIG:
		{
			sWM_GET_MASTER_CONFIG* req = (sWM_GET_MASTER_CONFIG*)pPacket->GetPacketData();
			CNtlPacket packet(sizeof(sWM_GET_MASTER_CONFIG_RES));
			sWM_GET_MASTER_CONFIG_RES* res = (sWM_GET_MASTER_CONFIG_RES*)packet.GetPacketData();
			res->wOpCode = WM_GET_MASTER_CONFIG_RES;
			res->requestId = req->requestId;
			res->serverAcceptLimit = app->m_config.ServerAcceptLimit;
			res->serverPlayerLimit = (DWORD)app->m_config.ServerPlayerLimit;
			NTL_STRNCPY_S_FULL(res->authAddress, NTL_MAX_LENGTH_OF_IP + 1, app->m_config.strAuthServerAcceptIP.c_str());
			res->authPort = app->m_config.wAuthServerAcceptPort;
			NTL_STRNCPY_S_FULL(res->charAddress, NTL_MAX_LENGTH_OF_IP + 1, app->m_config.strCharServerAcceptIP.c_str());
			res->charPort = app->m_config.wCharServerAcceptPort;
			NTL_STRNCPY_S_FULL(res->chatAddress, NTL_MAX_LENGTH_OF_IP + 1, app->m_config.strChatServerAcceptIP.c_str());
			res->chatPort = app->m_config.wChatServerAcceptPort;
			NTL_STRNCPY_S_FULL(res->gameAddress, NTL_MAX_LENGTH_OF_IP + 1, app->m_config.strGameServerAcceptIP.c_str());
			res->gamePort = app->m_config.wGameServerAcceptPort;
			NTL_STRNCPY_S_FULL(res->webAddress, NTL_MAX_LENGTH_OF_IP + 1, app->m_config.strWebServerAcceptIP.c_str());
			res->webPort = app->m_config.wWebServerAcceptPort;
			packet.SetPacketLen(sizeof(sWM_GET_MASTER_CONFIG_RES));
			app->Send(GetHandle(), &packet);
		}
		break;

		case WM_GET_SERVER_COUNTS:
		{
			sWM_GET_SERVER_COUNTS* req = (sWM_GET_SERVER_COUNTS*)pPacket->GetPacketData();
			CNtlPacket packet(sizeof(sWM_GET_SERVER_COUNTS_RES));
			sWM_GET_SERVER_COUNTS_RES* res = (sWM_GET_SERVER_COUNTS_RES*)packet.GetPacketData();
			res->wOpCode = WM_GET_SERVER_COUNTS_RES;
			res->requestId = req->requestId;
			res->authCount = (WORD)(g_pSrvMgr->GetRawServerList(NTL_SERVER_TYPE_AUTH) ? g_pSrvMgr->GetRawServerList(NTL_SERVER_TYPE_AUTH)->size() : 0);
			res->charCount = (WORD)(g_pSrvMgr->GetRawServerList(NTL_SERVER_TYPE_CHARACTER) ? g_pSrvMgr->GetRawServerList(NTL_SERVER_TYPE_CHARACTER)->size() : 0);
			res->chatCount = (WORD)(g_pSrvMgr->GetRawServerList(NTL_SERVER_TYPE_COMMUNITY) ? g_pSrvMgr->GetRawServerList(NTL_SERVER_TYPE_COMMUNITY)->size() : 0);
			res->gameCount = (WORD)(g_pSrvMgr->GetRawServerList(NTL_SERVER_TYPE_GAME) ? g_pSrvMgr->GetRawServerList(NTL_SERVER_TYPE_GAME)->size() : 0);
			res->queryCount = (WORD)(g_pSrvMgr->GetRawServerList(NTL_SERVER_TYPE_QUERY) ? g_pSrvMgr->GetRawServerList(NTL_SERVER_TYPE_QUERY)->size() : 0);
			res->communityCount = (WORD)(g_pSrvMgr->GetRawServerList(NTL_SERVER_TYPE_COMMUNITY) ? g_pSrvMgr->GetRawServerList(NTL_SERVER_TYPE_COMMUNITY)->size() : 0);
			packet.SetPacketLen(sizeof(sWM_GET_SERVER_COUNTS_RES));
			app->Send(GetHandle(), &packet);
		}
		break;

		case WM_GET_SESSION_COUNTS:
		{
			sWM_GET_SESSION_COUNTS* req = (sWM_GET_SESSION_COUNTS*)pPacket->GetPacketData();
			CNtlPacket packet(sizeof(sWM_GET_SESSION_COUNTS_RES));
			sWM_GET_SESSION_COUNTS_RES* res = (sWM_GET_SESSION_COUNTS_RES*)packet.GetPacketData();
			res->wOpCode = WM_GET_SESSION_COUNTS_RES;
			res->requestId = req->requestId;
			res->authSessions = (WORD)g_pSrvMgr->GetAuthServerSessionCount();
			res->charSessions = (WORD)g_pSrvMgr->GetCharServerSessionCount();
			res->chatSessions = (WORD)g_pSrvMgr->GetChatServerSessionCount();
			res->gameSessions = (WORD)g_pSrvMgr->GetGameServerSessionCount();
			packet.SetPacketLen(sizeof(sWM_GET_SESSION_COUNTS_RES));
			app->Send(GetHandle(), &packet);
		}
		break;

		case WM_GET_PLAYER_COUNTS:
		{
			sWM_GET_PLAYER_COUNTS* req = (sWM_GET_PLAYER_COUNTS*)pPacket->GetPacketData();
			CNtlPacket packet(sizeof(sWM_GET_PLAYER_COUNTS_RES));
			sWM_GET_PLAYER_COUNTS_RES* res = (sWM_GET_PLAYER_COUNTS_RES*)packet.GetPacketData();
			res->wOpCode = WM_GET_PLAYER_COUNTS_RES;
			res->requestId = req->requestId;
			res->charPlayers = (DWORD)g_pSrvMgr->GetPlayerInCharServer();
			res->gamePlayers = (DWORD)g_pSrvMgr->GetPlayerInGameServer();
			packet.SetPacketLen(sizeof(sWM_GET_PLAYER_COUNTS_RES));
			app->Send(GetHandle(), &packet);
		}
		break;

		case WM_GET_GAME_FARM_LIST:
		{
			sWM_GET_GAME_FARM_LIST* req = (sWM_GET_GAME_FARM_LIST*)pPacket->GetPacketData();
			CNtlPacket packet(sizeof(sWM_GET_GAME_FARM_LIST_RES));
			sWM_GET_GAME_FARM_LIST_RES* res = (sWM_GET_GAME_FARM_LIST_RES*)packet.GetPacketData();
			res->wOpCode = WM_GET_GAME_FARM_LIST_RES;
			res->requestId = req->requestId;
			res->count = 0;

			std::map<SERVERFARMID, sSERVER_FARM_INFO*> farms = g_pSrvMgr->GetRawServerFarmList();
			for (std::map<SERVERFARMID, sSERVER_FARM_INFO*>::iterator it = farms.begin();
				it != farms.end() && res->count < NTL_MAX_WEB_FARM_LIST; ++it)
			{
				sSERVER_FARM_INFO* farm = it->second;
				if (!farm) continue;
				sWEB_GAME_FARM_INFO& out = res->farms[res->count++];
				out.serverFarmId = farm->serverFarmId;
				out.byServerStatus = farm->byServerStatus;
				out.dwLoad = farm->dwLoad;
				out.dwMaxLoad = farm->dwMaxLoad;
				NTL_WCSCPY_S(out.wszGameServerFarmName, NTL_MAX_SIZE_SERVER_FARM_NAME_UNICODE + 1, farm->wszGameServerFarmName);
			}

			packet.SetPacketLen(sizeof(sWM_GET_GAME_FARM_LIST_RES));
			app->Send(GetHandle(), &packet);
		}
		break;

		case WM_GET_GAME_CHANNEL_LIST:
		{
			sWM_GET_GAME_CHANNEL_LIST* req = (sWM_GET_GAME_CHANNEL_LIST*)pPacket->GetPacketData();
			CNtlPacket packet(sizeof(sWM_GET_GAME_CHANNEL_LIST_RES));
			sWM_GET_GAME_CHANNEL_LIST_RES* res = (sWM_GET_GAME_CHANNEL_LIST_RES*)packet.GetPacketData();
			res->wOpCode = WM_GET_GAME_CHANNEL_LIST_RES;
			res->requestId = req->requestId;
			res->count = 0;

			std::map<SERVERFARMID, sSERVER_FARM_INFO*> farms = g_pSrvMgr->GetRawServerFarmList();
			for (std::map<SERVERFARMID, sSERVER_FARM_INFO*>::iterator it = farms.begin();
				it != farms.end() && res->count < NTL_MAX_WEB_CHANNEL_LIST; ++it)
			{
				sSERVER_FARM_INFO* farm = it->second;
				if (!farm) continue;
				for (std::map<SERVERCHANNELID, sSERVER_CHANNEL_INFO*>::iterator ct = farm->mapGameServerChannelInfo.begin();
					ct != farm->mapGameServerChannelInfo.end() && res->count < NTL_MAX_WEB_CHANNEL_LIST; ++ct)
				{
					sSERVER_CHANNEL_INFO* channel = ct->second;
					if (!channel) continue;
					sWEB_GAME_CHANNEL_INFO& out = res->channels[res->count++];
					out.serverFarmId = channel->serverFarmId;
					out.byServerChannelIndex = channel->byServerChannelIndex;
					out.byServerStatus = channel->byServerStatus;
					out.dwLoad = channel->dwLoad;
					out.dwMaxLoad = channel->dwMaxLoad;
					out.bIsVisible = channel->bIsVisible;
					out.bIsScrambleChannel = channel->bIsScrambleChannel;
					NTL_WCSCPY_S(out.wszServerChannelName, NTL_MAX_SIZE_SERVER_CHANNEL_NAME_UNICODE + 1, channel->sChannelBuff.wszServerChannelName);
				}
			}

			packet.SetPacketLen(sizeof(sWM_GET_GAME_CHANNEL_LIST_RES));
			app->Send(GetHandle(), &packet);
		}
		break;

		case WM_GET_AUTH_SERVER_LIST:
		case WM_GET_CHAR_SERVER_LIST:
		case WM_GET_CHAT_SERVER_LIST:
		case WM_GET_GAME_SERVER_LIST:
		case WM_GET_QUERY_SERVER_LIST:
		case WM_GET_SERVER_LIST_ALL:
		{
			DWORD requestId = 0;
			BYTE serverType = NTL_SERVER_TYPE_INVALID;
			WORD opRes = WM_GET_AUTH_SERVER_LIST_RES;
			if (pHeader->wOpCode == WM_GET_AUTH_SERVER_LIST)
			{
				requestId = ((sWM_GET_AUTH_SERVER_LIST*)pPacket->GetPacketData())->requestId;
				serverType = NTL_SERVER_TYPE_AUTH;
				opRes = WM_GET_AUTH_SERVER_LIST_RES;
			}
			else if (pHeader->wOpCode == WM_GET_CHAR_SERVER_LIST)
			{
				requestId = ((sWM_GET_CHAR_SERVER_LIST*)pPacket->GetPacketData())->requestId;
				serverType = NTL_SERVER_TYPE_CHARACTER;
				opRes = WM_GET_CHAR_SERVER_LIST_RES;
			}
			else if (pHeader->wOpCode == WM_GET_CHAT_SERVER_LIST)
			{
				requestId = ((sWM_GET_CHAT_SERVER_LIST*)pPacket->GetPacketData())->requestId;
				serverType = NTL_SERVER_TYPE_COMMUNITY;
				opRes = WM_GET_CHAT_SERVER_LIST_RES;
			}
			else if (pHeader->wOpCode == WM_GET_GAME_SERVER_LIST)
			{
				requestId = ((sWM_GET_GAME_SERVER_LIST*)pPacket->GetPacketData())->requestId;
				serverType = NTL_SERVER_TYPE_GAME;
				opRes = WM_GET_GAME_SERVER_LIST_RES;
			}
			else if (pHeader->wOpCode == WM_GET_QUERY_SERVER_LIST)
			{
				requestId = ((sWM_GET_QUERY_SERVER_LIST*)pPacket->GetPacketData())->requestId;
				serverType = NTL_SERVER_TYPE_QUERY;
				opRes = WM_GET_QUERY_SERVER_LIST_RES;
			}
			else
			{
				requestId = ((sWM_GET_SERVER_LIST_ALL*)pPacket->GetPacketData())->requestId;
				serverType = NTL_SERVER_TYPE_INVALID;
				opRes = WM_GET_SERVER_LIST_ALL_RES;
			}

			CNtlPacket packet(sizeof(sWM_GET_AUTH_SERVER_LIST_RES));
			sWM_GET_AUTH_SERVER_LIST_RES* res = (sWM_GET_AUTH_SERVER_LIST_RES*)packet.GetPacketData();
			res->wOpCode = opRes;
			res->requestId = requestId;
			res->count = 0;

			if (serverType == NTL_SERVER_TYPE_INVALID)
			{
				BYTE types[] = { NTL_SERVER_TYPE_AUTH, NTL_SERVER_TYPE_CHARACTER, NTL_SERVER_TYPE_COMMUNITY, NTL_SERVER_TYPE_GAME, NTL_SERVER_TYPE_QUERY };
				for (int ti = 0; ti < 5 && res->count < NTL_MAX_WEB_SERVER_LIST; ++ti)
				{
					std::list<sDBO_SERVER_INFO*>* list = g_pSrvMgr->GetRawServerList(types[ti]);
					if (!list) continue;
					for (std::list<sDBO_SERVER_INFO*>::iterator it = list->begin();
						it != list->end() && res->count < NTL_MAX_WEB_SERVER_LIST; ++it)
					{
						FillWebServerEntry(res->servers[res->count++], *it, types[ti]);
					}
				}
			}
			else
			{
				std::list<sDBO_SERVER_INFO*>* list = g_pSrvMgr->GetRawServerList(serverType);
				if (list)
				{
					for (std::list<sDBO_SERVER_INFO*>::iterator it = list->begin();
						it != list->end() && res->count < NTL_MAX_WEB_SERVER_LIST; ++it)
					{
						FillWebServerEntry(res->servers[res->count++], *it, serverType);
					}
				}
			}

			packet.SetPacketLen(sizeof(sWM_GET_AUTH_SERVER_LIST_RES));
			app->Send(GetHandle(), &packet);
		}
		break;

		case WM_GET_ONLINE_COUNT_BY_CHANNEL:
		{
			sWM_GET_ONLINE_COUNT_BY_CHANNEL* req = (sWM_GET_ONLINE_COUNT_BY_CHANNEL*)pPacket->GetPacketData();
			CNtlPacket packet(sizeof(sWM_GET_ONLINE_COUNT_BY_CHANNEL_RES));
			sWM_GET_ONLINE_COUNT_BY_CHANNEL_RES* res = (sWM_GET_ONLINE_COUNT_BY_CHANNEL_RES*)packet.GetPacketData();
			res->wOpCode = WM_GET_ONLINE_COUNT_BY_CHANNEL_RES;
			res->requestId = req->requestId;
			res->count = 0;

			std::map<DWORD, DWORD> counts;
			const std::unordered_map<ACCOUNTID, _SERVER_PAIR_ID>& players = g_pSrvMgr->GetGameServerPlayerMap();
			for (std::unordered_map<ACCOUNTID, _SERVER_PAIR_ID>::const_iterator it = players.begin(); it != players.end(); ++it)
			{
				DWORD key = (DWORD(it->second.serverFarmId) << 8) | DWORD(it->second.serverChannelId);
				counts[key] += 1;
			}

			for (std::map<DWORD, DWORD>::iterator it = counts.begin();
				it != counts.end() && res->count < NTL_MAX_WEB_CHANNEL_COUNT_LIST; ++it)
			{
				sWEB_CHANNEL_COUNT& out = res->channels[res->count++];
				out.serverFarmId = (SERVERFARMID)((it->first >> 8) & 0xFF);
				out.serverChannelId = (SERVERCHANNELID)(it->first & 0xFF);
				out.dwCount = it->second;
			}

			packet.SetPacketLen(sizeof(sWM_GET_ONLINE_COUNT_BY_CHANNEL_RES));
			app->Send(GetHandle(), &packet);
		}
		break;

		case WM_GET_ONLINE_COUNT_BY_FARM:
		{
			sWM_GET_ONLINE_COUNT_BY_FARM* req = (sWM_GET_ONLINE_COUNT_BY_FARM*)pPacket->GetPacketData();
			CNtlPacket packet(sizeof(sWM_GET_ONLINE_COUNT_BY_FARM_RES));
			sWM_GET_ONLINE_COUNT_BY_FARM_RES* res = (sWM_GET_ONLINE_COUNT_BY_FARM_RES*)packet.GetPacketData();
			res->wOpCode = WM_GET_ONLINE_COUNT_BY_FARM_RES;
			res->requestId = req->requestId;
			res->count = 0;

			std::map<SERVERFARMID, DWORD> counts;
			const std::unordered_map<ACCOUNTID, _SERVER_PAIR_ID>& players = g_pSrvMgr->GetGameServerPlayerMap();
			for (std::unordered_map<ACCOUNTID, _SERVER_PAIR_ID>::const_iterator it = players.begin(); it != players.end(); ++it)
			{
				counts[it->second.serverFarmId] += 1;
			}

			for (std::map<SERVERFARMID, DWORD>::iterator it = counts.begin();
				it != counts.end() && res->count < NTL_MAX_WEB_FARM_COUNT_LIST; ++it)
			{
				sWEB_FARM_COUNT& out = res->farms[res->count++];
				out.serverFarmId = it->first;
				out.dwCount = it->second;
			}

			packet.SetPacketLen(sizeof(sWM_GET_ONLINE_COUNT_BY_FARM_RES));
			app->Send(GetHandle(), &packet);
		}
		break;

		case WM_GET_ONLINE_ACCOUNT_LIST:
		{
			sWM_GET_ONLINE_ACCOUNT_LIST* req = (sWM_GET_ONLINE_ACCOUNT_LIST*)pPacket->GetPacketData();
			DWORD pageSize = req->pageSize;
			if (pageSize == 0 || pageSize > NTL_MAX_WEB_ACCOUNT_LIST)
				pageSize = NTL_MAX_WEB_ACCOUNT_LIST;

			CNtlPacket packet(sizeof(sWM_GET_ONLINE_ACCOUNT_LIST_RES));
			sWM_GET_ONLINE_ACCOUNT_LIST_RES* res = (sWM_GET_ONLINE_ACCOUNT_LIST_RES*)packet.GetPacketData();
			res->wOpCode = WM_GET_ONLINE_ACCOUNT_LIST_RES;
			res->requestId = req->requestId;
			res->totalCount = (DWORD)g_pSrvMgr->GetGameServerPlayerMap().size();
			res->count = 0;

			DWORD start = req->page * pageSize;
			DWORD end = start + pageSize;
			DWORD index = 0;
			const std::unordered_map<ACCOUNTID, _SERVER_PAIR_ID>& players = g_pSrvMgr->GetGameServerPlayerMap();
			for (std::unordered_map<ACCOUNTID, _SERVER_PAIR_ID>::const_iterator it = players.begin(); it != players.end(); ++it, ++index)
			{
				if (index < start)
					continue;
				if (index >= end || res->count >= NTL_MAX_WEB_ACCOUNT_LIST)
					break;
				sWEB_ONLINE_ACCOUNT& out = res->accounts[res->count++];
				out.accountId = it->first;
				out.serverFarmId = it->second.serverFarmId;
				out.serverChannelId = it->second.serverChannelId;
			}

			packet.SetPacketLen(sizeof(sWM_GET_ONLINE_ACCOUNT_LIST_RES));
			app->Send(GetHandle(), &packet);
		}
		break;

		case WM_GET_WEB_REQUEST_STATS:
		{
			sWM_GET_WEB_REQUEST_STATS* req = (sWM_GET_WEB_REQUEST_STATS*)pPacket->GetPacketData();
			CNtlPacket packet(sizeof(sWM_GET_WEB_REQUEST_STATS_RES));
			sWM_GET_WEB_REQUEST_STATS_RES* res = (sWM_GET_WEB_REQUEST_STATS_RES*)packet.GetPacketData();
			res->wOpCode = WM_GET_WEB_REQUEST_STATS_RES;
			res->requestId = req->requestId;
			res->onlineRequests = (DWORD)app->m_webOnlineRequests.size();
			res->actionRequests = (DWORD)app->m_webActionRequests.size();
			packet.SetPacketLen(sizeof(sWM_GET_WEB_REQUEST_STATS_RES));
			app->Send(GetHandle(), &packet);
		}
		break;

		case WM_GET_SERVER_LOAD_SUMMARY:
		{
			sWM_GET_SERVER_LOAD_SUMMARY* req = (sWM_GET_SERVER_LOAD_SUMMARY*)pPacket->GetPacketData();
			CNtlPacket packet(sizeof(sWM_GET_SERVER_LOAD_SUMMARY_RES));
			sWM_GET_SERVER_LOAD_SUMMARY_RES* res = (sWM_GET_SERVER_LOAD_SUMMARY_RES*)packet.GetPacketData();
			res->wOpCode = WM_GET_SERVER_LOAD_SUMMARY_RES;
			res->requestId = req->requestId;
			res->totalLoad = 0;
			res->totalMaxLoad = 0;
			res->farmCount = 0;
			res->channelCount = 0;

			std::map<SERVERFARMID, sSERVER_FARM_INFO*> farms = g_pSrvMgr->GetRawServerFarmList();
			for (std::map<SERVERFARMID, sSERVER_FARM_INFO*>::iterator it = farms.begin(); it != farms.end(); ++it)
			{
				sSERVER_FARM_INFO* farm = it->second;
				if (!farm) continue;
				res->farmCount++;
				res->totalLoad += farm->dwLoad;
				res->totalMaxLoad += farm->dwMaxLoad;
				res->channelCount += (WORD)farm->mapGameServerChannelInfo.size();
			}

			packet.SetPacketLen(sizeof(sWM_GET_SERVER_LOAD_SUMMARY_RES));
			app->Send(GetHandle(), &packet);
		}
		break;

		case WM_GET_CHANNEL_VISIBILITY_LIST:
		case WM_GET_SCRAMBLE_CHANNEL_LIST:
		{
			DWORD requestId = 0;
			WORD opRes = WM_GET_CHANNEL_VISIBILITY_LIST_RES;
			bool scrambleOnly = false;
			if (pHeader->wOpCode == WM_GET_CHANNEL_VISIBILITY_LIST)
			{
				requestId = ((sWM_GET_CHANNEL_VISIBILITY_LIST*)pPacket->GetPacketData())->requestId;
				opRes = WM_GET_CHANNEL_VISIBILITY_LIST_RES;
				scrambleOnly = false;
			}
			else
			{
				requestId = ((sWM_GET_SCRAMBLE_CHANNEL_LIST*)pPacket->GetPacketData())->requestId;
				opRes = WM_GET_SCRAMBLE_CHANNEL_LIST_RES;
				scrambleOnly = true;
			}

			CNtlPacket packet(sizeof(sWM_GET_CHANNEL_VISIBILITY_LIST_RES));
			sWM_GET_CHANNEL_VISIBILITY_LIST_RES* res = (sWM_GET_CHANNEL_VISIBILITY_LIST_RES*)packet.GetPacketData();
			res->wOpCode = opRes;
			res->requestId = requestId;
			res->count = 0;

			std::map<SERVERFARMID, sSERVER_FARM_INFO*> farms = g_pSrvMgr->GetRawServerFarmList();
			for (std::map<SERVERFARMID, sSERVER_FARM_INFO*>::iterator it = farms.begin();
				it != farms.end() && res->count < NTL_MAX_WEB_CHANNEL_FLAG_LIST; ++it)
			{
				sSERVER_FARM_INFO* farm = it->second;
				if (!farm) continue;
				for (std::map<SERVERCHANNELID, sSERVER_CHANNEL_INFO*>::iterator ct = farm->mapGameServerChannelInfo.begin();
					ct != farm->mapGameServerChannelInfo.end() && res->count < NTL_MAX_WEB_CHANNEL_FLAG_LIST; ++ct)
				{
					sSERVER_CHANNEL_INFO* channel = ct->second;
					if (!channel) continue;
					if (scrambleOnly && !channel->bIsScrambleChannel)
						continue;
					sWEB_CHANNEL_FLAG& out = res->channels[res->count++];
					out.serverFarmId = channel->serverFarmId;
					out.serverChannelId = channel->byServerChannelIndex;
					out.bFlag = scrambleOnly ? channel->bIsScrambleChannel : channel->bIsVisible;
				}
			}

			packet.SetPacketLen(sizeof(sWM_GET_CHANNEL_VISIBILITY_LIST_RES));
			app->Send(GetHandle(), &packet);
		}
		break;

		case WM_SEND_CHAR_ITEM:
		{
			sWM_SEND_CHAR_ITEM* req = (sWM_SEND_CHAR_ITEM*)pPacket->GetPacketData();

			printf("Web Server | pReq->characterID %u, req->item %u \n", req->characterID, req->item);
			/*
			CNtlPacket packet(sizeof(sMG_MOVE_RES));
			sMG_MOVE_RES* res = (sMG_MOVE_RES*)packet.GetPacketData();
			res->wOpCode = MG_MOVE_RES;
			res->accountId = 1;
			res->wResultCode = 1;
			packet.SetPacketLen(sizeof(sMG_MOVE_RES));
			app->Send(GetHandle(), &packet);
			*/
			
		}
		break;

		case 10012:
		{
			sMC_GAME_SERVER_FARM_INFO_CHANGED_NFY* req = (sMC_GAME_SERVER_FARM_INFO_CHANGED_NFY*)pPacket->GetPacketData();
			printf("Web Server | id %u, status %u, maxload %u, load %u \n", req->serverFarmId, req->byServerStatus, req->dwMaxLoad, req->dwLoad);
		}
		break;

		default: ERR_LOG(LOG_NETWORK, "CWebSession: Undefined Packet. wOpCode = %u", pHeader->wOpCode); break;
	}


	return NTL_SUCCESS;
}