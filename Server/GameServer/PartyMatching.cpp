#include "stdafx.h"
#include "GameServer.h"
#include "PartyMatching.h"
#include "CPlayer.h" 
#include "NtlPacketGU.h"
#include "NtlResultCode.h"
#include "NtlPacketTU.h"
#include "NtlStringW.h"
#include "DungeonManager.h"
#include "TableContainerManager.h"
#include "SystemEffectTable.h"
#include "UseItemTable.h"

CPartyMatching::CPartyMatching()
{
	m_map_PartyMatching.clear();
}

CPartyMatching::~CPartyMatching()
{
	for (std::map<PARTYID, sPARTY_MARCHING*>::iterator it = m_map_PartyMatching.begin(); it != m_map_PartyMatching.end(); it++)
	{
		delete it->second;
	}
	m_map_PartyMatching.clear();
}

BYTE CPartyMatching::GetRole(BYTE byRoleplay)
{
	BYTE role = ROLEPLAY_TYPE_INVALID;

	switch (byRoleplay)
	{
			//TANK
		case PC_CLASS_NAMEK_FIGHTER:
		case PC_CLASS_MIGHTY_MAJIN:
		case PC_CLASS_DARK_WARRIOR:
		case PC_CLASS_SHADOW_KNIGHT:
		case PC_CLASS_ULTI_MA:
		case PC_CLASS_GRAND_MA:
		{ role = ROLEPLAY_TYPE_DEFENCE; } break;

		//HEAL
		case PC_CLASS_NAMEK_MYSTIC:
		case PC_CLASS_DENDEN_HEALER:
		case PC_CLASS_POCO_SUMMONER:
		{ role = ROLEPLAY_TYPE_HEAL; }  break;

		//DAMAGE
		case PC_CLASS_HUMAN_FIGHTER:
		case PC_CLASS_HUMAN_MYSTIC:
		case PC_CLASS_WONDER_MAJIN:
		case PC_CLASS_STREET_FIGHTER:
		case PC_CLASS_SWORD_MASTER:
		case PC_CLASS_CRANE_ROSHI:
		case PC_CLASS_TURTLE_ROSHI:
		case PC_CLASS_PLAS_MA:
		case PC_CLASS_KAR_MA:
		{ role = ROLEPLAY_TYPE_ATTACK; } break;
	}

	return role;
}

void CPartyMatching::LoadPartyMatchingList(CPlayer* player, BYTE byPage, BYTE byDungeonType, BYTE byRegion)
{
	if (m_map_PartyMatching.size() == 0)
	{
		CNtlPacket packet(sizeof(sGU_PARTY_MATCHING_LIST_RES));
		sGU_PARTY_MATCHING_LIST_RES * res = (sGU_PARTY_MATCHING_LIST_RES *)packet.GetPacketData();
		res->wOpCode = GU_PARTY_MATCHING_LIST_RES;
		res->byCount = 0;
		res->byTotalCount = 0;
		packet.SetPacketLen(sizeof(sGU_PARTY_MATCHING_LIST_RES));
		player->SendPacket(&packet);
	}
	else
	{
		BYTE byCount = 0;
		BYTE i = 0;
		BYTE bySkipCount = (byPage * NTL_PARTY_MATCHING_PARTY_LIST_COUNT) - NTL_PARTY_MATCHING_PARTY_LIST_COUNT;

		CNtlPacket packet(sizeof(sGU_PARTY_MATCHING_LIST_RES));
		sGU_PARTY_MATCHING_LIST_RES * res = (sGU_PARTY_MATCHING_LIST_RES *)packet.GetPacketData();
		res->wOpCode = GU_PARTY_MATCHING_LIST_RES;
		res->byTotalCount = (BYTE)m_map_PartyMatching.size();

		for (std::map<PARTYID, sPARTY_MARCHING*>::iterator it = m_map_PartyMatching.begin(); it != m_map_PartyMatching.end(); it++)
		{
			byCount++;
		//	printf("byCount %d bySkipCount %d \n", byCount, bySkipCount);
			if (byCount > bySkipCount)
			{
				sPARTY_MARCHING* matching = it->second;

				if (matching->pkParty)
				{
		//			printf("matching->byDungeonType %d byDungeonTyp %d matching->byRegionType %d byRegion %d \n", matching->byDungeonType, byDungeonType, matching->byRegionType, byRegion);
					if (matching->byDungeonType == byDungeonType && (matching->byRegionType == byRegion || byRegion == 0) ) //if byregion == 0 then we select all
					{
						res->partyList[i].partyId = it->first;
						res->partyList[i].byCurMemberCount = matching->pkParty->GetPartyMemberCount();
						NTL_SAFE_WCSCPY(res->partyList[i].wszPartyName, matching->pkParty->GetPartyName());
						res->partyList[i].byDifficulty = matching->byDifficulty;
						res->partyList[i].byDungeonType = matching->byDungeonType;
						res->partyList[i].byRegionType = matching->byRegionType;

						if (i++ == NTL_PARTY_MATCHING_PARTY_LIST_COUNT)
							break;
					}
				}
			}
		}

		res->byCount = i;
		packet.SetPacketLen(sizeof(sGU_PARTY_MATCHING_LIST_RES));
		player->SendPacket(&packet);
	}
}

