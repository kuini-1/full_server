#include "stdafx.h"
#include "../MasterServer/PacketHead.h"
#include "MasterServer.h"


////////////////////////////////////////////////////////////////////////////////////////////////
//// RECEIVE PACKETS FROM CONNECTED SERVERS
////////////////////////////////////////////////////////////////////////////////////////////////

int CGameServerPassiveSession::OnAccept()
{
	NTL_PRINT(PRINT_APP, "GAME SERVER CONNECTED");

	return CNtlSession::OnAccept();
}


void CGameServerPassiveSession::OnClose()
{
	NTL_PRINT(PRINT_APP, "GAME SERVER (server %u channel %u) DISCONNECTED", serverFarmID, serverChannelID);
	g_pSrvMgr->SetServerOff(NTL_SERVER_TYPE_GAME, serverFarmID, serverChannelID, serverChannelID);
}


int CGameServerPassiveSession::OnDispatch(CNtlPacket * pPacket)
{
	CMasterServer * app = (CMasterServer*) NtlSfxGetApp();
	sNTLPACKETHEADER * pHeader = (sNTLPACKETHEADER *)pPacket->GetPacketData();

	switch( pHeader->wOpCode )
	{
		case GM_NOTIFY_SERVER_BEGIN:{	Gm_NfyServerBegin(pPacket,app);	} break;
		case GM_LOGIN_REQ:			{	Gm_PlayerEnter(pPacket,app);	} break;
		case GM_LOGOUT_REQ:			{	Gm_PlayerLeave(pPacket,app);	} break;
		case GM_MOVE_REQ:			{	Gm_PlayerMove(pPacket,app);	} break;
		
		case GM_PLAYER_SWITCH_CHANNEL_REQ:{	Gm_PlayerSwitchChannel(pPacket, app);	} break;
		case GM_CHAR_SERVER_TELEPORT_REQ: {	RecvCharServerTeleportReq(pPacket, app);	} break;

		case GM_PING_RES: 
		{ 
			sGM_PING_RES * req = (sGM_PING_RES*)pPacket->GetPacketData();

			ResetAliveTime();
			
			DWORD dwTick = GetTickCount();
			if(dwTick - req->dwTick > 10)
				ERR_LOG(LOG_GENERAL, "Game Server %d Channel %d Ping: %d ", req->serverId, req->serverChannelId, dwTick - req->dwTick);
		}
		break;

		case GM_SERVER_SHUT_DOWN:	Gm_RecvServerShutdownNfy(pPacket, app);	break;

		case GM_DRAGONBALL_SCRAMBLE_SEASON_STATE_NFY: RecvDragonballScrambleSeasonStateNfy(pPacket, app); break;

		case GM_WEB_ONLINE_PLAYERS_RES:
		{
			sGM_WEB_ONLINE_PLAYERS_RES* req = (sGM_WEB_ONLINE_PLAYERS_RES*)pPacket->GetPacketData();
			app->HandleOnlineListResponse(req);
		}
		break;

		case GM_WEB_SEND_NOTICE_RES:
		{
			sGM_WEB_SEND_NOTICE_RES* req = (sGM_WEB_SEND_NOTICE_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_KICK_PLAYER_RES:
		{
			sGM_WEB_KICK_PLAYER_RES* req = (sGM_WEB_KICK_PLAYER_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_GIVE_ITEM_RES:
		{
			sGM_WEB_GIVE_ITEM_RES* req = (sGM_WEB_GIVE_ITEM_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_GIVE_ITEM_ALL_RES:
		{
			sGM_WEB_GIVE_ITEM_ALL_RES* req = (sGM_WEB_GIVE_ITEM_ALL_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_EXEC_GM_COMMAND_RES:
		{
			sGM_WEB_EXEC_GM_COMMAND_RES* req = (sGM_WEB_EXEC_GM_COMMAND_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_APPLY_BUFF_RES:
		{
			sGM_WEB_APPLY_BUFF_RES* req = (sGM_WEB_APPLY_BUFF_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_REMOVE_BUFF_SKILL_RES:
		{
			sGM_WEB_REMOVE_BUFF_SKILL_RES* req = (sGM_WEB_REMOVE_BUFF_SKILL_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_REMOVE_BUFF_EFFECT_RES:
		{
			sGM_WEB_REMOVE_BUFF_EFFECT_RES* req = (sGM_WEB_REMOVE_BUFF_EFFECT_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_CLEAR_BUFFS_RES:
		{
			sGM_WEB_CLEAR_BUFFS_RES* req = (sGM_WEB_CLEAR_BUFFS_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_HEAL_FULL_RES:
		{
			sGM_WEB_HEAL_FULL_RES* req = (sGM_WEB_HEAL_FULL_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_ADD_ZENI_RES:
		{
			sGM_WEB_ADD_ZENI_RES* req = (sGM_WEB_ADD_ZENI_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_GIVE_ITEM_CUSTOM_RES:
		{
			sGM_WEB_GIVE_ITEM_CUSTOM_RES* req = (sGM_WEB_GIVE_ITEM_CUSTOM_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_APPLY_BUFF_SKILL_ALL_RES:
		{
			sGM_WEB_APPLY_BUFF_SKILL_ALL_RES* req = (sGM_WEB_APPLY_BUFF_SKILL_ALL_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_APPLY_BUFF_ITEM_RES:
		{
			sGM_WEB_APPLY_BUFF_ITEM_RES* req = (sGM_WEB_APPLY_BUFF_ITEM_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_APPLY_BUFF_ITEM_ALL_RES:
		{
			sGM_WEB_APPLY_BUFF_ITEM_ALL_RES* req = (sGM_WEB_APPLY_BUFF_ITEM_ALL_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_REMOVE_BUFF_SKILL_ALL_RES:
		{
			sGM_WEB_REMOVE_BUFF_SKILL_ALL_RES* req = (sGM_WEB_REMOVE_BUFF_SKILL_ALL_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_REMOVE_BUFF_EFFECT_ALL_RES:
		{
			sGM_WEB_REMOVE_BUFF_EFFECT_ALL_RES* req = (sGM_WEB_REMOVE_BUFF_EFFECT_ALL_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_CLEAR_BUFFS_ALL_RES:
		{
			sGM_WEB_CLEAR_BUFFS_ALL_RES* req = (sGM_WEB_CLEAR_BUFFS_ALL_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_SET_LEVEL_RES:
		{
			sGM_WEB_SET_LEVEL_RES* req = (sGM_WEB_SET_LEVEL_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_SET_CLASS_RES:
		{
			sGM_WEB_SET_CLASS_RES* req = (sGM_WEB_SET_CLASS_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_KILL_PLAYER_RES:
		{
			sGM_WEB_KILL_PLAYER_RES* req = (sGM_WEB_KILL_PLAYER_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_TELEPORT_PORTAL_RES:
		{
			sGM_WEB_TELEPORT_PORTAL_RES* req = (sGM_WEB_TELEPORT_PORTAL_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_TELEPORT_WORLD_RES:
		{
			sGM_WEB_TELEPORT_WORLD_RES* req = (sGM_WEB_TELEPORT_WORLD_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_TELEPORT_COORDS_RES:
		{
			sGM_WEB_TELEPORT_COORDS_RES* req = (sGM_WEB_TELEPORT_COORDS_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_MUTE_PLAYER_RES:
		{
			sGM_WEB_MUTE_PLAYER_RES* req = (sGM_WEB_MUTE_PLAYER_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_UNMUTE_PLAYER_RES:
		{
			sGM_WEB_UNMUTE_PLAYER_RES* req = (sGM_WEB_UNMUTE_PLAYER_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_TOGGLE_EXP_RES:
		{
			sGM_WEB_TOGGLE_EXP_RES* req = (sGM_WEB_TOGGLE_EXP_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		case GM_WEB_RESET_EXP_RES:
		{
			sGM_WEB_RESET_EXP_RES* req = (sGM_WEB_RESET_EXP_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;
		case GM_WEB_LEARN_SKILL_RES:
		{
			sGM_WEB_LEARN_SKILL_RES* req = (sGM_WEB_LEARN_SKILL_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;
		case GM_WEB_ADD_TITLE_RES:
		{
			sGM_WEB_ADD_TITLE_RES* req = (sGM_WEB_ADD_TITLE_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;
		case GM_WEB_REMOVE_TITLE_RES:
		{
			sGM_WEB_REMOVE_TITLE_RES* req = (sGM_WEB_REMOVE_TITLE_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;
		case GM_WEB_SET_SOLO_EXP_BONUS_RES:
		{
			sGM_WEB_SET_SOLO_EXP_BONUS_RES* req = (sGM_WEB_SET_SOLO_EXP_BONUS_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;
		case GM_WEB_SET_PARTY_EXP_BONUS_RES:
		{
			sGM_WEB_SET_PARTY_EXP_BONUS_RES* req = (sGM_WEB_SET_PARTY_EXP_BONUS_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;
		case GM_WEB_SET_QUEST_EXP_BONUS_RES:
		{
			sGM_WEB_SET_QUEST_EXP_BONUS_RES* req = (sGM_WEB_SET_QUEST_EXP_BONUS_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;
		case GM_WEB_SET_CRAFT_EXP_BONUS_RES:
		{
			sGM_WEB_SET_CRAFT_EXP_BONUS_RES* req = (sGM_WEB_SET_CRAFT_EXP_BONUS_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;
		case GM_WEB_SET_ZENI_DROP_BONUS_RES:
		{
			sGM_WEB_SET_ZENI_DROP_BONUS_RES* req = (sGM_WEB_SET_ZENI_DROP_BONUS_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;
		case GM_WEB_SET_QUEST_MONEY_BONUS_RES:
		{
			sGM_WEB_SET_QUEST_MONEY_BONUS_RES* req = (sGM_WEB_SET_QUEST_MONEY_BONUS_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;
		case GM_WEB_SET_UPGRADE_RATE_BONUS_RES:
		{
			sGM_WEB_SET_UPGRADE_RATE_BONUS_RES* req = (sGM_WEB_SET_UPGRADE_RATE_BONUS_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;
		case GM_WEB_SET_MONSTER_AGGRESSIVE_RES:
		{
			sGM_WEB_SET_MONSTER_AGGRESSIVE_RES* req = (sGM_WEB_SET_MONSTER_AGGRESSIVE_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;
		case GM_WEB_SET_MONSTER_STAT_BONUS_RES:
		{
			sGM_WEB_SET_MONSTER_STAT_BONUS_RES* req = (sGM_WEB_SET_MONSTER_STAT_BONUS_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;
		case GM_WEB_APPLY_MONSTER_BUFF_SKILL_ALL_RES:
		{
			sGM_WEB_APPLY_MONSTER_BUFF_SKILL_ALL_RES* req = (sGM_WEB_APPLY_MONSTER_BUFF_SKILL_ALL_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;
		case GM_WEB_CLEAR_MONSTER_BUFFS_ALL_RES:
		{
			sGM_WEB_CLEAR_MONSTER_BUFFS_ALL_RES* req = (sGM_WEB_CLEAR_MONSTER_BUFFS_ALL_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;
		case GM_WEB_SET_KILL_DEBUFF_RES:
		{
			sGM_WEB_SET_KILL_DEBUFF_RES* req = (sGM_WEB_SET_KILL_DEBUFF_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;
		case GM_WEB_RESET_SKILL_COOLDOWN_RES:
		{
			sGM_WEB_RESET_SKILL_COOLDOWN_RES* req = (sGM_WEB_RESET_SKILL_COOLDOWN_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;
		case GM_WEB_RESET_SKILL_COOLDOWN_ALL_RES:
		{
			sGM_WEB_RESET_SKILL_COOLDOWN_ALL_RES* req = (sGM_WEB_RESET_SKILL_COOLDOWN_ALL_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;
		case GM_WEB_SET_CHANNEL_STAT_BONUS_RES:
		{
			sGM_WEB_SET_CHANNEL_STAT_BONUS_RES* req = (sGM_WEB_SET_CHANNEL_STAT_BONUS_RES*)pPacket->GetPacketData();
			app->HandleActionResponse(req);
		}
		break;

		default: ERR_LOG(LOG_NETWORK, "Game: Undefined Packet. wOpCode = %u", pHeader->wOpCode); break;
	}
	

	return NTL_SUCCESS;
}