void CPartyMatching::Register(CPlayer* player, BYTE byDifficulty, BYTE byDungeonType, BYTE byRegion, HOBJECT hItem, TBLIDX rankBattleWorldId)
{
	WORD resultcode = GAME_SUCCESS;
	printf("Find Party Match \n");
	if (player->GetPartyID() == INVALID_PARTYID || player->GetParty() == NULL)//Check if party exist
	{
		resultcode = GAME_PARTYMATCHING_REGISTER_WRONG_STATE;
	}
	else if (player->GetParty()->GetPartyLeaderID() != player->GetID()) //check if party leader
	{
		resultcode = GAME_COMMON_YOU_ARE_NOT_A_PARTY_LEADER;
	}
	else //last check: check if party already registered
	{
		std::map<PARTYID, sPARTY_MARCHING*>::const_iterator it = m_map_PartyMatching.find(player->GetPartyID());
		if (it != m_map_PartyMatching.end())
			resultcode = GAME_PARTYMATCHING_ALREADY_REGISTERED;
	}

	if (resultcode == GAME_SUCCESS)
	{
		printf("Register byDungeonType %u, byDifficulty %u \n", byDungeonType, byDifficulty);
		sPARTY_MARCHING* partymatching = new sPARTY_MARCHING(player->GetParty(), byDifficulty, byDungeonType, byRegion);
		m_map_PartyMatching.insert(std::make_pair(player->GetPartyID(), partymatching));
		player->GetParty()->SetPartySearch(true);
		//printf("Dugeon type %u Region %u \n", byDungeonType, byRegion);
		CGameServer* app = (CGameServer*)g_pApp;
		CNtlPacket packet(sizeof(sTU_CHAT_MESSAGE_FIND_PARTY));
		sTU_CHAT_MESSAGE_FIND_PARTY* res = (sTU_CHAT_MESSAGE_FIND_PARTY*)packet.GetPacketData();
		res->wOpCode = TU_CHAT_MESSAGE_FIND_PARTY;
		//Type 0 UD/Bid
		//Type 1 TMQ
		//Type 2 CCBD
		//Type 3 Rank Battle
		/*CNtlStringW msg;
		msg.Format(L"Party search has created to UD%u check it now in Party search menu", byRegion);*/
		WCHAR msg[255];
		if (byDungeonType == ePARTY_MATCHING_DUNGEON_TYPE_ULTIMATE_DUNGEON)
		{
			WCHAR wszFormatBuf[256];
			WCharTLiteralToWCHAR(L"Party matching [UD %u] register in Party Search Menu", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
			NTL_SWPRINTF(msg, 255, wszFormatBuf, byRegion);
			NTL_SAFE_WCSNCPY(res->awchMessage, msg, WCHARLen(msg));
			res->wMessageLengthInUnicode = WCHARLen(msg);

		}
		else if (byDungeonType == ePARTY_MATCHING_DUNGEON_TYPE_TIMEMACHINE_QUEST)
		{
			WCHAR wszFormatBuf[256];
			WCharTLiteralToWCHAR(L"Party matching [TMQ %u] register in Party Search Menu", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
			NTL_SWPRINTF(msg, 255, wszFormatBuf, byRegion);
			NTL_SAFE_WCSNCPY(res->awchMessage, msg, WCHARLen(msg));
			res->wMessageLengthInUnicode = WCHARLen(msg);
		}
		else if (byDungeonType == ePARTY_MATCHING_DUNGEON_TYPE_CC_BATTLE_DUNGEON)
		{
			WCHAR wszFormatBuf[256];
			WCharTLiteralToWCHAR(L"Party matching [CCBD %u] register in Party Search Menu", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
			NTL_SWPRINTF(msg, 255, wszFormatBuf, byRegion);
			NTL_SAFE_WCSNCPY(res->awchMessage, msg, WCHARLen(msg));
			res->wMessageLengthInUnicode = WCHARLen(msg);
		}
		else if (byDungeonType == ePARTY_MATCHING_DUNGEON_TYPE_RANK_BATTLE)
		{
			WCHAR wszFormatBuf[256];
			WCharTLiteralToWCHAR(L"Party matching [Ranked Battle %u] register in Party Search Menu", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
			NTL_SWPRINTF(msg, 255, wszFormatBuf, byRegion);
			NTL_SAFE_WCSNCPY(res->awchMessage, msg, WCHARLen(msg));
			res->wMessageLengthInUnicode = WCHARLen(msg);
		}
		else
			return;

		res->hSubject = player->GetID();
		WCHAR wszSystemBuf[32];
		WCharTLiteralToWCHAR(L"System", wszSystemBuf, sizeof(wszSystemBuf)/sizeof(WCHAR));
		NTL_SAFE_WCSCPY(res->awchSenderCharName, wszSystemBuf);
		res->serverChannelId = app->GetGsChannel();
		packet.SetPacketLen(sizeof(sTU_CHAT_MESSAGE_FIND_PARTY));
		g_pObjectManager->SendPacketToAll(&packet);		
	}

	CNtlPacket packet(sizeof(sGU_PARTY_MATCHING_REGISTER_RES));
	sGU_PARTY_MATCHING_REGISTER_RES * res = (sGU_PARTY_MATCHING_REGISTER_RES *)packet.GetPacketData();
	res->wOpCode = GU_PARTY_MATCHING_REGISTER_RES;
	res->byDifficulty = byDifficulty;
	res->byDungeonType = byDungeonType;
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_PARTY_MATCHING_REGISTER_RES));
	player->SendPacket(&packet);

	if (resultcode == GAME_SUCCESS)
	{
		/*
		
		* ROLE_SELECTION
		
		CNtlPacket packet2(sizeof(sGU_PARTY_MATCHING_ROLEPLAY_SELECT_START_NFY));
		sGU_PARTY_MATCHING_ROLEPLAY_SELECT_START_NFY* res2 = (sGU_PARTY_MATCHING_ROLEPLAY_SELECT_START_NFY*)packet2.GetPacketData();
		res2->wOpCode = GU_PARTY_MATCHING_ROLEPLAY_SELECT_START_NFY;
		res2->byDungeonType = byDungeonType;
		res2->byType = byRegion;
		res2->byRoleplay[0] = ROLEPLAY_TYPE_DEFENCE;
		res2->byRoleplay[1] = ROLEPLAY_TYPE_HEAL;
		res2->byRoleplay[2] = ROLEPLAY_TYPE_ATTACK;
		packet2.SetPacketLen(sizeof(sGU_PARTY_MATCHING_ROLEPLAY_SELECT_START_NFY));
		player->SendPacket(&packet2);

		sPARTY_MEMBER_INFO* playerInfo = &player->GetParty()->GetMemberInfo(player->GetParty()->GetPartyMemberSlot(player->GetID()));
		playerInfo->byRole = ROLEPLAY_TYPE_INVALID;
		*/
		
		/*
		sPARTY_MEMBER_INFO* playerInfo = &player->GetParty()->GetMemberInfo(player->GetParty()->GetPartyMemberSlot(player->GetID()));
		switch (playerInfo->byClass)
		{
				//TANK
			case PC_CLASS_NAMEK_FIGHTER:
			case PC_CLASS_MIGHTY_MAJIN: 
			case PC_CLASS_DARK_WARRIOR:
			case PC_CLASS_SHADOW_KNIGHT:
			case PC_CLASS_ULTI_MA:
			case PC_CLASS_GRAND_MA:
			{ playerInfo->byRole = ROLEPLAY_TYPE_DEFENCE; } break;

				//HEAL
			case PC_CLASS_NAMEK_MYSTIC: 
			case PC_CLASS_DENDEN_HEALER:
			case PC_CLASS_POCO_SUMMONER:
			{ playerInfo->byRole = ROLEPLAY_TYPE_HEAL; }  break;

				//DAMAGE
			case PC_CLASS_HUMAN_FIGHTER:
			case PC_CLASS_HUMAN_MYSTIC:
			case PC_CLASS_WONDER_MAJIN:
			case PC_CLASS_STREET_FIGHTER:
			case PC_CLASS_SWORD_MASTER:
			case PC_CLASS_CRANE_ROSHI:
			case PC_CLASS_TURTLE_ROSHI:
			case PC_CLASS_PLAS_MA:
			case PC_CLASS_KAR_MA:
			{ playerInfo->byRole = ROLEPLAY_TYPE_ATTACK; } break;
		}
		*/
	}
}

void CPartyMatching::Unregister(CPlayer* player, PARTYID partyid)
{
	std::map<PARTYID, sPARTY_MARCHING*>::iterator it;

	if (player == NULL)
	{
		it = m_map_PartyMatching.find(partyid);
		if (it != m_map_PartyMatching.end())
		{
			m_map_PartyMatching.erase(partyid);
			delete it->second;
		}
	}
	else
	{
		WORD resultcode = GAME_SUCCESS;

		if (player->GetPartyID() == INVALID_PARTYID || player->GetParty() == NULL)//Check if party exist
		{
			resultcode = GAME_PARTYMATCHING_REGISTER_WRONG_STATE;
		}
		else if (player->GetParty()->GetPartyLeaderID() != player->GetID()) //check if party leader
		{
			resultcode = GAME_COMMON_YOU_ARE_NOT_A_PARTY_LEADER;
		}
		else //last check: check if party already registered
		{
			it = m_map_PartyMatching.find(player->GetPartyID());
			if (it == m_map_PartyMatching.end())
				resultcode = GAME_PARTYMATCHING_PARTY_IS_NOT_REGISTER;
		}

		if (resultcode == GAME_SUCCESS)
		{
			m_map_PartyMatching.erase(player->GetPartyID());
			delete it->second;

			player->GetParty()->SetPartySearch(false);
		}

		CNtlPacket packet(sizeof(sGU_PARTY_MATCHING_UNREGISTER_RES));
		sGU_PARTY_MATCHING_UNREGISTER_RES * res = (sGU_PARTY_MATCHING_UNREGISTER_RES *)packet.GetPacketData();
		res->wOpCode = GU_PARTY_MATCHING_UNREGISTER_RES;
		res->wResultCode = resultcode;
		packet.SetPacketLen(sizeof(sGU_PARTY_MATCHING_UNREGISTER_RES));
		player->SendPacket(&packet);
	}
}

void CPartyMatching::GetPartyInfo(CPlayer* player, PARTYID partyid, BYTE byDungeonType, BYTE byRegion)
{
	CGameServer* app = (CGameServer*)g_pApp;

	std::map<PARTYID, sPARTY_MARCHING*>::const_iterator it = m_map_PartyMatching.find(partyid);
	if (it != m_map_PartyMatching.end())
	{
		sPARTY_MARCHING* matching = it->second;
		if (matching->pkParty)
		{
			CNtlPacket packet(sizeof(sGU_PARTY_MATCHING_INFO_RES));
			sGU_PARTY_MATCHING_INFO_RES * res = (sGU_PARTY_MATCHING_INFO_RES *)packet.GetPacketData();
			res->wOpCode = GU_PARTY_MATCHING_INFO_RES;
			res->byCount = matching->pkParty->GetPartyMemberCount();
			res->byLootingMethod = matching->pkParty->GetItemLootingMethod();

			//printf("GetPartyInfo matching->byDungeonType %u, matching->byDifficulty %u \n", matching->byDungeonType, matching->byDifficulty);
			
			for (int i = 0; i < res->byCount; i++)
			{
				sPARTY_MEMBER_INFO* memberinfo = &matching->pkParty->GetMemberInfo(i);

				NTL_WCSCPY_S(res->member[i].awchCharName, NTL_MAX_SIZE_CHAR_NAME + 1, memberinfo->awchMemberName);
				res->member[i].bPartyLeader = memberinfo->hHandle == matching->pkParty->GetPartyLeaderID() ? true : false;
				res->member[i].byLevel = memberinfo->byLevel;
				res->member[i].byRoleplay = GetRole(memberinfo->byClass);
				res->member[i].byServerChannelIndex = app->GetGsChannel();
			}

			packet.SetPacketLen(sizeof(sGU_PARTY_MATCHING_INFO_RES));
			player->SendPacket(&packet);
		}
	}
}


void CPartyMatching::Join(CPlayer* player, PARTYID partyid, BYTE byDungeonType, BYTE byRegion)
{
	WORD resultcode = GAME_SUCCESS;

	CNtlPacket packet(sizeof(sGU_PARTY_MATCHING_JOIN_RES));
	sGU_PARTY_MATCHING_JOIN_RES * res = (sGU_PARTY_MATCHING_JOIN_RES *)packet.GetPacketData();
	res->wOpCode = GU_PARTY_MATCHING_JOIN_RES;

	if (player->GetParty() == NULL && player->GetPartyID() == INVALID_PARTYID)
	{
		std::map<PARTYID, sPARTY_MARCHING*>::const_iterator it = m_map_PartyMatching.find(partyid);
		if (it != m_map_PartyMatching.end())
		{
			sPARTY_MARCHING* matching = it->second;
			if (matching->pkParty)
			{
				if (matching->pkParty->IsPartyMember(player->GetID()) == false)
				{
					if (matching->pkParty->AddPartyMember(player) == false)
						resultcode = GAME_PARTY_NO_ROOM_FOR_NEW_MEMBER;
				}
				else resultcode = GAME_PARTY_ALREADY_IN_PARTY;
			}
			else resultcode = GAME_PARTY_NO_SUCH_A_PARTY;
		}
		else resultcode = GAME_PARTYMATCHING_PARTY_IS_NOT_REGISTER;
	}
	else resultcode = GAME_PARTY_ALREADY_IN_PARTY;

	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_PARTY_MATCHING_JOIN_RES));
	player->SendPacket(&packet);

	if (resultcode == GAME_SUCCESS)
	{
		/*

		* ROLE_SELECTION

		CNtlPacket packet2(sizeof(sGU_PARTY_MATCHING_ROLEPLAY_SELECT_START_NFY));
		sGU_PARTY_MATCHING_ROLEPLAY_SELECT_START_NFY* res2 = (sGU_PARTY_MATCHING_ROLEPLAY_SELECT_START_NFY*)packet2.GetPacketData();
		res2->wOpCode = GU_PARTY_MATCHING_ROLEPLAY_SELECT_START_NFY;
		res2->byDungeonType = byDungeonType;
		res2->byType = byRegion;
		res2->byRoleplay[0] = ROLEPLAY_TYPE_DEFENCE;
		res2->byRoleplay[1] = ROLEPLAY_TYPE_HEAL;
		res2->byRoleplay[2] = ROLEPLAY_TYPE_ATTACK;
		packet2.SetPacketLen(sizeof(sGU_PARTY_MATCHING_ROLEPLAY_SELECT_START_NFY));
		player->SendPacket(&packet2);

		sPARTY_MEMBER_INFO* playerInfo = &player->GetParty()->GetMemberInfo(player->GetParty()->GetPartyMemberSlot(player->GetID()));
		playerInfo->byRole = ROLEPLAY_TYPE_INVALID;
		*/



		
	}
}

void CPartyMatching::role(CPlayer* player, PARTYID partyId, BYTE byType, BYTE byRoleplay)
{
	WORD resultcode = GAME_SUCCESS;

	if (byRoleplay == ROLEPLAY_TYPE_INVALID)
		resultcode = GAME_PARTYMATCHING_ROLEPLAY_CANCEL;

	CNtlPacket packet(sizeof(sGU_PARTY_MATCHING_ROLEPLAY_RES));
	sGU_PARTY_MATCHING_ROLEPLAY_RES* res = (sGU_PARTY_MATCHING_ROLEPLAY_RES*)packet.GetPacketData();
	res->wOpCode = GU_PARTY_MATCHING_ROLEPLAY_RES;
	res->wResultCode = resultcode;
	res->hObject = player->GetID();
	res->byRoleplay = byRoleplay;
	packet.SetPacketLen(sizeof(sGU_PARTY_MATCHING_ROLEPLAY_RES));
	player->SendPacket(&packet);

	printf("role byType %u, byRoleplay %u, partyId %u \n", byType, byRoleplay, partyId);

	//sPARTY_MEMBER_INFO* playerInfo = &player->GetParty()->GetMemberInfo(player->GetParty()->GetPartyMemberSlot(player->GetID()));
	//playerInfo->byRole = byRoleplay;
}

void CPartyMatching::EnterReq(CPlayer* player)
{
	WORD resultcode = GAME_SUCCESS;

	if (player->GetPartyID() == INVALID_PARTYID)
		resultcode = GAME_FAIL;
	else if (player->GetParty()->GetPartyLeaderID() != player->GetID())
		resultcode = GAME_FAIL;
	else if (player->GetParty()->IsSomeoneInDynamic(INVALID_WORLDID) != false)
		resultcode = GAME_FAIL;
	else if (player->GetRankBattleRoomTblidx() != INVALID_TBLIDX)
		resultcode = GAME_FAIL;
	else if (player->GetParty()->IsInPartySearch() == false)
		resultcode = GAME_FAIL;
	else if (player->GetParty()->IsEveryoneInLeaderRange(player, NTL_MAX_RADIUS_OF_VISIBLE_AREA) == false)
	{
		resultcode = GAME_FAIL;
		std::map<PARTYID, sPARTY_MARCHING*>::const_iterator it = m_map_PartyMatching.find(player->GetPartyID());
		if (it != m_map_PartyMatching.end())
		{
			sPARTY_MARCHING* matching = it->second;
			if (matching->pkParty)
			{
				for (BYTE i = 0; i < player->GetParty()->GetPartyMemberCount(); i++)
				{
					sPARTY_MEMBER_INFO* memberinfo = &matching->pkParty->GetMemberInfo(i);
					CPlayer* cPlayer = g_pObjectManager->GetPC(memberinfo->hHandle);

					if (cPlayer->GetCharStateID() != CHARSTATE_TELEPORTING && cPlayer->GetID() != player->GetID())
						cPlayer->StartTeleport(player->GetCurLoc(), player->GetCurDir(), 1, TELEPORT_TYPE_COMMAND);
				}
			}
		}
		
		
		//player->GetParty()->StartPartyTeleport(player);
	}
	
	if (resultcode == GAME_SUCCESS)
	{
		CNtlPacket packet(sizeof(sGU_PARTY_MATCHING_ENTER_DUNGEON_AGREE_START_NFY));
		sGU_PARTY_MATCHING_ENTER_DUNGEON_AGREE_START_NFY* res = (sGU_PARTY_MATCHING_ENTER_DUNGEON_AGREE_START_NFY*)packet.GetPacketData();
		res->wOpCode = GU_PARTY_MATCHING_ENTER_DUNGEON_AGREE_START_NFY;
		packet.SetPacketLen(sizeof(sGU_PARTY_MATCHING_ENTER_DUNGEON_AGREE_START_NFY));
		player->SendPacket(&packet);
	}
}

void CPartyMatching::EnterRes(CPlayer* player)
{
	
	CGameServer* app = (CGameServer*)g_pApp;

	std::map<PARTYID, sPARTY_MARCHING*>::const_iterator it = m_map_PartyMatching.find(player->GetPartyID());
	if (it != m_map_PartyMatching.end())
	{
		sPARTY_MARCHING* matching = it->second;
		if (matching->pkParty)
		{
			WORD wResultcode = GAME_SUCCESS;
			switch (matching->byDungeonType)
			{
				case ePARTY_MATCHING_DUNGEON_TYPE_ULTIMATE_DUNGEON:
				{
					if (player->GetParty() && player->GetPartyID() != INVALID_PARTYID)
					{
						if (player->GetParty()->GetPartyLeaderID() == player->GetID())
						{
							if (player->GetParty()->IsSomeoneInDynamic(INVALID_WORLDID) == false)
							{
								if (player->GetRankBattleRoomTblidx() == INVALID_TBLIDX)
								{
									CUltimateDungeon* pDungeon = g_pDungeonManager->CreateUltimateDungeon(player, matching->byRegionType, matching->byDifficulty);
									if (pDungeon == NULL) wResultcode = GAME_PARTY_DUNGEON_IS_NOT_CREATED;
								}
								else wResultcode = GAME_RANKBATTLE_MEMBER_ALREADY_JOINED_RANKBATTLE;
							}
							else wResultcode = GAME_PARTYMATCHING_ANY_MEMBER_IN_DYNAMIC_WORLD;
						}
						else wResultcode = GAME_COMMON_YOU_ARE_NOT_A_PARTY_LEADER;
					}
					else wResultcode = GAME_COMMON_YOU_ARE_NOT_IN_A_PARTY;


					CNtlPacket packet(sizeof(sGU_ULTIMATE_DUNGEON_ENTER_RES));
					sGU_ULTIMATE_DUNGEON_ENTER_RES* res = (sGU_ULTIMATE_DUNGEON_ENTER_RES*)packet.GetPacketData();
					res->wOpCode = GU_ULTIMATE_DUNGEON_ENTER_RES;
					res->byDifficulty = matching->byDifficulty;
					res->wResultCode = wResultcode;
					packet.SetPacketLen(sizeof(sGU_ULTIMATE_DUNGEON_ENTER_RES));
					player->SendPacket(&packet);
				} break;
				case ePARTY_MATCHING_DUNGEON_TYPE_TIMEMACHINE_QUEST:
				{
					CNtlPacket packet(sizeof(sGU_TIMEQUEST_ENTER_RES));
					sGU_TIMEQUEST_ENTER_RES* res = (sGU_TIMEQUEST_ENTER_RES*)packet.GetPacketData();
					res->wOpCode = GU_TIMEQUEST_ENTER_RES;
					wResultcode = GAME_SUCCESS;
					res->byDifficult = matching->byDifficulty;
					//res->hTimeQuestNpc = req->hTimeQuestNpc;

					if (matching->byDifficulty >= MAX_TIMEQUEST_DIFFICULTY)
						wResultcode = GAME_FAIL;
					else if (player->GetPartyID() == INVALID_PARTYID)
						wResultcode = GAME_PARTY_YOU_ARE_NOT_IN_PARTY;
					else if (player->GetParty()->GetPartyLeaderID() != player->GetID())
						wResultcode = GAME_PARTY_ONLY_ALLOWED_TO_PARTY_LEADER;
					else if (player->GetParty()->IsSomeoneInDynamic(INVALID_WORLDID))
						wResultcode = GAME_PARTYMATCHING_ANY_MEMBER_IN_DYNAMIC_WORLD;
					else if (app->IsDojoChannel() == true)
						wResultcode = GAME_FAIL;
					else
					{
						res->wResultCode = g_pDungeonManager->CreateTimeQuest(player, matching->byRegionType, matching->byDifficulty, TIMEQUEST_MODE_PARTY);
					}

					res->wResultCode = wResultcode;
					packet.SetPacketLen(sizeof(sGU_TIMEQUEST_ENTER_RES));
					player->SendPacket(&packet);
				} break;
				case ePARTY_MATCHING_DUNGEON_TYPE_CC_BATTLE_DUNGEON:
				{
					if (player->GetParty() && player->GetPartyID() != INVALID_PARTYID)
					{
						if (player->GetParty()->GetPartyLeaderID() == player->GetID())
						{
							BYTE byBeginStage = 1;
							CItem* pItem = NULL;

							CBattleDungeon* pDungeon = g_pDungeonManager->CreateBattleDungeon(player, wResultcode, byBeginStage);
							if (pDungeon == NULL)
								wResultcode = GAME_PARTY_DUNGEON_IS_NOT_CREATED;
						}
						else wResultcode = GAME_COMMON_YOU_ARE_NOT_A_PARTY_LEADER;
					}
					else wResultcode = GAME_COMMON_YOU_ARE_NOT_IN_A_PARTY;

					CNtlPacket packet(sizeof(sGU_BATTLE_DUNGEON_ENTER_RES));
					sGU_BATTLE_DUNGEON_ENTER_RES* res = (sGU_BATTLE_DUNGEON_ENTER_RES*)packet.GetPacketData();
					res->wOpCode = GU_BATTLE_DUNGEON_ENTER_RES;
					res->wResultCode = wResultcode;
					packet.SetPacketLen(sizeof(sGU_BATTLE_DUNGEON_ENTER_RES));
					player->SendPacket(&packet);
				} break;
				case ePARTY_MATCHING_DUNGEON_TYPE_RANK_BATTLE:
				{

				} break;
			}
			if (wResultcode == GAME_SUCCESS)
			{

				//sPARTY_MEMBER_INFO* playerInfo = &player->GetParty()->GetMemberInfo(player->GetParty()->GetPartyMemberSlot(player->GetID()));

				
				/*
				std::vector<TBLIDX> m_buffs{ 850076, 850077, 850078 };

				for (TBLIDX i : m_buffs)
				{
					if (CBuff* buff = player->GetBuffManager()->FindBuff(i, 1))
						player->GetBuffManager()->RemoveBuff(buff->GetBuffIndex(), buff->GetBuffType(), CBuff::BUFF_REMOVAL_REASON_BY_ITSELF);
				}

				sBUFF_INFO buffInfo2;
				eSYSTEM_EFFECT_CODE effectCode2[NTL_MAX_EFFECT_IN_ITEM];
				sUSE_ITEM_TBLDAT* pUseItemTbldat = (sUSE_ITEM_TBLDAT*)g_pTableContainer->GetUseItemTable()->FindData(m_buffs[playerInfo->byRole]);
				if (pUseItemTbldat)
				{
					buffInfo2.buffIndex = INVALID_BYTE;
					if (playerInfo->byRole != ROLEPLAY_TYPE_INVALID)
						buffInfo2.sourceTblidx = m_buffs[playerInfo->byRole];
					buffInfo2.dwTimeRemaining = 700000;
					buffInfo2.dwInitialDuration = 700000;
					buffInfo2.bySourceType = DBO_OBJECT_SOURCE_ITEM;

					for (int x = 0; x < NTL_MAX_EFFECT_IN_ITEM; x++)
					{
						effectCode2[x] = g_pTableContainer->GetSystemEffectTable()->GetEffectCodeWithTblidx(pUseItemTbldat->aSystem_Effect[x]);
						buffInfo2.aBuffParameter[x].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_DEFAULT;
						buffInfo2.aBuffParameter[x].buffParameter.dwRemainTime = 700000;
						buffInfo2.aBuffParameter[x].buffParameter.fParameter = (float)pUseItemTbldat->aSystem_Effect_Value[x];
					}

					WORD wTemp;
					if (playerInfo->byRole != ROLEPLAY_TYPE_INVALID)
						player->GetBuffManager()->RegisterSubBuff(&buffInfo2, effectCode2, player->GetID(), pUseItemTbldat->byBuff_Group, wTemp, pUseItemTbldat->abySystem_Effect_Type);
				}
				*/

				for (int i = 0; i < player->GetParty()->GetPartyMemberCount(); i++)
				{
					sPARTY_MEMBER_INFO* memberinfo = &matching->pkParty->GetMemberInfo(i);
					CPlayer* cPlayer = g_pObjectManager->GetPC(memberinfo->hHandle);
					printf("EnterRes accountID %u, memberinfo->byRole %u \n", cPlayer->GetAccountID(), GetRole(memberinfo->byClass));

					std::vector<TBLIDX> m_buffs{ 850076, 850077, 850078 };

					for (TBLIDX i : m_buffs)
					{
						if (CBuff* buff = cPlayer->GetBuffManager()->FindBuff(i, DBO_OBJECT_SOURCE_ITEM))
							cPlayer->GetBuffManager()->RemoveBuff(buff->GetBuffIndex(), buff->GetBuffType(), CBuff::BUFF_REMOVAL_REASON_BY_ITSELF);
					}

					sUSE_ITEM_TBLDAT* pUseItemTbldat = (sUSE_ITEM_TBLDAT*)g_pTableContainer->GetUseItemTable()->FindData(m_buffs[GetRole(memberinfo->byClass)]);
					if (pUseItemTbldat)
					{
						sBUFF_INFO buffInfo;
						buffInfo.buffIndex = INVALID_BYTE;
						buffInfo.sourceTblidx = m_buffs[GetRole(memberinfo->byClass)];
						buffInfo.dwTimeRemaining = pUseItemTbldat->dwKeepTimeInMilliSecs;
						buffInfo.dwInitialDuration = pUseItemTbldat->dwKeepTimeInMilliSecs;
						buffInfo.bySourceType = DBO_OBJECT_SOURCE_ITEM;

						eSYSTEM_EFFECT_CODE effectCode[NTL_MAX_EFFECT_IN_ITEM];
						effectCode[0] = g_pTableContainer->GetSystemEffectTable()->GetEffectCodeWithTblidx(pUseItemTbldat->aSystem_Effect[0]);
						effectCode[1] = g_pTableContainer->GetSystemEffectTable()->GetEffectCodeWithTblidx(pUseItemTbldat->aSystem_Effect[1]);

						for (int x = 0; x < NTL_MAX_EFFECT_IN_ITEM; x++)
						{
							switch (effectCode[x])
							{
							case ACTIVE_ALL_OFFENCE_UP: case ACTIVE_ALL_DEFENCE_UP: case ACTIVE_HOT_POWER_UP_IN_PERCENT:
							{
								buffInfo.aBuffParameter[x].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_DEFAULT;
								buffInfo.aBuffParameter[x].buffParameter.dwRemainTime = pUseItemTbldat->dwKeepTimeInMilliSecs;
								buffInfo.aBuffParameter[x].buffParameter.fParameter = (float)pUseItemTbldat->aSystem_Effect_Value[x];
							}
							break;
							case ACTIVE_EP_OVER_TIME:
							{
								buffInfo.aBuffParameter[x].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_HOT;
								buffInfo.aBuffParameter[x].buffParameter.dwRemainTime = pUseItemTbldat->dwKeepTimeInMilliSecs;
								buffInfo.aBuffParameter[x].buffParameter.fParameter = (float)pUseItemTbldat->aSystem_Effect_Value[x];
							}
							break;
							default: break;
							}
						}
						WORD wTemp;
						cPlayer->GetBuffManager()->RegisterSubBuff(&buffInfo, effectCode, cPlayer->GetID(), pUseItemTbldat->byBuff_Group, wTemp, pUseItemTbldat->abySystem_Effect_Type);
					}
				}
				Unregister(player, player->GetPartyID());
			}
		}
	}
	

	CNtlPacket packet2(sizeof(sGU_PARTY_MATCHING_ENTER_DUNGEON_AGREE_NFY));
	sGU_PARTY_MATCHING_ENTER_DUNGEON_AGREE_NFY* res2 = (sGU_PARTY_MATCHING_ENTER_DUNGEON_AGREE_NFY*)packet2.GetPacketData();
	res2->wOpCode = GU_PARTY_MATCHING_ENTER_DUNGEON_AGREE_NFY;
	res2->hObject = player->GetID();
	res2->byAgreeType = 1;
	packet2.SetPacketLen(sizeof(sGU_PARTY_MATCHING_ENTER_DUNGEON_AGREE_NFY));
	player->SendPacket(&packet2);
}