#include "stdafx.h"
#include "NtlPacketGU.h"
#include "NtlPacketUG.h"
#include "NtlPacketGM.h"
#include "NtlPacketGT.h"
#include "NtlPacketGQ.h"
#include "NtlResultCode.h"
#include "GameServer.h"
#include "GameData.h"
#include "GameMain.h"
#include "SubNeighborServerInfoManager.h"
#include "NtlRandom.h"
#include "CPlayer.h"
#include "trade.h"
#include "privateshop.h"
#include "Monster.h"
#include "Guild.h"
#include "ItemPet.h"
#include "SummonPet.h"
#include "item.h"
#include "ItemDrop.h"
#include "ItemManager.h"
#include "TriggerObject.h"

#include "MerchantTable.h"
#include "HLSItemTable.h"
#include "DragonBallRewardTable.h"
#include "UseItemTable.h"
#include "QuestProbabilityTable.h"
#include "ItemDisassembleTable.h"
#include "ItemUpgradeRateNewTable.h"
#include "PortalTable.h"
#include "ItemRecipeTable.h"
#include "QuestRewardSelectTable.h"
#include "QuestRewardTable.h"
#include "ItemEnchantRateTable.h"
#include "SystemEffectTable.h"
#include "CharTitleTable.h"
#include "TextAllTable.h"
#include "EventSystemTable.h"
#include "ActionTable.h"

#include "DojoManager.h"
#include "RankBattle.h"
#include "PartyMatching.h"
#include "freebattle.h"
#include "gm.h"
#include "TriggerManager.h"
#include "DungeonManager.h"
#include "WpsAlgoObject.h"
#include "DynamicFieldSystemEvent.h"
#include "EventSystemEvent.h"
#include "DragonballHunt.h"
#include "DragonballScramble.h"
#include "ExpEvent.h"
#include "HoneyBeeEvent.h"
#include "WinterEvent.h"
#include "BattleRoyaleEvent.h"

#include "NtlNavi.h"
#include "battle.h"
#include "DojoWar.h"
#include "BudokaiManager.h"
#include "BusSystem.h"
#include "scsManager.h"

// Helper functions for WCHAR string literals (Linux compatibility)
static const WCHAR* GetUnnamedPartyName() {
	static WCHAR s_wszUnnamed[NTL_MAX_SIZE_PARTY_NAME + 1];
	static bool initialized = false;
	if (!initialized) {
		WCharTLiteralToWCHAR(L"Unnamed", s_wszUnnamed, sizeof(s_wszUnnamed)/sizeof(WCHAR));
		initialized = true;
	}
	return s_wszUnnamed;
}

static const WCHAR* GetEmptyString() {
	static WCHAR s_wszEmpty[1] = {0};
	return s_wszEmpty;
}

//--------------------------------------------------------------------------------------//
//		WHEN RECEIVE INVALID PACKET
//--------------------------------------------------------------------------------------//
void CClientSession::OnInvalid(CNtlPacket *pPacket)
{
	sNTLPACKETHEADER * pHeader = (sNTLPACKETHEADER *)pPacket->GetPacketData();

	//printf("Session %u receive invalid opcode %u \n", GetHandle(), pHeader->wOpCode);
}


//--------------------------------------------------------------------------------------//
//		Log into Game Server
//--------------------------------------------------------------------------------------//
void CClientSession::RecvGameEnterReq(CNtlPacket *pPacket)
{
	CGameServer* app = (CGameServer*)g_pApp;

	sUG_GAME_ENTER_REQ * req = (sUG_GAME_ENTER_REQ *)pPacket->GetPacketData();
	
	WORD resultcode = GAME_SUCCESS;
	//ERR_LOG(LOG_USER, "Account %d connected\n", req->accountId);

	if(req->accountId == 0 || req->charId == 0)
	{
		ERR_LOG(LOG_USER,"req->charId %d accountId %d invalid \n", req->charId, req->accountId);
		resultcode = GAME_INPUT_PARAMETER_WRONG;
	}
	else
	{
		CCharacter* pPlayer = g_pObjectManager->FindByAccount(req->accountId);
		if(pPlayer && pPlayer->IsInitialized())
		{
			ERR_LOG(LOG_USER,"req->charId %u accountId %u ALREADY EXIST \n", req->charId, req->accountId);
			resultcode = GAME_USER_EXIST;

			if(pPlayer->GetClientSession())
				pPlayer->GetClientSession()->Disconnect(false);
		}
	}

	if(resultcode == GAME_SUCCESS)
	{
		CPlayer* player = (CPlayer*)g_pObjectManager->CreateCharacter(OBJTYPE_PC, req->charId);
		if (player)
		{
			player->SetCharID(req->charId);
			player->SetAccountID(req->accountId);
			player->SetClientSession(this);
			player->SetClientSessionID(this->GetHandle());

			this->SetPlayer(player);

			if (req->bTutorialMode)
				player->SetTutorial(true);

			//auth check 
			CNtlPacket pMaster(sizeof(sGM_LOGIN_REQ));
			sGM_LOGIN_REQ * rMaster = (sGM_LOGIN_REQ *)pMaster.GetPacketData();
			rMaster->wOpCode = GM_LOGIN_REQ;
			rMaster->accountId = req->accountId;
			rMaster->handle = player->GetID();
			rMaster->serverChannelId = app->GetGsChannel();
			rMaster->serverId = app->GetGsServerId();
			rMaster->charId = req->charId;
			NTL_SAFE_WCSCPY(rMaster->awchCharName, player->GetCharName());
			memcpy(rMaster->abyAuthKey, req->abyAuthKey, NTL_MAX_SIZE_AUTH_KEY);
			pMaster.SetPacketLen(sizeof(sGM_LOGIN_REQ));
			app->SendTo(app->GetMasterServerSession(), &pMaster);
		}
		else
		{
			ERR_LOG(LOG_USER, "Creating player failed. Disconnecting..");
			this->Disconnect(false);
		}
	}
	else
	{
		CNtlPacket packet(sizeof(sGU_GAME_ENTER_RES));
		sGU_GAME_ENTER_RES * res = (sGU_GAME_ENTER_RES *)packet.GetPacketData();
		res->wOpCode = GU_GAME_ENTER_RES;
		res->wResultCode = resultcode;
		packet.SetPacketLen( sizeof(sGU_GAME_ENTER_RES) );
		app->Send(GetHandle(), &packet);

		this->Disconnect(false);
	}
}


//--------------------------------------------------------------------------------------//
//		Enter World
//--------------------------------------------------------------------------------------//
void CClientSession::RecvEnterWorld(CNtlPacket * pPacket)
{
	CGameServer* app = (CGameServer*)g_pApp;

	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	bool bSendAvatarWorldInfo = true;
	WORLDID preWorldID = cPlayer->GetWorldID();
	bool bNewWorldDynamic = false;
	BYTE byRuleType = GAMERULE_NORMAL;

	if (cPlayer->GetCharStateID() == CHARSTATE_TELEPORTING)
	{
		bool bIsInRange = cPlayer->IsInRange(cPlayer->GetTeleportLoc(), NTL_MAX_RADIUS_OF_VISIBLE_AREA + 50.f);

		if (cPlayer->GetWorldID() == cPlayer->GetTeleportWorldID() && bIsInRange == true) //dont send worldinfo when we teleport within 100m range
		{
			bSendAvatarWorldInfo = false;
		}
	}

	if (bSendAvatarWorldInfo)
	{
		// cancel dice
		cPlayer->CancelDice();

		CNtlPacket packet(sizeof(sGU_AVATAR_WORLD_INFO));
		sGU_AVATAR_WORLD_INFO * res = (sGU_AVATAR_WORLD_INFO *)packet.GetPacketData();
		res->wOpCode = GU_AVATAR_WORLD_INFO;
		res->worldInfo.sRuleInfo.byRuleType = GAMERULE_NORMAL;

		CWorld* pWorld = NULL;
		bool bSuccess = false;

		if (cPlayer->GetCharStateID() == CHARSTATE_TELEPORTING)
		{
			pWorld = app->GetGameMain()->GetWorldManager()->FindWorld(cPlayer->GetTeleportWorldID());
			if (pWorld)
			{
				cPlayer->GetTeleportLoc().CopyTo(res->vCurLoc);
				cPlayer->GetTeleportDir().CopyTo(res->vCurDir);
				pWorld->CopyToInfo(&res->worldInfo);

				if (cPlayer->GetTMQ()) //send tmq rule info if inside
				{
					cPlayer->GetTMQ()->CopyRuleInfoTo(&res->worldInfo.sRuleInfo.sTimeQuestRuleInfo);
				}

				bSuccess = true;
			}
			else ERR_LOG(LOG_GENERAL, "ERROR: Could not find WorldID %u. Teleport Failed", cPlayer->GetTeleportWorldID());
		}
		else if (cPlayer->GetTMQ() && cPlayer->IsTutorial())
		{
			pWorld = app->GetGameMain()->GetWorldManager()->FindWorld(cPlayer->GetTMQ()->GetWorld()->GetID());
			if (pWorld)
			{
				pWorld->GetTbldat()->vStart1Loc.CopyTo(res->vCurLoc);
				pWorld->GetTbldat()->vStart1Dir.CopyTo(res->vCurDir);
				pWorld->CopyToInfo(&res->worldInfo);

				cPlayer->GetTMQ()->CopyRuleInfoTo(&res->worldInfo.sRuleInfo.sTimeQuestRuleInfo);

				bSuccess = true;
			}
			else ERR_LOG(LOG_GENERAL, "ERROR: Could not find WorldID %u. Teleport Failed", cPlayer->GetTMQ()->GetWorld()->GetID());
		}
		else //login
		{
			pWorld = app->GetGameMain()->GetWorldManager()->FindWorld(cPlayer->GetWorldID());
			if (pWorld && pWorld->IsInBoundary(cPlayer->GetCurLoc()))
			{
				if (pWorld->GetRuleType() == GAMERULE_NORMAL
					|| pWorld->GetRuleType() == GAMERULE_DOJO
					|| pWorld->GetRuleType() == GAMERULE_MINORMATCH
					|| pWorld->GetRuleType() == GAMERULE_MAJORMATCH
					|| pWorld->GetRuleType() == GAMERULE_FINALMATCH
					|| pWorld->GetRuleType() == GAMERULE_TEINKAICHIBUDOKAI) //required when teleport from prelim to budokai map
				{
					bSuccess = true;
					cPlayer->GetCurLoc().CopyTo(res->vCurLoc);
					cPlayer->GetCurDir().CopyTo(res->vCurDir);
					pWorld->CopyToInfo(&res->worldInfo);
				}
				else
				{
					bSuccess = true;

					pWorld->GetTbldat()->outWorldLoc.CopyTo(res->vCurLoc);
					pWorld->GetTbldat()->outWorldDir.CopyTo(res->vCurDir);

					CWorld* pOutWorld = app->GetGameMain()->GetWorldManager()->FindWorld(pWorld->GetTbldat()->outWorldTblidx);
					if (pOutWorld)
					{
						pOutWorld->CopyToInfo(&res->worldInfo);
					}
					else
					{
						ERR_LOG(LOG_SYSTEM, "Could not find outworldidx = %u", pWorld->GetTbldat()->outWorldTblidx);
					}
				}
			}
		}

		if (!bSuccess) //if failed login/teleport then send to default position
		{
			//ERR_LOG(LOG_GENERAL, "Player %u Login/teleport failed. Use default world..", cPlayer->GetCharID());
			if (app->GetGsChannel() == DOJO_CHANNEL_INDEX)
			{
				ERR_LOG(LOG_GENERAL, "Failed to login into event channel. Disconnect player... ");
				this->Disconnect(false);
				return;
			}

			pWorld = app->GetGameMain()->GetWorldManager()->FindWorld(cPlayer->GetBindWorldID());
			if (pWorld)
			{
				res->vCurLoc = cPlayer->GetBindLoc();
				res->vCurDir = cPlayer->GetBindDir();
				pWorld->CopyToInfo(&res->worldInfo);
			}
			else
			{
				pWorld = app->GetGameMain()->GetWorldManager()->GetDefaultWorld();
				if (pWorld)
				{
					app->GetGameMain()->GetWorldManager()->GetDefaultWorldLoc().CopyTo(res->vCurLoc);
					app->GetGameMain()->GetWorldManager()->GetDefaultWorldDir().CopyTo(res->vCurDir);
					pWorld->CopyToInfo(&res->worldInfo);
				}
				else
				{
					ERR_LOG(LOG_GENERAL, "Cant find default world. Disconnect player.");
					this->Disconnect(false);
					return;
				}
			}
		}
		//printf("EnterWorld: %f %f %f, bSuccess %i, cPlayer->GetCharStateID() %u \n", res->vCurLoc.x, res->vCurLoc.y, res->vCurLoc.z, bSuccess, cPlayer->GetCharStateID());

		cPlayer->SetWorldID(res->worldInfo.worldID);
		cPlayer->SetCurLoc(res->vCurLoc, pWorld);
		cPlayer->SetCurDir(res->vCurDir);
		cPlayer->SetMapNameTblidx(GetNaviEngine()->GetTextAllIndex(pWorld->GetNaviInstanceHandle(), res->vCurLoc.x, res->vCurLoc.z));

		bNewWorldDynamic = pWorld->GetTbldat()->bDynamic;
		byRuleType = pWorld->GetTbldat()->byWorldRuleType;

		//load dojos
		res->byDojoCount = 0;
		g_pDojoManager->FillDojoData(res->sDojoData, res->byDojoCount);

		packet.SetPacketLen(sizeof(sGU_AVATAR_WORLD_INFO));
		g_pApp->Send(GetHandle(), &packet);


		//Enables / Disables sub buffs
		cPlayer->GetBuffManager()->CheckSubBuffs(res->worldInfo.sRuleInfo.byRuleType);

		//load zone info
		if (cPlayer->GetCurWorldZone())
		{
			CNtlPacket pZone(sizeof(sGU_AVATAR_ZONE_INFO));
			sGU_AVATAR_ZONE_INFO * rZone = (sGU_AVATAR_ZONE_INFO *)pZone.GetPacketData();
			rZone->wOpCode = GU_AVATAR_ZONE_INFO;
			cPlayer->GetCurWorldZone()->CopyToInfo(rZone->zoneInfo);
			pZone.SetPacketLen(sizeof(sGU_AVATAR_ZONE_INFO));
			g_pApp->Send(GetHandle(), &pZone);
		}
	}
	else
	{
		cPlayer->SetCurLoc(cPlayer->GetTeleportLoc(), cPlayer->GetCurWorld());
		cPlayer->SetCurDir(cPlayer->GetTeleportDir());
	}

	CNtlPacket packet2(sizeof(sGU_ENTER_WORLD_RES));
	sGU_ENTER_WORLD_RES * res2 = (sGU_ENTER_WORLD_RES *)packet2.GetPacketData();
	res2->wOpCode = GU_ENTER_WORLD_RES;
	res2->wResultCode = GAME_SUCCESS;
	packet2.SetPacketLen(sizeof(sGU_ENTER_WORLD_RES));
	g_pApp->Send(GetHandle(), &packet2);


	//remove buffs when enter dungeon like UD CCBD etc
	if (cPlayer->GetCharStateID() == CHARSTATE_TELEPORTING)
	{
		if (bNewWorldDynamic && cPlayer->GetWorldID() != preWorldID)
		{
			cPlayer->GetBuffManager()->RemoveAllBuffExceptApplySelf();
			//cPlayer->GetBuffManager()->CheckSubBuffs(byRuleType);
		}
	}

	//update attributes // MAKE SURE THIS IS BELOW "REMOVEALLBUFF" to make sure stats getting updated
	cPlayer->GetCharAtt()->CalculateAll();

	if (bSendAvatarWorldInfo)
	{
		//enter the world
		if (app->GetGameMain()->GetWorldManager()->ChangeWorld(cPlayer, cPlayer->GetWorldID()) != NTL_SUCCESS)
		{
			if (app->GetGameMain()->GetWorldManager()->EnterObject(cPlayer, cPlayer->GetWorldID()) != NTL_SUCCESS)
			{
				//ERR_LOG(LOG_USER, "ERROR: Char %u Enter world failed. WorldTblidx %u WorldID %u Loc(%f,%f,%f)", cPlayer->GetCharID(), res->worldInfo.tblidx, res->worldInfo.worldID, res->vCurLoc.x, res->vCurLoc.y, res->vCurLoc.z);
				return;
			}
		}
	}

	//we have to do this after we entered world or issues when enter rank battle
	if (cPlayer->GetCharStateID() == CHARSTATE_TELEPORTING)
	{
		cPlayer->ResetDirectPlay(); //reset teleport data
		cPlayer->event_TeleportProposal(); //reset teleport shit
	}

	cPlayer->SendCharStateSpawning(cPlayer->GetTeleportType()); //if we dont send spawning, then player will always faint after revivin

	CNtlPacket packetEnd(sizeof(sGU_ENTER_WORLD_COMPLETE));
	sGU_ENTER_WORLD_COMPLETE * resEnd = (sGU_ENTER_WORLD_COMPLETE *)packetEnd.GetPacketData();
	resEnd->wOpCode = GU_ENTER_WORLD_COMPLETE;
	packetEnd.SetPacketLen(sizeof(sGU_ENTER_WORLD_COMPLETE));
	g_pApp->Send(GetHandle(), &packetEnd);
}


//--------------------------------------------------------------------------------------//
//		Character loading complete
//--------------------------------------------------------------------------------------//
void CClientSession::RecvLoadingCompleteNfy(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;
	//ERR_LOG(LOG_USER, "RecvLoadingCompleteNfy player %d \n", this->GetClientAccID());
	cPlayer->SetSkipSave(false); //only start storing player data when login complete

	if (m_bCashItemInfoLoaded == false)
	{
		m_bCashItemInfoLoaded = true;

		// SEND REQUEST TO DB SERVER TO LOAD CASH ITEMS
		CNtlPacket packet(sizeof(sGQ_CASHITEM_INFO_REQ));
		sGQ_CASHITEM_INFO_REQ * res = (sGQ_CASHITEM_INFO_REQ *)packet.GetPacketData();
		res->wOpCode = GQ_CASHITEM_INFO_REQ;
		res->accountId = cPlayer->GetAccountID();
		res->charId = cPlayer->GetCharID();
		res->handle = cPlayer->GetID();
		packet.SetPacketLen(sizeof(sGQ_CASHITEM_INFO_REQ));
		app->SendTo(app->GetQueryServerSession(), &packet);
	}
}

//--------------------------------------------------------------------------------------//
//		Character ready to spawn
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharReadyToSpawn(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	if (GetUserState() == NTL_USER_STATE_ENTERING_GAME)
	{
		SetUserState(NTL_USER_STATE_IN_GAME);

		cPlayer->UpdateMaxRpBalls();

		//update current ap because in game its bugged when login
		CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_AP));
		sGU_UPDATE_CHAR_AP * res = (sGU_UPDATE_CHAR_AP *)packet.GetPacketData();
		res->wOpCode = GU_UPDATE_CHAR_AP;
		res->handle = cPlayer->GetID();
		res->curAP = cPlayer->GetCurAP();
		res->maxAP = cPlayer->GetCharAtt()->GetLastMaxAP();
		packet.SetPacketLen(sizeof(sGU_UPDATE_CHAR_AP));
		app->Send(GetHandle(), &packet);

		//update current rp and balls because in game its bugged when login
		CNtlPacket packet2(sizeof(sGU_UPDATE_CHAR_RP));
		sGU_UPDATE_CHAR_RP * res2 = (sGU_UPDATE_CHAR_RP *)packet2.GetPacketData();
		res2->wOpCode = GU_UPDATE_CHAR_RP;
		res2->handle = cPlayer->GetID();
		res2->bHitDelay = false;
		res2->wCurRP = cPlayer->GetCurRP();
		res2->wMaxRP = cPlayer->GetCharAtt()->GetLastMaxRP();
		packet2.SetPacketLen(sizeof(sGU_UPDATE_CHAR_RP));
		app->Send(GetHandle(), &packet2);

		//anti-air stuck fix
		if (cPlayer->GetAirState() == AIR_STATE_ON)
		{
			if (cPlayer->GetSkillManager()->FindSkillWithSystemEffectCode(PASSIVE_AIR_MASTERY) == NULL && cPlayer->GetBuffManager()->HasAirSkillBuff() == false)
				cPlayer->SendCharStateFalling(NTL_MOVE_HEIGHT_DOWN);
		}

		g_pDragonballHuntEvent->LoadEvent(cPlayer);

		if (cPlayer->GetDragonballScramble() == false)
			g_pDragonballScramble->LoadEvent(cPlayer);
		else
			g_pDragonballScramble->RefreshEvent(cPlayer);

		g_pDynamicFieldSystemEvent->LoadDynamicField(cPlayer);
		g_pEventSystemEvent->LoadEvent(cPlayer);
		g_pExpEvent->LoadEvent(cPlayer);
		g_pBudokaiManager->LoadBudokaiStateInfo(cPlayer);
		//g_pHoneyBeeEvent->LoadEvent(GetHandle());
		//g_pWinterEvent->LoadEvent(GetHandle());

		if (cPlayer->GetCurWorld() && cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_DOJO) //if enter dojo, then add item
		{
			if (CDojo * pDojo = g_pDojoWarManager->GetDojo(cPlayer->GetGuildID()))
			{
				if(cPlayer->GetPlayerItemContainer()->GetItemByIdx(19451) == NULL)
					g_pItemManager->CreateItem(cPlayer, 19451, 1);

				pDojo->SendSealState(GetHandle());
			}
		}

		std::vector<TBLIDX> m_buffs{ 850076, 850077, 850078 };

		for (TBLIDX i : m_buffs)
		{
			if (CBuff* buff = cPlayer->GetBuffManager()->FindBuff(i, DBO_OBJECT_SOURCE_ITEM))
				cPlayer->GetBuffManager()->RemoveBuff(buff->GetBuffIndex(), buff->GetBuffType(), CBuff::BUFF_REMOVAL_REASON_BY_ITSELF);
		}
	}

	cPlayer->StandUpIfPossible(cPlayer->GetCharStateID());
}

//--------------------------------------------------------------------------------------//
//		CHAR READY FOR COMMUNITY SERVER
//--------------------------------------------------------------------------------------//
void CClientSession::RecCharReadyForCommunityServerNfy(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer * app = (CGameServer*)g_pApp;

	CNtlPacket packet(sizeof(sGT_CHAR_READY_FOR_COMMUNITY_SERVER_NFY));
	sGT_CHAR_READY_FOR_COMMUNITY_SERVER_NFY * res = (sGT_CHAR_READY_FOR_COMMUNITY_SERVER_NFY *)packet.GetPacketData();
	res->wOpCode = GT_CHAR_READY_FOR_COMMUNITY_SERVER_NFY;
	res->charId = cPlayer->GetCharID();
	packet.SetPacketLen(sizeof(sGT_CHAR_READY_FOR_COMMUNITY_SERVER_NFY));
	app->SendTo(app->GetChatServerSession(), &packet);
}

//--------------------------------------------------------------------------------------//
//		Auth community Server
//--------------------------------------------------------------------------------------//
void CClientSession::RecvAuthKeyForCumminityServerReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	CNtlPacket packet(sizeof(sGT_USER_AUTH_KEY_CREATED_NFY));
	sGT_USER_AUTH_KEY_CREATED_NFY * res = (sGT_USER_AUTH_KEY_CREATED_NFY *)packet.GetPacketData();
	res->wOpCode = GT_USER_AUTH_KEY_CREATED_NFY;
	res->accountId = cPlayer->GetAccountID();
	g_pServerInfoManager->GenerateAuthKey(res->abyAuthKey); //generate auth key
	app->SendTo(app->GetChatServerSession(), &packet );
}

//--------------------------------------------------------------------------------------//
//		TUTORIAL HINT
//--------------------------------------------------------------------------------------//
void CClientSession::RecvTutorialHintReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_TUTORIAL_HINT_UPDATE_REQ * req = (sUG_TUTORIAL_HINT_UPDATE_REQ *)pPacket->GetPacketData();
	
	CNtlPacket packet(sizeof(sGU_TUTORIAL_HINT_UPDATE_RES));
	sGU_TUTORIAL_HINT_UPDATE_RES * res = (sGU_TUTORIAL_HINT_UPDATE_RES *)packet.GetPacketData();
	res->wOpCode = GU_TUTORIAL_HINT_UPDATE_RES;
	res->wResultCode = GAME_SUCCESS;
	res->dwTutorialHint = req->dwTutorialHint;
	packet.SetPacketLen( sizeof(sGU_TUTORIAL_HINT_UPDATE_RES) );
	g_pApp->Send( GetHandle(), &packet );

	cPlayer->SetTutorialHint(req->dwTutorialHint);
}
//--------------------------------------------------------------------------------------//
//		TUTORIAL CANCEL WAIT
//--------------------------------------------------------------------------------------//
void CClientSession::RecvTutorialWaitCancelReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_TUTORIAL_WAIT_CANCEL_REQ * req = (sUG_TUTORIAL_WAIT_CANCEL_REQ *)pPacket->GetPacketData();
	
	CNtlPacket packet(sizeof(sGU_TUTORIAL_WAIT_CANCEL_RES));
	sGU_TUTORIAL_WAIT_CANCEL_RES * res = (sGU_TUTORIAL_WAIT_CANCEL_RES *)packet.GetPacketData();
	res->wOpCode = GU_TUTORIAL_WAIT_CANCEL_RES;
	res->wResultCode = GAME_SUCCESS;
	packet.SetPacketLen( sizeof(sGU_TUTORIAL_WAIT_CANCEL_RES) );
	g_pApp->Send(GetHandle(), &packet );
}
//--------------------------------------------------------------------------------------//
//		TUTORIAL QUIT
//--------------------------------------------------------------------------------------//
void CClientSession::RecvTutorialQuitReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_TUTORIAL_PLAY_QUIT_REQ * req = (sUG_TUTORIAL_PLAY_QUIT_REQ *)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_TUTORIAL_PLAY_QUIT_RES));
	sGU_TUTORIAL_PLAY_QUIT_RES * res = (sGU_TUTORIAL_PLAY_QUIT_RES *)packet.GetPacketData();
	res->wOpCode = GU_TUTORIAL_PLAY_QUIT_RES;
	res->wResultCode = GAME_SUCCESS;

	if (cPlayer->GetTMQ() && cPlayer->GetTMQ()->GetMode() == TIMEQUEST_MODE_INDIVIDUAL) //check if inside tmq and check if inside tutorial
	{
		if(cPlayer->GetTMQ()->GetState() != TIMEQUEST_GAME_STATE_END && cPlayer->GetTMQ()->GetState() != TIMEQUEST_GAME_STATE_LEAVE && cPlayer->GetTMQ()->GetState() != TIMEQUEST_GAME_STATE_FAIL && cPlayer->GetTMQ()->GetState() != TIMEQUEST_GAME_STATE_CLOSE)
			cPlayer->GetTMQ()->SetState(TIMEQUEST_GAME_STATE_LEAVE);
		else 
			res->wResultCode = GAME_FAIL;
	}
	else 
		res->wResultCode = GAME_FAIL;
	
	packet.SetPacketLen(sizeof(sGU_TUTORIAL_PLAY_QUIT_RES));
	g_pApp->Send(GetHandle(), &packet );
}


//--------------------------------------------------------------------------------------//
//		GET RANK BATTLE INFO
//--------------------------------------------------------------------------------------//
void CClientSession::RecvRankBattleInfoReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_RANKBATTLE_INFO_REQ * req = (sUG_RANKBATTLE_INFO_REQ *)pPacket->GetPacketData();

	g_pRankbattleManager->LoadRankBattleInfo(cPlayer, req->byBattleMode);
}

//--------------------------------------------------------------------------------------//
//		JOIN RANK BATTLE
//--------------------------------------------------------------------------------------//
void CClientSession::RecvJoinRankBattleReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_RANKBATTLE_JOIN_REQ * req = (sUG_RANKBATTLE_JOIN_REQ *)pPacket->GetPacketData();

	g_pRankbattleManager->JoinRoom(cPlayer, req->rankBattleTblidx, req->hBoardObject);
}

//--------------------------------------------------------------------------------------//
//		LEAVE RANK BATTLE
//--------------------------------------------------------------------------------------//
void CClientSession::RecvLeaveRankBattleReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_RANKBATTLE_LEAVE_REQ * req = (sUG_RANKBATTLE_LEAVE_REQ *)pPacket->GetPacketData();

	g_pRankbattleManager->LeaveRoom(cPlayer, req->rankBattleTblidx);
}

//--------------------------------------------------------------------------------------//
//		Char Ready 
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharReady(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_READY * req = (sUG_CHAR_READY *)pPacket->GetPacketData();

	if (cPlayer->GetCurWorld())
	{
		if (cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_RANKBATTLE)
		{
			if (cPlayer->GetRankBattleData()->eState == RANKBATTLE_MEMBER_STATE_NONE)
			{
				g_pRankbattleManager->EnterBattle(cPlayer);
			}
			else if (cPlayer->GetRankBattleData()->eState == RANKBATTLE_MEMBER_STATE_FAINT && cPlayer->IsReviving())
			{
				g_pRankbattleManager->UpdatePlayerState(cPlayer->GetRankBattleRoomTblidx(), cPlayer->GetRankBattleRoomId(), cPlayer, RANKBATTLE_MEMBER_STATE_ATTACKABLE);
			}
		}
		else if (cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_HUNT) //if entered ultimate dungeon and player dont have party then kick out again. Not sure if code needed.
		{
			if (cPlayer->GetUD() && cPlayer->GetUD()->GetWorld() && cPlayer->GetParty() == NULL)
			{
				ERR_LOG(LOG_USER, "Player %u entered ultimate dungeon without a party..", cPlayer->GetCharID());
				CWorld* pWorld = cPlayer->GetUD()->GetWorld();
				cPlayer->StartTeleport(pWorld->GetTbldat()->outWorldLoc, pWorld->GetTbldat()->outWorldDir, pWorld->GetTbldat()->outWorldTblidx, TELEPORT_TYPE_DUNGEON);
			}
		}
		else if (cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_DOJO) //when spawn in dojo map, then fill lp and ep
		{
			cPlayer->UpdateCurLP(cPlayer->GetLastMaxLP(), true, false);
			cPlayer->UpdateCurEP(cPlayer->GetLastMaxEP(), true, false);

			if (CDojo * pDojo = g_pDojoWarManager->GetDojo(cPlayer->GetGuildID()))
			{
				pDojo->SendSealState(GetHandle());
			}
		}
		else if (cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_MINORMATCH)
		{
			if (cPlayer->GetBudokaiPcState() == MATCH_MEMBER_STATE_RESCUE && cPlayer->IsReviving())
			{
				g_pBudokaiManager->MinorMatchUpdatePlayerState(MATCH_MEMBER_STATE_NORMAL, cPlayer);

				cPlayer->UpdateCurLP(cPlayer->GetLastMaxLP(), true, false);
				cPlayer->UpdateCurEP(cPlayer->GetLastMaxEP(), true, false);
			}
		}
		else if (cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_MAJORMATCH)
		{
			if (cPlayer->GetBudokaiPcState() == MATCH_MEMBER_STATE_RESCUE && cPlayer->IsReviving())
			{
				g_pBudokaiManager->MajorMatchUpdatePlayerState(MATCH_MEMBER_STATE_NORMAL, cPlayer);
			}
		}
		else if (cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_FINALMATCH)
		{
			if (cPlayer->GetBudokaiPcState() == MATCH_MEMBER_STATE_RESCUE && cPlayer->IsReviving())
			{
				g_pBudokaiManager->FinalMatchUpdatePlayerState(MATCH_MEMBER_STATE_NORMAL, cPlayer);
			}
		}
	}
	
	if (cPlayer->GetCharStateID() != CHARSTATE_FALLING) //if dont do this, then char will be unable to attack after spawning with "sendcharstatespawning"
		cPlayer->StandUpIfPossible(cPlayer->GetCharStateID());

	cPlayer->SetIsReviving(false);

	cPlayer->SetTeleportLoc(CNtlVector::ZERO);
	cPlayer->SetTeleportDir(CNtlVector::ZERO);
	cPlayer->SetTeleportWorldID(INVALID_WORLDID);
}

//--------------------------------------------------------------------------------------//
//		REVIVE CHARACTER
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharRevivalReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_REVIVAL_REQ * req = (sUG_CHAR_REVIVAL_REQ*)pPacket->GetPacketData();

	WORD wResultcode = GAME_SUCCESS;

	if (!cPlayer->IsFainting())
	{
		wResultcode = GAME_FAIL;
		ERR_LOG(LOG_GENERAL, "ERROR: USER IS NOT IN FAINT. CHAR ID %u STATE ID %u", cPlayer->GetCharID(), cPlayer->GetCharStateID());
	}
	else
	{
		if (cPlayer->GetCurWorld())
		{
			std::vector<TBLIDX> m_buffs{ 850076, 850077, 850078 };

			for (TBLIDX i : m_buffs)
			{
				if (CBuff* buff = cPlayer->GetBuffManager()->FindBuff(i, DBO_OBJECT_SOURCE_ITEM))
					cPlayer->GetBuffManager()->RemoveBuff(buff->GetBuffIndex(), buff->GetBuffType(), CBuff::BUFF_REMOVAL_REASON_BY_ITSELF);
			}

			if(cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_RANKBATTLE || cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_MINORMATCH || cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_MAJORMATCH || cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_FINALMATCH)
				wResultcode = GAME_FAIL;
			else if (cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_DOJO)
			{
				CDojo* pDojo = g_pDojoManager->GetDojo(cPlayer->GetGuildID());
				if (pDojo)
					cPlayer->Revival(cPlayer->GetCurWorld()->GetTbldat()->vStart2Loc, cPlayer->GetWorldID(), REVIVAL_TYPE_SPECIFIED_POSITION, TELEPORT_TYPE_DOJO); //if dojo found then spawn at defender loc
				else
					cPlayer->Revival(cPlayer->GetCurWorld()->GetTbldat()->vStart1Loc, cPlayer->GetWorldID(), REVIVAL_TYPE_SPECIFIED_POSITION, TELEPORT_TYPE_DOJO); //if no dojo found then spawn at attacker loc
			}
			else if (cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_CCBATTLEDUNGEON)
			{
				if (cPlayer->GetCCBD())
				{
					CWorld* pWorld = cPlayer->GetCCBD()->GetWorld();
					if (pWorld)
					{
						if (cPlayer->GetParty())
						{
							cPlayer->SetCCBDRejoin(false);
							cPlayer->LeaveParty(); //here we leave party and get teleported out
						}
						else
							cPlayer->StartTeleport(pWorld->GetTbldat()->outWorldLoc, cPlayer->GetCurDir(), pWorld->GetTbldat()->outWorldTblidx, TELEPORT_TYPE_WORLD_MOVE);
					}
					else
						wResultcode = GAME_WORLD_NOT_FOUND;
				}
				else wResultcode = GAME_FAIL;
			}
			else if (req->byRevivalRequestType == DBO_REVIVAL_REQUEST_TYPE_CURRENT_POSITION) //only allow respawn in current pos while in TLQ
			{
				if (cPlayer->GetTLQ())
				{
					cPlayer->Revival(cPlayer->GetCurLoc(), cPlayer->GetWorldID(), REVIVAL_TYPE_CURRENT_POSITION, TELEPORT_TYPE_DUNGEON);
				}
				else
				{
					wResultcode = GAME_FAIL;
					ERR_LOG(LOG_USER, "Can not revival. req->byRevivalRequestType = DBO_REVIVAL_REQUEST_TYPE_CURRENT_POSITION. WorldTblidx %u, Location (%f,%f,%f)", cPlayer->GetWorldTblidx(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().y, cPlayer->GetCurLoc().z);
				}
			}
			else
			{
				CNtlVector vecLoc(cPlayer->GetBindLoc());
				cPlayer->Revival(vecLoc, cPlayer->GetBindWorldID(), REVIVAL_TYPE_BIND_POINT, TELEPORT_TYPE_POPOSTONE);
			}
		}
		else wResultcode = GAME_FAIL;
	}

	CNtlPacket packet(sizeof(sGU_CHAR_REVIVAL_RES));
	sGU_CHAR_REVIVAL_RES * res = (sGU_CHAR_REVIVAL_RES *)packet.GetPacketData();
	res->wOpCode = GU_CHAR_REVIVAL_RES;
	res->wResultCode = wResultcode;
	packet.SetPacketLen(sizeof(sGU_CHAR_REVIVAL_RES));
	g_pApp->Send(GetHandle(), &packet);
}

//--------------------------------------------------------------------------------------//
//	Char Move         (THIS FUNCTION CALLED WHEN PLAYER START AND STOP MOVING)
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharMove(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	// Check if player can move in battle royale (waiting for start period)
	if (g_pBattleRoyaleEvent && !g_pBattleRoyaleEvent->CanPlayerMove(cPlayer))
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_CHAR_MOVE * req = (sUG_CHAR_MOVE*)pPacket->GetPacketData();
	//printf("ug_char_move: req->byMoveDirection %u \n", req->byMoveDirection);
	if (cPlayer->GetStateManager()->CanCharStateTransition(CHARSTATE_MOVING) == false)
	{
		return;
	}

	if (req->byMoveDirection > NTL_MOVE_KEYBOARD_LAST)
	{
		ERR_LOG(LOG_USER, "Player %u send wrong movedirection %u", cPlayer->GetCharID(), req->byMoveDirection);
		return;
	}
	
	CNtlVector vLoc;
	NtlLocationDecompress(&req->vCurLoc, &vLoc.x, &vLoc.y, &vLoc.z);

	float fMovedDistance = NtlGetDistance(cPlayer->GetCurLoc(), vLoc); // get distance from server and client
	if (fMovedDistance > (cPlayer->GetLastRunSpeed() + 1) * DBO_DISTANCE_CHECK_TOLERANCE)
	{
		ERR_LOG(LOG_HACK, "Player: %u seems to be speed hacking. Distance: %f CurLoc: %f, %f, %f NewLoc: %f %f %f", cPlayer->GetCharID(), fMovedDistance, cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().y, cPlayer->GetCurLoc().z, vLoc.x, vLoc.y, vLoc.z);

		if (cPlayer->DisconnectForSpeedHack(app->GetCurTickCount()) || fMovedDistance > 100.f)
		{
			this->Disconnect(false);
			return;
		}
	}

	if (cPlayer->SetCurLoc(vLoc, cPlayer->GetCurWorld()))
	{
		sVECTOR3 sDir;
		NtlDirectionDecompress(&req->vCurDir, &sDir.x, &sDir.y, &sDir.z);

		cPlayer->SetCurDir(sDir);

		// Update movement for battle royale idle detection
		if (cPlayer->GetCurWorld() && cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_RAID && cPlayer->GetCurWorld()->GetTbldat() && cPlayer->GetCurWorld()->GetTbldat()->tblidx == 212000)
		{
			if (g_pBattleRoyaleEvent)
				g_pBattleRoyaleEvent->UpdatePlayerMovement(cPlayer);
		}

		if (req->byMoveDirection == 0)
		{
			if (cPlayer->GetCharStateID() != CHARSTATE_CASTING && cPlayer->GetCharStateID() != CHARSTATE_SKILL_AFFECTING && cPlayer->GetCharStateID() != CHARSTATE_KEEPING_EFFECT)
				cPlayer->SendCharStateStanding();
		}
		else
		{
			cPlayer->SendCharStateMoving(req->byMoveDirection, NTL_MOVE_FLAG_RUN, NTL_MOVE_STATUS_NONE);
		}
	}
	else
	{
		ERR_LOG(LOG_HACK, "Player: %u seems to be location hacking. CurLoc: %f, %f, %f NewLoc: %f %f %f", cPlayer->GetCharID(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().y, cPlayer->GetCurLoc().z, vLoc.x, vLoc.y, vLoc.z);
	}
}

//--------------------------------------------------------------------------------------//
//		Char Air Move
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharAirMove(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	// Check if player can move in battle royale (waiting for start period)
	if (g_pBattleRoyaleEvent && !g_pBattleRoyaleEvent->CanPlayerMove(cPlayer))
		return;

	sUG_CHAR_AIR_MOVE * req = (sUG_CHAR_AIR_MOVE*)pPacket->GetPacketData();

	if (cPlayer->IsFainting() && cPlayer->GetAirState() == AIR_STATE_OFF)
		return;

	if (cPlayer->GetStateManager()->CanCharStateTransition(CHARSTATE_MOVING) == false)
		return;

	if (req->byMoveDirection > NTL_MOVE_KEYBOARD_LAST)
	{
		ERR_LOG(LOG_USER, "Player %u send wrong movedirection %u", cPlayer->GetCharID(), req->byMoveDirection);
		return;
	}

	CNtlVector vLoc;
	NtlLocationDecompress(&req->vCurLoc, &vLoc.x, &vLoc.y, &vLoc.z);

	if (cPlayer->SetCurLoc(vLoc, cPlayer->GetCurWorld()))
	{
		sVECTOR3 sDir;
		NtlDirectionDecompress(&req->vCurDir, &sDir.x, &sDir.y, &sDir.z);

		// Update movement for battle royale idle detection
		if (cPlayer->GetCurWorld() && cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_RAID && cPlayer->GetCurWorld()->GetTbldat() && cPlayer->GetCurWorld()->GetTbldat()->tblidx == 212000)
		{
			if (g_pBattleRoyaleEvent)
				g_pBattleRoyaleEvent->UpdatePlayerMovement(cPlayer);
		}

		if(req->byMoveDirection == 0)
		{
			cPlayer->SetCurDir(sDir);

			cPlayer->SendCharStateStanding();
		}
		else
		{
			cPlayer->SetCurDir(sDir);

			cPlayer->SendCharStateMoving(req->byMoveDirection, NTL_MOVE_FLAG_FLY, NTL_MOVE_STATUS_AIR_MOVE);
		}
	}
	else
	{
		ERR_LOG(LOG_HACK, "Player: %u seems to be location hacking. CurLoc: %f, %f, %f NewLoc: %f %f %f", cPlayer->GetCharID(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().y, cPlayer->GetCurLoc().z, vLoc.x, vLoc.y, vLoc.z);
	}
}
//--------------------------------------------------------------------------------------//
//		Char Destination Move
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharDestMove(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_CHAR_DEST_MOVE * req = (sUG_CHAR_DEST_MOVE*)pPacket->GetPacketData();

	if (cPlayer->GetStateManager()->CanCharStateTransition(CHARSTATE_DESTMOVE) == false)
	{
		return;
	}

	CNtlVector vLoc;
	NtlLocationDecompress(&req->vCurLoc, &vLoc.x, &vLoc.y, &vLoc.z);

	float fMovedDistance = NtlGetDistance(cPlayer->GetCurLoc(), vLoc); // get distance from server and client
	if (fMovedDistance > (cPlayer->GetLastRunSpeed() + 1) * DBO_DISTANCE_CHECK_TOLERANCE)
	{
		ERR_LOG(LOG_HACK, "Player: %u seems to be speed hacking. Distance: %f CurLoc: %f, %f, %f NewLoc: %f %f %f", cPlayer->GetCharID(), fMovedDistance, cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().y, cPlayer->GetCurLoc().z, vLoc.x, vLoc.y, vLoc.z);

		if (cPlayer->DisconnectForSpeedHack(app->GetCurTickCount()) || fMovedDistance > 100.f)
		{
			this->Disconnect(false);
			return;
		}
	}

	if (cPlayer->SetCurLoc(vLoc, cPlayer->GetCurWorld()))
	{
		sVECTOR3 destloc;
		NtlLocationDecompress(&req->vDestLoc, &destloc.x, &destloc.y, &destloc.z);

		CNtlPacket pStatePacket(sizeof(sGU_UPDATE_CHAR_STATE));
		sGU_UPDATE_CHAR_STATE * res2 = (sGU_UPDATE_CHAR_STATE *)pStatePacket.GetPacketData();
		res2->wOpCode = GU_UPDATE_CHAR_STATE;
		res2->handle = cPlayer->GetID();
		res2->sCharState.sCharStateBase.byStateID = CHARSTATE_DESTMOVE;
		cPlayer->GetStateManager()->CopyAspectTo(&res2->sCharState.sCharStateBase.aspectState);
		vLoc.CopyTo(res2->sCharState.sCharStateBase.vCurLoc);
		cPlayer->GetCurDir().CopyTo(res2->sCharState.sCharStateBase.vCurDir);
		res2->sCharState.sCharStateBase.eAirState = cPlayer->GetAirState();
		res2->sCharState.sCharStateBase.bFightMode = cPlayer->GetFightMode();
		res2->sCharState.sCharStateBase.bIsValidLoc = true;
		res2->sCharState.sCharStateBase.dwConditionFlag = cPlayer->GetConditionState();
		res2->sCharState.sCharStateBase.dwStateTime = 0;
		res2->sCharState.sCharStateDetail.sCharStateDestMove.actionPatternIndex = INVALID_ACTIONPATTERNID;
		res2->sCharState.sCharStateDetail.sCharStateDestMove.avDestLoc[0].x = destloc.x;
		res2->sCharState.sCharStateDetail.sCharStateDestMove.avDestLoc[0].y = destloc.y;
		res2->sCharState.sCharStateDetail.sCharStateDestMove.avDestLoc[0].z = destloc.z;
		res2->sCharState.sCharStateDetail.sCharStateDestMove.bHaveSecondDestLoc = false;
		res2->sCharState.sCharStateDetail.sCharStateDestMove.byDestLocCount = 1;
		res2->sCharState.sCharStateDetail.sCharStateDestMove.byMoveFlag = NTL_MOVE_FLAG_RUN;
		res2->sCharState.sCharStateDetail.sCharStateDestMove.dwTimeStamp = 0;
		pStatePacket.SetPacketLen(sizeof(sGU_UPDATE_CHAR_STATE));

		if (cPlayer->GetStateManager()->CopyFrom(&res2->sCharState))
		{
			cPlayer->Broadcast(&pStatePacket);
		}
	}
	else
	{
		ERR_LOG(LOG_HACK, "Player: %u seems to be location hacking. CurLoc: %f, %f, %f NewLoc: %f %f %f", cPlayer->GetCharID(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().y, cPlayer->GetCurLoc().z, vLoc.x, vLoc.y, vLoc.z);
	}
}
//--------------------------------------------------------------------------------------//
//		Char Move Sync
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharMoveSync(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_DEST_MOVE_SYNC * req = (sUG_CHAR_DEST_MOVE_SYNC*)pPacket->GetPacketData();
		
	CNtlVector vLoc;
	NtlLocationDecompress(&req->vCurLoc, &vLoc.x, &vLoc.y, &vLoc.z);

	if (cPlayer->SetCurLoc(vLoc, cPlayer->GetCurWorld()))
	{
		sVECTOR3 sDir;
		NtlDirectionDecompress(&req->vCurDir, &sDir.x, &sDir.y, &sDir.z);
		cPlayer->SetCurDir(sDir);
	}
	else
	{
		ERR_LOG(LOG_HACK, "Player: %u seems to be location hacking. CurLoc: %f, %f, %f NewLoc: %f %f %f", cPlayer->GetCharID(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().y, cPlayer->GetCurLoc().z, vLoc.x, vLoc.y, vLoc.z);
	}
}

//--------------------------------------------------------------------------------------//
//		RECEIVED WHILE JUMPING
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharNpcServerMoveSync(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_NPCSERVER_MOVE_SYNC * req = (sUG_CHAR_NPCSERVER_MOVE_SYNC*)pPacket->GetPacketData();

	CNtlVector vLoc;
	NtlLocationDecompress(&req->vCurLoc, &vLoc.x, &vLoc.y, &vLoc.z);

	if (cPlayer->SetCurLoc(vLoc, cPlayer->GetCurWorld()))
	{

	}
	else
	{
		ERR_LOG(LOG_HACK, "Player: %u seems to be location hacking. CurLoc: %f, %f, %f NewLoc: %f %f %f", cPlayer->GetCharID(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().y, cPlayer->GetCurLoc().z, vLoc.x, vLoc.y, vLoc.z);
	}
}


//--------------------------------------------------------------------------------------//
//		Char air move sync
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharAirMoveSync(CNtlPacket * pPacket)
{
	sUG_CHAR_AIR_MOVE_SYNC * req = (sUG_CHAR_AIR_MOVE_SYNC*)pPacket->GetPacketData();

	if (!cPlayer || !cPlayer->IsInitialized())
		return;
	
	CNtlVector vLoc;
	NtlLocationDecompress(&req->vCurLoc, &vLoc.x, &vLoc.y, &vLoc.z);

	if (cPlayer->SetCurLoc(vLoc, cPlayer->GetCurWorld()))
	{
		CNtlVector sDir;
		NtlDirectionDecompress(&req->vCurDir, &sDir.x, &sDir.y, &sDir.z);

		if (sDir.IsInvalid())
			return;

		cPlayer->SetCurDir(sDir);

		//IF WE DONT SEND THIS PACKET THEN PLAYER IS NOT MOVING WHILE ON VEHICLE FOR OTHERS

		CNtlPacket packet(sizeof(sGU_CHAR_AIR_MOVE_SYNC));
		sGU_CHAR_AIR_MOVE_SYNC * res = (sGU_CHAR_AIR_MOVE_SYNC *)packet.GetPacketData();
		res->wOpCode = GU_CHAR_AIR_MOVE_SYNC;
		res->handle = cPlayer->GetID();
		res->vCurDir = req->vCurDir;
		res->vCurLoc = req->vCurLoc;
		packet.SetPacketLen(sizeof(sGU_CHAR_AIR_MOVE_SYNC));
		cPlayer->Broadcast(&packet, cPlayer);
	}
	else
	{
		ERR_LOG(LOG_HACK, "Player: %u seems to be location hacking. CurLoc: %f, %f, %f NewLoc: %f %f %f", cPlayer->GetCharID(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().y, cPlayer->GetCurLoc().z, vLoc.x, vLoc.y, vLoc.z);
	}
}

//--------------------------------------------------------------------------------------//                    
//		Char location sync                                                                                                                                                               
//--------------------------------------------------------------------------------------//                            
void CClientSession::RecvCharLocationSync(CNtlPacket * pPacket)
{
	sUG_CHAR_LOCATION_SYNC * req = (sUG_CHAR_LOCATION_SYNC*)pPacket->GetPacketData();

	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CNtlVector vLoc;
	NtlLocationDecompress(&req->vCurLoc, &vLoc.x, &vLoc.y, &vLoc.z);

	if (cPlayer->SetCurLoc(vLoc, cPlayer->GetCurWorld()))
	{
		CNtlVector sDir;
		NtlDirectionDecompress(&req->vCurDir, &sDir.x, &sDir.y, &sDir.z);

		if (sDir.IsInvalid())
			return;

		cPlayer->SetCurDir(sDir);
	}
	else
	{
		ERR_LOG(LOG_HACK, "Player: %u seems to be location hacking. CurLoc: %f, %f, %f NewLoc: %f %f %f", cPlayer->GetCharID(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().y, cPlayer->GetCurLoc().z, vLoc.x, vLoc.y, vLoc.z);
	}
}

//--------------------------------------------------------------------------------------//
//		Char Change Heading
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharChangeHeading(CNtlPacket * pPacket)
{
	sUG_CHAR_CHANGE_HEADING * req = (sUG_CHAR_CHANGE_HEADING*)pPacket->GetPacketData();

	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CNtlVector sDir;
	NtlDirectionDecompress(&req->vCurrentHeading, &sDir.x, &sDir.y, &sDir.z);

	if (sDir.IsInvalid())
		return;

	cPlayer->SetCurDir(sDir);

	//printf("ChangeHeading: %f %f %f \n", sDir.x, sDir.y, sDir.z);
	
	CNtlPacket packet(sizeof(sGU_CHAR_CHANGE_HEADING));
	sGU_CHAR_CHANGE_HEADING * res = (sGU_CHAR_CHANGE_HEADING *)packet.GetPacketData();
	res->wOpCode = GU_CHAR_CHANGE_HEADING;
	res->handle = cPlayer->GetID();
	res->vNewHeading = req->vCurrentHeading;
	packet.SetPacketLen(sizeof(sGU_CHAR_CHANGE_HEADING));
	cPlayer->Broadcast(&packet, cPlayer); //dont send to ourself or there will be issues when dashing
}


//--------------------------------------------------------------------------------------//
//		
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharMoveCollision(CNtlPacket * pPacket)
{
	sUG_CHAR_MOVE_COLLISION * req = (sUG_CHAR_MOVE_COLLISION *)pPacket->GetPacketData();

	if (!cPlayer || !cPlayer->IsInitialized())
		return;
	
	if (cPlayer->GetCollision() == false)
	{
		CNtlVector vLoc;
		NtlLocationDecompress(&req->vCurLoc, &vLoc.x, &vLoc.y, &vLoc.z);

		if (cPlayer->SetCurLoc(vLoc, cPlayer->GetCurWorld()))
		{
			cPlayer->SetCollision(true);

			CNtlVector vDir;
			NtlDirectionDecompress(&req->vCurDir, &vDir.x, &vDir.y, &vDir.z);
		
			if (vDir.IsInvalid())
				return;

			cPlayer->SetCurDir(vDir);

			CNtlPacket packet(sizeof(sGU_CHAR_MOVE_COLLISION));
			sGU_CHAR_MOVE_COLLISION * res = (sGU_CHAR_MOVE_COLLISION *)packet.GetPacketData();
			res->wOpCode = GU_CHAR_MOVE_COLLISION;
			res->handle = cPlayer->GetID();
			res->vCurLoc = req->vCurLoc;
			res->vCurDir = req->vCurDir;
			packet.SetPacketLen(sizeof(sGU_CHAR_MOVE_COLLISION));
			cPlayer->Broadcast(&packet);
		}
		else
		{
			ERR_LOG(LOG_HACK, "Player: %u seems to be location hacking. CurLoc: %f, %f, %f NewLoc: %f %f %f", cPlayer->GetCharID(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().y, cPlayer->GetCurLoc().z, vLoc.x, vLoc.y, vLoc.z);
		}
	}
}

//--------------------------------------------------------------------------------------//
//		
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharMoveCollisionEnd(CNtlPacket * pPacket)
{
	sUG_CHAR_MOVE_COLLISION_END * req = (sUG_CHAR_MOVE_COLLISION_END *)pPacket->GetPacketData();

	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	if (cPlayer->GetCollision())
	{
		CNtlVector vLoc;
		NtlLocationDecompress(&req->vCurLoc, &vLoc.x, &vLoc.y, &vLoc.z);

		if (cPlayer->SetCurLoc(vLoc, cPlayer->GetCurWorld()))
		{
			cPlayer->SetCollision(false);

			CNtlPacket packet(sizeof(sGU_CHAR_MOVE_COLLISION_END));
			sGU_CHAR_MOVE_COLLISION_END * res = (sGU_CHAR_MOVE_COLLISION_END *)packet.GetPacketData();
			res->wOpCode = GU_CHAR_MOVE_COLLISION_END;
			res->handle = cPlayer->GetID();
			res->vCurLoc = req->vCurLoc;
			packet.SetPacketLen(sizeof(sGU_CHAR_MOVE_COLLISION_END));
			cPlayer->Broadcast(&packet);
		}
		else
		{
			ERR_LOG(LOG_HACK, "Player: %u seems to be location hacking. CurLoc: %f, %f, %f NewLoc: %f %f %f", cPlayer->GetCharID(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().y, cPlayer->GetCurLoc().z, vLoc.x, vLoc.y, vLoc.z);
		}
	}
}

//--------------------------------------------------------------------------------------//
//		
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharStandingSlip(CNtlPacket * pPacket)
{
	//sUG_CHAR_STANDING_SLIP * req = (sUG_CHAR_STANDING_SLIP *)pPacket->GetPacketData();
}

//--------------------------------------------------------------------------------------//
//		
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharStandingSlipEnd(CNtlPacket * pPacket)
{
	//sUG_CHAR_STANDING_SLIP_END * req = (sUG_CHAR_STANDING_SLIP_END *)pPacket->GetPacketData();
}


//--------------------------------------------------------------------------------------//
//		Char Jump
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharJump(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_JUMP * req = (sUG_CHAR_JUMP*)pPacket->GetPacketData();

	if (req->byMoveDirection >= NTL_MOVE_COUNT)
	{
		ERR_LOG(LOG_USER, "Player %u send wrong movedirection %u", cPlayer->GetCharID(), req->byMoveDirection);
		return;
	}
	//printf("req->byMoveDirection: %u \n", req->byMoveDirection);
	CNtlVector vLoc;
	NtlLocationDecompress(&req->vCurLoc, &vLoc.x, &vLoc.y, &vLoc.z);

	if (cPlayer->GetStateManager()->CanCharStateTransition(CHARSTATE_MOVING))
	{
		if (cPlayer->SetCurLoc(vLoc, cPlayer->GetCurWorld()))
		{
			CNtlVector vCurDir(cPlayer->GetCurDir());
			if (cPlayer->JumpStart(vCurDir))
			{
				CNtlVector svHeading;
				cPlayer->GetJumpHeading().CopyTo(svHeading.x, svHeading.y, svHeading.z);

				if (svHeading.IsInvalid())
					return;

				CNtlVector sDir;
				NtlDirectionDecompress(&req->vCurDir, &sDir.x, &sDir.y, &sDir.z);
				cPlayer->SetCurDir(sDir);

				if (sDir.IsInvalid())
					return;

				cPlayer->SendCharStateMoving(req->byMoveDirection, NTL_MOVE_FLAG_RUN, NTL_MOVE_STATUS_JUMP);

				CNtlPacket packet(sizeof(sGU_CHAR_JUMP));
				sGU_CHAR_JUMP * res = (sGU_CHAR_JUMP *)packet.GetPacketData();
				res->wOpCode = GU_CHAR_JUMP;
				res->handle = cPlayer->GetID();
				NtlDirectionCompress(&res->vJumpDir, svHeading.x, svHeading.y, svHeading.z);
				res->vCurrentHeading = req->vCurDir;
				res->byMoveDirection = req->byMoveDirection;
				res->vCurLoc = req->vCurLoc;
				packet.SetPacketLen(sizeof(sGU_CHAR_JUMP));
				cPlayer->BroadcastToNeighbor(&packet);
			}
		}
		else
		{
			ERR_LOG(LOG_HACK, "Player: %u seems to be location hacking. CurLoc: %f, %f, %f NewLoc: %f %f %f", cPlayer->GetCharID(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().y, cPlayer->GetCurLoc().z, vLoc.x, vLoc.y, vLoc.z);
		}
	}
}
//--------------------------------------------------------------------------------------//
//		Char Jump END
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharJumpEnd(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_JUMP_END * req = (sUG_CHAR_JUMP_END*)pPacket->GetPacketData();

	if (cPlayer->GetAirState() == AIR_STATE_OFF)
		cPlayer->JumpEnd();

	if (cPlayer->GetStateManager()->CanCharStateTransition(CHARSTATE_MOVING))
	{
		CNtlPacket packet(sizeof(sGU_CHAR_JUMP_END));
		sGU_CHAR_JUMP_END * res = (sGU_CHAR_JUMP_END *)packet.GetPacketData();
		res->wOpCode = GU_CHAR_JUMP_END;
		res->handle = cPlayer->GetID();
		packet.SetPacketLen(sizeof(sGU_CHAR_JUMP_END));
		cPlayer->BroadcastToNeighbor(&packet);
	}
}

//--------------------------------------------------------------------------------------//
//		Char AIR Jump
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharAirJump(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_AIR_JUMP * req = (sUG_CHAR_AIR_JUMP*)pPacket->GetPacketData();

	if (req->byMoveDirection >= NTL_MOVE_COUNT)
	{
		ERR_LOG(LOG_USER, "Player %u send wrong movedirection %u", cPlayer->GetCharID(), req->byMoveDirection);
		return;
	}

	//printf("RecvCharAirJump | req->byMoveDirection: %u \n", (int)req->byMoveDirection);

	//check if player has skill/item effect
	if (cPlayer->GetSkillManager()->FindSkillWithSystemEffectCode(PASSIVE_AIR_MASTERY) || cPlayer->GetBuffManager()->HasAirSkillBuff())
	{
		if (cPlayer->GetCurWorld() && cPlayer->GetCurWorld()->CanFly() && cPlayer->GetDragonballScramble() == false)
		{
			if (cPlayer->GetCharStateID() == CHARSTATE_STANDING || cPlayer->GetCharStateID() == CHARSTATE_MOVING)
			{
				CNtlVector vLoc;
				NtlLocationDecompress(&req->vCurLoc, &vLoc.x, &vLoc.y, &vLoc.z);

				if (cPlayer->SetCurLoc(vLoc, cPlayer->GetCurWorld()))
				{
					CNtlVector sDir;
					NtlDirectionDecompress(&req->vCurDir, &sDir.x, &sDir.y, &sDir.z);

					if (sDir.IsInvalid() == false)
						cPlayer->SetCurDir(sDir);

					CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_STATE));
					sGU_UPDATE_CHAR_STATE* res = (sGU_UPDATE_CHAR_STATE*)packet.GetPacketData();
					res->handle = cPlayer->GetID();
					res->wOpCode = GU_UPDATE_CHAR_STATE;
					res->sCharState.sCharStateBase.byStateID = CHARSTATE_AIR_JUMP;
					cPlayer->GetStateManager()->CopyAspectTo(&res->sCharState.sCharStateBase.aspectState);
					vLoc.CopyTo(res->sCharState.sCharStateBase.vCurLoc);
					cPlayer->GetCurDir().CopyTo(res->sCharState.sCharStateBase.vCurDir);
					res->sCharState.sCharStateBase.bFightMode = cPlayer->GetFightMode();
					res->sCharState.sCharStateBase.bIsValidLoc = true;
					res->sCharState.sCharStateBase.dwConditionFlag = cPlayer->GetConditionState();
					res->sCharState.sCharStateBase.dwStateTime = 0;
					res->sCharState.sCharStateBase.eAirState = AIR_STATE_ON;
					res->sCharState.sCharStateDetail.sCharStateAirJumping.byMoveDirection = req->byMoveDirection;
					res->sCharState.sCharStateDetail.sCharStateAirJumping.byMoveFlag = NTL_MOVE_FLAG_FLY_DASH;
					res->sCharState.sCharStateDetail.sCharStateAirJumping.dwTimeStamp = GetTickCount();
					packet.SetPacketLen(sizeof(sGU_UPDATE_CHAR_STATE));

					if (cPlayer->GetStateManager()->CopyFrom(&res->sCharState))
						cPlayer->Broadcast(&packet);
				}
				else
				{
					ERR_LOG(LOG_HACK, "Player: %u seems to be location hacking. CurLoc: %f, %f, %f NewLoc: %f %f %f", cPlayer->GetCharID(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().y, cPlayer->GetCurLoc().z, vLoc.x, vLoc.y, vLoc.z);
				}
			}
			else
			{
				CNtlPacket packet(sizeof(sGU_CHAR_STATE_WRONG));
				sGU_CHAR_STATE_WRONG* res = (sGU_CHAR_STATE_WRONG*)packet.GetPacketData();
				res->wOpCode = GU_CHAR_STATE_WRONG;
				res->wPrevOPCode = cPlayer->GetCharStateID();
				res->handle = cPlayer->GetID();
				res->byStateID = CHARSTATE_AIR_JUMP;
				packet.SetPacketLen(sizeof(sGU_CHAR_STATE_WRONG));
				g_pApp->Send(GetHandle(), &packet);
			}
		}
	}
	else
	{
		ERR_LOG(LOG_USER, "Player %u try to use air skill but dont have that skill !!!", cPlayer->GetCharID());
		this->Disconnect(false);
	}
}

//--------------------------------------------------------------------------------------//
//		Change Char Direction on floating
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharChangeDirOnFloating(CNtlPacket * pPacket)
{
	sUG_CHAR_CHANGE_DIRECTION_ON_FLOATING * req = (sUG_CHAR_CHANGE_DIRECTION_ON_FLOATING*)pPacket->GetPacketData();

	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	if (req->byMoveDirection >= NTL_MOVE_COUNT)
	{
		ERR_LOG(LOG_USER, "Player %u send wrong movedirection %u", cPlayer->GetCharID(), req->byMoveDirection);
		return;
	}

	cPlayer->SetMoveDirection(req->byMoveDirection);

	CNtlVector sDir;
	NtlDirectionDecompress(&req->vCurDir, &sDir.x, &sDir.y, &sDir.z);

	if (sDir.IsInvalid())
		return;

	cPlayer->SetCurDir(sDir);

	CNtlPacket packet(sizeof(sGU_CHAR_CHANGE_DIRECTION_ON_FLOATING));
	sGU_CHAR_CHANGE_DIRECTION_ON_FLOATING * res = (sGU_CHAR_CHANGE_DIRECTION_ON_FLOATING *)packet.GetPacketData();
	res->wOpCode = GU_CHAR_CHANGE_DIRECTION_ON_FLOATING;
	res->hSubject = cPlayer->GetID();
	res->vCurDir = req->vCurDir;
	res->byMoveDirection = req->byMoveDirection;
	packet.SetPacketLen(sizeof(sGU_CHAR_CHANGE_DIRECTION_ON_FLOATING));
	cPlayer->BroadcastToNeighbor(&packet);
}
//--------------------------------------------------------------------------------------//
//		Char falling
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharFalling(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_FALLING * req = (sUG_CHAR_FALLING*)pPacket->GetPacketData();

	if (req->byMoveDirection >= NTL_MOVE_COUNT)
	{
		ERR_LOG(LOG_USER, "Player %u send wrong movedirection %u", cPlayer->GetCharID(), req->byMoveDirection);
		return;
	}

	CNtlVector vLoc;
	NtlLocationDecompress(&req->vCurLoc, &vLoc.x, &vLoc.y, &vLoc.z);

	if (cPlayer->SetCurLoc(vLoc, cPlayer->GetCurWorld()))
	{
		CNtlVector sDir;
		NtlDirectionDecompress(&req->vCurDir, &sDir.x, &sDir.y, &sDir.z);

		if (sDir.IsInvalid() == false)
			cPlayer->SetCurDir(sDir);

		if(req->bIsFalling)
		{
			cPlayer->SendCharStateFalling(req->byMoveDirection);
		}
		else if(cPlayer->GetCharStateID() == CHARSTATE_FALLING)
		{
			cPlayer->SendCharStateStanding();
		}
	}
	else
	{
		ERR_LOG(LOG_HACK, "Player: %u seems to be location hacking. CurLoc: %f, %f, %f NewLoc: %f %f %f", cPlayer->GetCharID(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().y, cPlayer->GetCurLoc().z, vLoc.x, vLoc.y, vLoc.z);
	}
}

//--------------------------------------------------------------------------------------//
//		Char AIR falling called when flying to ground
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharAirFalling(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_AIR_FALLING * req = (sUG_CHAR_AIR_FALLING*)pPacket->GetPacketData();

	if (req->byMoveDirection >= NTL_MOVE_COUNT)
	{
		ERR_LOG(LOG_USER, "Player %u send wrong movedirection %u", cPlayer->GetCharID(), req->byMoveDirection);
		return;
	}

	CNtlVector vLoc;
	NtlLocationDecompress(&req->vCurLoc, &vLoc.x, &vLoc.y, &vLoc.z);

	if (cPlayer->SetCurLoc(vLoc, cPlayer->GetCurWorld()))
	{
		CNtlVector sDir;
		NtlDirectionDecompress(&req->vCurDir, &sDir.x, &sDir.y, &sDir.z);

		if (sDir.IsInvalid() == false)
			cPlayer->SetCurDir(sDir);

		cPlayer->SendCharStateFalling(req->byMoveDirection);
	}
	else
	{
		ERR_LOG(LOG_HACK, "Player: %u seems to be location hacking. CurLoc: %f, %f, %f NewLoc: %f %f %f", cPlayer->GetCharID(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().y, cPlayer->GetCurLoc().z, vLoc.x, vLoc.y, vLoc.z);
	}
}

//--------------------------------------------------------------------------------------//                    
//		CHAR AIR END
//--------------------------------------------------------------------------------------//                            
void CClientSession::RecvCharAirEnd(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_AIR_END * req = (sUG_CHAR_AIR_END*)pPacket->GetPacketData();

	CNtlVector vLoc;
	NtlLocationDecompress(&req->vCurLoc, &vLoc.x, &vLoc.y, &vLoc.z);

	if (cPlayer->SetCurLoc(vLoc, cPlayer->GetCurWorld()))
	{
		CNtlVector sDir;
		NtlDirectionDecompress(&req->vCurDir, &sDir.x, &sDir.y, &sDir.z);

		if (sDir.IsInvalid() == false)
			cPlayer->SetCurDir(sDir);

		CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_STATE));
		sGU_UPDATE_CHAR_STATE * res = (sGU_UPDATE_CHAR_STATE *)packet.GetPacketData();
		res->wOpCode = GU_UPDATE_CHAR_STATE;
		res->handle = cPlayer->GetID();
		res->sCharState.sCharStateBase.byStateID = CHARSTATE_STANDING;
		cPlayer->GetStateManager()->CopyAspectTo(&res->sCharState.sCharStateBase.aspectState);
		cPlayer->GetCurLoc().CopyTo(res->sCharState.sCharStateBase.vCurLoc);
		cPlayer->GetCurDir().CopyTo(res->sCharState.sCharStateBase.vCurDir);
		res->sCharState.sCharStateBase.eAirState = AIR_STATE_OFF;
		res->sCharState.sCharStateBase.bFightMode = cPlayer->GetFightMode();
		res->sCharState.sCharStateBase.bIsValidLoc = true;
		res->sCharState.sCharStateBase.dwConditionFlag = cPlayer->GetConditionState();
		res->sCharState.sCharStateBase.dwStateTime = 0;
		packet.SetPacketLen(sizeof(sGU_UPDATE_CHAR_STATE));

		if (cPlayer->GetStateManager()->CopyFrom(&res->sCharState))
			cPlayer->Broadcast(&packet);
	}
	else
	{
		ERR_LOG(LOG_HACK, "Player: %u seems to be location hacking. CurLoc: %f, %f, %f NewLoc: %f %f %f", cPlayer->GetCharID(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().y, cPlayer->GetCurLoc().z, vLoc.x, vLoc.y, vLoc.z);
	}
}


//--------------------------------------------------------------------------------------//
//		face target
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharTargetFacing(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_TARGET_FACING * req = (sUG_CHAR_TARGET_FACING*)pPacket->GetPacketData();
	
	if (req->hTarget != INVALID_HOBJECT)
	{
		CNpc* pNpc = g_pObjectManager->GetNpc(req->hTarget);
		if (pNpc && pNpc->IsNPC())
		{
			if (pNpc->GetTbldat()->dwFunc_Bit_Flag & NPC_FUNC_FLAG_FACING)
			{
				CObjMsg_YouFaced pMsg;
				pMsg.hSource = cPlayer->GetID();
				pNpc->SendObjectMsg(&pMsg);

				//set facing handle
				cPlayer->SetFacingHandle(req->hTarget);
			}
		}
	}
}
//--------------------------------------------------------------------------------------//
//		target info
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharTargetInfo(CNtlPacket * pPacket)
{
	sUG_CHAR_TARGET_INFO * req = (sUG_CHAR_TARGET_INFO*)pPacket->GetPacketData();
	
	//cPlayer->SetTargetUniqueID(req->hTarget);
	//NTL_PRINT(PRINT_APP,"UG_CHAR_TARGET_INFO TARGET UNIQUE ID %i ", req->hTarget);
}
//--------------------------------------------------------------------------------------//
//		Send game leave request
//--------------------------------------------------------------------------------------//
void CClientSession::RecvGameLeaveReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

}


void CClientSession::RecvCharServerChangeReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	if (cPlayer->GetServerTeleportInfo().worldId != INVALID_WORLDID)
	{
		CNtlPacket packet(sizeof(sGM_CHAR_SERVER_TELEPORT_REQ));
		sGM_CHAR_SERVER_TELEPORT_REQ * res = (sGM_CHAR_SERVER_TELEPORT_REQ *)packet.GetPacketData();
		res->wOpCode = GM_CHAR_SERVER_TELEPORT_REQ;
		res->accountId = cPlayer->GetAccountID();
		res->destServerChannelId = cPlayer->GetServerTeleportInfo().serverChannelId;
		res->serverId = app->GetGsServerId();
		res->serverChannelId = app->GetGsChannel();
		packet.SetPacketLen(sizeof(sGM_CHAR_SERVER_TELEPORT_REQ));
		app->SendTo(app->GetMasterServerSession(), &packet);
	}
	else
	{
		ERR_LOG(LOG_USER, "Player %u tried to teleport to another server with invalid dest world id..", cPlayer->GetCharID());
	}
}


//--------------------------------------------------------------------------------------//
//		Exit game (Exit)
//--------------------------------------------------------------------------------------//
void CClientSession::RecvGameExitReq(CNtlPacket * pPacket)
{
	CNtlPacket packet2(sizeof(sGU_GAME_EXIT_RES));
	sGU_GAME_EXIT_RES * res2 = (sGU_GAME_EXIT_RES *)packet2.GetPacketData();
	res2->wOpCode = GU_GAME_EXIT_RES;
	packet2.SetPacketLen( sizeof(sGU_GAME_EXIT_RES) );
	g_pApp->Send( GetHandle(), &packet2 );
}

//--------------------------------------------------------------------------------------//
//		Char exit to char server (Log Out)
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharExitReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

//	WORD wResultcode = GAME_SUCCESS;

	//if (cPlayer->GetCombatMode())
	//	wResultcode = GAME_CHAR_IS_WRONG_STATE;


	//if (wResultcode == GAME_SUCCESS)
	{
		CNtlPacket packet(sizeof(sGM_MOVE_REQ));
		sGM_MOVE_REQ* res = (sGM_MOVE_REQ *)packet.GetPacketData();
		res->wOpCode = GM_MOVE_REQ;
		res->accountId = cPlayer->GetAccountID();
		packet.SetPacketLen(sizeof(sGM_MOVE_REQ));
		app->SendTo(app->GetMasterServerSession(), &packet);
	}
//	else
//	{
//		CNtlPacket packet(sizeof(sGU_CHAR_EXIT_RES));
//		sGU_CHAR_EXIT_RES* res = (sGU_CHAR_EXIT_RES *)packet.GetPacketData();
//		res->wOpCode = GU_CHAR_EXIT_RES;
//		res->wResultCode = wResultcode;
	//	packet.SetPacketLen(sizeof(sGU_CHAR_EXIT_RES));
	//	app->Send(GetHandle(), &packet);
//	}
}

//--------------------------------------------------------------------------------------//
//		SWITCH CHANNEL REQUEST
//--------------------------------------------------------------------------------------//
void CClientSession::RecvChangeChannelReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_CHANNEL_CHANGE_REQ * req = (sUG_CHAR_CHANNEL_CHANGE_REQ*)pPacket->GetPacketData();

	WORD wResultcode = GAME_FAIL;
	CGameServer* app = (CGameServer*)g_pApp;

/*	if(cPlayer->IsGameMaster() == false) //check if player is game master
		wResultcode = GAME_FAIL;
	else*/ if(cPlayer->IsStanding() == false)
		wResultcode = GAME_CHAR_IS_WRONG_STATE;
	else if (cPlayer->GetCombatMode())
		wResultcode = GAME_CHAR_IS_WRONG_STATE;
	else if(cPlayer->GetCurWorld() == NULL || cPlayer->GetCurWorld()->GetTbldat()->bDynamic) //dont allow to change channel if not inside a world or inside dynamic world
		wResultcode = GAME_FAIL;
	else if(req->destServerChannelId == app->GetGsChannel())
		wResultcode = GAME_WRONG_SERVER_CHANNEL_HAS_BEEN_SPECIFIED;
	else if(app->GetGsChannel() == DOJO_CHANNEL_INDEX)
		wResultcode = GAME_FAIL;
	else
	{
		sSERVER_CHANNEL_INFO* pChannel = g_pServerInfoManager->GetServerChannelInfo(app->GetGsServerId(), req->destServerChannelId);
		if (pChannel == NULL)
			wResultcode = GAME_WRONG_SERVER_CHANNEL_HAS_BEEN_SPECIFIED;
		else if (pChannel->byServerStatus != DBO_SERVER_STATUS_UP || pChannel->bIsVisible == false)
			wResultcode = GAME_SERVER_LOCKED;
		else if (pChannel->dwLoad >= pChannel->dwMaxLoad)
			wResultcode = GAME_CANNOT_CONNECT_TARGET_CHANNEL_FULL;
		else if (strcmp(pChannel->sChannelBuff.szServerChannelName, "DEV") == 0 && cPlayer->IsGameMaster() == false)
			wResultcode = GAME_SERVER_LOCKED;
		else
		{
			CNtlPacket packet2(sizeof(sGM_PLAYER_SWITCH_CHANNEL_REQ));
			sGM_PLAYER_SWITCH_CHANNEL_REQ * res2 = (sGM_PLAYER_SWITCH_CHANNEL_REQ *)packet2.GetPacketData();
			res2->wOpCode = GM_PLAYER_SWITCH_CHANNEL_REQ;
			res2->accountId = cPlayer->GetAccountID();
			res2->serverChannelId = app->GetGsChannel();
			res2->serverId = app->GetGsServerId();
			res2->destServerChannelId = req->destServerChannelId;
			packet2.SetPacketLen(sizeof(sGM_PLAYER_SWITCH_CHANNEL_REQ));
			app->SendTo(app->GetMasterServerSession(), &packet2);

			return;
		}
	}

	CNtlPacket packet(sizeof(sGU_CHAR_CHANNEL_CHANGE_RES));
	sGU_CHAR_CHANNEL_CHANGE_RES * res = (sGU_CHAR_CHANNEL_CHANGE_RES *)packet.GetPacketData();
	res->wOpCode = GU_CHAR_CHANNEL_CHANGE_RES;
	res->wResultCode = wResultcode;
	packet.SetPacketLen(sizeof(sGU_CHAR_CHANNEL_CHANGE_RES));
	app->Send(GetHandle(), &packet);
}

//--------------------------------------------------------------------------------------//
//		SELECT TARGET
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharTargetSelect(CNtlPacket * pPacket)
{
	sUG_CHAR_TARGET_SELECT * req = (sUG_CHAR_TARGET_SELECT*)pPacket->GetPacketData();

	CGameServer* app = (CGameServer*)g_pApp;

	if (cPlayer && cPlayer->IsInitialized() && cPlayer->GetCurWorld())
	{
		//printf("RecvCharTargetSelect: req->hTarget: %u \n", req->hTarget);

		if (req->hTarget == INVALID_HOBJECT || g_pObjectManager->GetObjectA(req->hTarget) || cPlayer->GetCurWorld()->FindStaticObject(req->hTarget))
		{
			if (cPlayer->ChangeTarget(req->hTarget))
			{
				if ((req->hTarget == INVALID_HOBJECT || req->hTarget == cPlayer->GetID()) && cPlayer->GetStateManager()->IsCharCondition(CHARCOND_CONFUSED) && cPlayer->IsFollowing())
					cPlayer->SendCharStateStanding();
			}
		}
	}
}


//--------------------------------------------------------------------------------------//
//		Char sit down
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharSitDown(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_TOGG_SITDOWN * req = (sUG_CHAR_TOGG_SITDOWN*)pPacket->GetPacketData();
	
	if (req->bSitDown)
	{
		cPlayer->GetStateManager()->ChangeCharState(CHARSTATE_SITTING, NULL, true);
	}
	else
	{
		if (cPlayer->GetCharStateID() == CHARSTATE_SITTING) //if already sit then stand up
			cPlayer->GetStateManager()->ChangeCharState(CHARSTATE_STANDING, NULL, true);
	}
}


//--------------------------------------------------------------------------------------//
//		Char togg fight
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharToggFight(CNtlPacket * pPacket)
{
	sUG_CHAR_TOGG_FIGHTING * req = (sUG_CHAR_TOGG_FIGHTING *)pPacket->GetPacketData();

	if (cPlayer && cPlayer->IsInitialized())
		cPlayer->ChangeFightMode(req->bFightMode);
}


//--------------------------------------------------------------------------------------//
//		CHAR START MAIL
//--------------------------------------------------------------------------------------//
void CClientSession::RecvMailStartReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_MAIL_START_REQ * req = (sUG_MAIL_START_REQ*)pPacket->GetPacketData();

	//ERR_LOG(LOG_USER, "User %u load mails. hObject %u byPartsType %u\n", cPlayer->GetCharID(), req->hObject, req->byPartsType);

	cPlayer->LoadMails(req->hObject, req->byPartsType);
}

//--------------------------------------------------------------------------------------//
//		
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvMailLoadReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_MAIL_LOAD_REQ * req = (sUG_MAIL_LOAD_REQ*)pPacket->GetPacketData();

}
//--------------------------------------------------------------------------------------//
//		reload mail
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvMailReloadReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_MAIL_RELOAD_REQ * req = (sUG_MAIL_RELOAD_REQ*)pPacket->GetPacketData();

	cPlayer->ManualReloadMails(req->hObject);
}

//--------------------------------------------------------------------------------------//
//		read mails
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvMailReadReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_MAIL_READ_REQ * req = (sUG_MAIL_READ_REQ*)pPacket->GetPacketData();

	CGameServer* app = (CGameServer*)g_pApp;

	sMAIL_NEW_PROFILE* mail = cPlayer->GetMail(req->mailID);

	CNtlPacket packet(sizeof(sGU_MAIL_READ_RES));
	sGU_MAIL_READ_RES * res = (sGU_MAIL_READ_RES *)packet.GetPacketData();
	res->wOpCode = GU_MAIL_READ_RES;
	res->hObject = req->hObject;
	res->mailID = req->mailID;

	if (mail)
	{
		res->wResultCode = GAME_SUCCESS;
		res->byRemainDay = mail->byExpired;
		res->endTime = mail->endTime;

		if (mail->bIsRead == false)
		{
			CNtlPacket pQry(sizeof(sGQ_MAIL_READ_REQ));
			sGQ_MAIL_READ_REQ * qRes = (sGQ_MAIL_READ_REQ *)pQry.GetPacketData();
			qRes->wOpCode = GQ_MAIL_READ_REQ;
			qRes->handle = cPlayer->GetID();
			qRes->hObject = req->hObject;
			qRes->charID = cPlayer->GetCharID();
			qRes->mailID = req->mailID;
			pQry.SetPacketLen(sizeof(sGQ_MAIL_READ_REQ));
			app->SendTo(app->GetQueryServerSession(), &pQry);

			mail->bIsRead = true;
		}
	}
	else
		res->wResultCode = GAME_MAIL_NOT_FOUND;
	
	packet.SetPacketLen(sizeof(sGU_MAIL_READ_RES));
	g_pApp->Send(GetHandle(), &packet);
}
//--------------------------------------------------------------------------------------//
//		send mails
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvMailSendReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_MAIL_SEND_REQ * req = (sUG_MAIL_SEND_REQ*)pPacket->GetPacketData();

	ITEMID itemid = 0;
	WORD sendmailres = GAME_SUCCESS;
	char* chText = NULL;
	BYTE byTextSize = 0;
	DWORD dwFee = NTL_MAX_BASIC_MAIL_SEND_ZENNY;
	CItem* item = NULL;

	char* chname = Ntl_WC2MB(req->wszTargetName);
	std::string charname = chname;

	if (charname.length() < NTL_MIN_SIZE_CHAR_NAME)
	{
		sendmailres = CHARACTER_TOO_SHORT_NAME;
		Ntl_CleanUpHeapString(chname);
		goto _end;
	}
	else if (charname.length() > NTL_MAX_SIZE_CHAR_NAME)
	{
		sendmailres = CHARACTER_TOO_LONG_NAME;
		Ntl_CleanUpHeapString(chname);
		goto _end;
	}
	//else if (charname.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890") != std::string::npos)
	//{
	//	sendmailres = CHARACTER_BLOCK_STRING_INCLUDED;
	//	Ntl_CleanUpHeapString(chname);
	//	goto _end;
	//}

	//dont allow sending mail while trading or while having private shop
	if (cPlayer->IsTrading() || cPlayer->HasPrivateShop())
	{
		sendmailres = GAME_FAIL;
		goto _end;
	}

	//send mail limit per time
	if (cPlayer->CanSendMail(GetTickCount()) == false)
	{
		sendmailres = GAME_FAIL;
		goto _end;
	}

	if (cPlayer->GetLevel() < MAIL_SEND_REQUIRED_LEVEL) //only lv 29 and higher can send mails
	{
		sendmailres = GAME_FAIL;
		goto _end;
	}

	chText = Ntl_WC2MB(req->wszText);
	byTextSize = (BYTE)strlen(chText);
	Ntl_CleanUpHeapString(chText);

	//check mail size
	if (byTextSize != req->byTextSize || req->byTextSize > NTL_MAX_LENGTH_OF_MAIL_MESSAGE)
	{
		sendmailres = GAME_FAIL;
		goto _end;
	}

	if (_wcsicmp(cPlayer->GetCharName(), req->wszTargetName) == 0)
	{
		sendmailres = GAME_MAIL_CANT_SEND_MAIL_TO_YOURSELF;
		goto _end;
	}

	dwFee = NTL_MAX_BASIC_MAIL_SEND_ZENNY;
	if (req->byMailType != eMAIL_TYPE_BASIC)
		dwFee = NTL_MAX_ATTACH_MAIL_SEND_ZENNY;

	if (req->byMailType != eMAIL_TYPE_ITEM_ZENNY_REQ && req->byMailType != eMAIL_TYPE_ZENNY_REQ)
		dwFee += req->dwZenny;

	// dont allow to send mails with request zeni (Only used to scam)
	if (req->byMailType == eMAIL_TYPE_ZENNY_REQ)
	{
		sendmailres = GAME_FAIL;
		goto _end;
	}

	//check zeni
	if (cPlayer->GetZeni() < dwFee)
	{
		sendmailres = GAME_ZENNY_NOT_ENOUGH;
		goto _end;
	}

	if (req->dwZenny > NTL_MAX_MAIL_SEND_ZENNY_AMOUNTS)
	{
		ERR_LOG(LOG_USER, "HACK ALERT. User %u added more zeni than possible. [%u]", cPlayer->GetCharID(), req->dwZenny);
		sendmailres = GAME_FAIL;
		goto _end;
	}

	if (req->byMailType == eMAIL_TYPE_ITEM || req->byMailType == eMAIL_TYPE_ITEM_ZENNY || req->byMailType ==  eMAIL_TYPE_ITEM_ZENNY_REQ)
	{
		item = cPlayer->GetPlayerItemContainer()->GetItem(req->sItemData.byPlace, req->sItemData.byPos);
		if (!item || item->GetCount() == 0 || IsInvenContainer(item->GetPlace()) == false)
			sendmailres = GAME_MAIL_MAILING_PARTS_NOT_FOUND;
		else if (item->GetCount() > item->GetTbldat()->byMax_Stack)
			sendmailres = GAME_ITEM_STACK_FAIL;
		else if(!item->CanMail() || item->IsLocked())
			sendmailres = GAME_FAIL;
	}
	
	if (sendmailres == GAME_SUCCESS)
	{
		CNtlPacket pQry(sizeof(sGQ_MAIL_SEND_REQ));
		sGQ_MAIL_SEND_REQ * qRes = (sGQ_MAIL_SEND_REQ *)pQry.GetPacketData();
		qRes->wOpCode = GQ_MAIL_SEND_REQ;
		qRes->handle = cPlayer->GetID();
		qRes->hObject = req->hObject;
		qRes->charID = cPlayer->GetCharID();
		NTL_WCSCPY_S(qRes->wszName, NTL_MAX_SIZE_CHAR_NAME + 1, cPlayer->GetCharName());
		qRes->byMailType = req->byMailType;
		if (item)
		{
			qRes->sItemData.byPlace = item->GetPlace();
			qRes->sItemData.byPos = item->GetPos();
			qRes->sItemData.itemID = item->GetItemID();
			item->SetLocked(true);
		}
		qRes->dwZenny = req->dwZenny;
		qRes->byDay = req->byDay;
		NTL_SAFE_WCSCPY(qRes->wszTargetName, req->wszTargetName);
		qRes->byTextSize = req->byTextSize;
		NTL_SAFE_WCSCPY(qRes->wszText, req->wszText);
		pQry.SetPacketLen(sizeof(sGQ_MAIL_SEND_REQ));
		app->SendTo(app->GetQueryServerSession(), &pQry);

		cPlayer->SetMailSentTime(GetTickCount() + 30000);

		return;
	}

	_end:

	CNtlPacket packet(sizeof(sGU_MAIL_SEND_RES));
	sGU_MAIL_SEND_RES * res = (sGU_MAIL_SEND_RES *)packet.GetPacketData();
	res->wOpCode = GU_MAIL_SEND_RES;
	res->hObject = req->hObject;
	res->wResultCode = sendmailres;
	NTL_SAFE_WCSCPY(res->wszTargetName, req->wszTargetName);
	packet.SetPacketLen(sizeof(sGU_MAIL_SEND_RES));
	app->Send(GetHandle(), &packet);
}
//--------------------------------------------------------------------------------------//
//		delete mails
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvMailDelReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_MAIL_DEL_REQ * req = (sUG_MAIL_DEL_REQ*)pPacket->GetPacketData();
	
	CNtlPacket packet(sizeof(sGU_MAIL_DEL_RES));
	sGU_MAIL_DEL_RES * res = (sGU_MAIL_DEL_RES *)packet.GetPacketData();
	res->wOpCode = GU_MAIL_DEL_RES;
	res->hObject = req->hObject;
	res->wResultCode = GAME_SUCCESS;

	if (sMAIL_NEW_PROFILE* mail = cPlayer->GetMail(req->mailID))
	{
		CNtlPacket pQry(sizeof(sGQ_MAIL_DEL_REQ));
		sGQ_MAIL_DEL_REQ * qRes = (sGQ_MAIL_DEL_REQ *)pQry.GetPacketData();
		qRes->wOpCode = GQ_MAIL_DEL_REQ;
		qRes->charID = cPlayer->GetCharID();
		qRes->mailID = req->mailID;
		pQry.SetPacketLen(sizeof(sGQ_MAIL_DEL_REQ));
		app->SendTo(app->GetQueryServerSession(), &pQry);

		cPlayer->DeleteMail(req->mailID);
	}
	else
		res->wResultCode = GAME_MAIL_NOT_FOUND;

	res->mailID = req->mailID;
	packet.SetPacketLen(sizeof(sGU_MAIL_DEL_RES));
	app->Send(GetHandle(), &packet);
}
//--------------------------------------------------------------------------------------//
//		receive item with mail
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvMailItemReceiveReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_MAIL_ITEM_RECEIVE_REQ * req = (sUG_MAIL_ITEM_RECEIVE_REQ*)pPacket->GetPacketData();

	WORD wResultCode = GAME_SUCCESS;

	sMAIL_NEW_PROFILE* mail = cPlayer->GetMail(req->mailID);
	if (mail)
	{
		if (mail->bIsAccept == true) //check if already taken
		{
			wResultCode = GAME_MAIL_INVALID_ACCEPT;
			goto END;
		}

		std::pair<BYTE, BYTE> inv = std::make_pair(INVALID_BYTE, INVALID_BYTE);

		switch (mail->byMailType)
		{
			case eMAIL_TYPE_ITEM: //receive item
			{
				inv = cPlayer->GetPlayerItemContainer()->GetEmptyInventory();
				if (inv.first == INVALID_BYTE)
				{
					wResultCode = GAME_ITEM_INVEN_FULL;
				}
			}
			break;
			case eMAIL_TYPE_ZENNY: //receive zeni
			case eMAIL_TYPE_ITEM_ZENNY: //receive item and zeni
			{
				if (mail->dwZenny == INVALID_DWORD) //check zeni
				{
					wResultCode = GAME_MAIL_INVALID_ZENNY;
				}
				else if (mail->dwZenny > 0)
				{
					if (!cPlayer->CanReceiveZeni(mail->dwZenny))
					{
						wResultCode = GAME_ZENNY_OVER;
					}
				}
				
				if (mail->byMailType == eMAIL_TYPE_ITEM_ZENNY)
				{
					inv = cPlayer->GetPlayerItemContainer()->GetEmptyInventory();
					if (inv.first == INVALID_BYTE)
					{
						wResultCode = GAME_ITEM_INVEN_FULL;
					}
				}
			}
			break;
			case eMAIL_TYPE_ITEM_ZENNY_REQ: //request zeni(send zeni to mail-sender) and give item as reward
			case eMAIL_TYPE_ZENNY_REQ: //request zeni(send zeni to mail-sender)
			{
				if (mail->bySenderType != eMAIL_SENDER_TYPE_RETURN) // only need to check zeni if NOT returned.
				{
					if (cPlayer->GetZeni() < mail->dwZenny) //check if enough zeni
					{
						wResultCode = GAME_MAIL_INVALID_ZENNY;
					}
				}

				if (mail->byMailType == eMAIL_TYPE_ITEM_ZENNY_REQ)
				{
					inv = cPlayer->GetPlayerItemContainer()->GetEmptyInventory();
					if (inv.first == INVALID_BYTE)
					{
						wResultCode = GAME_ITEM_INVEN_FULL;
					}
				}
			}
			break;

			default: break;
		}

		if (wResultCode == GAME_SUCCESS)
		{
			mail->bIsAccept = true; //accept now or people can bug by sending this packet multiple times in row while query server is processing the mail

			if (inv.first != INVALID_BYTE)
				cPlayer->GetPlayerItemContainer()->AddReservedInventory(inv.first, inv.second);

			CNtlPacket pQry(sizeof(sGQ_MAIL_ITEM_RECEIVE_REQ));
			sGQ_MAIL_ITEM_RECEIVE_REQ * qRes = (sGQ_MAIL_ITEM_RECEIVE_REQ *)pQry.GetPacketData();
			qRes->wOpCode = GQ_MAIL_ITEM_RECEIVE_REQ;
			qRes->byMailType = mail->byMailType;
			qRes->charID = cPlayer->GetCharID();
			qRes->handle = cPlayer->GetID();
			qRes->hObject = req->hObject;
			qRes->mailID = mail->mailID;
			qRes->sInven.byPlace = inv.first;
			qRes->sInven.byPos = inv.second;
			pQry.SetPacketLen(sizeof(sGQ_MAIL_ITEM_RECEIVE_REQ));
			app->SendTo(app->GetQueryServerSession(), &pQry);

			return;
		}
	}
	else wResultCode = GAME_MAIL_NOT_FOUND;


END:

	CNtlPacket packet(sizeof(sGU_MAIL_ITEM_RECEIVE_RES));
	sGU_MAIL_ITEM_RECEIVE_RES * res = (sGU_MAIL_ITEM_RECEIVE_RES *)packet.GetPacketData();
	res->wOpCode = GU_MAIL_ITEM_RECEIVE_RES;
	res->hObject = req->hObject;
	res->wResultCode = wResultCode;
	res->mailID = req->mailID;
	packet.SetPacketLen(sizeof(sGU_MAIL_ITEM_RECEIVE_RES));
	app->Send(GetHandle(), &packet);

}
//--------------------------------------------------------------------------------------//
//		delete multiple mails //dont know if this still exist
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvCharMailMultiDelReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_MAIL_MULTI_DEL_REQ * req = (sUG_MAIL_MULTI_DEL_REQ*)pPacket->GetPacketData();

	/*CNtlPacket packet(sizeof(sGU_MAIL_MULTI_DEL_RES));
	sGU_MAIL_MULTI_DEL_RES * res = (sGU_MAIL_MULTI_DEL_RES *)packet.GetPacketData();
	res->wOpCode = GU_MAIL_MULTI_DEL_RES;
	res->wResultCode = GAME_SUCCESS;
	res->hObject = req->hObject;
	res->byCount = req->byCount;

	for (int i = 0; i < req->byCount; i++)
	{
		if (req->byCount == NTL_MAX_COUNT_OF_MULTI_DEL)
			break;

		if (sMAIL_NEW_PROFILE* mail = cPlayer->GetMail(req->aMailID[i]))
		{
			res->aMailID[i] = mail->mailID;

			cPlayer->DeleteMail(mail->mailID);
		}
	}

	packet.SetPacketLen(sizeof(sGU_MAIL_MULTI_DEL_RES));
	app->Send(GetHandle(), &packet);*/
}
//--------------------------------------------------------------------------------------//
//		lock mail
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvMailLockReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_MAIL_LOCK_REQ * req = (sUG_MAIL_LOCK_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_MAIL_LOCK_RES));
	sGU_MAIL_LOCK_RES * res = (sGU_MAIL_LOCK_RES *)packet.GetPacketData();
	res->wOpCode = GU_MAIL_LOCK_RES;
	res->hObject = req->hObject;

	if (sMAIL_NEW_PROFILE* mail = cPlayer->GetMail(req->mailID))
	{
		res->wResultCode = GAME_SUCCESS;
		res->byExpiredDay = mail->byExpired;
		res->endTime = mail->endTime;

		if (mail->bIsLock != req->bIsLock)
		{
			CNtlPacket pQry(sizeof(sGQ_MAIL_LOCK_REQ));
			sGQ_MAIL_LOCK_REQ * qRes = (sGQ_MAIL_LOCK_REQ *)pQry.GetPacketData();
			qRes->wOpCode = GQ_MAIL_LOCK_REQ;
			qRes->handle = cPlayer->GetID();
			qRes->hObject = req->hObject;
			qRes->charID = cPlayer->GetCharID();
			qRes->mailID = mail->mailID;
			qRes->bIsLock = req->bIsLock;
			pQry.SetPacketLen(sizeof(sGQ_MAIL_LOCK_REQ));
			app->SendTo(app->GetQueryServerSession(), &pQry);
		}

		mail->bIsLock = req->bIsLock;
	}
	else
		res->wResultCode = GAME_MAIL_NOT_FOUND;

	res->bIsLock = req->bIsLock;
	res->mailID = req->mailID;
	packet.SetPacketLen(sizeof(sGU_MAIL_LOCK_RES));
	app->Send(GetHandle(), &packet);
}
//--------------------------------------------------------------------------------------//
//		RETURN MAIL
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvCharMailReturnReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_MAIL_RETURN_REQ * req = (sUG_MAIL_RETURN_REQ*)pPacket->GetPacketData();
	
	WORD wResultcode = GAME_SUCCESS;

	if (sMAIL_NEW_PROFILE* mail = cPlayer->GetMail(req->mailID))
	{
		if (mail->bIsAccept == false && mail->bIsLock == false)
		{
			if (mail->bySenderType == eMAIL_SENDER_TYPE_BASIC)
			{
				mail->bIsLock = true; //set temp lock because return can fail.

				CNtlPacket pQry(sizeof(sGQ_MAIL_RETURN_REQ));
				sGQ_MAIL_RETURN_REQ * qRes = (sGQ_MAIL_RETURN_REQ *)pQry.GetPacketData();
				qRes->wOpCode = GQ_MAIL_RETURN_REQ;
				qRes->handle = cPlayer->GetID();
				qRes->hObject = req->hObject;
				qRes->charID = cPlayer->GetCharID();
				qRes->mailID = mail->mailID;
				pQry.SetPacketLen(sizeof(sGQ_MAIL_RETURN_REQ));
				app->SendTo(app->GetQueryServerSession(), &pQry);

				return;
			}
			else wResultcode = GAME_MAIL_INVALID_RETURN;
		}
		else wResultcode = GAME_FAIL;
	}
	else wResultcode = GAME_MAIL_NOT_FOUND;

	CNtlPacket packet(sizeof(sGU_MAIL_RETURN_RES));
	sGU_MAIL_RETURN_RES * res = (sGU_MAIL_RETURN_RES *)packet.GetPacketData();
	res->wOpCode = GU_MAIL_RETURN_RES;
	res->hObject = req->hObject;
	res->mailID = req->mailID;
	res->wResultCode = wResultcode;
	packet.SetPacketLen(sizeof(sGU_MAIL_RETURN_RES));
	app->Send(GetHandle(), &packet); //Send to player
}
//--------------------------------------------------------------------------------------//
//		char away req
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvCharAwayReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_CHAR_AWAY_REQ * req = (sUG_CHAR_AWAY_REQ*)pPacket->GetPacketData();
	
	CNtlPacket packet(sizeof(sGU_CHAR_AWAY_RES));
	sGU_CHAR_AWAY_RES * res = (sGU_CHAR_AWAY_RES *)packet.GetPacketData();
	res->wOpCode = GU_CHAR_AWAY_RES;
	res->wResultCode = GAME_SUCCESS;
	res->bIsAway = req->bIsAway;
	packet.SetPacketLen( sizeof(sGU_CHAR_AWAY_RES) );
	g_pApp->Send( GetHandle(), &packet );

	//update database
	if (cPlayer->IsMailAway() != req->bIsAway)
	{
		CNtlPacket pQry(sizeof(sGQ_CHAR_AWAY_REQ));
		sGQ_CHAR_AWAY_REQ * qRes = (sGQ_CHAR_AWAY_REQ *)pQry.GetPacketData();
		qRes->wOpCode = GQ_CHAR_AWAY_REQ;
		qRes->handle = cPlayer->GetID();
		qRes->charID = cPlayer->GetCharID();
		qRes->bIsAway = req->bIsAway;
		pQry.SetPacketLen(sizeof(sGQ_CHAR_AWAY_REQ));
		app->SendTo(app->GetQueryServerSession(), &pQry);
	}

	cPlayer->SetIsMailAway(req->bIsAway);
}
//--------------------------------------------------------------------------------------//
//		char follow move
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharFollowMove(CNtlPacket * pPacket)
{
	sUG_CHAR_FOLLOW_MOVE * req = (sUG_CHAR_FOLLOW_MOVE*)pPacket->GetPacketData();

	if (!cPlayer || !cPlayer->IsInitialized() || cPlayer->GetCurWorld() == NULL)
		return;

	if (cPlayer->GetStateManager()->CanCharStateTransition(CHARSTATE_FOLLOWING) == false)
	{
		CNtlPacket packet(sizeof(sGU_CHAR_STATE_WRONG));
		sGU_CHAR_STATE_WRONG * res = (sGU_CHAR_STATE_WRONG *)packet.GetPacketData();
		res->wOpCode = GU_CHAR_STATE_WRONG;
		res->wPrevOPCode = UG_CHAR_FOLLOW_MOVE;
		res->handle = cPlayer->GetID();
		res->byStateID = cPlayer->GetCharStateID();
		g_pApp->Send(GetHandle(), &packet);

		return;
	}

	//printf("byAvatarType:%u, byMovementReason:%u, dwTimeStamp:%u, fDistance:%f, hTarget:%u \n", req->byAvatarType, req->byMovementReason, req->dwTimeStamp, req->fDistance, req->hTarget);

	if (req->byMovementReason == DBO_MOVE_FOLLOW_FRIENDLY
		&& (cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_DOJO || cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_RANKBATTLE)
		)
	{
		return;
	}
	
	bool bSuccess = false;
	sVECTOR3 newdestloc;

	CCharacter* pChar = g_pObjectManager->GetChar(req->hTarget);
	if (pChar && pChar->IsInitialized())
	{
		pChar->GetCurLoc().CopyTo(newdestloc);
		bSuccess = cPlayer->SendCharStateFollowing(req->hTarget, req->fDistance, req->byMovementReason, newdestloc, true);
	}
	else
	{
		CTriggerObject* pObject = cPlayer->GetCurWorld()->FindStaticObject(req->hTarget);
		if (pObject)
		{
			pObject->GetCurLoc().CopyTo(newdestloc);
			bSuccess = cPlayer->SendCharStateFollowing(req->hTarget, req->fDistance, req->byMovementReason, newdestloc, true);
		}
	}
}

//--------------------------------------------------------------------------------------//
//		char follow move air
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharAirFollowMove(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_AIR_FOLLOW_MOVE * req = (sUG_CHAR_AIR_FOLLOW_MOVE*)pPacket->GetPacketData();
	
	CCharacter* pChar = g_pObjectManager->GetChar(req->hTarget);
	if (pChar == NULL || pChar->IsInitialized() == false)
		return;

	if (req->fDistance > 100.f)
	{
		ERR_LOG(LOG_HACK, "Player: %u cant start follow because range is over 100f", cPlayer->GetID());
	}

	if (req->byMovementReason > DBO_MOVE_FOLLOW_SKILL_ATTACK)
	{
		ERR_LOG(LOG_HACK, "Player: %u cant start follow because range is over 100f");
		return;
	}

	sVECTOR3 newdestloc;
	pChar->GetCurLoc().CopyTo(newdestloc);

	//CNtlPacket packet(sizeof(sGU_CHAR_FOLLOW_MOVE));
	//sGU_CHAR_FOLLOW_MOVE * res = (sGU_CHAR_FOLLOW_MOVE *)packet.GetPacketData();
	//res->wOpCode = GU_CHAR_FOLLOW_MOVE;
	//res->handle = cPlayer->GetID();
	//res->hTarget = req->hTarget;
	//res->fDistance = req->fDistance;
	//res->byMovementReason = req->byMovementReason;
	//res->byMoveFlag = cPlayer->GetMoveFlag();
	//res->byMoveStatus = NTL_MOVE_STATUS_AIR_MOVE;
	//res->dwTimeStamp = req->dwTimeStamp;
	//res->vDestLoc.x = newdestloc.x;
	//res->vDestLoc.y = newdestloc.y;
	//res->vDestLoc.z = newdestloc.z;
	//packet.SetPacketLen( sizeof(sGU_CHAR_FOLLOW_MOVE) );
	//cPlayer->Broadcast(&packet);

	cPlayer->SendCharStateFollowing(req->hTarget, req->fDistance, req->byMovementReason, newdestloc, true);
}

//--------------------------------------------------------------------------------------//
//		char follow move sync
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharFollowMoveSync(CNtlPacket * pPacket)
{
	sUG_CHAR_FOLLOW_MOVE_SYNC * req = (sUG_CHAR_FOLLOW_MOVE_SYNC*)pPacket->GetPacketData();
	
	//CNtlPacket packet(sizeof(sGU_CHAR_FOLLOW_MOVE_SYNC));
	//sGU_CHAR_FOLLOW_MOVE_SYNC * res = (sGU_CHAR_FOLLOW_MOVE_SYNC *)packet.GetPacketData();
	//res->wOpCode = GU_CHAR_FOLLOW_MOVE_SYNC;
	//res->handle = req->hSubject;
	//packet.SetPacketLen( sizeof(sGU_CHAR_FOLLOW_MOVE_SYNC) );
	//cPlayer->SendPacket(&packet);
}

//--------------------------------------------------------------------------------------//
//		Create Guild
//--------------------------------------------------------------------------------------//
void CClientSession::RecvGuildCreateReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_GUILD_CREATE_REQ * req = (sUG_GUILD_CREATE_REQ*)pPacket->GetPacketData();

	std::string guildname = Ntl_WC2MB(req->wszGuildName);
	WORD result = GAME_SUCCESS;

	CNpc* pNpc = g_pObjectManager->GetNpc(req->hGuildManagerNpc);

	if (pNpc == NULL) //check if npc exist
		result = GAME_GUILD_NO_GUILD_MANAGER_NPC_FOUND;
	else if (pNpc->GetTbldat()->byJob != NPC_JOB_GUILD_MANAGER) //check if npc is a guild manager
		result = GAME_GUILD_NOT_GUILD_MANAGER_NPC;
	else if (!cPlayer->IsInRange(pNpc, DBO_DISTANCE_CHECK_TOLERANCE)) //check if npc is in distance
		result = GAME_GUILD_GUILD_MANAGER_IS_TOO_FAR;
	else if (guildname.length() < NTL_MIN_SIZE_GUILD_NAME || guildname.length() > NTL_MAX_SIZE_GUILD_NAME)	//check guild name length
		result = GAME_GUILD_NOT_PROPER_GUILD_NAME_LENGTH;
	else if(cPlayer->GetGuildID() > 0)								//check if has guild
		result = COMMUNITY_GUILD_YOU_ARE_ALREADY_IN_A_GUILD;
	else if(cPlayer->GetZeni() < DBO_ZENNY_FOR_NEW_GUILD)			//check if enough zeni
		result = GAME_GUILD_NEED_MORE_ZENNY_FOR_NEW_GUILD;
	else if(cPlayer->GetLevel() < DBO_LEVEL_FOR_NEW_GUILD)
		result = GAME_CHAR_LEVEL_FAIL;
	//else if (guildname.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890") != std::string::npos)	//check guild name
	//	result = GAME_GUILD_GUILD_NAME_HAS_INVALID_CHARACTER;
	else if(cPlayer->GetParty() == NULL)	//check if has no party
		result = GAME_COMMON_YOU_ARE_NOT_IN_A_PARTY;
	else if(cPlayer->GetParty()->GetPartyMemberCount() < NTL_MAX_MEMBER_IN_PARTY) //check if party member count less than max member count
		result = COMMUNITY_GUILD_NEED_MORE_PARTY_MEMBER_FOR_NEW_GUILD;
	else if (cPlayer->GetParty()->GetPartyLeaderID() != cPlayer->GetID()) //check if player is not party leader
		result = GAME_COMMON_YOU_ARE_NOT_A_PARTY_LEADER;
	else if(cPlayer->GetParty()->IsMemberInsideGuild()) //check if a party member already inside a guild
		result = COMMUNITY_GUILD_NEED_MORE_PARTY_MEMBER_FOR_NEW_GUILD;

	if(result == GAME_SUCCESS)
	{
		CNtlPacket cPacket(sizeof(sGT_GUILD_CREATE));
		sGT_GUILD_CREATE * cRes = (sGT_GUILD_CREATE *)cPacket.GetPacketData();
		cRes->wOpCode = GT_GUILD_CREATE;
		cRes->creatorCharId = cPlayer->GetCharID();
		NTL_SAFE_WCSCPY(cRes->wszGuildName, req->wszGuildName );
		cPacket.SetPacketLen( sizeof(sGT_GUILD_CREATE) );
		app->SendTo(app->GetChatServerSession(), &cPacket); //Send to chat server
	}
	else
	{
		CNtlPacket packet(sizeof(sGU_GUILD_CREATE_RES));
		sGU_GUILD_CREATE_RES * res = (sGU_GUILD_CREATE_RES *)packet.GetPacketData();
		res->wOpCode = GU_GUILD_CREATE_RES;
		res->wResultCode = result;
		packet.SetPacketLen(sizeof(sGU_GUILD_CREATE_RES));
		app->Send(GetHandle(), &packet);
	}

	//clear string
	guildname.erase();
}

//--------------------------------------------------------------------------------------//
//		INVITE Guild
//--------------------------------------------------------------------------------------//
void CClientSession::RecvGuildInviteReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_GUILD_INVITE_REQ * req = (sUG_GUILD_INVITE_REQ*)pPacket->GetPacketData();

	WORD resultcode = GAME_SUCCESS;

	CNtlPacket packet(sizeof(sGU_GUILD_INVITE_RES));
	sGU_GUILD_INVITE_RES * res = (sGU_GUILD_INVITE_RES *)packet.GetPacketData();
	res->wOpCode = GU_GUILD_INVITE_RES;

	if (cPlayer->GetGuildID() > 0) //check if player has guild
	{
		CPlayer* target = g_pObjectManager->GetPC(req->hTarget);
		if (target && target->IsInitialized()) //check if target is online
		{
			NTL_WCSCPY_S(res->wszTargetName, NTL_MAX_SIZE_CHAR_NAME + 1, target->GetCharName());

			if (target->GetGuildID() == 0) //check if has no guild.
			{
				if (NtlGetDistance(cPlayer->GetCurLoc(), target->GetCurLoc()) < 50.0f) //check target range
				{
					CNtlPacket cPacket(sizeof(sGT_GUILD_INVITE_REQ));
					sGT_GUILD_INVITE_REQ * cRes = (sGT_GUILD_INVITE_REQ *)cPacket.GetPacketData();
					cRes->wOpCode = GT_GUILD_INVITE_REQ;
					cRes->invitorCharId = cPlayer->GetCharID();
					cRes->targetCharId = target->GetCharID();
					NTL_WCSCPY_S(cRes->wszTargetName, NTL_MAX_SIZE_CHAR_NAME + 1, target->GetCharName());
					cPacket.SetPacketLen(sizeof(sGT_GUILD_INVITE_REQ));
					app->SendTo(app->GetChatServerSession(), &cPacket); //Send to chat server

					return;
				}
				else resultcode = GAME_TARGET_TOO_FAR;
			}
			else resultcode = COMMUNITY_GUILD_TARGET_IS_ALREADY_IN_A_GUILD;
		}
		else resultcode = GAME_TARGET_NOT_FOUND;
	}
	else resultcode = GAME_GUILD_NO_GUILD_FOUND;
	
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_GUILD_INVITE_RES));
	app->Send(GetHandle(), &packet);
}

//--------------------------------------------------------------------------------------//
//		ADD GUILD FUNCTION REQUEST
//--------------------------------------------------------------------------------------//
void CClientSession::RecvAddGuildFunctionReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;
	WORD resultcode = GAME_SUCCESS;
	sUG_GUILD_FUNCTION_ADD_REQ * req = (sUG_GUILD_FUNCTION_ADD_REQ*)pPacket->GetPacketData();

	CNpc* pNpc = g_pObjectManager->GetNpc(req->hGuildManagerNpc);
	eDBO_GUILD_FUNCTION eGuildFunction = static_cast<eDBO_GUILD_FUNCTION>(req->byFunction);
	DWORD dwNeedZeni = 0;

	if (pNpc == NULL) //check if npc exist
		resultcode = GAME_GUILD_NO_GUILD_MANAGER_NPC_FOUND;
	else if (pNpc->GetTbldat()->byJob != NPC_JOB_GUILD_MANAGER) //check if npc is a guild manager
		resultcode = GAME_GUILD_NOT_GUILD_MANAGER_NPC;
	else if (!cPlayer->IsInRange(pNpc, DBO_DISTANCE_CHECK_TOLERANCE)) //check if npc is in distance
		resultcode = GAME_GUILD_GUILD_MANAGER_IS_TOO_FAR;
	else if (cPlayer->GetGuildID() == 0) //check if has guild
		resultcode = GAME_GUILD_NO_GUILD_FOUND;
	else if (!IsGuildFunction(eGuildFunction)) //check if function exist
		resultcode = GAME_GUILD_NON_EXISTING_GUILD_FUNCTION;
	else
	{
		if (CGuild* pGuild = g_pGuildManager->GetGuild(cPlayer->GetGuildID()))
		{
			if (BIT_FLAG_TEST(pGuild->GetGuildFunctionFlag(), MAKE_BIT_FLAG64(req->byFunction)) == false)
			{
				dwNeedZeni = Dbo_GetGuildFunctionInfo(eGuildFunction)->dwRequiredZenny;

				if (cPlayer->GetZeni() < dwNeedZeni)
					resultcode = GAME_GUILD_NEED_MORE_ZENNY;
				else
				{
					CNtlPacket cPacket(sizeof(sGT_GUILD_FUNCTION_ADD_REQ));
					sGT_GUILD_FUNCTION_ADD_REQ * cRes = (sGT_GUILD_FUNCTION_ADD_REQ *)cPacket.GetPacketData();
					cRes->wOpCode = GT_GUILD_FUNCTION_ADD_REQ;
					cRes->masterCharId = cPlayer->GetCharID();
					cRes->byFunction = req->byFunction;
					cRes->dwZenny = dwNeedZeni;
					cPacket.SetPacketLen(sizeof(sGT_GUILD_FUNCTION_ADD_REQ));
					app->SendTo(app->GetChatServerSession(), &cPacket); //Send to chat server

					return;
				}
			}
			else resultcode = GAME_GUILD_ALREADY_HAS_GUILD_FUNCTION;
		}
		else resultcode = GAME_GUILD_NO_GUILD_FOUND;
	}

	CNtlPacket packet(sizeof(sGU_GUILD_FUNCTION_ADD_RES));
	sGU_GUILD_FUNCTION_ADD_RES * res = (sGU_GUILD_FUNCTION_ADD_RES *)packet.GetPacketData();
	res->wOpCode = GU_GUILD_FUNCTION_ADD_RES;
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_GUILD_FUNCTION_ADD_RES));
	app->Send(GetHandle(), &packet);
}

//--------------------------------------------------------------------------------------//
//		GUILD DONATE ZENNY
//--------------------------------------------------------------------------------------//
void CClientSession::RecvGuildGiveZeniReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_GUILD_GIVE_ZENNY * req = (sUG_GUILD_GIVE_ZENNY*)pPacket->GetPacketData();
	
	WORD resultcode = GAME_SUCCESS;

	DWORD dwZeni = req->dwZenny;

	/*switch (dwZeni)
	{
		case 10000: dwZeni = 50000;
		case 50000: dwZeni = 100000;
		case 100000: dwZeni = 1000000;
		default: dwZeni = 50000;
	}*/

	CNpc* pNpc = g_pObjectManager->GetNpc(req->hGuildManagerNpc);
	
	if (pNpc == NULL) //check if npc exist
		resultcode = GAME_GUILD_NO_GUILD_MANAGER_NPC_FOUND;
	else if (pNpc->GetTbldat()->byJob != NPC_JOB_GUILD_MANAGER) //check if npc is a guild manager
		resultcode = GAME_GUILD_NOT_GUILD_MANAGER_NPC;
	else if (!cPlayer->IsInRange(pNpc, DBO_DISTANCE_CHECK_TOLERANCE)) //check if npc is in distance
		resultcode = GAME_GUILD_GUILD_MANAGER_IS_TOO_FAR;
	else if(cPlayer->GetZeni() < dwZeni) //check if has enough zeni
		resultcode = GAME_GUILD_GIVE_ZENNY_NOT_MATCHED;
	else if(cPlayer->GetGuildID() == 0) //check if has guild
		resultcode = GAME_GUILD_NO_GUILD_FOUND;
	else
	{
		CNtlPacket cPacket(sizeof(sGT_GUILD_GIVE_ZENNY_REQ));
		sGT_GUILD_GIVE_ZENNY_REQ * cRes = (sGT_GUILD_GIVE_ZENNY_REQ *)cPacket.GetPacketData();
		cRes->wOpCode = GT_GUILD_GIVE_ZENNY_REQ;
		cRes->charId = cPlayer->GetCharID();
		cRes->dwZenny = dwZeni;
		cPacket.SetPacketLen( sizeof(sGT_GUILD_GIVE_ZENNY_REQ) );
		app->SendTo(app->GetChatServerSession(), &cPacket); //Send to chat server

		return;
	}

	
	CNtlPacket packet(sizeof(sGU_GUILD_GIVE_ZENNY_RES));
	sGU_GUILD_GIVE_ZENNY_RES * res = (sGU_GUILD_GIVE_ZENNY_RES *)packet.GetPacketData();
	res->wOpCode = GU_GUILD_GIVE_ZENNY_RES;
	res->wResultCode = resultcode;
	packet.SetPacketLen( sizeof(sGU_GUILD_GIVE_ZENNY_RES) );
	app->Send(GetHandle(), &packet );
}


void CClientSession::RecvGuildBankStartReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_GUILD_BANK_START_REQ * req = (sUG_GUILD_BANK_START_REQ*)pPacket->GetPacketData();

	WORD wResultCode = GAME_SUCCESS;

	CNpc* pNpc = g_pObjectManager->GetNpc(req->handle);
	if (pNpc == NULL)	//check if NPC exist
		wResultCode = GAME_TARGET_NOT_FOUND;
	else if (cPlayer->IsInRange(pNpc, DBO_DISTANCE_CHECK_TOLERANCE) == false) //check if npc is in range
		wResultCode = GAME_TARGET_TOO_FAR;
	else if (pNpc->GetTbldat()->byJob != NPC_JOB_BANKER)	//check if npc is a banker
		wResultCode = GAME_TARGET_HAS_DIFFERENT_JOB;
	else if (cPlayer->IsUsingBank() == true)	//check if has the bank open
		wResultCode = GAME_FAIL;
	else if (cPlayer->IsBankLoaded() == false)	//bank is loaded before this packet is received
		wResultCode = GAME_FAIL;
	else if (app->GetChatServerSession() == NULL)
		wResultCode = GAME_FAIL;
	else if (cPlayer->GetGuildID() == 0)
		wResultCode = GAME_GUILD_NOT_EXIST;
	else if(cPlayer->GetPlayerItemContainer()->IsUsingGuildBank() == true)
		wResultCode = GAME_FAIL;
	else
	{
		//send open guild bank request to character server

		CNtlPacket packetChat(sizeof(sGT_GUILD_BANK_START_REQ));
		sGT_GUILD_BANK_START_REQ * resChat = (sGT_GUILD_BANK_START_REQ *)packetChat.GetPacketData();
		resChat->wOpCode = GT_GUILD_BANK_START_REQ;
		resChat->hNpcHandle = req->handle;
		resChat->charId = cPlayer->GetCharID();
		packetChat.SetPacketLen(sizeof(sGT_GUILD_BANK_START_REQ));
		g_pApp->SendTo(app->GetChatServerSession(), &packetChat);

		return;
	}

	CNtlPacket packet(sizeof(sGU_GUILD_BANK_START_RES));
	sGU_GUILD_BANK_START_RES * res = (sGU_GUILD_BANK_START_RES *)packet.GetPacketData();
	res->wOpCode = GU_GUILD_BANK_START_RES;
	res->wResultCode = wResultCode;
	res->handle = req->handle;
	packet.SetPacketLen(sizeof(sGU_GUILD_BANK_START_RES));
	g_pApp->Send(GetHandle(), &packet);
}


void CClientSession::RecvGuildBankMoveReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_GUILD_BANK_MOVE_REQ * req = (sUG_GUILD_BANK_MOVE_REQ*)pPacket->GetPacketData();
	WORD item_move_res = GAME_SUCCESS;

	CNtlPacket pQry(sizeof(sGQ_GUILD_BANK_MOVE_REQ));
	sGQ_GUILD_BANK_MOVE_REQ * rQry = (sGQ_GUILD_BANK_MOVE_REQ *)pQry.GetPacketData();
	rQry->wOpCode = GQ_GUILD_BANK_MOVE_REQ;
	rQry->handle = cPlayer->GetID();
	rQry->charId = cPlayer->GetCharID();
	rQry->guildID = cPlayer->GetGuildID();
	rQry->bySrcPlace = req->bySrcPlace;
	rQry->bySrcPos = req->bySrcPos;
	rQry->byDstPlace = req->byDestPlace;
	rQry->byDstPos = req->byDestPos;
	rQry->hNpcHandle = req->handle;

	CItem* src_item = NULL;
	if (cPlayer->IsUsingBank() == false || cPlayer->IsBankLoaded() == false || cPlayer->GetPlayerItemContainer()->IsUsingGuildBank() == false)
	{
		item_move_res = GAME_FAIL;
		goto END;
	}

	if (cPlayer->GetPlayerItemContainer()->IsInventoryReserved(req->bySrcPlace, req->bySrcPos) || cPlayer->GetPlayerItemContainer()->IsInventoryReserved(req->byDestPlace, req->byDestPos))
	{
		item_move_res = GAME_FAIL;
		goto END;
	}

	src_item = cPlayer->GetPlayerItemContainer()->GetItem(req->bySrcPlace, req->bySrcPos);
	if (src_item && src_item->GetCount() > 0)
	{
		//check if source item can be stored in guild bank
		if (src_item->CanGuildWarehouse() == false)
		{
			item_move_res = GAME_ITEM_NOT_GO_THERE;
			goto END;
		}

		if (src_item->IsEquipped() == false && src_item->IsLocked() || src_item->GetOwner() == NULL)
		{
			item_move_res = GAME_ITEM_IS_LOCK;
			goto END;
		}

		CItem* dest_item = cPlayer->GetPlayerItemContainer()->GetItem(req->byDestPlace, req->byDestPos);

		sITEM_TBLDAT* pItemData = src_item->GetTbldat();

		if (dest_item) //check if dest item can be stored in guild bank
		{
			if (dest_item->CanGuildWarehouse() == false)
			{
				item_move_res = GAME_ITEM_NOT_GO_THERE;
				goto END;
			}
			if (dest_item && ((dest_item->IsLocked() && dest_item->IsEquipped() == false) || dest_item->GetOwner() == NULL))
			{
				item_move_res = GAME_ITEM_IS_LOCK;
				goto END;
			}
		}

		if (req->bySrcPlace == CONTAINER_TYPE_BAGSLOT) //switch bag from bag slot
		{
			if (req->bySrcPos == BAGSLOT_POSITION_BAGSLOT_POSITION_0) //DONT ALLOW TO MOVE THE FIRST BAG SLOT.
			{
				item_move_res = GAME_ITEM_IS_LOCK;
				goto END;
			}

			if (!cPlayer->GetPlayerItemContainer()->IsBagEmpty(req->bySrcPos)) //check if bag is empty
			{
				item_move_res = GAME_ITEM_IS_LOCK;
				goto END;
			}
		}

		if (req->bySrcPlace == CONTAINER_TYPE_BANKSLOT || req->byDestPlace == CONTAINER_TYPE_BANKSLOT) //dont allow moving anything from/to bankslot
		{
			item_move_res = GAME_ITEM_IS_LOCK;
			goto END;
		}

		//check if can move into dest inventory
		if (IsInvenContainer(req->byDestPlace))
		{
			CItem* pBagItem = cPlayer->GetPlayerItemContainer()->GetActiveBag(req->byDestPlace - 1);
			if (pBagItem)
			{
				if (req->byDestPos >= pBagItem->GetBagSize())
				{
					item_move_res = GAME_ITEM_POSITION_FAIL;
					goto END;
				}

				if (pBagItem->IsExpired())
				{
					item_move_res = GAME_FAIL;
					goto END;
				}
			}
			else
			{
				item_move_res = GAME_ITEM_POSITION_FAIL;
				goto END;
			}
		}

		//check if guild bag pos is valid
		if(IsGuildContainer(req->byDestPlace) && req->byDestPos >= NTL_MAX_GUILD_ITEM_SLOT)
		{
			item_move_res = GAME_ITEM_POSITION_FAIL;
			goto END;
		}

		if (req->byDestPlace == CONTAINER_TYPE_BAGSLOT) //move bag to bag slot
		{
			if (dest_item)
			{
				item_move_res = GAME_ITEM_NOT_GO_THERE;
				goto END;
			}

			if (!src_item->IsBag()) //check if item is bag
			{
				item_move_res = GAME_ITEM_NOT_GO_THERE;
				goto END;
			}
			else if (dest_item && dest_item->IsBag()) //check if dest item exist and if its a bag
			{
				if (!cPlayer->GetPlayerItemContainer()->IsBagEmpty(req->byDestPos)) //check if dest bag is empty
				{
					item_move_res = GAME_FAIL;
					goto END;
				}
			}
		}

		if (req->bySrcPlace == CONTAINER_TYPE_EQUIP) //unequip item
		{
			if (dest_item == NULL)
			{
				if (cPlayer->UnequipItem(src_item) == false) //unequip
					item_move_res = GAME_FAIL;
			}
			else item_move_res = GAME_ITEM_INVEN_FULL;
		}
		if (req->byDestPlace == CONTAINER_TYPE_EQUIP) //check if equip item
		{
			if (cPlayer->GetLevel() < pItemData->byNeed_Min_Level) //check level
				item_move_res = GAME_ITEM_NEED_MORE_LEVEL;
			else if (cPlayer->GetLevel() > pItemData->byNeed_Max_Level)
				item_move_res = GAME_ITEM_TOO_HIGH_LEVEL_TO_USE_ITEM;

			else if (Dbo_CheckClass(cPlayer->GetClass(), pItemData->dwNeed_Class_Bit_Flag) == false) //check class
				item_move_res = GAME_ITEM_CLASS_FAIL;

			else if (BIT_FLAG_TEST(MAKE_BIT_FLAG(cPlayer->GetGender()), pItemData->dwNeed_Gender_Bit_Flag) == false) //check gender
				item_move_res = GAME_ITEM_GENDER_DOESNT_MATCH;

			else if (pItemData->byRace_Special != cPlayer->GetRace() && pItemData->byRace_Special != INVALID_BYTE) //check race
				item_move_res = GAME_CHAR_RACE_FAIL;

			else if (BIT_FLAG_TEST(MAKE_BIT_FLAG(req->byDestPos), pItemData->dwEquip_Slot_Type_Bit_Flag) == false) //check if item can go to that position
				item_move_res = GAME_ITEM_POSITION_FAIL;

			else if (src_item->GetRestrictState() == ITEM_RESTRICT_STATE_TYPE_SEAL)
				item_move_res = GAME_FAIL;

			else if (dest_item && dest_item->GetRestrictState() == ITEM_RESTRICT_STATE_TYPE_SEAL)
				item_move_res = GAME_FAIL;

			if (item_move_res == GAME_SUCCESS)
			{
				if (cPlayer->GetPlayerItemContainer()->IsInventoryReserved(req->byDestPlace, req->byDestPos))
					item_move_res = GAME_FAIL;
				else
				{
					cPlayer->EquipItem(src_item, req->byDestPos);

					if (src_item->GetRestrictState() == 0 && src_item->GetTbldat()->byRestrictType > 0) //check if must restrict item
					{
						rQry->bRestrictUpdate = true;

						if (src_item->GetTbldat()->byRestrictType == ITEM_RESTRICT_TYPE_EQUIP)
							rQry->bySrcRestrictState = ITEM_RESTRICT_STATE_TYPE_LIMIT;
						else
							rQry->bySrcRestrictState = GetDefaultRestrictState(src_item->GetTbldat()->byRestrictType, src_item->GetTbldat()->byItem_Type, true);

						src_item->SetRestrictState(rQry->bySrcRestrictState);

						CNtlPacket packetItemUpdate(sizeof(sGU_ITEM_UPDATE));
						sGU_ITEM_UPDATE * resIU = (sGU_ITEM_UPDATE *)packetItemUpdate.GetPacketData();
						resIU->wOpCode = GU_ITEM_UPDATE;
						resIU->handle = src_item->GetID();
						resIU->sItemData = src_item->GetItemData();
						packetItemUpdate.SetPacketLen(sizeof(sGU_ITEM_UPDATE));
						app->Send(GetHandle(), &packetItemUpdate);
					}
				}
			}
		}


		if (req->byDestPlace == CONTAINER_TYPE_SCOUT) //dont allow to use scouter chips
			item_move_res = GAME_ITEM_NOT_GO_THERE;

		/*Update item in map and database if move success*/
		if (item_move_res == GAME_SUCCESS)
		{
			src_item->SetLocked(true);

			rQry->srcItemID = src_item->GetItemID();
			rQry->hSrcItem = src_item->GetID();

			if (dest_item)
			{
				dest_item->SetLocked(true);

				rQry->dstItemId = dest_item->GetItemID();
				rQry->hDstItem = dest_item->GetID();

				if (req->byDestPlace == CONTAINER_TYPE_EQUIP)
					dest_item->SetEquipped(false);
			}
			else
			{
				rQry->hDstItem = INVALID_HOBJECT;
				rQry->dstItemId = INVALID_ITEMID;

				//reseve dest place/pos to avoid item getting there while moving. Only need to do this when moving to a free slot
				cPlayer->GetPlayerItemContainer()->AddReservedInventory(req->byDestPlace, req->byDestPos);
			}

			pQry.SetPacketLen(sizeof(sGQ_GUILD_BANK_MOVE_REQ));
			app->SendTo(app->GetQueryServerSession(), &pQry);

			return;
		}

	}
	else item_move_res = GAME_ITEM_NOT_FOUND;


END:
	CNtlPacket packet(sizeof(sGU_GUILD_BANK_MOVE_RES));
	sGU_GUILD_BANK_MOVE_RES * res = (sGU_GUILD_BANK_MOVE_RES *)packet.GetPacketData();
	res->wOpCode = GU_GUILD_BANK_MOVE_RES;
	res->handle = req->handle;
	res->wResultCode = item_move_res;
	packet.SetPacketLen(sizeof(sGU_GUILD_BANK_MOVE_RES));
	app->Send(GetHandle(), &packet);
}


void CClientSession::RecvGuildBankMoveStackReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_GUILD_BANK_MOVE_STACK_REQ * req = (sUG_GUILD_BANK_MOVE_STACK_REQ*)pPacket->GetPacketData();
	WORD resultcode = GAME_SUCCESS;

	CNtlPacket packet(sizeof(sGQ_GUILD_BANK_MOVE_STACK_REQ));
	sGQ_GUILD_BANK_MOVE_STACK_REQ * res = (sGQ_GUILD_BANK_MOVE_STACK_REQ *)packet.GetPacketData();
	res->wOpCode = GQ_GUILD_BANK_MOVE_STACK_REQ;
	res->handle = cPlayer->GetID();
	res->hNpcHandle = req->handle;
	res->charId = cPlayer->GetCharID();
	res->guildID = cPlayer->GetGuildID();
	res->bySrcPlace = req->bySrcPlace;
	res->bySrcPos = req->bySrcPos;
	res->byDstPlace = req->byDestPlace;
	res->byDstPos = req->byDestPos;

	CItem* pSrcItem = NULL;
	CItem* pDestItem = NULL;
	if (cPlayer->IsUsingBank() == false || cPlayer->IsBankLoaded() == false || cPlayer->GetPlayerItemContainer()->IsUsingGuildBank() == false)
	{
		resultcode = GAME_FAIL;
		goto END;
	}

	if (cPlayer->GetPlayerItemContainer()->IsInventoryReserved(req->bySrcPlace, req->bySrcPos) || cPlayer->GetPlayerItemContainer()->IsInventoryReserved(req->byDestPlace, req->byDestPos))
	{
		resultcode = GAME_FAIL;
		goto END;
	}

	pSrcItem = cPlayer->GetPlayerItemContainer()->GetItem(req->bySrcPlace, req->bySrcPos);
	pDestItem = cPlayer->GetPlayerItemContainer()->GetItem(req->byDestPlace, req->byDestPos);

	if (pSrcItem == NULL || pSrcItem->GetCount() == 0)
	{
		resultcode = GAME_ITEM_NOT_FOUND;
		goto END;
	}

	if (pSrcItem->IsLocked() || pSrcItem->GetOwner() == NULL)
	{
		resultcode = GAME_ITEM_IS_LOCK;
		goto END;
	}

	if ((!IsInvenContainer(req->bySrcPlace) && !IsGuildContainer(req->byDestPlace)) //only allow to stack items inside inventory<->bank with this packet
		&& (!IsGuildContainer(req->bySrcPlace) && !IsInvenContainer(req->byDestPlace))
		)
	{
		resultcode = GAME_ITEM_STACK_FAIL;
		goto END;
	}


	if (pDestItem == NULL)	//UNSTACK ITEM
	{
		if (pSrcItem->GetTbldat()->byMax_Stack == 1 || req->byStackCount == 0) //is the item even stack-able?
		{
			resultcode = GAME_ITEM_STACK_FAIL;
		}
		else if (pSrcItem->GetCount() <= req->byStackCount) //check if has enough stack
		{
			resultcode = GAME_ITEM_STACK_FAIL;
		}
		else
		{
			pSrcItem->SetLocked(true);

			cPlayer->GetPlayerItemContainer()->AddReservedInventory(req->byDestPlace, req->byDestPos); //reserve dest place & pos

			res->srcItemID = pSrcItem->GetItemID();
			res->dstItemID = INVALID_ITEMID;
			res->hSrcItem = pSrcItem->GetID();
			res->hDstItem = INVALID_HOBJECT;
			res->byStackCount1 = UnsignedSafeDecrease<BYTE>(pSrcItem->GetCount(), req->byStackCount);
			res->byStackCount2 = req->byStackCount;
		}
	}
	else					// STACK ITEM
	{
		if (pDestItem->IsLocked())
		{
			resultcode = GAME_ITEM_IS_LOCK;
		}
		else if (pSrcItem->GetTblidx() != pDestItem->GetTblidx()) //is source and dest item even same?
		{
			resultcode = GAME_ITEM_NOT_SAME;
		}
		else if (pSrcItem->GetTbldat()->byMax_Stack == 1 || pDestItem->GetTbldat()->byMax_Stack == 1) //is item stack-able
		{
			resultcode = GAME_ITEM_STACK_FAIL;
		}
		else if (pSrcItem->GetCount() < req->byStackCount || pSrcItem->GetTbldat()->byMax_Stack == 1 || req->byStackCount == 0) //check: has less than required items? is item possible to stack? is the stack request == 0
		{
			resultcode = GAME_ITEM_STACK_FAIL;
		}
		else if (pDestItem->GetCount() == 0 || pDestItem->GetCount() == pDestItem->GetTbldat()->byMax_Stack) //check if dest item already max stack
		{
			resultcode = GAME_ITEM_STACK_FULL;
		}
		else if (pDestItem->GetRestrictState() != pSrcItem->GetRestrictState())
			resultcode = GAME_ITEM_STACK_FAIL;
		else
		{
			BYTE stackCount1 = pSrcItem->GetCount() - req->byStackCount;
			int stackCount2 = req->byStackCount + pDestItem->GetCount();

			if (stackCount2 > pSrcItem->GetTbldat()->byMax_Stack)
			{
				stackCount1 = stackCount2 - pSrcItem->GetTbldat()->byMax_Stack;
				stackCount2 = (int)pSrcItem->GetTbldat()->byMax_Stack;
			}

			pSrcItem->SetLocked(true);
			pDestItem->SetLocked(true);

			res->srcItemID = pSrcItem->GetItemID();
			res->dstItemID = pDestItem->GetItemID();
			res->hSrcItem = pSrcItem->GetID();
			res->hDstItem = pDestItem->GetID();
			res->byStackCount1 = stackCount1;
			res->byStackCount2 = (BYTE)stackCount2;
		}
	}

	if (resultcode == GAME_SUCCESS)
	{
		packet.SetPacketLen(sizeof(sGQ_GUILD_BANK_MOVE_STACK_REQ));
		g_pApp->SendTo(app->GetQueryServerSession(), &packet);
	}
	else
	{
	END:
		CNtlPacket packetEnd(sizeof(sGU_GUILD_BANK_MOVE_STACK_RES));
		sGU_GUILD_BANK_MOVE_STACK_RES * resEnd = (sGU_GUILD_BANK_MOVE_STACK_RES *)packetEnd.GetPacketData();
		resEnd->wOpCode = GU_GUILD_BANK_MOVE_STACK_RES;
		resEnd->wResultCode = resultcode;
		g_pApp->Send(GetHandle(), &packetEnd);
	}
}


void CClientSession::RecvGuildBankEndReq(CNtlPacket * pPacket)
{
	CGameServer* app = (CGameServer*)g_pApp;

	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	if (cPlayer->IsUsingBank() && app->GetChatServerSession() && cPlayer->GetGuildID() > 0 && cPlayer->GetPlayerItemContainer()->IsUsingGuildBank())
	{
		//send notification to character server

		CNtlPacket packetChat(sizeof(sGT_GUILD_BANK_END_NFY));
		sGT_GUILD_BANK_END_NFY * resChat = (sGT_GUILD_BANK_END_NFY *)packetChat.GetPacketData();
		resChat->wOpCode = GT_GUILD_BANK_END_NFY;
		resChat->charId = cPlayer->GetCharID();
		packetChat.SetPacketLen(sizeof(sGT_GUILD_BANK_END_NFY));
		app->SendTo(app->GetChatServerSession(), &packetChat);
	}
	else
	{
		CNtlPacket packet(sizeof(sGU_BANK_END_RES));
		sGU_BANK_END_RES * res = (sGU_BANK_END_RES *)packet.GetPacketData();
		res->wOpCode = GU_BANK_END_RES;
		res->wResultCode = GAME_FAIL;
		packet.SetPacketLen(sizeof(sGU_BANK_END_RES));
		app->Send(GetHandle(), &packet);
	}
}


void CClientSession::RecvGuildBankZeniReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_GUILD_BANK_ZENNY_REQ * req = (sUG_GUILD_BANK_ZENNY_REQ*)pPacket->GetPacketData();

	WORD wResultCode = GAME_SUCCESS;

	CNpc* pNpc = g_pObjectManager->GetNpc(req->handle);

	if (pNpc == NULL)	//check if NPC exist
		wResultCode = GAME_TARGET_NOT_FOUND;

	else if (cPlayer->IsInRange(pNpc, DBO_DISTANCE_CHECK_TOLERANCE) == false) //check if npc is in range
		wResultCode = GAME_TARGET_TOO_FAR;

	else if (pNpc->GetTbldat()->byJob != NPC_JOB_BANKER)	//check if npc is a banker
		wResultCode = GAME_TARGET_HAS_DIFFERENT_JOB;

	else if (cPlayer->IsUsingBank() == false)	//check if bank is not open
		wResultCode = GAME_FAIL;

	else if (cPlayer->IsBankLoaded() == false)	//bank is loaded before this packet is received
		wResultCode = GAME_FAIL;

	else if (app->GetChatServerSession() == NULL)
		wResultCode = GAME_FAIL;

	else if (cPlayer->GetGuildID() == 0)
		wResultCode = GAME_GUILD_NOT_EXIST;

	else if (cPlayer->GetPlayerItemContainer()->IsUsingGuildBank() == false) //check if is not using guild bank
		wResultCode = GAME_FAIL;

	else if(req->dwZenny == 0 || req->dwZenny == INVALID_DWORD)
		wResultCode = GAME_FAIL;

	else if(req->bIsSave && req->dwZenny > cPlayer->GetZeni())  //´save zeni | check if player has enough zeni
		wResultCode = GAME_FAIL;

	else if(req->bIsSave && (NTL_CHAR_MAX_SAVE_ZENNY < cPlayer->GetPlayerItemContainer()->GetGuildBankZeni() + req->dwZenny || cPlayer->GetPlayerItemContainer()->GetGuildBankZeni() + req->dwZenny == INVALID_DWORD)) //save zeni | check if zeni over
		wResultCode = GAME_ZENNY_OVER;

	else if(req->bIsSave == false && req->dwZenny > cPlayer->GetPlayerItemContainer()->GetGuildBankZeni()) // take zeni | check if enough zeni in bank
		wResultCode = GAME_FAIL;

	else if(req->bIsSave == false && cPlayer->CanReceiveZeni(req->dwZenny) == false) // take zeni | check if zeni over
		wResultCode = GAME_ZENNY_OVER;

	else
	{
		CNtlPacket packetQry(sizeof(sGQ_GUILD_BANK_ZENNY_REQ));
		sGQ_GUILD_BANK_ZENNY_REQ * resQry = (sGQ_GUILD_BANK_ZENNY_REQ *)packetQry.GetPacketData();
		resQry->wOpCode = GQ_GUILD_BANK_ZENNY_REQ;
		resQry->handle = cPlayer->GetID();
		resQry->npchandle = req->handle;
		resQry->guildID = cPlayer->GetGuildID();
		resQry->dwZenny = req->dwZenny;
		resQry->bIsSave = req->bIsSave;
	//	resQry->byType = req->byt
		resQry->charId = cPlayer->GetCharID();
		packetQry.SetPacketLen(sizeof(sGQ_GUILD_BANK_ZENNY_REQ));
		g_pApp->SendTo(app->GetQueryServerSession(), &packetQry);

		return;
	}

	CNtlPacket packet(sizeof(sGU_GUILD_BANK_ZENNY_RES));
	sGU_GUILD_BANK_ZENNY_RES * res = (sGU_GUILD_BANK_ZENNY_RES *)packet.GetPacketData();
	res->wOpCode = GU_GUILD_BANK_ZENNY_RES;
	res->wResultCode = wResultCode;
	res->handle = req->handle;
	packet.SetPacketLen(sizeof(sGU_GUILD_BANK_ZENNY_RES));
	g_pApp->Send(GetHandle(), &packet);
}


//--------------------------------------------------------------------------------------//
//		CREATE GUILD MARK
//--------------------------------------------------------------------------------------//
void CClientSession::RecvGuildCreateMarkReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_GUILD_CREATE_MARK_REQ * req = (sUG_GUILD_CREATE_MARK_REQ*)pPacket->GetPacketData();

	WORD resultcode = GAME_SUCCESS;

	if (cPlayer->GetGuildID() == 0)
		resultcode = GAME_GUILD_NO_GUILD_FOUND;
	else if (cPlayer->GetGuildMark()->IsIntialized())
		resultcode = GAME_GUILD_MARK_ARLEADY_CREATED;
	else
	{
		CNtlPacket cPacket(sizeof(sGT_GUILD_CREATE_MARK_REQ));
		sGT_GUILD_CREATE_MARK_REQ * cRes = (sGT_GUILD_CREATE_MARK_REQ *)cPacket.GetPacketData();
		cRes->wOpCode = GT_GUILD_CREATE_MARK_REQ;
		cRes->charId = cPlayer->GetCharID();
		memcpy(&cRes->sMark, &req->sMark, sizeof(sDBO_GUILD_MARK));
		cPacket.SetPacketLen(sizeof(sGT_GUILD_CREATE_MARK_REQ));
		app->SendTo(app->GetChatServerSession(), &cPacket); //Send to chat server

		return;
	}

	CNtlPacket packet(sizeof(sGU_GUILD_CREATE_MARK_RES));
	sGU_GUILD_CREATE_MARK_RES * res = (sGU_GUILD_CREATE_MARK_RES *)packet.GetPacketData();
	res->wOpCode = GU_GUILD_CREATE_MARK_RES;
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_GUILD_CREATE_MARK_RES));
	app->Send(GetHandle(), &packet);
}
//--------------------------------------------------------------------------------------//
//		UPDATE GUILD MARK
//--------------------------------------------------------------------------------------//
void CClientSession::RecvGuildChangeMarkReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_GUILD_CHANGE_MARK_REQ * req = (sUG_GUILD_CHANGE_MARK_REQ*)pPacket->GetPacketData();

	WORD resultcode = GAME_SUCCESS;

	if (cPlayer->GetZeni() < DBO_ZENNY_FOR_GUILD_CHANGE_MARK)
		resultcode = GAME_GUILD_NEED_MORE_ZENNY;
	else if (cPlayer->GetGuildID() == 0)
		resultcode = GAME_GUILD_NO_GUILD_FOUND;
	else
	{
		CNtlPacket cPacket(sizeof(sGT_GUILD_CHANGE_MARK_REQ));
		sGT_GUILD_CHANGE_MARK_REQ * cRes = (sGT_GUILD_CHANGE_MARK_REQ *)cPacket.GetPacketData();
		cRes->wOpCode = GT_GUILD_CHANGE_MARK_REQ;
		cRes->charId = cPlayer->GetCharID();
		memcpy(&cRes->sMark, &req->sMark, sizeof(sDBO_GUILD_MARK));
		cRes->dwZenny = DBO_ZENNY_FOR_GUILD_CHANGE_MARK;
		cPacket.SetPacketLen(sizeof(sGT_GUILD_CHANGE_MARK_REQ));
		app->SendTo(app->GetChatServerSession(), &cPacket); //Send to chat server

		return;
	}
	
	CNtlPacket packet(sizeof(sGU_GUILD_CHANGE_MARK_RES));
	sGU_GUILD_CHANGE_MARK_RES * res = (sGU_GUILD_CHANGE_MARK_RES *)packet.GetPacketData();
	res->wOpCode = GU_GUILD_CHANGE_MARK_RES;
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_GUILD_CHANGE_MARK_RES));
	app->Send(GetHandle(), &packet);
}

void CClientSession::RecvGuildChangeNameReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_GUILD_CHANGE_NAME_REQ * req = (sUG_GUILD_CHANGE_NAME_REQ*)pPacket->GetPacketData();

	WORD resultcode = GAME_SUCCESS;

	CItem* pItem = cPlayer->GetPlayerItemContainer()->GetItem(req->byPlace, req->byPos);
	if (pItem == NULL || pItem->GetCount() == 0 || IsInvenContainer(pItem->GetPlace()) == false)
		resultcode = GAME_ITEM_NOT_FOUND;
	else if (pItem->IsLocked())
		resultcode = GAME_ITEM_IS_LOCK;
	else if (pItem->GetTbldat()->byItem_Type != ITEM_TYPE_GUILD_NAME_CHANGE)
		resultcode = GAME_ITEM_TYPE_NOT_MISMATCHED;
	else if(cPlayer->GetGuildID() == 0)
		resultcode = GAME_GUILD_NO_GUILD_FOUND;
	else
	{
		std::string guildname = Ntl_WC2MB(req->wszGuildName);

		if (guildname.length() < NTL_MIN_SIZE_GUILD_NAME || guildname.length() > NTL_MAX_SIZE_GUILD_NAME)	//check guild name length
			resultcode = GAME_GUILD_NOT_PROPER_GUILD_NAME_LENGTH;
		//else if (guildname.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890") != std::string::npos)	//check guild name
		//	resultcode = GAME_GUILD_GUILD_NAME_HAS_INVALID_CHARACTER;

		guildname.erase();
	}
	
	if (resultcode == GAME_SUCCESS)
	{
		CNtlPacket cPacket(sizeof(sGT_GUILD_CHANGE_NAME_REQ));
		sGT_GUILD_CHANGE_NAME_REQ * cRes = (sGT_GUILD_CHANGE_NAME_REQ *)cPacket.GetPacketData();
		cRes->wOpCode = GT_GUILD_CHANGE_NAME_REQ;
		cRes->charId = cPlayer->GetCharID();
		NTL_SAFE_WCSCPY(cRes->wszGuildName, req->wszGuildName);
		cRes->byPlace = req->byPlace;
		cRes->byPos = req->byPos;
		cRes->itemId = pItem->GetItemID();
		cPacket.SetPacketLen(sizeof(sGT_GUILD_CHANGE_NAME_REQ));
		app->SendTo(app->GetChatServerSession(), &cPacket); //Send to chat server
	}
	else
	{
		CNtlPacket packet(sizeof(sGU_GUILD_CHANGE_NAME_RES));
		sGU_GUILD_CHANGE_NAME_RES * res = (sGU_GUILD_CHANGE_NAME_RES *)packet.GetPacketData();
		res->wOpCode = GU_GUILD_CHANGE_NAME_RES;
		res->wResultCode = resultcode;
		packet.SetPacketLen(sizeof(sGU_GUILD_CHANGE_NAME_RES));
		app->Send(GetHandle(), &packet);
	}
}


//--------------------------------------------------------------------------------------//
//		Create Party
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCreatePartyReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_PARTY_CREATE_REQ * req = (sUG_PARTY_CREATE_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_PARTY_CREATE_RES));
	sGU_PARTY_CREATE_RES * res = (sGU_PARTY_CREATE_RES *)packet.GetPacketData();
	res->wOpCode = GU_PARTY_CREATE_RES;
	
	char* chname = Ntl_WC2MB(req->wszPartyName);
	std::string charname = chname;

	if (charname.length() < NTL_MIN_SIZE_PARTY_NAME)
		res->wResultCode = CHARACTER_TOO_SHORT_NAME;
	else if (charname.length() > NTL_MAX_SIZE_PARTY_NAME)
		res->wResultCode = CHARACTER_TOO_LONG_NAME;
	//else if (charname.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890") != std::string::npos)
	//	res->wResultCode = CHARACTER_BLOCK_STRING_INCLUDED;
	else if (app->GetGsChannel() == DOJO_CHANNEL_INDEX)
		res->wResultCode = GAME_FAIL;
	else if(cPlayer->GetParty() == NULL)
	{
		memcpy(res->wszPartyName, req->wszPartyName, (NTL_MAX_SIZE_PARTY_NAME + 1) * sizeof(WCHAR));

		CParty * party = g_pPartyManager->CreateParty(cPlayer, req->wszPartyName);
		if (party)
		{
			res->wResultCode = GAME_SUCCESS;
			res->partyID = party->GetPartyID();
		}
		else res->wResultCode = GAME_PARTY_NOT_CREATED_FOR_SOME_REASON;
	}
	else res->wResultCode = GAME_PARTY_ALREADY_IN_PARTY;
	
	packet.SetPacketLen( sizeof(sGU_PARTY_CREATE_RES) );
	g_pApp->Send(GetHandle(), &packet );
}

//--------------------------------------------------------------------------------------//
//		Disband Party
//--------------------------------------------------------------------------------------//
void CClientSession::RecvDisbandPartyReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	WORD wResultCode = GAME_SUCCESS;

	if (cPlayer->GetParty() == NULL)
		wResultCode = GAME_PARTY_YOU_ARE_NOT_IN_PARTY;
	else if (cPlayer->GetParty()->GetPartyLeaderID() != cPlayer->GetID())
		wResultCode = GAME_PARTY_ONLY_ALLOWED_TO_PARTY_LEADER;
	else if (cPlayer->GetParty()->GetRankBattleRoomId() != INVALID_ROOMID)
		wResultCode = GAME_TIMEQUEST_CANNOT_LEAVE_IN_PARTY_WHEN_PLAYING_RANKBATTLE;
	else if (cPlayer->GetTMQ() || cPlayer->GetCCBD())
		wResultCode = GAME_FAIL;
	else
		g_pPartyManager->DisbandParty(cPlayer->GetParty());

	CNtlPacket packet(sizeof(sGU_PARTY_DISBAND_RES));
	sGU_PARTY_DISBAND_RES * res = (sGU_PARTY_DISBAND_RES *)packet.GetPacketData();
	res->wOpCode = GU_PARTY_DISBAND_RES;
	res->wResultCode = wResultCode;
	packet.SetPacketLen(sizeof(sGU_PARTY_DISBAND_RES));
	g_pApp->Send(GetHandle(), &packet);
}

//--------------------------------------------------------------------------------------//
//		Send party invite request
//--------------------------------------------------------------------------------------//
void CClientSession::RecvPartyInviteReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PARTY_INVITE_REQ * req = (sUG_PARTY_INVITE_REQ*)pPacket->GetPacketData();

	WORD resultcode = GAME_FAIL;
	CPlayer* target = NULL;

	//Invite player
	CNtlPacket packet(sizeof(sGU_PARTY_INVITE_RES));
	sGU_PARTY_INVITE_RES * res = (sGU_PARTY_INVITE_RES *)packet.GetPacketData();
	res->wOpCode = GU_PARTY_INVITE_RES;
	res->partyID = cPlayer->GetPartyID();

	if (cPlayer->GetTMQ() == NULL && cPlayer->GetTLQ() == NULL)// && cPlayer->GetCCBD() == NULL) ccreconnect
	{
		target = g_pObjectManager->GetPC(req->hTarget);
		if (target)
		{
			resultcode = target->CanJoinParty();
			if (resultcode == GAME_SUCCESS)
			{
				if (cPlayer->GetParty() != NULL)
				{
					if (cPlayer->GetParty()->GetPartyLeaderID() != cPlayer->GetID())
						resultcode = GAME_PARTY_ONLY_ALLOWED_TO_PARTY_LEADER;
					else
					{
						resultcode = GAME_SUCCESS;
						NTL_WCSCPY_S(res->wszTargetName, NTL_MAX_SIZE_CHAR_NAME + 1, target->GetCharName());
						target->SetIsPartyInvite(true);
						target->SetPartyInviteID(res->partyID);
						target->SetPartyInvitedByID(cPlayer->GetCharID());
					}
				}
				else
				{
					resultcode = GAME_SUCCESS;
					NTL_WCSCPY_S(res->wszTargetName, NTL_MAX_SIZE_CHAR_NAME + 1, target->GetCharName());
					target->SetIsPartyInvite(true);
					target->SetPartyInviteID(res->partyID);
					target->SetPartyInvitedByID(cPlayer->GetCharID());
				}
			}
		}
		else resultcode = GAME_PARTY_NO_SUCH_A_PLAYER;
	}
	else resultcode = GAME_PARTYMATCHING_ANY_MEMBER_IN_DYNAMIC_WORLD;

	res->wResultCode = resultcode;
	packet.SetPacketLen( sizeof(sGU_PARTY_INVITE_RES) );
	g_pApp->Send(GetHandle(), &packet );

	if(resultcode == GAME_SUCCESS)
	{
		CNtlPacket packet2(sizeof(sGU_PARTY_INVITE_NFY));
		sGU_PARTY_INVITE_NFY * res2 = (sGU_PARTY_INVITE_NFY *)packet2.GetPacketData();
		res2->wOpCode = GU_PARTY_INVITE_NFY;
		NTL_WCSCPY_S(res2->wszInvitorPcName, NTL_MAX_SIZE_CHAR_NAME + 1, cPlayer->GetCharName() );
		packet2.SetPacketLen( sizeof(sGU_PARTY_INVITE_NFY));
		g_pApp->Send( target->GetClientSessionID(), &packet2 );
	}
}
//--------------------------------------------------------------------------------------//
//		Send party invite request (char id)
//--------------------------------------------------------------------------------------//
void CClientSession::RecvPartyInviteCharIdReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PARTY_INVITE_CHARID_REQ * req = (sUG_PARTY_INVITE_CHARID_REQ*)pPacket->GetPacketData();
	
	WORD resultcode = GAME_FAIL;
	CPlayer* target = NULL;

	//Invite player
	CNtlPacket packet(sizeof(sGU_PARTY_INVITE_RES));
	sGU_PARTY_INVITE_RES * res = (sGU_PARTY_INVITE_RES *)packet.GetPacketData();
	res->wOpCode = GU_PARTY_INVITE_RES;
	res->partyID = cPlayer->GetPartyID();

	if (cPlayer->GetTMQ() == NULL && cPlayer->GetTLQ() == NULL)// && cPlayer->GetCCBD() == NULL) ccreconnect
	{
		target = g_pObjectManager->FindByChar(req->targetCharId);
		if(target)
		{
			resultcode = target->CanJoinParty();
			if(resultcode == GAME_SUCCESS)
			{
				if(cPlayer->GetParty() != NULL)
				{
					if(cPlayer->GetParty()->GetPartyLeaderID() != cPlayer->GetID() )
						resultcode = GAME_PARTY_ONLY_ALLOWED_TO_PARTY_LEADER;
					else
					{
						resultcode = GAME_SUCCESS;
						NTL_WCSCPY_S(res->wszTargetName, NTL_MAX_SIZE_CHAR_NAME + 1, target->GetCharName());
						target->SetIsPartyInvite(true);	
						target->SetPartyInviteID(res->partyID);	
						target->SetPartyInvitedByID(cPlayer->GetCharID());
					}
				}
				else
				{
					resultcode = GAME_SUCCESS;
					NTL_WCSCPY_S(res->wszTargetName, NTL_MAX_SIZE_CHAR_NAME + 1, target->GetCharName() );
					target->SetIsPartyInvite(true);	
					target->SetPartyInviteID(res->partyID);	
					target->SetPartyInvitedByID(cPlayer->GetCharID());
				}
			}
		}
		else resultcode = GAME_PARTY_NO_SUCH_A_PLAYER;
	}
	else resultcode = GAME_PARTYMATCHING_ANY_MEMBER_IN_DYNAMIC_WORLD;

	res->wResultCode = resultcode;
	packet.SetPacketLen( sizeof(sGU_PARTY_INVITE_RES) );
	g_pApp->Send(GetHandle(), &packet );

	if(resultcode == GAME_SUCCESS)
	{
		CNtlPacket packet2(sizeof(sGU_PARTY_INVITE_NFY));
		sGU_PARTY_INVITE_NFY * res2 = (sGU_PARTY_INVITE_NFY *)packet2.GetPacketData();
		res2->wOpCode = GU_PARTY_INVITE_NFY;
		NTL_WCSCPY_S(res2->wszInvitorPcName, NTL_MAX_SIZE_CHAR_NAME + 1, cPlayer->GetCharName());
		packet2.SetPacketLen( sizeof(sGU_PARTY_INVITE_NFY));
		g_pApp->Send(target->GetClientSessionID(), &packet2 );
	}
}
//--------------------------------------------------------------------------------------//
//		Send party invite request (char name)
//--------------------------------------------------------------------------------------//
void CClientSession::RecvPartyInviteCharNameReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PARTY_INVITE_CHAR_NAME_REQ * req = (sUG_PARTY_INVITE_CHAR_NAME_REQ*)pPacket->GetPacketData();
	
	WORD resultcode = GAME_FAIL;
	CPlayer* target = NULL;
	
	//Invite player
	CNtlPacket packet(sizeof(sGU_PARTY_INVITE_RES));
	sGU_PARTY_INVITE_RES * res = (sGU_PARTY_INVITE_RES *)packet.GetPacketData();
	res->wOpCode = GU_PARTY_INVITE_RES;
	res->partyID = cPlayer->GetPartyID();


	char* chname = Ntl_WC2MB(req->wszTargetName);
	std::string charname = chname;

	if (charname.length() < NTL_MIN_SIZE_CHAR_NAME)
		resultcode = CHARACTER_TOO_SHORT_NAME;
	else if (charname.length() > NTL_MAX_SIZE_CHAR_NAME)
		resultcode = CHARACTER_TOO_LONG_NAME;
	//else if (charname.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890") != std::string::npos)
	//	resultcode = CHARACTER_BLOCK_STRING_INCLUDED;

	else if (cPlayer->GetTMQ() == NULL && cPlayer->GetTLQ() == NULL)// && cPlayer->GetCCBD() == NULL) ccreconnect
	{
		target = g_pObjectManager->FindByName(req->wszTargetName);
		if(target)
		{
			resultcode = target->CanJoinParty();
			if(resultcode == GAME_SUCCESS)
			{
				if(cPlayer->GetParty() != NULL)
				{
					if(cPlayer->GetParty()->GetPartyLeaderID() != cPlayer->GetID() )
						resultcode = GAME_PARTY_ONLY_ALLOWED_TO_PARTY_LEADER;
					else
					{
						resultcode = GAME_SUCCESS;
						NTL_WCSCPY_S(res->wszTargetName, NTL_MAX_SIZE_CHAR_NAME + 1, target->GetCharName() );
						target->SetIsPartyInvite(true);	
						target->SetPartyInviteID(res->partyID);	
						target->SetPartyInvitedByID(cPlayer->GetCharID());
					}
				}
				else
				{
					resultcode = GAME_SUCCESS;
					NTL_WCSCPY_S(res->wszTargetName, NTL_MAX_SIZE_CHAR_NAME + 1, target->GetCharName());
					target->SetIsPartyInvite(true);	
					target->SetPartyInviteID(res->partyID);	
					target->SetPartyInvitedByID(cPlayer->GetCharID());
				}
			}
		}
		else resultcode = GAME_PARTY_NO_SUCH_A_PLAYER;
	}
	else resultcode = GAME_PARTYMATCHING_ANY_MEMBER_IN_DYNAMIC_WORLD;

	res->wResultCode = resultcode;
	packet.SetPacketLen( sizeof(sGU_PARTY_INVITE_RES) );
	g_pApp->Send(GetHandle(), &packet );

	Ntl_CleanUpHeapString(chname);

	if(resultcode == GAME_SUCCESS)
	{
		CNtlPacket packet2(sizeof(sGU_PARTY_INVITE_NFY));
		sGU_PARTY_INVITE_NFY * res2 = (sGU_PARTY_INVITE_NFY *)packet2.GetPacketData();
		res2->wOpCode = GU_PARTY_INVITE_NFY;
		NTL_WCSCPY_S(res2->wszInvitorPcName, NTL_MAX_SIZE_CHAR_NAME + 1, cPlayer->GetCharName());
		packet2.SetPacketLen( sizeof(sGU_PARTY_INVITE_NFY));
		g_pApp->Send( target->GetClientSessionID(), &packet2 );
	}
}
//--------------------------------------------------------------------------------------//
//		Party invitation response
//--------------------------------------------------------------------------------------//
void CClientSession::RecvPartyResponse(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PARTY_RESPONSE_INVITATION * req = (sUG_PARTY_RESPONSE_INVITATION*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_PARTY_RESPONSE_INVITATION_RES));
	sGU_PARTY_RESPONSE_INVITATION_RES * res = (sGU_PARTY_RESPONSE_INVITATION_RES *)packet.GetPacketData();
	res->wOpCode = GU_PARTY_RESPONSE_INVITATION_RES;
	res->wResultCode = GAME_FAIL;

	if (cPlayer->GetIsPartyInvite())
	{
		if (cPlayer->GetTMQ() == NULL && cPlayer->GetTLQ() == NULL)// && cPlayer->GetCCBD() == NULL) ccreconnect
		{
			CPlayer* invitor = g_pObjectManager->FindByChar(cPlayer->GetPartyInvitedByID());
			if (invitor && invitor->IsInitialized())
			{
				if (req->byResponse == NTL_INVITATION_RESPONSE_ACCEPT)
				{
					if (invitor->GetTMQ() == NULL && invitor->GetTLQ() == NULL)// && invitor->GetCCBD() == NULL) ccreconnect
					{
						if (cPlayer->GetParty() == NULL && cPlayer->GetPartyID() == INVALID_PARTYID)
						{
							//check if a party already exist. if not then create one and make invitor to leader
							if (invitor->GetParty() == NULL && cPlayer->GetPartyID() == INVALID_PARTYID)
							{
								CParty * party = g_pPartyManager->CreateParty(invitor, const_cast<WCHAR*>(GetUnnamedPartyName()), true);
								if (party)
								{
									if (party->AddPartyMember(cPlayer) == true)
										res->wResultCode = GAME_SUCCESS;
									else
										res->wResultCode = GAME_PARTY_NO_ROOM_FOR_NEW_MEMBER;
								}
								else res->wResultCode = GAME_PARTY_NOT_CREATED_FOR_SOME_REASON;
							}
							else
							{
								CParty * party = g_pPartyManager->GetParty(invitor->GetPartyID());
								if (party)
								{
									if (party->GetPartyLeaderID() == invitor->GetID())
									{
										if (party->GetPlayer(0) && party->GetPlayer(0)->GetTMQ()) //dont allow new members to join while in tmq
											res->wResultCode = GAME_FAIL;
										else
										{
											if (party->AddPartyMember(cPlayer) == true)
											{
												res->wResultCode = GAME_SUCCESS;
												if (invitor->GetCCBD()) //ccreconnect
												{
													if (cPlayer->GetCCBDRejoin() == true)
													{
														CBattleDungeon* pDungeon = g_pDungeonManager->FindBattleDungeon(invitor->GetPartyID());
														pDungeon->AddPlayer(cPlayer->GetID());
														pDungeon->JoinDungeon(cPlayer);
														cPlayer->GetParty()->ReEnterBattleDungeon(invitor, cPlayer, pDungeon, invitor->GetCurWorld(), invitor->GetScript(83000));
														res->wResultCode = GAME_SUCCESS;
													}
													else res->wResultCode = GAME_PARTY_COULDNT_JOIN_FOR_SOME_REASON;
												}
											}
											else res->wResultCode = GAME_PARTY_NO_ROOM_FOR_NEW_MEMBER;
										}
									}
									else res->wResultCode = GAME_FAIL;
								}
								else res->wResultCode = GAME_PARTY_NO_SUCH_A_PARTY;
							}

						}
						else res->wResultCode = GAME_PARTY_ALREADY_IN_PARTY;
					}
					else res->wResultCode = GAME_PARTYMATCHING_ANY_MEMBER_IN_DYNAMIC_WORLD;
				}
				else if (req->byResponse == NTL_INVITATION_RESPONSE_DECLINE)
				{
					CNtlPacket packet2(sizeof(sGU_PARTY_INVITATION_DECLINED_NFY));
					sGU_PARTY_INVITATION_DECLINED_NFY * res2 = (sGU_PARTY_INVITATION_DECLINED_NFY *)packet2.GetPacketData();
					res2->wOpCode = GU_PARTY_INVITATION_DECLINED_NFY;
					NTL_WCSCPY_S(res2->wszPlayerName, NTL_MAX_SIZE_CHAR_NAME + 1, cPlayer->GetCharName());
					packet2.SetPacketLen(sizeof(sGU_PARTY_INVITATION_DECLINED_NFY));
					g_pApp->Send(invitor->GetClientSessionID(), &packet2);

					res->wResultCode = GAME_SUCCESS;
				}
				else // req->byResponse == NTL_INVITATION_RESPONSE_EXPIRE
				{
					CNtlPacket packet2(sizeof(sGU_PARTY_INVITATION_EXPIRED_NFY));
					sGU_PARTY_INVITATION_EXPIRED_NFY * res2 = (sGU_PARTY_INVITATION_EXPIRED_NFY *)packet2.GetPacketData();
					res2->wOpCode = GU_PARTY_INVITATION_EXPIRED_NFY;
					NTL_WCSCPY_S(res2->wszPlayerName, NTL_MAX_SIZE_CHAR_NAME + 1, cPlayer->GetCharName());
					packet2.SetPacketLen(sizeof(sGU_PARTY_INVITATION_EXPIRED_NFY));
					g_pApp->Send(invitor->GetClientSessionID(), &packet2);

					res->wResultCode = GAME_SUCCESS;
				}
			}
			else res->wResultCode = GAME_PARTY_COULDNT_JOIN_FOR_SOME_REASON;
		}
		else res->wResultCode = GAME_PARTYMATCHING_ANY_MEMBER_IN_DYNAMIC_WORLD;
	}

	cPlayer->SetIsPartyInvite(false);
	cPlayer->SetPartyInviteID(INVALID_PARTYID);

	packet.SetPacketLen( sizeof(sGU_PARTY_RESPONSE_INVITATION_RES) );
	g_pApp->Send( GetHandle(), &packet );
}
//--------------------------------------------------------------------------------------//
//		Leave Party
//--------------------------------------------------------------------------------------//
void CClientSession::RecvPartyLeaveReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	WORD resultcode = GAME_SUCCESS;

	if (cPlayer->GetParty())
	{
		if (cPlayer->HasEventType(EVENT_TELEPORT_PROPOSAL))
			resultcode = GAME_PARTY_LEAVING_IS_NOT_ALLOWED;
		else if(cPlayer->GetCurWorld() == NULL || cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_RANKBATTLE)
			resultcode = GAME_TIMEQUEST_CANNOT_LEAVE_IN_PARTY_WHEN_PLAYING_RANKBATTLE;
		else if (cPlayer->GetTMQ())
			resultcode = GAME_FAIL;
		else
		{
			std::vector<TBLIDX> m_buffs{ 850076, 850077, 850078 };

			for (TBLIDX i : m_buffs)
			{
				if (CBuff* buff = cPlayer->GetBuffManager()->FindBuff(i, DBO_OBJECT_SOURCE_ITEM))
					cPlayer->GetBuffManager()->RemoveBuff(buff->GetBuffIndex(), buff->GetBuffType(), CBuff::BUFF_REMOVAL_REASON_BY_ITSELF);
			}

			cPlayer->GetParty()->LeaveParty(cPlayer);
		}
	}
	else resultcode = GAME_COMMON_YOU_ARE_NOT_IN_A_PARTY;

	CNtlPacket packet(sizeof(sGU_PARTY_LEAVE_RES));
	sGU_PARTY_LEAVE_RES * res = (sGU_PARTY_LEAVE_RES *)packet.GetPacketData();
	res->wOpCode = GU_PARTY_LEAVE_RES;
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_PARTY_DISBAND_RES));
	g_pApp->Send(GetHandle(), &packet);
}
//--------------------------------------------------------------------------------------//
//		UPDATE PARTY LEADER
//--------------------------------------------------------------------------------------//
void CClientSession::RecvChangePartyLeaderReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PARTY_CHANGE_LEADER_REQ * req = (sUG_PARTY_CHANGE_LEADER_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_PARTY_CHANGE_LEADER_RES));
	sGU_PARTY_CHANGE_LEADER_RES * res = (sGU_PARTY_CHANGE_LEADER_RES *)packet.GetPacketData();
	res->wOpCode = GU_PARTY_CHANGE_LEADER_RES;
	res->wResultCode = GAME_SUCCESS;
	res->hNewLeader = req->hTargetMember;
	packet.SetPacketLen( sizeof(sGU_PARTY_CHANGE_LEADER_RES) );
	g_pApp->Send(GetHandle(), &packet );

	if(cPlayer->GetParty())
		cPlayer->GetParty()->UpdatePartyLeader(req->hTargetMember);
}
//--------------------------------------------------------------------------------------//
//		KICK PARTY MEMBER
//--------------------------------------------------------------------------------------//
void CClientSession::RecvKickPartyMemberReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PARTY_KICK_OUT_REQ * req = (sUG_PARTY_KICK_OUT_REQ*)pPacket->GetPacketData();

	WORD resultcode = GAME_SUCCESS;

	CPlayer* target = g_pObjectManager->GetPC(req->hTargetMember);
	if (target)
	{
		if (target->GetParty() && cPlayer->GetParty())
		{
			if (target->HasEventType(EVENT_TELEPORT_PROPOSAL))
				resultcode = GAME_PARTY_KICKING_OUT_IS_NOT_ALLOWED;
			else if (cPlayer->GetParty()->GetPartyLeaderID() != cPlayer->GetID())
				resultcode = GAME_COMMON_YOU_ARE_NOT_A_PARTY_LEADER;
			else if (cPlayer->GetRankBattleRoomId() != INVALID_ROOMID && cPlayer->GetRankBattleRoomId() == target->GetRankBattleRoomId())
				resultcode = GAME_TIMEQUEST_CANNOT_LEAVE_IN_PARTY_WHEN_PLAYING_RANKBATTLE;
			else if (cPlayer->GetTMQ())
				resultcode = GAME_FAIL;
		}
		else resultcode = GAME_COMMON_YOU_ARE_NOT_IN_A_PARTY;
	}
	else resultcode = GAME_PARTY_NO_SUCH_A_PLAYER_IN_THE_PARTY;

	CNtlPacket packet(sizeof(sGU_PARTY_KICK_OUT_RES));
	sGU_PARTY_KICK_OUT_RES * res = (sGU_PARTY_KICK_OUT_RES *)packet.GetPacketData();
	res->wOpCode = GU_PARTY_KICK_OUT_RES;
	res->wResultCode = resultcode;
	res->hTargetMember = req->hTargetMember;
	packet.SetPacketLen( sizeof(sGU_PARTY_KICK_OUT_RES) );
	g_pApp->Send(GetHandle(), &packet );

	if(resultcode == GAME_SUCCESS)
		cPlayer->GetParty()->KickPartyMember(target);
}
//--------------------------------------------------------------------------------------//
//		SHARE PARTY TARGET
//--------------------------------------------------------------------------------------//
void CClientSession::RecvPartyShareTargetReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PARTY_SHARETARGET_REQ * req = (sUG_PARTY_SHARETARGET_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_PARTY_SHARETARGET_RES));
	sGU_PARTY_SHARETARGET_RES * res = (sGU_PARTY_SHARETARGET_RES *)packet.GetPacketData();
	res->wOpCode = GU_PARTY_SHARETARGET_RES;
	res->wResultCode = GAME_SUCCESS;

	if (cPlayer->GetParty())
	{
		CCharacter* pChar = g_pObjectManager->GetChar(req->hTarget);
		if (pChar && pChar->IsInitialized() && !pChar->IsFainting())
		{
			cPlayer->GetParty()->ShareTarget(pChar, req->bySlot);
		}
		else res->wResultCode = GAME_TARGET_NOT_FOUND;
	}
	else res->wResultCode = GAME_PARTY_YOU_ARE_NOT_IN_PARTY;

	packet.SetPacketLen( sizeof(sGU_PARTY_SHARETARGET_RES) );
	g_pApp->Send(GetHandle(), &packet );

}
//--------------------------------------------------------------------------------------//
//		CHANGE PARTY ITEM LOOTING METHOD
//--------------------------------------------------------------------------------------//
void CClientSession::RecvPartyChangeItemLootMethodReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PARTY_CHANGE_ITEM_LOOTING_METHOD_REQ* req = (sUG_PARTY_CHANGE_ITEM_LOOTING_METHOD_REQ*)pPacket->GetPacketData();
	
	WORD wRes = GAME_SUCCESS;

	if (cPlayer->GetParty() == NULL)
		wRes = GAME_COMMON_YOU_ARE_NOT_IN_A_PARTY;
	else if(cPlayer->GetParty()->GetPartyLeaderID() != cPlayer->GetID())
		wRes = GAME_COMMON_YOU_ARE_NOT_A_PARTY_LEADER;
	else if(req->byLootingMethod > NTL_PARTY_ITEM_LOOTING_LAST)
		wRes = GAME_PARTY_UNKNOWN_ITEM_LOOTING_METHOD;
	else
		cPlayer->GetParty()->SetItemLootingMethod(req->byLootingMethod);

	CNtlPacket packet(sizeof(sGU_PARTY_CHANGE_ITEM_LOOTING_METHOD_RES));
	sGU_PARTY_CHANGE_ITEM_LOOTING_METHOD_RES* res = (sGU_PARTY_CHANGE_ITEM_LOOTING_METHOD_RES*)packet.GetPacketData();
	res->wOpCode = GU_PARTY_CHANGE_ITEM_LOOTING_METHOD_RES;
	res->wResultCode = wRes;
	res->byNewLootingMethod = req->byLootingMethod;
	packet.SetPacketLen(sizeof(sGU_PARTY_CHANGE_ITEM_LOOTING_METHOD_RES));
	g_pApp->Send(GetHandle(), &packet);
}
//--------------------------------------------------------------------------------------//
//		CHANGE PARTY ZENI LOOTING METHOD
//--------------------------------------------------------------------------------------//
void CClientSession::RecvPartyChangeZeniLootMethodReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PARTY_CHANGE_ZENNY_LOOTING_METHOD_REQ* req = (sUG_PARTY_CHANGE_ZENNY_LOOTING_METHOD_REQ*)pPacket->GetPacketData();

	WORD wRes = GAME_SUCCESS;

	if (cPlayer->GetParty() == NULL)
		wRes = GAME_COMMON_YOU_ARE_NOT_IN_A_PARTY;
	else if (cPlayer->GetParty()->GetPartyLeaderID() != cPlayer->GetID())
		wRes = GAME_COMMON_YOU_ARE_NOT_A_PARTY_LEADER;
	else if (req->byLootingMethod > NTL_PARTY_ZENNY_LOOTING_LAST)
		wRes = GAME_PARTY_UNKNOWN_ZENNY_LOOTING_METHOD;
	else
		cPlayer->GetParty()->SetZeniLootingMethod(req->byLootingMethod);

	CNtlPacket packet(sizeof(sGU_PARTY_CHANGE_ZENNY_LOOTING_METHOD_RES));
	sGU_PARTY_CHANGE_ZENNY_LOOTING_METHOD_RES* res = (sGU_PARTY_CHANGE_ZENNY_LOOTING_METHOD_RES*)packet.GetPacketData();
	res->wOpCode = GU_PARTY_CHANGE_ZENNY_LOOTING_METHOD_RES;
	res->wResultCode = wRes;
	res->byNewLootingMethod = req->byLootingMethod;
	packet.SetPacketLen(sizeof(sGU_PARTY_CHANGE_ZENNY_LOOTING_METHOD_RES));
	g_pApp->Send(GetHandle(), &packet);
}
//--------------------------------------------------------------------------------------//
//		CHANGE PARTY ITEM RANK LOOTING
//--------------------------------------------------------------------------------------//
void CClientSession::RecvPartyChangeItemLottingankMethodReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PARTY_CHANGE_ITEM_LOOTING_RANK_METHOD_REQ* req = (sUG_PARTY_CHANGE_ITEM_LOOTING_RANK_METHOD_REQ*)pPacket->GetPacketData();

	WORD wRes = GAME_SUCCESS;

	if (cPlayer->GetParty() == NULL)
		wRes = GAME_COMMON_YOU_ARE_NOT_IN_A_PARTY;
	else if (cPlayer->GetParty()->GetPartyLeaderID() != cPlayer->GetID())
		wRes = GAME_COMMON_YOU_ARE_NOT_A_PARTY_LEADER;
	else if (req->byRank > ITEM_RANK_LAST)
		wRes = GAME_FAIL;
	else
		cPlayer->GetParty()->SetItemLootingMethodRank(req->byRank);

	CNtlPacket packet(sizeof(sGU_PARTY_CHANGE_ITEM_LOOTING_RANK_METHOD_RES));
	sGU_PARTY_CHANGE_ITEM_LOOTING_RANK_METHOD_RES* res = (sGU_PARTY_CHANGE_ITEM_LOOTING_RANK_METHOD_RES*)packet.GetPacketData();
	res->wOpCode = GU_PARTY_CHANGE_ITEM_LOOTING_RANK_METHOD_RES;
	res->wResultCode = wRes;
	res->byRank = req->byRank;
	packet.SetPacketLen(sizeof(sGU_PARTY_CHANGE_ITEM_LOOTING_RANK_METHOD_RES));
	g_pApp->Send(GetHandle(), &packet);
}


void CClientSession::RecvItemDiceReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_ITEM_DICE_REQ* req = (sUG_ITEM_DICE_REQ*)pPacket->GetPacketData();

	WORD wRes = GAME_SUCCESS;

	if (cPlayer->GetParty() == NULL)
		wRes = GAME_COMMON_YOU_ARE_NOT_IN_A_PARTY;
	else if(req->byDice < DBO_ITEM_DICE_DICE || req->byDice > DBO_ITEM_DICE_CANCELED)
		wRes = GAME_ITEM_DICE_FAIL_INVALID_DICE;
	else if(cPlayer->HasItemDice() == false)
		wRes = GAME_ITEM_DICE_FAIL_NOT_FIND;
	else
	{
		if (req->byDice != DBO_ITEM_DICE_CANCELED && cPlayer->GetPlayerItemContainer()->CountEmptyInventory() == 0)
		{
			req->byDice = DBO_ITEM_DICE_CANCELED;
			wRes = GAME_ITEM_INVEN_FULL;
		}

		if (req->byDice == DBO_ITEM_DICE_CANCELED)
		{
			cPlayer->GetParty()->LeaveDice(cPlayer->GetID());
		}
		else
		{
			cPlayer->GetParty()->RollDice(cPlayer->GetID(), req->byDice);
		}
	}

	CNtlPacket packet(sizeof(sGU_ITEM_DICE_RES));
	sGU_ITEM_DICE_RES* res = (sGU_ITEM_DICE_RES*)packet.GetPacketData();
	res->wOpCode = GU_ITEM_DICE_RES;
	res->wResultCode = wRes;
	res->byDice = req->byDice;
	res->hItemHandle = req->hItemHandle;
	packet.SetPacketLen(sizeof(sGU_ITEM_DICE_RES));
	g_pApp->Send(GetHandle(), &packet);
}


//--------------------------------------------------------------------------------------//
//		INIT PARTY DUNGEON
//--------------------------------------------------------------------------------------//
void CClientSession::RecvInitPartyDungeonReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	WORD wRes = GAME_SUCCESS;

	if (cPlayer->GetParty() == NULL)
		wRes = GAME_COMMON_YOU_ARE_NOT_IN_A_PARTY;
	else if (cPlayer->GetParty()->GetPartyLeaderID() != cPlayer->GetID())
		wRes = GAME_COMMON_YOU_ARE_NOT_A_PARTY_LEADER;
	else
		cPlayer->GetParty()->InitDungeon();

	CNtlPacket packet(sizeof(sGU_PARTY_DUNGEON_INIT_RES));
	sGU_PARTY_DUNGEON_INIT_RES* res = (sGU_PARTY_DUNGEON_INIT_RES*)packet.GetPacketData();
	res->wOpCode = GU_PARTY_DUNGEON_INIT_RES;
	res->wResultCode = wRes;
	packet.SetPacketLen(sizeof(sGU_PARTY_DUNGEON_INIT_RES));
	g_pApp->Send(GetHandle(), &packet);
}
//--------------------------------------------------------------------------------------//
//		UPDATE PARTY DUNGEON DIFFICULTY
//--------------------------------------------------------------------------------------//
void CClientSession::RecvPartyChangeDiffReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PARTY_DIFF_CHANGE_REQ* req = (sUG_PARTY_DIFF_CHANGE_REQ*)pPacket->GetPacketData();

	WORD wRes = GAME_SUCCESS;

	if (cPlayer->GetParty() == NULL)
		wRes = GAME_COMMON_YOU_ARE_NOT_IN_A_PARTY;
	else if (cPlayer->GetParty()->GetPartyLeaderID() != cPlayer->GetID())
		wRes = GAME_COMMON_YOU_ARE_NOT_A_PARTY_LEADER;
	else if(req->eDiff > PARTY_DUNGEON_STATE_LAST)
		wRes = GAME_FAIL;
	else
		cPlayer->GetParty()->SetDungeonDiff(req->eDiff);
	
	CNtlPacket packet(sizeof(sGU_PARTY_DUNGEON_DIFF_RES));
	sGU_PARTY_DUNGEON_DIFF_RES* res = (sGU_PARTY_DUNGEON_DIFF_RES*)packet.GetPacketData();
	res->wOpCode = GU_PARTY_DUNGEON_DIFF_RES;
	res->wResultCode = wRes;
	res->eDiff = req->eDiff;
	packet.SetPacketLen(sizeof(sGU_PARTY_DUNGEON_DIFF_RES));
	g_pApp->Send(GetHandle(), &packet);
}
//--------------------------------------------------------------------------------------//

//--------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------//
//		START PARTY MATCHING

void CClientSession::RecPartyMatchingRegisterReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PARTY_MATCHING_REGISTER_REQ* req = (sUG_PARTY_MATCHING_REGISTER_REQ*)pPacket->GetPacketData();

	g_pPartyMatching->Register(cPlayer, req->byDifficulty, req->byDungeonType, req->byRegionType, req->hItem, req->rankBattleWorldTblidx);
}

void CClientSession::RecPartyMatchingJoinReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PARTY_MATCHING_JOIN_REQ* req = (sUG_PARTY_MATCHING_JOIN_REQ*)pPacket->GetPacketData();

	g_pPartyMatching->Join(cPlayer, req->partyId, req->byDungeonType, req->byRegionType);
}

void CClientSession::RecPartyMatchingRoleplayReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PARTY_MATCHING_ROLEPLAY_REQ* req = (sUG_PARTY_MATCHING_ROLEPLAY_REQ*)pPacket->GetPacketData();

	g_pPartyMatching->role(cPlayer, req->partyId, req->byType, req->byRoleplay);
}

void CClientSession::RecPartyMatchingUnregisterReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PARTY_MATCHING_UNREGISTER_REQ* req = (sUG_PARTY_MATCHING_UNREGISTER_REQ*)pPacket->GetPacketData();

	g_pPartyMatching->Unregister(cPlayer);
}

void CClientSession::RecPartyMatchingListReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PARTY_MATCHING_LIST_REQ* req = (sUG_PARTY_MATCHING_LIST_REQ*)pPacket->GetPacketData();

	g_pPartyMatching->LoadPartyMatchingList(cPlayer, req->byPageNumber, req->bySelectDungeonType, req->bySelectRegion);
}

void CClientSession::RecPartyMatchingInfoReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PARTY_MATCHING_INFO_REQ* req = (sUG_PARTY_MATCHING_INFO_REQ*)pPacket->GetPacketData();

	g_pPartyMatching->GetPartyInfo(cPlayer, req->partyId, req->byDungeonType, req->byRegionType);
}

void CClientSession::RecPartyMatchingEnterDungeonReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PARTY_MATCHING_ENTER_DUNGEON_REQ* req = (sUG_PARTY_MATCHING_ENTER_DUNGEON_REQ*)pPacket->GetPacketData();

	g_pPartyMatching->EnterReq(cPlayer);
}

void CClientSession::RecPartyMatchingEnterDungeonAgreeNfy(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PARTY_MATCHING_ENTER_DUNGEON_AGREE_NFY* req = (sUG_PARTY_MATCHING_ENTER_DUNGEON_AGREE_NFY*)pPacket->GetPacketData();

	g_pPartyMatching->EnterRes(cPlayer);
}

//		END PARTY MATCHING
//--------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------//

//--------------------------------------------------------------------------------------//


//--------------------------------------------------------------------------------------//
//		Character bind to world
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharBindReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_BIND_REQ * req = (sUG_CHAR_BIND_REQ*)pPacket->GetPacketData();

	WORD wResultCode = GAME_SUCCESS;
	CTriggerObject* pPopoObject = NULL;

	if(cPlayer->GetBindObjectTblidx() == req->bindObjectTblidx)
		wResultCode = GAME_FAIL;
	else if (cPlayer->GetCurWorld() == NULL)
		wResultCode = GAME_WORLD_NOT_FOUND;
	else
	{
		pPopoObject = cPlayer->GetCurWorld()->FindStaticObjectByIdx(req->bindObjectTblidx);

		if (pPopoObject == NULL)
			wResultCode = GAME_TARGET_NOT_FOUND;
		else if (!cPlayer->IsInRange(pPopoObject->GetCurLoc(), DBO_DISTANCE_CHECK_TOLERANCE))
			wResultCode = GAME_TARGET_TOO_FAR;
		else if(BIT_FLAG_TEST(pPopoObject->GetFunc(), eDBO_TRIGGER_OBJECT_FUNC_BIND) == false)
			wResultCode = GAME_TARGET_HAS_NOT_FUNCTION;

	}

	CGameServer* app = (CGameServer*)g_pApp;

	CNtlPacket packet(sizeof(sGU_CHAR_BIND_RES));
	sGU_CHAR_BIND_RES * res = (sGU_CHAR_BIND_RES *)packet.GetPacketData();
	res->wOpCode = GU_CHAR_BIND_RES;
	res->wResultCode = wResultCode;
	res->byBindType = DBO_BIND_TYPE_POPO_STONE;
	res->bindObjectTblidx = req->bindObjectTblidx;
	res->bindWorldId = cPlayer->GetWorldID();
	app->Send(GetHandle(), &packet );

	if (wResultCode == GAME_SUCCESS)
	{
		sVECTOR3 loc, dir;

		loc.x = pPopoObject->GetTbldat()->vMax.x;
		loc.y = pPopoObject->GetTbldat()->vMax.y;
		loc.z = pPopoObject->GetTbldat()->vMax.z;

		dir.x = cPlayer->GetCurDir().x;
		dir.y = cPlayer->GetCurDir().y;
		dir.z = cPlayer->GetCurDir().z;

		CNtlPacket packetQry(sizeof(sGQ_PC_UPDATE_BIND_REQ));
		sGQ_PC_UPDATE_BIND_REQ * resQry = (sGQ_PC_UPDATE_BIND_REQ *)packetQry.GetPacketData();
		resQry->wOpCode = GQ_PC_UPDATE_BIND_REQ;
		resQry->charId = cPlayer->GetCharID();
		resQry->handle = cPlayer->GetID();
		resQry->byBindType = DBO_BIND_TYPE_POPO_STONE;
		resQry->bindObjectTblidx = req->bindObjectTblidx;
		resQry->bindWorldId = cPlayer->GetWorldID();
		resQry->vBindDir.x = dir.x;
		resQry->vBindDir.y = dir.y;
		resQry->vBindDir.z = dir.z;
		resQry->vBindLoc.x = loc.x;
		resQry->vBindLoc.y = loc.y;
		resQry->vBindLoc.z = loc.z;
		packetQry.SetPacketLen(sizeof(sGQ_PC_UPDATE_BIND_REQ));
		app->SendTo(app->GetQueryServerSession(), &packetQry);

		cPlayer->SetBindLoc(loc);
		cPlayer->SetBindDir(dir);
		cPlayer->SetBindObjectTblidx(req->bindObjectTblidx);
		cPlayer->SetBindWorldID(cPlayer->GetWorldID());
		cPlayer->SetBindType(DBO_BIND_TYPE_POPO_STONE);
	}
}

//--------------------------------------------------------------------------------------//
//		PORTAL ADD REQUEST
//--------------------------------------------------------------------------------------//
void CClientSession::RecvPortalAddReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_PORTAL_ADD_REQ* req = (sUG_PORTAL_ADD_REQ *)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_PORTAL_ADD_RES));
	sGU_PORTAL_ADD_RES * res = (sGU_PORTAL_ADD_RES *)packet.GetPacketData();
	res->wOpCode = GU_PORTAL_ADD_RES;
	res->wResultCode = GAME_SUCCESS;
	res->hNpcHandle = req->handle;
	
	CNpc* pNpc = g_pObjectManager->GetNpc(req->handle);
	if(pNpc)
	{
		if (!cPlayer->IsInRange(pNpc->GetCurLoc(), DBO_DISTANCE_CHECK_TOLERANCE * DBO_DISTANCE_CHECK_TOLERANCE))
			res->wResultCode = GAME_TARGET_TOO_FAR;
		else if (pNpc->GetTbldat()->byJob != NPC_JOB_PORTAL_MAN)
			res->wResultCode = GAME_TARGET_TOO_FAR;
		else if(cPlayer->HasPortalPoint((PORTALID)pNpc->GetTbldat()->contentsTblidx))
			res->wResultCode = GAME_PORTAL_ARLEADY_ADDED;
		else
		{
			sPORTAL_TBLDAT* pPortalData = (sPORTAL_TBLDAT*)g_pTableContainer->GetPortalTable()->FindData(pNpc->GetTbldat()->contentsTblidx);
			if (pPortalData)
			{
				res->PortalID = (PORTALID)pPortalData->tblidx;
				
				cPlayer->AddPortalPoint(res->PortalID);

				CNtlPacket packetQry(sizeof(sGQ_PORTAL_ADD_REQ));
				sGQ_PORTAL_ADD_REQ * resQry = (sGQ_PORTAL_ADD_REQ *)packetQry.GetPacketData();
				resQry->wOpCode = GQ_PORTAL_ADD_REQ;
				resQry->hNpcHandle = req->handle;
				resQry->charID = cPlayer->GetCharID();
				resQry->handle = cPlayer->GetID();
				resQry->PortalID = res->PortalID;
				packetQry.SetPacketLen(sizeof(sGQ_PORTAL_ADD_REQ));
				app->SendTo(app->GetQueryServerSession(), &packetQry);
			}
			else res->wResultCode = GAME_PORTAL_NOT_EXIST;
		}
	}
	else res->wResultCode = GAME_TARGET_NOT_FOUND;

	packet.SetPacketLen(sizeof(sGU_PORTAL_ADD_RES));
	app->Send(GetHandle(), &packet);
}

//--------------------------------------------------------------------------------------//
//		TELEPORT USING PORTAL
//--------------------------------------------------------------------------------------//
void CClientSession::RecvPortalReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PORTAL_REQ* req = (sUG_PORTAL_REQ *)pPacket->GetPacketData();

	sPORTAL_TBLDAT* pPortalTblData = (sPORTAL_TBLDAT*) g_pTableContainer->GetPortalTable()->FindData( req->byPoint+1 );
	CNtlVector dir, loc;

	CNtlPacket packet(sizeof(sGU_PORTAL_RES));
	sGU_PORTAL_RES * res = (sGU_PORTAL_RES *)packet.GetPacketData();
	res->wOpCode = GU_PORTAL_RES;
	res->hNpcHandle = req->handle;
	res->wResultCode = GAME_SUCCESS;

	if (cPlayer->HasPortalPoint(req->byPoint + 1))
	{
		CNpc* pNpc = g_pObjectManager->GetNpc(req->handle);
		if (pNpc && pPortalTblData)
		{
			//check distance
			if (!cPlayer->IsInRange(pNpc->GetCurLoc(), DBO_DISTANCE_CHECK_TOLERANCE))
				res->wResultCode = GAME_TARGET_TOO_FAR;
			else if (pNpc->GetTbldat()->byJob != NPC_JOB_PORTAL_MAN)
				res->wResultCode = GAME_TARGET_TOO_FAR;
			else if (cPlayer->GetZeni() < pPortalTblData->adwPointZenny[pNpc->GetTbldat()->contentsTblidx - 1])
				res->wResultCode = GAME_ZENNY_NOT_ENOUGH;
			else if (cPlayer->GetDragonballScrambleBallFlag() > 0)
				res->wResultCode = GAME_CAN_NOT_TELEPORT;
			else
			{
				res->vDir = pPortalTblData->vDir;
				res->vLoc = pPortalTblData->vLoc;
				res->byPoint = pPortalTblData->tblidx;
				res->worldID = pPortalTblData->worldId;

				dir = pPortalTblData->vDir;
				loc = pPortalTblData->vLoc;

				cPlayer->UpdateZeni(ZENNY_CHANGE_TYPE_PORTAL_ADD, pPortalTblData->adwPointZenny[pNpc->GetTbldat()->contentsTblidx - 1], false, true);
			}
		}
		else res->wResultCode = GAME_TARGET_NOT_FOUND;
	}
	else res->wResultCode = GAME_PORTAL_NOT_EXIST;

	packet.SetPacketLen(sizeof(sGU_PORTAL_RES));
	g_pApp->Send(GetHandle(), &packet);

	if (res->wResultCode == GAME_SUCCESS)
		cPlayer->StartTeleport(loc, dir, pPortalTblData->worldId, TELEPORT_TYPE_NPC_PORTAL);
			
}
//--------------------------------------------------------------------------------------//
//		TELEPORT
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharTeleportReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_TELEPORT_REQ* req = (sUG_CHAR_TELEPORT_REQ *)pPacket->GetPacketData();

	CGameServer* app = (CGameServer*)g_pApp;


	CNtlPacket packet(sizeof(sGU_CHAR_TELEPORT_RES));
	sGU_CHAR_TELEPORT_RES * res = (sGU_CHAR_TELEPORT_RES *)packet.GetPacketData();
	res->wOpCode = GU_CHAR_TELEPORT_RES;
	res->bIsToMoveAnotherServer = cPlayer->GetTeleportAnotherServer();
	res->bSyncDirectPlay = cPlayer->GetTeleportSyncDirectPlay();
	res->spawnDirectPlay = cPlayer->GetTeleportDirectPlay();
	res->wResultCode = GAME_SUCCESS;

	if(cPlayer->GetTeleportWorldID() == INVALID_WORLDID)
		res->wResultCode = GAME_FAIL;
	else if (cPlayer->GetStateManager()->CanCharStateTransition(CHARSTATE_TELEPORTING) == false) //check if can go into teleport state
		res->wResultCode = GAME_CHAR_IS_WRONG_STATE;
	else if (cPlayer->GetTeleportAnotherServer())
	{
		if (cPlayer->GetTeleportProposalType() != TELEPORT_TYPE_BUDOKAI
			&& cPlayer->GetTeleportProposalType() != TELEPORT_TYPE_MINORMATCH
			&& cPlayer->GetTeleportProposalType() != TELEPORT_TYPE_MAJORMATCH
			&& cPlayer->GetTeleportProposalType() != TELEPORT_TYPE_FINALMATCH
			&& cPlayer->GetTeleportProposalType() != TELEPORT_TYPE_DOJO)
			res->wResultCode = GAME_FAIL;
	}
	else
	{
		CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld(cPlayer->GetTeleportWorldID());
		if(pWorld)
		{
			bool bIsInRange = cPlayer->IsInRange(cPlayer->GetTeleportLoc(), NTL_MAX_RADIUS_OF_VISIBLE_AREA + 50.f);
			cPlayer->GetTeleportLoc().CopyTo(res->vNewLoc);
			cPlayer->GetTeleportDir().CopyTo(res->vNewDir);

		//	printf("cPlayer->GetTeleportWorldID() %u, pWorld->GetIdx() %u, %f %f\n", cPlayer->GetTeleportWorldID(), pWorld->GetIdx(), cPlayer->GetCurLoc().x, cPlayer->GetTeleportLoc().x);

			if (cPlayer->GetWorldID() != cPlayer->GetTeleportWorldID() || bIsInRange == true) // if we move different world or not further than 100m then send world info. Otherwise loading screen forever
			{
				pWorld->CopyToInfo(&res->sWorldInfo);
			}
		}
		else res->wResultCode = GAME_FAIL;
	}
	
	if (res->wResultCode == GAME_SUCCESS)
	{
		//send char state teleporting
		cPlayer->SendCharStateTeleporting();

		if (cPlayer->GetCurWorld())
		{
			//if teleport out of TLQ then destroy.
			if (cPlayer->GetTeleportWorldID() != cPlayer->GetWorldID() && cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_TLQ)
			{
				cPlayer->GetQuests()->GetEventMap()->Clear();

				cPlayer->RemAllScript();

				cPlayer->GetQuests()->LeaveWorld();

				g_pDungeonManager->DestroyTimeLeapDungeon(cPlayer->GetCharID(), cPlayer->GetTLQ());
				cPlayer->SetTLQ(NULL);
			}

			//if teleport out of ultimate dungeon
			else if (cPlayer->GetTeleportWorldID() != cPlayer->GetWorldID() && cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_HUNT)
			{
				cPlayer->GetQuests()->GetEventMap()->Clear();

				cPlayer->RemAllScript();

				if (cPlayer->GetUD())
					cPlayer->GetUD()->LeaveDungeon(cPlayer->GetID());

				cPlayer->SetUD(NULL);

				std::vector<TBLIDX> m_buffs{ 850076, 850077, 850078 };

				for (TBLIDX i : m_buffs)
				{
					if (CBuff* buff = cPlayer->GetBuffManager()->FindBuff(i, DBO_OBJECT_SOURCE_ITEM))
						cPlayer->GetBuffManager()->RemoveBuff(buff->GetBuffIndex(), buff->GetBuffType(), CBuff::BUFF_REMOVAL_REASON_BY_ITSELF);
				}
			}

			//if teleport out of battle dungeon
			else if (cPlayer->GetTeleportWorldID() != cPlayer->GetWorldID() && cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_CCBATTLEDUNGEON)
			{
				cPlayer->GetQuests()->GetEventMap()->Clear();

				cPlayer->RemAllScript();

				if (cPlayer->GetCCBD())
					cPlayer->GetCCBD()->LeaveDungeon(cPlayer->GetID());

				cPlayer->SetCCBD(NULL);

				if (cPlayer->GetParty())
					cPlayer->LeaveParty(); //when we leave ccbd then we will be removed from party. ALWAYS.

				std::vector<TBLIDX> m_buffs{ 850076, 850077, 850078 };

				for (TBLIDX i : m_buffs)
				{
					if (CBuff* buff = cPlayer->GetBuffManager()->FindBuff(i, DBO_OBJECT_SOURCE_ITEM))
						cPlayer->GetBuffManager()->RemoveBuff(buff->GetBuffIndex(), buff->GetBuffType(), CBuff::BUFF_REMOVAL_REASON_BY_ITSELF);
				}
			}

			//if teleport out of rank battle
			else if (cPlayer->GetTeleportWorldID() != cPlayer->GetWorldID() && cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_RANKBATTLE)
			{
				g_pRankbattleManager->LeaveBattle(cPlayer);
			}

			//if teleport out of time machine dungeon
			else if (cPlayer->GetTeleportWorldID() != cPlayer->GetWorldID() && (cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_TIMEQUEST || cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_TUTORIAL))
			{
				cPlayer->GetQuests()->GetEventMap()->Clear();

				cPlayer->RemAllScript();
				cPlayer->GetQuests()->LeaveWorld(false, true); //remove all active quests

				if (cPlayer->GetTMQ())
					cPlayer->GetTMQ()->RemoveMember(cPlayer->GetID());

				cPlayer->SetTMQ(NULL);

				std::vector<TBLIDX> m_buffs{ 850076, 850077, 850078 };

				for (TBLIDX i : m_buffs)
				{
					if (CBuff* buff = cPlayer->GetBuffManager()->FindBuff(i, DBO_OBJECT_SOURCE_ITEM))
						cPlayer->GetBuffManager()->RemoveBuff(buff->GetBuffIndex(), buff->GetBuffType(), CBuff::BUFF_REMOVAL_REASON_BY_ITSELF);
				}
			}
		}
	}
	//printf("res->wResultCode %u, new loc %f %f %f \n", res->wResultCode, res->vNewLoc.x, res->vNewLoc.y, res->vNewLoc.z);
	
	packet.SetPacketLen(sizeof(sGU_CHAR_TELEPORT_RES));
	app->Send(GetHandle(), &packet);
}


//--------------------------------------------------------------------------------------//
//		ATTACK BEGIN
//--------------------------------------------------------------------------------------//
void CClientSession::RecvAttackBegin(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_ATTACK_BEGIN* req = (sUG_CHAR_ATTACK_BEGIN *)pPacket->GetPacketData();

	//byType 0 = player / 1 = pet
	
	if(req->byType == 0)
	{
		HOBJECT hTarget = cPlayer->GetTargetHandle();

		if (hTarget == cPlayer->GetID())
			return;

		CCharacter*	victim = g_pObjectManager->GetChar(hTarget);
		if (victim == NULL || victim->IsInitialized() == false)
			return;

		if (cPlayer->GetCurWorld() == NULL)
			return;

		BYTE byWorldRuleType = cPlayer->GetCurWorld()->GetTbldat()->byWorldRuleType;

		if (byWorldRuleType == GAMERULE_RANKBATTLE)
		{
			if (cPlayer->GetRankBattleData()->eState != RANKBATTLE_MEMBER_STATE_ATTACKABLE)
				return;
		}
		else if (byWorldRuleType == GAMERULE_MINORMATCH || byWorldRuleType == GAMERULE_MAJORMATCH || byWorldRuleType == GAMERULE_FINALMATCH)
		{
			if (cPlayer->GetBudokaiPcState() != MATCH_MEMBER_STATE_NORMAL)
				return;
		}

		if (cPlayer->GetCurrentPetId() != INVALID_HOBJECT)
		{
			if (CSummonPet* pPet = cPlayer->GetSummonPet())
			{
				if (pPet->GetToggleAttack())
					pPet->SetAttackTarget(victim->GetID());
			}
		}

		if (cPlayer->IsKnockedDown() || cPlayer->IsSliding())
			return;

		cPlayer->SetAttackTarget(victim->GetID());
		cPlayer->ChangeAttackProgress(true);
	}
	else if(req->byType == 1)
	{
		ERR_LOG(LOG_USER,"An error is occured in RecvAttackBegin: req->byType == 1");
	}
}
//--------------------------------------------------------------------------------------//
//		ATTACK END
//---------------------------------------------------------------------------------------//
void CClientSession::RecvAttackEnd(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_ATTACK_END* req = (sUG_CHAR_ATTACK_END *)pPacket->GetPacketData();

	if(req->byType == 0)
	{
		cPlayer->ChangeAttackProgress(false);
		cPlayer->SetAttackTarget(INVALID_HOBJECT);
	}
	else if(req->byType == 1)
	{
		ERR_LOG(LOG_USER,"An error is occured in RecvAttackEnd: req->byType == 1");
	}
}



void CClientSession::RecvCharSkillReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_SKILL_REQ *req = (sUG_CHAR_SKILL_REQ*)pPacket->GetPacketData();
	WORD resultcode = GAME_SUCCESS;
	CGameServer* app = (CGameServer*)g_pApp;

	//printf("byAvatarType:%u, ahApplyTarget:%u, byApplyTargetCount:%u, byAvatarType:%u, byRpBonusType:%u, bySlotIndex:%u, hTarget:%u\n", req->byAvatarType, req->ahApplyTarget[0], req->byApplyTargetCount, req->byAvatarType, req->byRpBonusType, req->bySlotIndex, req->hTarget);

	CNtlVector vLoc;
	NtlLocationDecompress(&req->vCurLoc, &vLoc.x, &vLoc.y, &vLoc.z);

	sVECTOR3 sDir;
	NtlDirectionDecompress(&req->vCurDir, &sDir.x, &sDir.y, &sDir.z);
	cPlayer->SetCurDir(sDir);

	CSkillManagerPc* pSkillManager = cPlayer->GetSkillManager();
	if (pSkillManager)
	{
		CSkillPc* pSkill = (CSkillPc*)pSkillManager->GetSkillWithSkillIndex(req->bySlotIndex);
		if (pSkill)
		{
			if (pSkill->GetOriginalTableData()->byRequire_Epuip_Slot_Type != INVALID_BYTE) //check if has required item
			{
				CItem* pRequireItem = cPlayer->GetPlayerItemContainer()->GetItem(CONTAINER_TYPE_EQUIP, pSkill->GetOriginalTableData()->byRequire_Epuip_Slot_Type);
				if (pRequireItem == NULL || pRequireItem->GetTbldat()->byItem_Type != pSkill->GetOriginalTableData()->byRequire_Item_Type)
					resultcode = GAME_SKILL_NO_REQUIRED_ITEM;
			}

			if (cPlayer->GetCurWorld())
			{
				//check ep
				if (pSkill->GetOriginalTableData()->wRequire_EP > 0)
				{
					float fRequireEP = (float)pSkill->GetOriginalTableData()->wRequire_EP;

					if (cPlayer->GetCharAtt()->GetRequiredEpChangePercent() != 0.0f)
						fRequireEP += fRequireEP * cPlayer->GetCharAtt()->GetRequiredEpChangePercent() / 100.0f;

					if (req->byRpBonusType == DBO_RP_BONUS_TYPE_EP_MINUS)
						fRequireEP -= fRequireEP * pSkill->GetOriginalTableData()->afRpEffectValue[DBO_RP_BONUS_SLOT_EP_MINUS] / 100;

					if (fRequireEP > 1.0f)
					{
						if((WORD)fRequireEP > cPlayer->GetCurEP())
							resultcode = GAME_SKILL_NOT_ENOUGH_EP;
					}
				}

				//check LP
				if (pSkill->GetOriginalTableData()->dwRequire_LP > 0)
				{
					if ((int)pSkill->GetOriginalTableData()->dwRequire_LP > cPlayer->GetCurLP())
						resultcode = GAME_SKILL_NOT_ENOUGH_LP;
				}

				if (resultcode == GAME_SUCCESS)
				{
					BYTE byWorldRuleType = cPlayer->GetCurWorld()->GetTbldat()->byWorldRuleType;

					if (byWorldRuleType == GAMERULE_RANKBATTLE)
					{
						if (cPlayer->GetRankBattleData()->eState != RANKBATTLE_MEMBER_STATE_ATTACKABLE)
							resultcode = GAME_SKILL_CANT_CAST_NOW;
					}
					else if (byWorldRuleType == GAMERULE_MINORMATCH || byWorldRuleType == GAMERULE_MAJORMATCH || byWorldRuleType == GAMERULE_FINALMATCH)
					{
						if (cPlayer->GetBudokaiPcState() != MATCH_MEMBER_STATE_NORMAL)
							resultcode = GAME_SKILL_CANT_CAST_NOW;
					}

					bool bIsHarmful = Dbo_IsHarmfulEffectType(pSkill->GetOriginalTableData()->bySkill_Active_Type) && pSkill->GetOriginalTableData()->byApply_Target != DBO_SKILL_APPLY_TARGET_PARTY;

					if (bIsHarmful && cPlayer->IsPvpZone() == false && cPlayer->GetPcIsFreeBattle() == false && cPlayer->GetCurWorld()->GetTbldat()->bDynamic == false && GetNaviEngine()->IsBasicAttributeSet(cPlayer->GetCurWorld()->GetNaviInstanceHandle(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().z, DBO_WORLD_ATTR_BASIC_FORBID_PC_BATTLE))
					{
						resultcode = GAME_SKILL_INVALID_TARGET_APPOINTED;
					}
					else if (pSkill->GetOriginalTableData()->bySkill_Class == NTL_SKILL_CLASS_PASSIVE) //do this check here and not inside skill class because we can use counter-attack(passive)
					{
						resultcode = GAME_SKILL_NOT_ACTIVE_TYPE;
					}
					else if (req->byRpBonusType != DBO_RP_BONUS_TYPE_INVALID) //check RP
					{
						resultcode = GAME_SKILL_CANT_USE_THAT_RP_BONUS_IN_SKILL;

						for (int i = 0; i < DBO_MAX_RP_BONUS_COUNT_PER_SKILL; i++)
						{
							if (pSkill->GetOriginalTableData()->abyRpEffect[i] == req->byRpBonusType)
							{
								resultcode = GAME_SUCCESS;
								break;
							}
						}
					}
				}
			}
			else
			{
				resultcode = GAME_SKILL_CANT_CAST_NOW;
			}
			
		
			if (resultcode == GAME_SUCCESS)
			{
				//RP Ball Shit
				BYTE byRpBonusType = (req->byRpBonusType != DBO_RP_BONUS_TYPE_INVALID && cPlayer->GetCurRPBall() > 0) ? req->byRpBonusType : DBO_RP_BONUS_TYPE_INVALID;

				sVECTOR3 sFinalSubjectLoc;
				NtlLocationDecompress(&req->vFinalSubjectLoc, &sFinalSubjectLoc.x, &sFinalSubjectLoc.y, &sFinalSubjectLoc.z);

				BYTE byTargetCount = req->byApplyTargetCount;

				if (byTargetCount > pSkill->GetOriginalTableData()->byApply_Target_Max)
					byTargetCount = pSkill->GetOriginalTableData()->byApply_Target_Max;

			//	NTL_PRINT(PRINT_APP,"StartSkill: %f %f %f | %f %f %f | %f %f %f | %u\n", vLoc.x, vLoc.y, vLoc.z, sFinalSubjectLoc.x, sFinalSubjectLoc.y, sFinalSubjectLoc.z, cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().y, cPlayer->GetCurLoc().z, cPlayer->GetCharStateID());
				CNtlVector vFinalSubjectLoc(sFinalSubjectLoc);
				pSkill->UseSkill(byRpBonusType, req->hTarget, vFinalSubjectLoc, byTargetCount, req->ahApplyTarget, resultcode);
			}
		}
		else resultcode = GAME_SKILL_YOU_DONT_HAVE_THE_SKILL;
	}
	else resultcode = GAME_SKILL_YOU_DONT_HAVE_THE_SKILL;

	CNtlPacket packet(sizeof(sGU_CHAR_SKILL_RES));
	sGU_CHAR_SKILL_RES * res = (sGU_CHAR_SKILL_RES *)packet.GetPacketData();
	res->wOpCode = GU_CHAR_SKILL_RES;
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_CHAR_SKILL_RES));
	g_pApp->Send(GetHandle(), &packet);
}




//--------------------------------------------------------------------------------------//
//		SKILL SHOP REQUEST
//--------------------------------------------------------------------------------------//
void CClientSession::SendShopSkillReq(CNtlPacket * pPacket)
{
	sUG_SHOP_SKILL_BUY_REQ * req = (sUG_SHOP_SKILL_BUY_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_SHOP_SKILL_BUY_RES));
	sGU_SHOP_SKILL_BUY_RES * res = (sGU_SHOP_SKILL_BUY_RES *)packet.GetPacketData();
	res->hNpchandle = req->hNpchandle;
	res->wOpCode = GU_SHOP_SKILL_BUY_RES;
	res->wResultCode  = GAME_SUCCESS;
	packet.SetPacketLen( sizeof(sGU_SHOP_SKILL_BUY_RES) );
	g_pApp->Send(GetHandle(), &packet );
}

//--------------------------------------------------------------------------------------//
//		Char learn skill
//--------------------------------------------------------------------------------------//
void CClientSession::RecvLearnSkillReq(CNtlPacket * pPacket)
{
	sUG_SKILL_LEARN_REQ * req = (sUG_SKILL_LEARN_REQ*)pPacket->GetPacketData();
	
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CNtlPacket packet(sizeof(sGU_SKILL_LEARN_RES));
	sGU_SKILL_LEARN_RES * res = (sGU_SKILL_LEARN_RES *)packet.GetPacketData();
	res->wOpCode = GU_SKILL_LEARN_RES;

	printf("skill id %u\n", req->skillTblidx);

	if (cPlayer->GetSkillManager())
		cPlayer->GetSkillManager()->LearnSkill(req->skillTblidx, res->wResultCode);
	else
		res->wResultCode = GAME_FAIL;

	packet.SetPacketLen( sizeof(sGU_SKILL_LEARN_RES) );
	g_pApp->Send(GetHandle(), &packet);
}

//--------------------------------------------------------------------------------------//
//		Char learn skill by item
//--------------------------------------------------------------------------------------//
void CClientSession::RecvLearnSkillByItemReq(CNtlPacket * pPacket)
{
	sUG_SKILL_LEARN_BY_ITEM_REQ * req = (sUG_SKILL_LEARN_BY_ITEM_REQ*)pPacket->GetPacketData();

	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	WORD wResultcode = GAME_SUCCESS;

	CItem* item = cPlayer->GetPlayerItemContainer()->GetItem(req->byRequiredItemPlace, req->byRequiredItemPos);
	if(item && item->GetCount() > 0 && IsInvenContainer(item->GetPlace()))
	{
		sITEM_TBLDAT* itemTbl = item->GetTbldat();

		if (item->IsLocked())
			wResultcode = GAME_ITEM_IS_LOCK;
		else if (cPlayer->GetLevel() < itemTbl->byNeed_Min_Level)
			wResultcode = GAME_ITEM_TOO_LOW_LEVEL_TO_USE_ITEM;
		else if (cPlayer->GetLevel() > itemTbl->byNeed_Max_Level)
			wResultcode = GAME_ITEM_TOO_HIGH_LEVEL_TO_USE_ITEM;
		else
		{
			sUSE_ITEM_TBLDAT* useitem = (sUSE_ITEM_TBLDAT*)g_pTableContainer->GetUseItemTable()->FindData(itemTbl->Use_Item_Tblidx);
			if(useitem)
			{
				cPlayer->GetSkillManager()->LearnSkill((TBLIDX)useitem->aSystem_Effect_Value[0], wResultcode, false);
			}
			else
			{
				wResultcode = GAME_FAIL;
				ERR_LOG(LOG_SYSTEM, "COULD NOT FIND %u IN USE_ITEM_TABLE", itemTbl->Use_Item_Tblidx);
			}
		}
	}
	else wResultcode = GAME_ITEM_NOT_FOUND;

	CNtlPacket packet(sizeof(sGU_SKILL_LEARN_BY_ITEM_RES));
	sGU_SKILL_LEARN_BY_ITEM_RES * res = (sGU_SKILL_LEARN_BY_ITEM_RES *)packet.GetPacketData();
	res->wOpCode = GU_SKILL_LEARN_BY_ITEM_RES;
	res->wResultCode = wResultcode;
	packet.SetPacketLen(sizeof(sGU_SKILL_LEARN_BY_ITEM_RES));
	g_pApp->Send(GetHandle(), &packet);

	if (wResultcode == GAME_SUCCESS)
		item->SetCount(item->GetCount() - 1, false, true);
}

//--------------------------------------------------------------------------------------//
//		Char learn HTB skill
//--------------------------------------------------------------------------------------//
void CClientSession::RecvHTBLearnReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_HTB_LEARN_REQ * req = (sUG_HTB_LEARN_REQ*)pPacket->GetPacketData();
	WORD resultcode = GAME_FAIL;
	//sHTB_SET_TBLDAT *pHTBSetTblData = (sHTB_SET_TBLDAT*)g_pTableContainer->GetHTBSetTable()->FindData(req->skillId);	

	/////////////////
	////TO-DO: CHECK IF HTB ALREADY LEARNED ETC
	/////////////////

	CNtlPacket packetHTB(sizeof(sGU_HTB_LEARN_RES));
	sGU_HTB_LEARN_RES * res = (sGU_HTB_LEARN_RES*)packetHTB.GetPacketData();
	res->wOpCode = GU_HTB_LEARN_RES;
	res->skillId = req->skillId;
	res->bySkillSlot = INVALID_BYTE;
	res->wResultCode = resultcode;
	packetHTB.SetPacketLen(sizeof(sGU_HTB_LEARN_RES));
	g_pApp->Send(GetHandle(), &packetHTB);
}

//--------------------------------------------------------------------------------------//
//		MOVE ITEM
//--------------------------------------------------------------------------------------//
void CClientSession::RecvItemMoveReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	WORD item_move_res = GAME_SUCCESS;
	sUG_ITEM_MOVE_REQ * req = (sUG_ITEM_MOVE_REQ*)pPacket->GetPacketData();

	CNtlPacket pQry(sizeof(sGQ_ITEM_MOVE_REQ));
	sGQ_ITEM_MOVE_REQ * rQry = (sGQ_ITEM_MOVE_REQ *)pQry.GetPacketData();
	rQry->wOpCode = GQ_ITEM_MOVE_REQ;
	rQry->handle = cPlayer->GetID();
	rQry->charId = cPlayer->GetCharID();
	rQry->bySrcPlace = req->bySrcPlace;
	rQry->bySrcPos = req->bySrcPos;
	rQry->byDstPlace = req->byDestPlace;
	rQry->byDstPos = req->byDestPos;

	CItem* src_item = NULL;
	if (cPlayer->GetPlayerItemContainer()->IsInventoryReserved(req->bySrcPlace, req->bySrcPos) || cPlayer->GetPlayerItemContainer()->IsInventoryReserved(req->byDestPlace, req->byDestPos))
	{
		item_move_res = GAME_FAIL;
		goto END;
	}
	
	src_item = cPlayer->GetPlayerItemContainer()->GetItem(req->bySrcPlace, req->bySrcPos);
	if(src_item)
	{
		CItem* dest_item = cPlayer->GetPlayerItemContainer()->GetItem(req->byDestPlace, req->byDestPos);

		sITEM_TBLDAT* pItemData = src_item->GetTbldat();

		if (src_item->IsEquipped() == false && src_item->IsLocked()) //only check if not equiped
		{
			item_move_res = GAME_ITEM_IS_LOCK;
			goto END;
		}

		if (dest_item && ((dest_item->IsLocked() && dest_item->IsEquipped() == false) || dest_item->GetOwner() == NULL) )
		{
			item_move_res = GAME_ITEM_IS_LOCK;
			goto END;
		}

		if (req->bySrcPlace == CONTAINER_TYPE_BAGSLOT) //switch bag from bag slot
		{
			if (req->bySrcPos == BAGSLOT_POSITION_BAGSLOT_POSITION_0) //DONT ALLOW TO MOVE THE FIRST BAG SLOT.
			{
				item_move_res = GAME_ITEM_IS_LOCK;
				goto END;
			}

			if (!cPlayer->GetPlayerItemContainer()->IsBagEmpty(req->bySrcPos)) //check if bag is empty
			{
				item_move_res = GAME_ITEM_IS_LOCK;
				goto END;
			}
		}

		if (req->byDestPlace == CONTAINER_TYPE_BAGSLOT) //move bag to bag slot
		{
			if (!src_item->IsBag()) //check if item is bag
			{
				item_move_res = GAME_ITEM_NOT_GO_THERE;
				goto END;
			}
			else if (dest_item && dest_item->IsBag()) //check if dest item exist and if its a bag
			{
				if (!cPlayer->GetPlayerItemContainer()->IsBagEmpty(req->byDestPos)) //check if dest bag is empty
				{
					item_move_res = GAME_FAIL;
					goto END;
				}
			}
		}


		//check if can move into dest inventory
		if (IsInvenContainer(req->byDestPlace))
		{
			CItem* pBagItem = cPlayer->GetPlayerItemContainer()->GetActiveBag(req->byDestPlace - 1);
			if (pBagItem)
			{
				if (req->byDestPos >= pBagItem->GetBagSize())
				{
					item_move_res = GAME_ITEM_POSITION_FAIL;
					goto END;
				}

				if (pBagItem->IsExpired())
				{
					item_move_res = GAME_FAIL;
					goto END;
				}
			}
			else
			{
				item_move_res = GAME_ITEM_POSITION_FAIL;
				goto END;
			}
		}


		if (req->byDestPlace == CONTAINER_TYPE_EQUIP)
		{
			if (cPlayer->GetLevel() < pItemData->byNeed_Min_Level) //check level
				item_move_res = GAME_ITEM_NEED_MORE_LEVEL;
			else if(cPlayer->GetLevel() > pItemData->byNeed_Max_Level)
				item_move_res = GAME_ITEM_TOO_HIGH_LEVEL_TO_USE_ITEM;

			else if (Dbo_CheckClass(cPlayer->GetClass(), pItemData->dwNeed_Class_Bit_Flag) == false) //check class
			{
				if (cPlayer->GetClass() >= PC_CLASS_1_FIRST && cPlayer->GetClass() <= PC_CLASS_1_LAST)
					item_move_res = GAME_ITEM_CLASS_FAIL;
				else
				{
					if (cPlayer->GetClass() == PC_CLASS_STREET_FIGHTER && pItemData->dwNeed_Class_Bit_Flag != 256)
						item_move_res = GAME_ITEM_CLASS_FAIL;
					else if (cPlayer->GetClass() == PC_CLASS_SWORD_MASTER && pItemData->dwNeed_Class_Bit_Flag != 128)
						item_move_res = GAME_ITEM_CLASS_FAIL;
					else if (cPlayer->GetClass() == PC_CLASS_CRANE_ROSHI && pItemData->dwNeed_Class_Bit_Flag != 1024)
						item_move_res = GAME_ITEM_CLASS_FAIL;
					else if (cPlayer->GetClass() == PC_CLASS_TURTLE_ROSHI && pItemData->dwNeed_Class_Bit_Flag != 512)
						item_move_res = GAME_ITEM_CLASS_FAIL;
					else if (cPlayer->GetClass() == PC_CLASS_DARK_WARRIOR && pItemData->dwNeed_Class_Bit_Flag != 16384)
						item_move_res = GAME_ITEM_CLASS_FAIL;
					else if (cPlayer->GetClass() == PC_CLASS_SHADOW_KNIGHT && pItemData->dwNeed_Class_Bit_Flag != 8192)
						item_move_res = GAME_ITEM_CLASS_FAIL;
					else if (cPlayer->GetClass() == PC_CLASS_DENDEN_HEALER && pItemData->dwNeed_Class_Bit_Flag != 65536)
						item_move_res = GAME_ITEM_CLASS_FAIL;
					else if (cPlayer->GetClass() == PC_CLASS_POCO_SUMMONER && pItemData->dwNeed_Class_Bit_Flag != 32768)
						item_move_res = GAME_ITEM_CLASS_FAIL;
					else if (cPlayer->GetClass() == PC_CLASS_ULTI_MA && pItemData->dwNeed_Class_Bit_Flag != 262144)
						item_move_res = GAME_ITEM_CLASS_FAIL;
					else if (cPlayer->GetClass() == PC_CLASS_GRAND_MA && pItemData->dwNeed_Class_Bit_Flag != 131072)
						item_move_res = GAME_ITEM_CLASS_FAIL;
					else if (cPlayer->GetClass() == PC_CLASS_PLAS_MA && pItemData->dwNeed_Class_Bit_Flag != 1048576)
						item_move_res = GAME_ITEM_CLASS_FAIL;
					else if (cPlayer->GetClass() == PC_CLASS_KAR_MA && pItemData->dwNeed_Class_Bit_Flag != 524288)
						item_move_res = GAME_ITEM_CLASS_FAIL;
				}
				if (cPlayer->GetGMLevel() == 10) item_move_res = GAME_SUCCESS;
			}

			else if(BIT_FLAG_TEST( MAKE_BIT_FLAG(cPlayer->GetGender()), pItemData->dwNeed_Gender_Bit_Flag ) == false) //check gender
				item_move_res = GAME_ITEM_GENDER_DOESNT_MATCH;

			else if(pItemData->byRace_Special != cPlayer->GetRace() && pItemData->byRace_Special != INVALID_BYTE) //check race
				item_move_res = GAME_CHAR_RACE_FAIL;
			
			else if (BIT_FLAG_TEST(MAKE_BIT_FLAG(req->byDestPos), pItemData->dwEquip_Slot_Type_Bit_Flag) == false) //check if item can go to that position
				item_move_res = GAME_ITEM_POSITION_FAIL;

			else if (src_item->GetRestrictState() == ITEM_RESTRICT_STATE_TYPE_SEAL)
				item_move_res = GAME_FAIL;

			else if (dest_item && dest_item->GetRestrictState() == ITEM_RESTRICT_STATE_TYPE_SEAL)
				item_move_res = GAME_FAIL;

			if(item_move_res == GAME_SUCCESS)
			{
				cPlayer->EquipItem(src_item, req->byDestPos);

				if (src_item->GetRestrictState() == 0 && src_item->GetTbldat()->byRestrictType > 0) //check if must restrict item
				{
					rQry->bRestrictUpdate = true;

					if (src_item->GetTbldat()->byRestrictType == ITEM_RESTRICT_TYPE_EQUIP)
						rQry->bySrcRestrictState = ITEM_RESTRICT_STATE_TYPE_LIMIT;
					else
						rQry->bySrcRestrictState = GetDefaultRestrictState(src_item->GetTbldat()->byRestrictType, src_item->GetTbldat()->byItem_Type, true);

					src_item->SetRestrictState(rQry->bySrcRestrictState);

					CNtlPacket packetItemUpdate(sizeof(sGU_ITEM_UPDATE));
					sGU_ITEM_UPDATE * resIU = (sGU_ITEM_UPDATE *)packetItemUpdate.GetPacketData();
					resIU->wOpCode = GU_ITEM_UPDATE;
					resIU->handle = src_item->GetID();
					resIU->sItemData = src_item->GetItemData();
					packetItemUpdate.SetPacketLen(sizeof(sGU_ITEM_UPDATE));
					app->Send(GetHandle(), &packetItemUpdate);
				}
			}
		}
		else if(req->bySrcPlace == CONTAINER_TYPE_EQUIP) //unequip
		{
		/*	if (req->bySrcPos == EQUIP_SLOT_TYPE_QUEST) //dont allow to unequip quest items
			{
				item_move_res = GAME_ITEM_IS_LOCK;
			}
			else*/ if(dest_item == NULL)
			{
				if (cPlayer->UnequipItem(src_item) == false)
				{
					item_move_res = GAME_FAIL;
				}
			} 
			else item_move_res = GAME_ITEM_INVEN_FULL;
		}


		if (req->byDestPlace == CONTAINER_TYPE_SCOUT) //dont allow to use scouter chips
			item_move_res = GAME_ITEM_NOT_GO_THERE;

		/*Update item in map and database if move success*/
		if(item_move_res == GAME_SUCCESS)
		{
			src_item->SetLocked(true);

			rQry->srcItemId = src_item->GetItemID();
			rQry->hSrcItem = src_item->GetID();

			if (dest_item)
			{
				dest_item->SetLocked(true);

				rQry->dstItemId = dest_item->GetItemID();
				rQry->hDstItem = dest_item->GetID();


				if (req->byDestPlace == CONTAINER_TYPE_EQUIP)
					dest_item->SetEquipped(false);
			}
			else
			{
				rQry->dstItemId = INVALID_ITEMID;
				rQry->hDstItem = INVALID_HOBJECT;

				//reseve dest place/pos to avoid item getting there while moving. Only need to do this when moving to a free slot
				cPlayer->GetPlayerItemContainer()->AddReservedInventory(req->byDestPlace, req->byDestPos);
			}

			pQry.SetPacketLen(sizeof(sGQ_ITEM_MOVE_REQ));
			app->SendTo(app->GetQueryServerSession(), &pQry);

			return;
		}

	}
	else item_move_res = GAME_ITEM_NOT_FOUND;


END:
	CNtlPacket packet(sizeof(sGU_ITEM_MOVE_RES));
	sGU_ITEM_MOVE_RES * res = (sGU_ITEM_MOVE_RES *)packet.GetPacketData();
	res->wOpCode = GU_ITEM_MOVE_RES;
	res->bySrcPlace = req->bySrcPlace;
	res->bySrcPos = req->bySrcPos;
	res->byDestPlace = req->byDestPlace;
	res->byDestPos = req->byDestPos;
	res->wResultCode = item_move_res; 
	app->Send( GetHandle() , &packet );
}
//--------------------------------------------------------------------------------------//
//		DELETE ITEM
//--------------------------------------------------------------------------------------//
void CClientSession::RecvItemDeleteReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_ITEM_DELETE_REQ * req = (sUG_ITEM_DELETE_REQ*)pPacket->GetPacketData();
	WORD resultcode = GAME_SUCCESS;
	
	CItem* pItem = cPlayer->GetPlayerItemContainer()->GetItem(req->bySrcPlace,req->bySrcPos);
	if (pItem == NULL)
		resultcode = GAME_ITEM_NOT_FOUND;
	else if (!pItem->CanDelete())
		resultcode = GAME_ITEM_CANNOT_DELETE;
	else if (IsInvenContainer(req->bySrcPlace) == false)
		resultcode = GAME_FAIL;
	else if(IsBagContainer(req->bySrcPlace) && cPlayer->GetPlayerItemContainer()->IsBagEmpty(req->bySrcPos) == false)
		resultcode = GAME_ITEM_CANNOT_DELETE;
	else
	{
		pItem->SetCount(0, false, true);
	}

	CNtlPacket packet(sizeof(sGU_ITEM_DELETE_RES));
	sGU_ITEM_DELETE_RES * res = (sGU_ITEM_DELETE_RES *)packet.GetPacketData();
	res->wOpCode = GU_ITEM_DELETE_RES;
	res->wResultCode = resultcode;
	res->byPlace = req->bySrcPlace;
	res->byPos = req->bySrcPos;
	packet.SetPacketLen( sizeof(sGU_ITEM_DELETE_RES) );
	g_pApp->Send(GetHandle(), &packet);
}
//--------------------------------------------------------------------------------------//
//		STACK ITEM
//--------------------------------------------------------------------------------------//
void CClientSession::RecvItemStackReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_ITEM_MOVE_STACK_REQ * req = (sUG_ITEM_MOVE_STACK_REQ*)pPacket->GetPacketData();
	WORD resultcode = GAME_SUCCESS;

	CNtlPacket packet(sizeof(sGQ_ITEM_MOVE_STACK_REQ));
	sGQ_ITEM_MOVE_STACK_REQ * res = (sGQ_ITEM_MOVE_STACK_REQ *)packet.GetPacketData();
	res->wOpCode = GQ_ITEM_MOVE_STACK_REQ;
	res->handle = cPlayer->GetID();
	res->charId = cPlayer->GetCharID();
	res->bySrcPlace = req->bySrcPlace;
	res->bySrcPos = req->bySrcPos;
	res->byDstPlace = req->byDestPlace;
	res->byDstPos = req->byDestPos;

	CItem* pSrcItem = NULL;
	CItem* pDestItem = NULL;
	if (req->bySrcPlace == req->byDestPlace && req->bySrcPos == req->byDestPos)
	{
		resultcode = GAME_FAIL;
		goto END;
	}

	if (cPlayer->GetPlayerItemContainer()->IsInventoryReserved(req->bySrcPlace, req->bySrcPos) || cPlayer->GetPlayerItemContainer()->IsInventoryReserved(req->byDestPlace, req->byDestPos))
	{
		resultcode = GAME_FAIL;
		goto END;
	}

	pSrcItem = cPlayer->GetPlayerItemContainer()->GetItem(req->bySrcPlace, req->bySrcPos);
	pDestItem = cPlayer->GetPlayerItemContainer()->GetItem(req->byDestPlace, req->byDestPos);

	if (pSrcItem == NULL || pSrcItem->GetCount() == 0)
	{
		resultcode = GAME_ITEM_NOT_FOUND;
		goto END;
	}

	if (pSrcItem->IsLocked() || pSrcItem->GetOwner() == NULL)
	{
		resultcode = GAME_ITEM_IS_LOCK;
		goto END;
	}
	
	if (!IsInvenContainer(req->bySrcPlace) || !IsInvenContainer(req->byDestPlace)) //only allow to stack items inside inventory with this packet
	{
		resultcode = GAME_ITEM_STACK_FAIL;
		goto END;
	}

	if (pDestItem == NULL)	//UNSTACK ITEM
	{
		if (pSrcItem->GetTbldat()->byMax_Stack == 1 || req->byStackCount == 0) //is the item even stack-able?
		{
			resultcode = GAME_ITEM_STACK_FAIL;
		}
		else if (pSrcItem->GetCount() <= req->byStackCount) //check if has enough stack
		{
			resultcode = GAME_ITEM_STACK_FAIL;
		}
		else
		{
			pSrcItem->SetLocked(true);

			//reseve dest place/pos to avoid item getting there while moving. Only need to do this when moving to a free slot
			cPlayer->GetPlayerItemContainer()->AddReservedInventory(req->byDestPlace, req->byDestPos);

			res->srcItemId = pSrcItem->GetItemID();
			res->dstItemId = INVALID_ITEMID;
			res->hSrcItem = pSrcItem->GetID();
			res->hDstItem = INVALID_HOBJECT;
			res->byStackCount1 = UnsignedSafeDecrease<BYTE>(pSrcItem->GetCount(), req->byStackCount);
			res->byStackCount2 = req->byStackCount;
		}
	}
	else					// STACK ITEM
	{
		if (pDestItem->IsLocked())
		{
			resultcode = GAME_ITEM_IS_LOCK;
		}
		else if (pSrcItem->GetTblidx() != pDestItem->GetTblidx()) //is source and dest item even same?
		{
			resultcode = GAME_ITEM_NOT_SAME;
		}
		else if (pSrcItem->GetTbldat()->byMax_Stack == 1 || pDestItem->GetTbldat()->byMax_Stack == 1) //is item stack-able
		{
			resultcode = GAME_ITEM_STACK_FAIL;
		}
		else if (pSrcItem->GetCount() < req->byStackCount || pSrcItem->GetTbldat()->byMax_Stack == 1 || req->byStackCount == 0) //check: has less than required items? is item possible to stack? is the stack request == 0
		{
			resultcode = GAME_ITEM_STACK_FAIL;
		}
		else if (pDestItem->GetCount() == 0 || pDestItem->GetCount() == pDestItem->GetTbldat()->byMax_Stack) //check if dest item already max stack
		{
			resultcode = GAME_ITEM_STACK_FULL;
		}
		else if (pDestItem->GetRestrictState() != pSrcItem->GetRestrictState())
			resultcode = GAME_ITEM_STACK_FAIL;
		else
		{
			BYTE stackCount1 = pSrcItem->GetCount() - req->byStackCount;
			int stackCount2 = req->byStackCount + pDestItem->GetCount();

			if (stackCount2 > pSrcItem->GetTbldat()->byMax_Stack)
			{
				stackCount1 = stackCount2 - pSrcItem->GetTbldat()->byMax_Stack;
				stackCount2 = (int)pSrcItem->GetTbldat()->byMax_Stack;
			}

			pSrcItem->SetLocked(true);
			pDestItem->SetLocked(true);

			res->srcItemId = pSrcItem->GetItemID();
			res->dstItemId = pDestItem->GetItemID();
			res->hSrcItem = pSrcItem->GetID();
			res->hDstItem = pDestItem->GetID();
			res->byStackCount1 = stackCount1;
			res->byStackCount2 = (BYTE)stackCount2;
		}
	}

	if (resultcode == GAME_SUCCESS)
	{
		packet.SetPacketLen(sizeof(sGQ_ITEM_MOVE_STACK_REQ));
		g_pApp->SendTo(app->GetQueryServerSession(), &packet);
	}
	else
	{
	END:
		CNtlPacket packetEnd(sizeof(sGU_ITEM_MOVE_STACK_RES));
		sGU_ITEM_MOVE_STACK_RES * resEnd = (sGU_ITEM_MOVE_STACK_RES *)packetEnd.GetPacketData();
		resEnd->wOpCode = GU_ITEM_MOVE_STACK_RES;
		resEnd->wResultCode = resultcode;
		packetEnd.SetPacketLen(sizeof(sGU_ITEM_MOVE_STACK_RES));
		g_pApp->Send(GetHandle(), &packetEnd);
	}
}

void CClientSession::RecvShopStartReq(CNtlPacket * pPacket)
{
	sUG_SHOP_START_REQ * req = (sUG_SHOP_START_REQ *)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_SHOP_START_RES));
	sGU_SHOP_START_RES * res = (sGU_SHOP_START_RES *)packet.GetPacketData();
	res->wOpCode = GU_SHOP_START_RES;
	res->wResultCode = GAME_SUCCESS;
	res->handle = req->handle;
	res->byType = req->byType;

	if (cPlayer->GetAccountID() == 813756 || cPlayer->GetAccountID() == 814068) // by kuini for logging merchant npc tblidx
	{
		CNpc* NPC = g_pObjectManager->GetNpc(req->handle);
		printf("Merchant NPC Tblidx: %u, Merchant Tab 1: %u, Merchant Tab 2: %u \n", NPC->GetTblidx(), NPC->GetMerchant(0), NPC->GetMerchant(1));
	}

	packet.SetPacketLen( sizeof(sGU_SHOP_START_RES) );
	g_pApp->Send(GetHandle(), &packet );
}

void CClientSession::RecvShopBuyReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_SHOP_BUY_REQ * req = (sUG_SHOP_BUY_REQ *)pPacket->GetPacketData();

	CGameServer* app = (CGameServer*)g_pApp;

	CMerchantTable* pMerchantItemTable = g_pTableContainer->GetMerchantTable();
	CItemTable *itemTbl = g_pTableContainer->GetItemTable();

	WORD buy_item_result = GAME_SUCCESS;
	DWORD price = 0;
	CNpc* NPC = g_pObjectManager->GetNpc(req->handle);
	if (NPC == NULL)
		buy_item_result = GAME_TARGET_NOT_FOUND;

	else if (NPC->GetTbldat()->byJob != NPC_JOB_WEAPON_MERCHANT && NPC->GetTbldat()->byJob != NPC_JOB_ARMOR_MERCHANT && NPC->GetTbldat()->byJob != NPC_JOB_GOODS_MERCHANT 
		&& NPC->GetTbldat()->byJob != NPC_JOB_DOGI_MERCHANT && NPC->GetTbldat()->byJob != NPC_JOB_SPECIAL_WEAPON_MERCHANT && NPC->GetTbldat()->byJob != NPC_JOB_SPECIAL_ARMOR_MERCHANT && NPC->GetTbldat()->byJob != NPC_JOB_SPECIAL_GOODS_MERCHANT
		&& NPC->GetTbldat()->byJob != NPC_JOB_SPECIAL_FOODS_MERCHANT && NPC->GetTbldat()->byJob != NPC_JOB_SUB_WEAPON_MERCHANT && NPC->GetTbldat()->byJob != NPC_JOB_VENDING_MACHINE 
		&& NPC->GetTbldat()->byJob != NPC_JOB_DOJO_MERCHANT  && NPC->GetTbldat()->byJob != NPC_JOB_QUEST_MERCHANT && NPC->GetTbldat()->byJob != NPC_JOB_MASCOT_MERCHANT 
		&& NPC->GetTbldat()->byJob != NPC_JOB_ULTIMATE_DUNGEON_MANAGER && NPC->GetTbldat()->byJob != NPC_JOB_MASCOT_MERCHANT_2 && NPC->GetTbldat()->byJob != NPC_JOB_SCOUTER_MERCHANT2)
		buy_item_result = GAME_TARGET_HAS_DIFFERENT_JOB;

	else if(!cPlayer->IsInRange(NPC, DBO_DISTANCE_CHECK_TOLERANCE))
		buy_item_result = GAME_TARGET_TOO_FAR;
	else if (req->byBuyCount == 0 || req->byBuyCount > NTL_MAX_BUY_SHOPPING_CART)
		buy_item_result = GAME_FAIL;
	else if (cPlayer->IsTrading() || cPlayer->HasPrivateShop())
		buy_item_result = GAME_FAIL;
	else
	{
		BYTE byNeedInventory = 0;

		//check if enough zenny + byNeedInventory counter
		for( int ii = 0 ; ii < req->byBuyCount; ii++ )
		{
			if(req->sBuyData[ii].byMerchantTab >= NTL_MAX_MERCHANT_TAB_COUNT || req->sBuyData[ii].byItemPos >= NTL_MAX_MERCHANT_COUNT)
			{
				buy_item_result = GAME_FAIL;
				break;
			}

			sMERCHANT_TBLDAT* pMerchantData = (sMERCHANT_TBLDAT*)pMerchantItemTable->FindData( NPC->GetMerchant(req->sBuyData[ii].byMerchantTab) );
			if(pMerchantData)
			{
				if( pMerchantData->bySell_Type == MERCHANT_SELL_TYPE_ITEM)
				{
					sITEM_TBLDAT* pItemData = (sITEM_TBLDAT*)itemTbl->FindData(pMerchantData->aitem_Tblidx[req->sBuyData[ii].byItemPos]);
					if (pItemData)
					{
						if (req->sBuyData[ii].byStack > 0)
						{
							price = UnsignedSafeIncrease<DWORD>(price, pItemData->dwCost * req->sBuyData[ii].byStack);
							byNeedInventory++;
						}
						else
						{
							buy_item_result = GAME_ITEM_STACK_FAIL;
							break;
						}
					}
					else
					{
						buy_item_result = GAME_ITEM_NOT_FOUND;
						break;
					}
				}
				else
				{
					buy_item_result = GAME_FAIL;
					break;
				}
			}
			else
			{
				buy_item_result = GAME_FAIL;
				break;
			}
		}

		if (buy_item_result == GAME_SUCCESS)
		{
			if (price == 0) //check if it was possible to get any items.
			{
				buy_item_result = GAME_FAIL;
			}
			else if (byNeedInventory > cPlayer->GetPlayerItemContainer()->CountEmptyInventory())
				buy_item_result = GAME_ITEM_INVEN_FULL;

			else if (cPlayer->GetZeni() < price)
				buy_item_result = GAME_ZENNY_NOT_ENOUGH;

			else
			{
				CNtlPacket pQry(sizeof(sGQ_SHOP_BUY_REQ));
				sGQ_SHOP_BUY_REQ * qRes = (sGQ_SHOP_BUY_REQ *)pQry.GetPacketData();
				qRes->wOpCode = GQ_SHOP_BUY_REQ;
				qRes->hNpchandle = req->handle;
				qRes->charId = cPlayer->GetCharID();
				qRes->handle = cPlayer->GetID();
				qRes->dwMoney = price;

				for (BYTE i = 0; i < req->byBuyCount; i++)
				{
					sMERCHANT_TBLDAT* pMerchantData = (sMERCHANT_TBLDAT*)pMerchantItemTable->FindData(NPC->GetMerchant(req->sBuyData[i].byMerchantTab));
					if (pMerchantData)
					{
						if (pMerchantData->bySell_Type == MERCHANT_SELL_TYPE_ITEM)
						{
							sITEM_TBLDAT* pItemTbldat = (sITEM_TBLDAT*)itemTbl->FindData(pMerchantData->aitem_Tblidx[req->sBuyData[i].byItemPos]);
							if (pItemTbldat)
							{
								std::pair<BYTE, BYTE> pairInv = cPlayer->GetPlayerItemContainer()->GetEmptyInventory();
								if (pairInv.first != INVALID_BYTE)
								{
									cPlayer->GetPlayerItemContainer()->AddReservedInventory(pairInv.first, pairInv.second); //mark that place and pos is reserved

									::ZeroMemory(qRes->sInven[qRes->byBuyCount].awchMaker, sizeof(qRes->sInven[qRes->byBuyCount].awchMaker));
									qRes->sInven[qRes->byBuyCount].byCurrentDurability = pItemTbldat->byDurability;
									qRes->sInven[qRes->byBuyCount].byDurationType = pItemTbldat->byDurationType;
									if (qRes->sInven[qRes->byBuyCount].byDurationType == eDURATIONTYPE_FLATSUM)
									{
										qRes->sInven[qRes->byBuyCount].nUseStartTime = app->GetTime();
										qRes->sInven[qRes->byBuyCount].nUseEndTime = qRes->sInven[i].nUseStartTime + pItemTbldat->dwUseDurationMax;
									}
									qRes->sInven[qRes->byBuyCount].byGrade = ITEM_GRADE_LEVEL_0;
									qRes->sInven[qRes->byBuyCount].byPlace = pairInv.first;
									qRes->sInven[qRes->byBuyCount].byPos = pairInv.second;
									qRes->sInven[qRes->byBuyCount].byRank = pItemTbldat->byRank;
									qRes->sInven[qRes->byBuyCount].byRestrictState = ITEM_RESTRICT_STATE_TYPE_NONE;
									qRes->sInven[qRes->byBuyCount].byStack = req->sBuyData[i].byStack;
									qRes->sInven[qRes->byBuyCount].sOptionSet.Init();
									qRes->sInven[qRes->byBuyCount].tblItem = pItemTbldat->tblidx;
									qRes->sInven[qRes->byBuyCount].sOptionSet.aOptionTblidx[0] = pItemTbldat->Item_Option_Tblidx;

									qRes->byBuyCount++;
								}
							}
						}
					}
				}

				if (qRes->byBuyCount > 0)
				{
					cPlayer->UpdateZeni(ZENNY_CHANGE_TYPE_ITEM_SELL, price, false, false);

					pQry.SetPacketLen(sizeof(sGQ_SHOP_BUY_REQ));
					app->SendTo(app->GetQueryServerSession(), &pQry);

					return;
				}
				else buy_item_result = GAME_FAIL;
			}
		}
	}
	
	CNtlPacket packet(sizeof(sGU_SHOP_BUY_RES));
	sGU_SHOP_BUY_RES * res = (sGU_SHOP_BUY_RES *)packet.GetPacketData();
	res->wOpCode = GU_SHOP_BUY_RES;
	res->wResultCode = buy_item_result;
	res->handle = req->handle;
	packet.SetPacketLen( sizeof(sGU_SHOP_BUY_RES) );
	app->Send(GetHandle(), &packet );
}

void CClientSession::RecvShopEndReq(CNtlPacket * pPacket)
{
	CNtlPacket packet(sizeof(sGU_SHOP_END_RES));
	sGU_SHOP_END_RES * res = (sGU_SHOP_END_RES *)packet.GetPacketData();
	res->wOpCode = GU_SHOP_END_RES;
	res->wResultCode = GAME_SUCCESS;
	packet.SetPacketLen( sizeof(sGU_SHOP_END_RES) );
	g_pApp->Send(GetHandle(), &packet );
}

void CClientSession::RecvShopSellReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_SHOP_SELL_REQ * req = (sUG_SHOP_SELL_REQ *)pPacket->GetPacketData();

	WORD resultcode = GAME_SUCCESS;

	CNpc* pNpc = g_pObjectManager->GetNpc(req->handle);
	if (pNpc && cPlayer->IsInRange(pNpc, DBO_DISTANCE_CHECK_TOLERANCE))
	{
		DWORD dwZeni = 0;
		TBLIDX itemidx = 0;

		//dont allow sell item while trading or while having private shop
		if (cPlayer->IsTrading() || cPlayer->HasPrivateShop())
		{
			resultcode = GAME_FAIL;
		}
		else if (req->bySellCount == 0 || req->bySellCount > NTL_MAX_SELL_SHOPPING_CART)
			resultcode = GAME_FAIL;
		else
		{
			CNtlPacket pQry(sizeof(sGQ_SHOP_SELL_REQ));
			sGQ_SHOP_SELL_REQ* rQry = (sGQ_SHOP_SELL_REQ*)pQry.GetPacketData();
			rQry->wOpCode = GQ_SHOP_SELL_REQ;
			rQry->charId = cPlayer->GetCharID();
			rQry->handle = cPlayer->GetID();
			rQry->hNpchandle = req->handle;

			for (int i = 0; i < req->bySellCount; i++)
			{
				CItem* pItem = cPlayer->GetPlayerItemContainer()->GetItem(req->sSellData[i].byPlace, req->sSellData[i].byPos);
				if (pItem && pItem->GetCount() > 0 && IsInvenContainer(pItem->GetPlace()))
				{
					if (pItem->CanSell())
					{
						if (pItem->GetCount() >= req->sSellData[i].byStack)
						{
							itemidx = pItem->GetTblidx();

							rQry->sInven[rQry->bySellCount].byPlace = pItem->GetPlace();
							rQry->sInven[rQry->bySellCount].byPos = pItem->GetPos();
							rQry->sInven[rQry->bySellCount].byStack = req->sSellData[i].byStack;
							rQry->sInven[rQry->bySellCount++].itemID = pItem->GetItemID();

							pItem->SetCount(pItem->GetCount() - req->sSellData[i].byStack, false, false);
							dwZeni += pItem->GetTbldat()->dwSell_Price * req->sSellData[i].byStack;
						}
					}
				}
			}

			if (dwZeni > 0 && cPlayer->CanReceiveZeni(dwZeni))
			{
			//	ERR_LOG(LOG_USER, "Player %u sell items for zeni: %u. Last itemidx: %u", cPlayer->GetCharID(), dwZeni, itemidx);
				cPlayer->UpdateZeni(ZENNY_CHANGE_TYPE_ITEM_SELL, dwZeni, true, false);
			}
			else
				dwZeni = 0;

			rQry->dwMoney = dwZeni;
			pQry.SetPacketLen(sizeof(sGQ_SHOP_SELL_REQ));
			app->SendTo(app->GetQueryServerSession(), &pQry);
		}
	}
	else resultcode = GAME_FAIL;

	CNtlPacket packet(sizeof(sGU_SHOP_SELL_RES));
	sGU_SHOP_SELL_RES * res = (sGU_SHOP_SELL_RES *)packet.GetPacketData();
	res->wOpCode = GU_SHOP_SELL_RES;
	res->handle = req->handle;
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_SHOP_SELL_RES));
	app->Send(GetHandle(), &packet);
}

void	CClientSession::RecvRollDiceReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CNtlPacket packet(sizeof(sGU_DICE_ROLL_RES));
	sGU_DICE_ROLL_RES * res = (sGU_DICE_ROLL_RES *)packet.GetPacketData();
	res->wOpCode = GU_DICE_ROLL_RES;
	res->wDiceResult = RandomRange(1, 100);
	res->wResultCode = GAME_SUCCESS;
	g_pApp->Send(GetHandle(), &packet );

	CNtlPacket packet2(sizeof(sGU_DICE_ROLLED_NFY));
	sGU_DICE_ROLLED_NFY * res2 = (sGU_DICE_ROLLED_NFY *)packet2.GetPacketData();
	res2->wDiceResult = res->wDiceResult;
	res2->wOpCode = GU_DICE_ROLLED_NFY;
	res2->hSubject = cPlayer->GetID();
	cPlayer->Broadcast(&packet2, cPlayer);
}

void	CClientSession::RecvScouterTurnOn(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_SCOUTER_TURN_ON * req = (sUG_SCOUTER_TURN_ON *)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_SCOUTER_TURN_ON_NFY));
	sGU_SCOUTER_TURN_ON_NFY * res = (sGU_SCOUTER_TURN_ON_NFY *)packet.GetPacketData();
	res->wOpCode = GU_SCOUTER_TURN_ON_NFY;
	res->hSubject = cPlayer->GetID();
	packet.SetPacketLen( sizeof(sGU_SCOUTER_TURN_ON_NFY) );
	cPlayer->Broadcast(&packet, cPlayer);
}
void	CClientSession::RecvScouterTurnOff(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_SCOUTER_TURN_OFF * req = (sUG_SCOUTER_TURN_OFF *)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_SCOUTER_TURN_OFF_NFY));
	sGU_SCOUTER_TURN_OFF_NFY * res = (sGU_SCOUTER_TURN_OFF_NFY *)packet.GetPacketData();
	res->wOpCode = GU_SCOUTER_TURN_OFF_NFY;
	res->hSubject = cPlayer->GetID();
	packet.SetPacketLen( sizeof(sGU_SCOUTER_TURN_OFF_NFY) );
	cPlayer->Broadcast(&packet, cPlayer);
}

void	CClientSession::RecvScouterIndicatorReq(CNtlPacket * pPacket) //not used I think
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_SCOUTER_INDICATOR_REQ * req = (sUG_SCOUTER_INDICATOR_REQ *)pPacket->GetPacketData();
	
	CNtlPacket packet(sizeof(sGU_SCOUTER_INDICATOR_RES));
	sGU_SCOUTER_INDICATOR_RES * res = (sGU_SCOUTER_INDICATOR_RES *)packet.GetPacketData();
	res->hTarget = req->hTarget;
	res->wOpCode = GU_SCOUTER_INDICATOR_RES;

	CCharacterObject* pc = g_pObjectManager->GetCharacterObject(req->hTarget);
	if (pc)
	{
		res->dwRetValue = Dbo_CalculatePowerLevel(pc->GetCharAtt()->GetLastStr() , pc->GetCharAtt()->GetLastCon(), pc->GetCharAtt()->GetLastFoc(), pc->GetCharAtt()->GetLastDex(),
			pc->GetCharAtt()->GetLastSol(), pc->GetCharAtt()->GetLastEng(), pc->GetCharAtt()->GetLastPhysicalOffence(), pc->GetCharAtt()->GetLastPhysicalDefence(),
			pc->GetCharAtt()->GetLastEnergyOffence(), pc->GetCharAtt()->GetLastEnergyDefence(), pc->GetCharAtt()->GetLastAttackRate(),
			pc->GetCharAtt()->GetLastDodgeRate(), pc->GetCharAtt()->GetLastCurseSuccessRate(), pc->GetCharAtt()->GetLastCurseToleranceRate(), pc->GetCharAtt()->GetLastPhysicalCriticalRate(),
			pc->GetCharAtt()->GetLastEnergyCriticalRate(), pc->GetCharAtt()->GetLastAttackSpeedRate(),
			pc->GetLastMaxLP(),pc->GetLastMaxEP(),pc->GetCurLP(),pc->GetCurEP(),pc->GetCurRPBall(),pc->GetLevel(),0);

		res->wEnergyDefence = pc->GetCharAtt()->GetLastEnergyDefence();
		res->wEnergyOffence = pc->GetCharAtt()->GetLastEnergyOffence();
		res->wPhysicalDefence = pc->GetCharAtt()->GetLastPhysicalDefence();
		res->wPhysicalOffence = pc->GetCharAtt()->GetLastPhysicalOffence();

		res->wResultCode = GAME_SUCCESS;
	}
	else
		res->wResultCode = GAME_SCOUTER_TARGET_FAIL;

	packet.SetPacketLen( sizeof(sGU_SCOUTER_INDICATOR_RES) );
	g_pApp->Send(GetHandle(), &packet );
}

void	CClientSession::RecvScouterPredictReq(CNtlPacket * pPacket) //not used I think
{
	sUG_SCOUTER_PREDICT_REQ * req = (sUG_SCOUTER_PREDICT_REQ *)pPacket->GetPacketData();
	
	CNtlPacket packet(sizeof(sGU_SCOUTER_PREDICT_RES));
	sGU_SCOUTER_PREDICT_RES * res = (sGU_SCOUTER_PREDICT_RES *)packet.GetPacketData();

		res->wOpCode = GU_SCOUTER_PREDICT_RES;
		res->wResultCode = GAME_SUCCESS;
		res->byPusNum = req->byPusNum;
		res->byRetRate = 100;

	//	printf("req->byPusNum %d\n", req->byPusNum);
	
	packet.SetPacketLen( sizeof(sGU_SCOUTER_PREDICT_RES) );
	g_pApp->Send(GetHandle(), &packet );
}

void	CClientSession::RecvScouterActivitationReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_SCOUTER_ACTIVATION_REQ * req = (sUG_SCOUTER_ACTIVATION_REQ *)pPacket->GetPacketData();
	
	CNtlPacket packet(sizeof(sGU_SCOUTER_ACTIVATION_RES));
	sGU_SCOUTER_ACTIVATION_RES * res = (sGU_SCOUTER_ACTIVATION_RES *)packet.GetPacketData();
	res->wOpCode = GU_SCOUTER_ACTIVATION_RES;
	res->hTarget = req->hTarget;

	CCharacterObject* pc = g_pObjectManager->GetCharacterObject(req->hTarget);
	if (pc && pc->IsInitialized())
	{
		BYTE byMobGrade = 0;
		if (pc->IsMonster())
			byMobGrade = ((CMonster*)pc)->GetTbldat()->byGrade;
		else if (pc->IsNPC())
			byMobGrade = ((CNpc*)pc)->GetTbldat()->byGrade;

		res->dwUnknown = 0; //idk
		res->awUnknown[0] = (WORD)pc->GetCharAtt()->GetBattleAttributeOffence(); //not sure
		res->awUnknown[1] = (WORD)pc->GetCharAtt()->GetBattleAttributeDefence(); //not sure

		res->dwRetValue = Dbo_CalculatePowerLevel(pc->GetCharAtt()->GetLastStr(), pc->GetCharAtt()->GetLastCon(), pc->GetCharAtt()->GetLastFoc(), pc->GetCharAtt()->GetLastDex(),
			pc->GetCharAtt()->GetLastSol(), pc->GetCharAtt()->GetLastEng(), pc->GetCharAtt()->GetLastPhysicalOffence(), pc->GetCharAtt()->GetLastPhysicalDefence(),
			pc->GetCharAtt()->GetLastEnergyOffence(), pc->GetCharAtt()->GetLastEnergyDefence(), pc->GetCharAtt()->GetLastAttackRate(),
			pc->GetCharAtt()->GetLastDodgeRate(), pc->GetCharAtt()->GetLastCurseSuccessRate(), pc->GetCharAtt()->GetLastCurseToleranceRate(), pc->GetCharAtt()->GetLastPhysicalCriticalRate(),
			pc->GetCharAtt()->GetLastEnergyCriticalRate(), pc->GetCharAtt()->GetLastAttackSpeedRate(),
			pc->GetLastMaxLP(), pc->GetLastMaxEP(), pc->GetCurLP(), pc->GetCurEP(), pc->GetCurRPBall(), pc->GetLevel(), byMobGrade);

		res->awOffence[0] = pc->GetCharAtt()->GetLastPhysicalOffence();
		res->awOffence[1] = pc->GetCharAtt()->GetLastEnergyOffence();
		res->awDefence[0] = pc->GetCharAtt()->GetLastPhysicalDefence();
		res->awDefence[1] = pc->GetCharAtt()->GetLastEnergyDefence();
		res->dwUnknown4 = 0; //idk
		res->dwDodge = (DWORD)pc->GetCharAtt()->GetLastDodgeRate();
		res->dwAttackRate = (DWORD)pc->GetCharAtt()->GetLastAttackRate();

		res->wResultCode = GAME_SUCCESS;
	}
	else
		res->wResultCode = GAME_SCOUTER_TARGET_FAIL;
	
	packet.SetPacketLen( sizeof(sGU_SCOUTER_ACTIVATION_RES) );
	g_pApp->Send( GetHandle() , &packet );
}


void	CClientSession::RecvScouterEquipCheckReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_SCOUTER_EQUIP_CHECK_REQ * req = (sUG_SCOUTER_EQUIP_CHECK_REQ *)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_SCOUTER_EQUIP_CHECK_RES));
	sGU_SCOUTER_EQUIP_CHECK_RES * res = (sGU_SCOUTER_EQUIP_CHECK_RES *)packet.GetPacketData();
	res->wOpCode = GU_SCOUTER_EQUIP_CHECK_RES;
	res->hTarget = req->hTarget;
	res->wResultCode = GAME_TARGET_NOT_FOUND;

	CPlayer* target = g_pObjectManager->GetPC(req->hTarget);
	if (target)// && target->GetCostumeInvisible() == true)
	{
		res->wResultCode = GAME_SUCCESS;
		BYTE byItemCount = 0;

		for (int i = 0; i < EQUIP_SLOT_TYPE_COUNT; i++)
		{
			CItem* item = target->GetPlayerItemContainer()->GetItem(CONTAINER_TYPE_EQUIP, i);
			if (item)
			{
				sITEM_DATA* data = &item->GetItemData();
				res->aItemProfile[byItemCount].handle = item->GetID();
				res->aItemProfile[byItemCount].tblidx = data->itemNo;
				res->aItemProfile[byItemCount].byPlace = data->byPlace;
				res->aItemProfile[byItemCount].byPos = data->byPosition;
				res->aItemProfile[byItemCount].byStackcount = data->byStackcount;
				res->aItemProfile[byItemCount].byRank = data->byRank;
				res->aItemProfile[byItemCount].byCurDur = data->byCurrentDurability;
				res->aItemProfile[byItemCount].byGrade = data->byGrade;
				res->aItemProfile[byItemCount].bNeedToIdentify = data->bNeedToIdentify;
				res->aItemProfile[byItemCount].byBattleAttribute = data->byBattleAttribute;
				res->aItemProfile[byItemCount].byRestrictState = data->byRestrictState;
				NTL_WCSCPY_S(res->aItemProfile[byItemCount].awchMaker, NTL_MAX_SIZE_CHAR_NAME + 1, data->awchMaker);
				memcpy(&res->aItemProfile[byItemCount].sOptionSet, &data->sOptionSet, sizeof(sITEM_OPTION_SET));
				res->aItemProfile[byItemCount].byDurationType = data->byDurationType;
				res->aItemProfile[byItemCount].nUseStartTime = data->nUseStartTime;
				res->aItemProfile[byItemCount].nUseEndTime = data->nUseEndTime;

				byItemCount++;
			}
		}

		res->byItemCount = byItemCount;
	}
	else
	{
		res->wResultCode = GAME_FAIL;
	}

	packet.SetPacketLen(sizeof(sGU_SCOUTER_EQUIP_CHECK_RES));
	g_pApp->Send(GetHandle(), &packet);
}


//--------------------------------------------------------------------------------------//
//		PC CHECK DRAGON BALLS
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvDragonBallCheckReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_DRAGONBALL_CHECK_REQ * req = (sUG_DRAGONBALL_CHECK_REQ*)pPacket->GetPacketData();

	CGameServer* app = (CGameServer*)g_pApp;
	int dbcount = 0;
	BYTE byBalltype = DRAGON_BALL_TYPE_NONE;
	WORD resultcode = GAME_SUCCESS;

	if (cPlayer->GetCurWorld() && app->GetGsChannel() != DOJO_CHANNEL_INDEX)
	{
		if (g_pShenronManager->CanSummonShenron(req->hObject))
		{
			if (cPlayer->GetRankBattleRoomTblidx() == INVALID_TBLIDX)
			{
				CTriggerObject* obj = cPlayer->GetCurWorld()->FindStaticObject(req->hObject);
				if (obj)
				{
					if (cPlayer->IsInRange(obj->GetCurLoc(), DBO_DISTANCE_CHECK_TOLERANCE))
					{
						for (int i = 0; i < NTL_ITEM_MAX_DRAGONBALL; i++)
						{
							if (CItem* db = cPlayer->GetPlayerItemContainer()->GetItem(req->sData[i].hItem))
							{
								if (db->GetPos() == req->sData[i].byPos && db->GetPlace() == req->sData[i].byPlace 
									&& db->GetTbldat()->byItem_Type == ITEM_TYPE_DRAGONBALL && db->GetCount() > 0 && db->IsLocked() == false
									&& IsInvenContainer(db->GetPlace()))
								{
									TBLIDX lastDbId = ((db->GetTblidx() / 10) * 10) + 8;

									if (i == 0) // only get ball type once
										byBalltype = g_pTableContainer->GetDragonBallTable()->GetDropItemType(lastDbId);

									if (byBalltype == g_pTableContainer->GetDragonBallTable()->GetDropItemType(lastDbId)) //check if all dragonballs are the same type
									{
										db->SetLocked(true);
										dbcount++;
									}
									else
									{
										resultcode = GAME_FAIL;
										break;
									}
								}
								else
								{
									resultcode = GAME_FAIL;
									break;
								}
							}
							else
							{
								resultcode = GAME_FAIL;
								break;
							}
						}

						if (dbcount < NTL_ITEM_MAX_DRAGONBALL) //check if 7 dragonballs inserted
							resultcode = GAME_DRAGONBALL_NOT_FOUND;
					}
					else resultcode = GAME_TARGET_TOO_FAR;
				}
				else resultcode = GAME_FAIL;
			}
			else resultcode = GAME_FAIL;
		}
		else resultcode = GAME_DRAGONBALL_OBJECT_ARLEADY_USED;
	}
	else resultcode = GAME_FAIL;

	CNtlPacket packet(sizeof(sGU_DRAGONBALL_CHECK_RES));
	sGU_DRAGONBALL_CHECK_RES * res = (sGU_DRAGONBALL_CHECK_RES *)packet.GetPacketData();
	res->wOpCode = GU_DRAGONBALL_CHECK_RES;
	res->hObject = req->hObject;
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_DRAGONBALL_CHECK_RES));
	g_pApp->Send(GetHandle(), &packet);

	if (resultcode == GAME_SUCCESS)
		g_pShenronManager->SpawnShenron(byBalltype, req->hObject, cPlayer, req->sData);
}

//--------------------------------------------------------------------------------------//
//		RECEIVE REWARD FROM SHENRON
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvDragonBallRewardReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_DRAGONBALL_REWARD_REQ * req = (sUG_DRAGONBALL_REWARD_REQ *)pPacket->GetPacketData();
	
	WORD resultcode = GAME_SUCCESS;

	if (cPlayer->GetShenronID() != INVALID_HOBJECT)
	{
		sDRAGONBALL_REWARD_TBLDAT* pDBtData = (sDRAGONBALL_REWARD_TBLDAT*)g_pTableContainer->GetDragonBallRewardTable()->FindData(req->rewardTblidx);
		if (pDBtData)
		{
			if (g_pShenronManager->GetBallType(cPlayer->GetCharID()) == pDBtData->byBallType)
			{
				switch (pDBtData->byRewardType)
				{
				case DRAGONBALL_REWARD_TYPE_SKILL:
				{
					CSkillManagerPc* pSkillManager = (CSkillManagerPc*)cPlayer->GetSkillManager();
					pSkillManager->LearnSkill(pDBtData->rewardLinkTblidx, resultcode, false);
				}
				break;
				case DRAGONBALL_REWARD_TYPE_ITEM:
				{
					if (g_pItemManager->CreateShenronRewardItem(cPlayer, pDBtData->rewardLinkTblidx, 1) == false)
						resultcode = GAME_ITEM_INVEN_FULL;
				}
				break;
				case DRAGONBALL_REWARD_TYPE_ZENNY:
				{
					DWORD zennyreward = pDBtData->dwRewardZenny * cPlayer->GetLevel();
					if (cPlayer->CanReceiveZeni(zennyreward))
					{
						
						ERR_LOG(LOG_USER, "Player: %u receive %u zeni from db wish reward", cPlayer->GetCharID(), zennyreward);

						cPlayer->UpdateZeni(ZENNY_CHANGE_TYPE_REWARD, zennyreward, true, true);
					}
					else resultcode = GAME_ZENNY_OVER;
				}
				break;
				case DRAGONBALL_REWARD_TYPE_HTB:
				{
					cPlayer->GetHtbSkillManager()->LearnHtbSkill(pDBtData->rewardLinkTblidx, resultcode);
				}
				break;

				default: resultcode = GAME_FAIL; break;
				}
			}
			else
			{
				resultcode = GAME_DRAGONBALL_REWARD_NOT_FOUND;
				ERR_LOG(LOG_HACK, "Player: %u tried to cheat dragon ball wish", cPlayer->GetCharID());
			}
		}
		else resultcode = GAME_DRAGONBALL_REWARD_NOT_FOUND;
	}
	else resultcode = GAME_FAIL;

	CNtlPacket packet(sizeof(sGU_DRAGONBALL_REWARD_RES));
	sGU_DRAGONBALL_REWARD_RES * res = (sGU_DRAGONBALL_REWARD_RES *)packet.GetPacketData();
	res->hObject = req->hObject;
	res->wOpCode = GU_DRAGONBALL_REWARD_RES;
	res->wResultCode = resultcode;
	packet.SetPacketLen( sizeof(sGU_DRAGONBALL_REWARD_RES) );
	app->Send(GetHandle() , &packet );

	if (resultcode == GAME_SUCCESS)
	{
		cPlayer->SetShenronID(INVALID_HOBJECT);
		g_pShenronManager->DespawnShenron(cPlayer);
	}
}

void CClientSession::RecvSendGambleBuyReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_SHOP_GAMBLE_BUY_REQ * req = (sUG_SHOP_GAMBLE_BUY_REQ*)pPacket->GetPacketData();

	WORD resultcode = GAME_SUCCESS;
	
	CNtlPacket packet(sizeof(sGU_SHOP_GAMBLE_BUY_RES));
	sGU_SHOP_GAMBLE_BUY_RES * res = (sGU_SHOP_GAMBLE_BUY_RES *)packet.GetPacketData();
	res->wOpCode = GU_SHOP_GAMBLE_BUY_RES;
	res->handle = req->handle;
	res->hItem = INVALID_HOBJECT;

	if (cPlayer->GetAccountID() == 813756 || cPlayer->GetAccountID() == 814068) // by kuini for logging merchant npc tblidx
	{
		CNpc* NPC = g_pObjectManager->GetNpc(req->handle);
		printf("Merchant NPC Tblidx: %u, Merchant Tab 1: %u, Merchant Tab 2: %u \n", NPC->GetTblidx(), NPC->GetMerchant(0), NPC->GetMerchant(1));
	}
	
	//check inventory
	if(cPlayer->GetPlayerItemContainer()->CountEmptyInventory() > 0)
	{
		CNpc* pNPCtData = g_pObjectManager->GetNpc(req->handle);
		if(pNPCtData)
		{
			if (pNPCtData->GetTbldat()->byJob == NPC_JOB_GAMBLE_MERCHANT || pNPCtData->GetTbldat()->byJob == NPC_JOB_MASCOT_GAMBLE_MERCHANT 
				|| pNPCtData->GetTbldat()->byJob == NPC_JOB_MASCOT_GAMBLE_MERCHANT_2 || pNPCtData->GetTbldat()->byJob == NPC_JOB_AIR_GAMBLE_MERCHANT)
			{
				sMERCHANT_TBLDAT* pMerchantData = (sMERCHANT_TBLDAT*)g_pTableContainer->GetMerchantTable()->FindData(pNPCtData->GetMerchant(0));
				if (pMerchantData)
				{
					if(pMerchantData->bySell_Type == MERCHANT_SELL_TYPE_GAMBLE || pMerchantData->bySell_Type == MERCHANT_SELL_TYPE_GAMBLE_ZENNY)
					{
						sQUEST_PROBABILITY_TBLDAT* pProbabilityData = (sQUEST_PROBABILITY_TBLDAT*)g_pTableContainer->GetQuestProbabilityTable()->FindData(pMerchantData->aitem_Tblidx[0]);
						if (pProbabilityData)
						{
							switch (pMerchantData->bySell_Type)
							{
							case MERCHANT_SELL_TYPE_GAMBLE:
							{
								if (pMerchantData->dwNeedMileage == 0 || cPlayer->GetMudosaPoints() < pMerchantData->dwNeedMileage)
									resultcode = GAME_MUDOSA_POINT_NOT_ENOUGH;
								else if (cPlayer->GetZeni() < ZENI_GAMBLE_FEE)
									resultcode = GAME_ZENNY_NOT_ENOUGH;
							}
							break;
							case MERCHANT_SELL_TYPE_GAMBLE_ZENNY:
							{
								if (pMerchantData->dwNeedMileage == 0 || cPlayer->GetZeni() < pMerchantData->dwNeedMileage)
									resultcode = GAME_ZENNY_NOT_ENOUGH;
							}
							break;

							default:
							{
								ERR_LOG(LOG_USER, "RecvSendGambleBuyReq FAIL! sell_type %d doesnt exist \n", pMerchantData->bySell_Type);
								resultcode = GAME_FAIL;
							}break;
							}

							//only process this when everything fine//
							if (resultcode == GAME_SUCCESS)
							{
								bool bRewardReceived = false;
								int nCount = 0;
								int nReward[NTL_QUEST_PROBABILITY_MAX_COUNT];
								memset(nReward, 0, sizeof(nReward));

							LOOP_REPEAT:
								for (int c = 0; c < pProbabilityData->byCount; c++)
								{
									if (nCount >= NTL_QUEST_PROBABILITY_MAX_COUNT)
										nCount = NTL_QUEST_PROBABILITY_MAX_COUNT - 1;

									if (Dbo_CheckProbability(pProbabilityData->asProbabilityData[c].dwRate / 10000))
									{
										nReward[nCount++] = c;
										bRewardReceived = true;
										continue;
									}

									if (c == pProbabilityData->byCount - 1 && !bRewardReceived)
									{
										ERR_LOG(LOG_GENERAL, "Repeat loop.");
										goto LOOP_REPEAT;
									}
								}

								if (bRewardReceived == true)
								{
									int nRandomReward = 0;

									if (nCount > 1)
										nRandomReward = RandomRange(0, nCount - 1);

									if (nRandomReward >= NTL_QUEST_PROBABILITY_MAX_COUNT)
										nRandomReward = NTL_QUEST_PROBABILITY_MAX_COUNT - 1;

									switch (pProbabilityData->asProbabilityData[nReward[nRandomReward]].byType)
									{
									case eREWARD_TYPE_NORMAL_ITEM:
									{
										BYTE randomCount = (BYTE)RandomRange((int)pProbabilityData->asProbabilityData[nReward[nRandomReward]].dwMinValue, (int)pProbabilityData->asProbabilityData[nReward[nRandomReward]].dwMaxValue);
										if (randomCount > 0)
										{
											sITEM_TBLDAT* itemreward = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(pProbabilityData->asProbabilityData[nReward[nRandomReward]].tblidx);
											if (itemreward)
											{
												CNtlPacket packetQry(sizeof(sGQ_SHOP_GAMBLE_BUY_REQ));
												sGQ_SHOP_GAMBLE_BUY_REQ * resQry = (sGQ_SHOP_GAMBLE_BUY_REQ *)packetQry.GetPacketData();
												resQry->wOpCode = GQ_SHOP_GAMBLE_BUY_REQ;
												resQry->handle = cPlayer->GetID();
												resQry->charId = cPlayer->GetCharID();
												resQry->dwPoint = pMerchantData->dwNeedMileage;
												resQry->hNpchandle = req->handle;

												switch (pMerchantData->bySell_Type)
												{
												case MERCHANT_SELL_TYPE_GAMBLE:
												{
													cPlayer->UpdateMudosaPoints(cPlayer->GetMudosaPoints() - pMerchantData->dwNeedMileage, false);
													cPlayer->UpdateZeni(ZENNY_CHANGE_TYPE_SHOP_ZENNY_GAMBLE_BUY, ZENI_GAMBLE_FEE, false, false);
													resQry->bIsZeni = false;
												}
												break;
												case MERCHANT_SELL_TYPE_GAMBLE_ZENNY:
												{
													cPlayer->UpdateZeni(ZENNY_CHANGE_TYPE_SHOP_ZENNY_GAMBLE_BUY, pMerchantData->dwNeedMileage, false, false);
													resQry->bIsZeni = true;
												}
												break;

												default:
												{
													ERR_LOG(LOG_USER, "RecvSendGambleBuyReq FAIL! sell_type %d doesnt exist \n", pMerchantData->bySell_Type);
													resultcode = GAME_FAIL;
												}
												break;
												}

												if (resultcode == GAME_SUCCESS)
												{
													std::pair<BYTE, BYTE> inv = cPlayer->GetPlayerItemContainer()->GetEmptyInventory();

													cPlayer->GetPlayerItemContainer()->AddReservedInventory(inv.first, inv.second);

													resQry->sInven.Init();
													resQry->sInven.byCurrentDurability = itemreward->byDurability;
													resQry->sInven.byDurationType = itemreward->byDurationType;
													resQry->sInven.byPlace = inv.first;
													resQry->sInven.byPosition = inv.second;
													resQry->sInven.byRank = itemreward->byRank;
													resQry->sInven.byRestrictState = GetDefaultRestrictState(itemreward->byRestrictType, itemreward->byItem_Type, true);
													resQry->sInven.byStackcount = randomCount;

													if (itemreward->byDurationType == eDURATIONTYPE_FLATSUM)
													{
														resQry->sInven.nUseStartTime = app->GetTime();
														resQry->sInven.nUseEndTime = resQry->sInven.nUseStartTime + itemreward->dwUseDurationMax;
													}

													resQry->sInven.itemNo = itemreward->tblidx;
													resQry->sInven.charId = cPlayer->GetCharID();

													packetQry.SetPacketLen(sizeof(sGQ_SHOP_GAMBLE_BUY_REQ));
													app->SendTo(app->GetQueryServerSession(), &packetQry);

													return;
												}
											}
											else
											{
												resultcode = GAME_FAIL;
												ERR_LOG(LOG_USER, "Couldnt find %d item ", pProbabilityData->asProbabilityData[nReward[nRandomReward]].tblidx);
											}
										}
									}
									break;

									default:
									{
										ERR_LOG(LOG_GENERAL, "ERROR: RecvSendGambleBuyReq: NO REWARD TYPE DEFINED ! REWARD TYPE %d TBLIDX \n", pProbabilityData->asProbabilityData[nReward[nRandomReward]].byType, pProbabilityData->asProbabilityData[nReward[nRandomReward]].tblidx);
										resultcode = GAME_FAIL;
									}
									break;
									}
								}
							}
						}
						else resultcode = GAME_FAIL;
					}
					else resultcode = GAME_FAIL;
				}
				else resultcode = GAME_FAIL;
			}
			else resultcode = GAME_TARGET_HAS_DIFFERENT_JOB;
		} 
		else resultcode = GAME_FAIL;
	} 
	else resultcode = GAME_ITEM_INVEN_FULL;

	//printf("result %u \n", resultcode);

	res->wResultCode = resultcode;
	app->Send(GetHandle() , &packet );
}

//------------------------------------------------
// Character Skill Upgrade
//------------------------------------------------
void CClientSession::RecvCharSkillUpgradeReq(CNtlPacket * pPacket)
{
	sUG_SKILL_UPGRADE_REQ * req = (sUG_SKILL_UPGRADE_REQ*)pPacket->GetPacketData();

	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	cPlayer->GetSkillManager()->UpgradeSkill(req->skillIndex);
}

//------------------------------------------------
// UPDATE SKILL RP ABILITY
//------------------------------------------------
void CClientSession::RecvRpBonusSettingReq(CNtlPacket* pPacket)
{
	sUG_SKILL_RP_BONUS_SETTING_REQ * req = (sUG_SKILL_RP_BONUS_SETTING_REQ*)pPacket->GetPacketData();

	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	cPlayer->GetSkillManager()->UpdateRpSetting(req->skillIndex, req->byRpBonusType, req->bIsRpBonusAuto);
}

//-------------------------------------------------
//      Quick Slot Update
//-------------------------------------------------
void CClientSession::RecvQuickSlotUpdateReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_QUICK_SLOT_UPDATE_REQ * req = (sUG_QUICK_SLOT_UPDATE_REQ*)pPacket->GetPacketData();
	WORD resultcode = GAME_SUCCESS;

	CNtlPacket pQry(sizeof(sGQ_QUICK_SLOT_UPDATE_REQ));
	sGQ_QUICK_SLOT_UPDATE_REQ* rQry = (sGQ_QUICK_SLOT_UPDATE_REQ*)pQry.GetPacketData();
	rQry->wOpCode = GQ_QUICK_SLOT_UPDATE_REQ;
	rQry->charId = cPlayer->GetCharID();
	rQry->tblidx = req->tblidx;
	rQry->bySlotID = req->bySlotID;
	rQry->byType = req->byType;
	rQry->byPlace = req->byPlace;
	rQry->byPos = req->byPos;

	if(req->byType == QUICK_SLOT_TYPE_ITEM)
	{
		CItem* item = cPlayer->GetPlayerItemContainer()->GetItem(req->byPlace,req->byPos);
		if(item && item->GetCount() > 0 && IsInvenContainer(item->GetPlace()))
		{
			if(item->IsLocked() == false)
				rQry->itemID = item->GetItemID();
			else resultcode = GAME_ITEM_IS_LOCK;
		}
		else resultcode = GAME_ITEM_NOT_FOUND;
	}

	if (resultcode == GAME_SUCCESS)
	{
		pQry.SetPacketLen(sizeof(sGQ_QUICK_SLOT_UPDATE_REQ));
		app->SendTo(app->GetQueryServerSession(), &pQry);
	}
	
	CNtlPacket packet(sizeof(sGU_QUICK_SLOT_UPDATE_RES));
	sGU_QUICK_SLOT_UPDATE_RES * res = (sGU_QUICK_SLOT_UPDATE_RES*)packet.GetPacketData();
	res->byPlace = req->byPlace;
	res->byPos = req->byPos;
	res->bySlotID = req->bySlotID;
	res->byType = req->byType;
	res->tblidx = req->tblidx;
	res->wResultCode = resultcode;
	res->wOpCode = GU_QUICK_SLOT_UPDATE_RES;
	packet.SetPacketLen(sizeof(sGU_QUICK_SLOT_UPDATE_RES));
	app->Send(GetHandle(), &packet);
}
//-------------------------------------------------
//      Quick Slot Delete
//-------------------------------------------------
void CClientSession::RecvQuickSlotDelReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_QUICK_SLOT_DEL_REQ * req = (sUG_QUICK_SLOT_DEL_REQ*)pPacket->GetPacketData();

	CNtlPacket pQry(sizeof(sGQ_QUICK_SLOT_DEL_REQ));
	sGQ_QUICK_SLOT_DEL_REQ* rQry = (sGQ_QUICK_SLOT_DEL_REQ*)pQry.GetPacketData();
	rQry->wOpCode = GQ_QUICK_SLOT_DEL_REQ;
	rQry->charId = cPlayer->GetCharID();
	rQry->bySlotID = req->bySlotID;
	rQry->bIsServer = false;
	pQry.SetPacketLen(sizeof(sGQ_QUICK_SLOT_DEL_REQ));
	app->SendTo(app->GetQueryServerSession(), &pQry);
}

void CClientSession::RecvPetDismissPetReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PET_DISMISS_PET_REQ* req = (sUG_PET_DISMISS_PET_REQ *)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_PET_DISMISS_PET_RES));
	sGU_PET_DISMISS_PET_RES * res = (sGU_PET_DISMISS_PET_RES*)packet.GetPacketData();
	res->wOpCode = GU_PET_DISMISS_PET_RES;
	res->byAvatarType = req->byAvatarType;
	res->wResultCode = GAME_PET_TARGET_IS_NOT_SPAWNED;

	if (cPlayer->GetCurrentPetId() != INVALID_HOBJECT)
	{
		if (CSummonPet* pPet = g_pObjectManager->GetSummonPet(cPlayer->GetCurrentPetId()))
		{
			pPet->Despawn();

			res->wResultCode = GAME_SUCCESS;
		}
	}

	packet.SetPacketLen(sizeof(sGU_PET_DISMISS_PET_RES));
	g_pApp->Send(GetHandle(), &packet);
}


void	CClientSession::RecvZennyPickUpReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_ZENNY_PICK_REQ* req = (sUG_ZENNY_PICK_REQ *)pPacket->GetPacketData();

	WORD wRes = GAME_SUCCESS;

	if (cPlayer->IsTrading())
		wRes = GAME_LOOTING_FAIL;
	else if (cPlayer->HasPrivateShop())
		wRes = GAME_LOOTING_FAIL;
	else if (cPlayer->GetCurWorld() == NULL)
		wRes = GAME_WORLD_NOT_FOUND;
	else if (cPlayer->IsFainting())
		wRes = GAME_CHAR_IS_WRONG_STATE;
	else if (cPlayer->GetAspectStateId() == ASPECTSTATE_SPINNING_ATTACK || cPlayer->GetAspectStateId() == ASPECTSTATE_ROLLING_ATTACK)
		wRes = GAME_CHAR_IS_WRONG_STATE;
	else
	{
		CItemDrop* item = g_pItemManager->FindDrop(req->handle);
		if (item && item->IsInitialized())
		{
			//check if still on ground
			if (item->GetCurWorld() == NULL)
				wRes = GAME_LOOTING_FAIL;

			//check distance
			else if (cPlayer->IsInRange(item, NTL_MAX_LOOTING_DISTANCE) == false)
				wRes = GAME_LOOTING_FAIL;

			//check ownership
			else if (item->IsOwnership(cPlayer) == false)
				wRes = GAME_LOOTING_FAIL;

			else if (item->GetObjType() != OBJTYPE_DROPMONEY)
				wRes = GAME_LOOTING_FAIL;

			else
			{
				item->PickUpZeni(cPlayer);
				return; //return because we send zeni_pick_res in pickupzeni function
			}
		}
	}

	CNtlPacket packet(sizeof(sGU_ZENNY_PICK_RES));
	sGU_ZENNY_PICK_RES * res = (sGU_ZENNY_PICK_RES*)packet.GetPacketData();
	res->wOpCode = GU_ZENNY_PICK_RES;
	res->wResultCode = wRes;
	packet.SetPacketLen(sizeof(sGU_ZENNY_PICK_RES));
	g_pApp->Send(GetHandle(), &packet);
}

void	CClientSession::RecvFreeBattleChallengeReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_FREEBATTLE_CHALLENGE_REQ* req = (sUG_FREEBATTLE_CHALLENGE_REQ *)pPacket->GetPacketData();

	WORD resultcode = GAME_SUCCESS;

	CNtlVector arenaLoc(DBO_ARENA_BASE_LOC_X, DBO_ARENA_BASE_LOC_Y, DBO_ARENA_BASE_LOC_Z);

	CPlayer* target = g_pObjectManager->GetPC(req->hTarget);
	if(target == NULL || !target->IsInitialized())
		resultcode = GAME_TARGET_NOT_FOUND;
	else if(target->GetFreeBattleTarget() != INVALID_CHARACTERID)
		resultcode = GAME_FREEBATTLE_TARGET_ALREADY_HAS_MATCH;
	else if (cPlayer->GetFreeBattleTarget() != INVALID_CHARACTERID)
		resultcode = GAME_FREEBATTLE_ALREADY_HAS_MATCH;
	else if (cPlayer->GetCurWorld() == NULL || GetNaviEngine()->IsBasicAttributeSet(cPlayer->GetCurWorld()->GetNaviInstanceHandle(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().z, DBO_WORLD_ATTR_BASIC_FORBID_PC_BATTLE))
		resultcode = GAME_FREEBATTLE_WRONG_PLACE;
	else if (cPlayer->GetAirState() == AIR_STATE_ON || target->GetAirState() == AIR_STATE_ON)
		resultcode = GAME_COMMON_CANT_DO_THAT_IN_AIR_MODE_STATE;
	else if (!cPlayer->IsInRange(target, DBO_FREEBATTLE_CHALLENGE_RANGE))
		resultcode = GAME_TARGET_TOO_FAR;
	else if (target->IsFainting() || cPlayer->IsFainting())
		resultcode = GAME_FAIL;
	else if (target->IsInRange(arenaLoc, DBO_ARENA_FREEBATTLELOC_RANGE) || cPlayer->IsInRange(arenaLoc, DBO_ARENA_FREEBATTLELOC_RANGE))
		resultcode = GAME_FREEBATTLE_WRONG_PLACE;
	else if(cPlayer->GetDragonballScramble() || target->GetDragonballScramble())
		resultcode = SCRAMBLE_CANNOT_DO_WHILE_JOINED;

	CNtlPacket packet(sizeof(sGU_FREEBATTLE_CHALLENGE_RES));
	sGU_FREEBATTLE_CHALLENGE_RES * res = (sGU_FREEBATTLE_CHALLENGE_RES *)packet.GetPacketData();
	res->wOpCode = GU_FREEBATTLE_CHALLENGE_RES;
	res->hTarget = req->hTarget;
	res->wResultCode = resultcode;
	packet.SetPacketLen( sizeof(sGU_FREEBATTLE_CHALLENGE_RES) );
	app->Send( GetHandle(), &packet );


	if( resultcode == GAME_SUCCESS )
	{
		g_pFreeBattleManager->CreateFreeBattle(cPlayer,target);

		CNtlPacket packet2(sizeof(sGU_FREEBATTLE_ACCEPT_REQ));
		sGU_FREEBATTLE_ACCEPT_REQ * res2 = (sGU_FREEBATTLE_ACCEPT_REQ *)packet2.GetPacketData();
		res2->hChallenger = cPlayer->GetID();
		res2->wOpCode = GU_FREEBATTLE_ACCEPT_REQ;
		packet2.SetPacketLen( sizeof(sGU_FREEBATTLE_ACCEPT_REQ) );
		app->Send( target->GetClientSessionID(), &packet2 );
	}
}
void	CClientSession::RecvFreeBattleAccpetReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	CPlayer* invitor = g_pObjectManager->FindByChar(cPlayer->GetFreeBattleTarget());
	if(invitor)
	{
		sUG_FREEBATTLE_ACCEPT_RES* req = (sUG_FREEBATTLE_ACCEPT_RES *)pPacket->GetPacketData();
		
		if (req->byAccept == 1)
		{
			WORD resultcode = GAME_SUCCESS;

			CNtlVector arenaLoc(DBO_ARENA_BASE_LOC_X, DBO_ARENA_BASE_LOC_Y, DBO_ARENA_BASE_LOC_Z);

			if (invitor == NULL || !invitor->IsInitialized())
				resultcode = GAME_TARGET_NOT_FOUND;
			else if (cPlayer->GetCurWorld() == NULL || GetNaviEngine()->IsBasicAttributeSet(cPlayer->GetCurWorld()->GetNaviInstanceHandle(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().z, DBO_WORLD_ATTR_BASIC_FORBID_PC_BATTLE))
				resultcode = GAME_FREEBATTLE_WRONG_PLACE;
			else if (cPlayer->GetAirState() == AIR_STATE_ON || invitor->GetAirState() == AIR_STATE_ON)
				resultcode = GAME_COMMON_CANT_DO_THAT_IN_AIR_MODE_STATE;
			else if (!cPlayer->IsInRange(invitor, DBO_FREEBATTLE_CHALLENGE_RANGE))
				resultcode = GAME_TARGET_TOO_FAR;
			else if (invitor->IsFainting() || cPlayer->IsFainting())
				resultcode = GAME_FAIL;
			else if (invitor->IsInRange(arenaLoc, DBO_ARENA_FREEBATTLELOC_RANGE) || cPlayer->IsInRange(arenaLoc, DBO_ARENA_FREEBATTLELOC_RANGE))
				resultcode = GAME_FREEBATTLE_WRONG_PLACE;
			else if (invitor->GetDragonballScramble() || cPlayer->GetDragonballScramble())
				resultcode = SCRAMBLE_CANNOT_DO_WHILE_JOINED;

			if(resultcode == GAME_SUCCESS)
			{
				if (g_pFreeBattleManager->BeginFreeBattle(invitor->GetFreeBattleID()))
				{
					//send packet to player me
					CNtlPacket packet(sizeof(sGU_FREEBATTLE_START_NFY));
					sGU_FREEBATTLE_START_NFY * res = (sGU_FREEBATTLE_START_NFY *)packet.GetPacketData();
					res->hTarget = invitor->GetID();
					res->vRefreeLoc.y = invitor->GetCurLoc().y;
					res->vRefreeLoc.x = invitor->GetCurLoc().x + 10;
					res->vRefreeLoc.z = invitor->GetCurLoc().z + 10;
					res->wOpCode = GU_FREEBATTLE_START_NFY;
					packet.SetPacketLen(sizeof(sGU_FREEBATTLE_START_NFY));
					app->Send(GetHandle(), &packet);

					//send packet to invitor
					CNtlPacket packet2(sizeof(sGU_FREEBATTLE_START_NFY));
					sGU_FREEBATTLE_START_NFY * res2 = (sGU_FREEBATTLE_START_NFY *)packet2.GetPacketData();
					res2->hTarget = cPlayer->GetID();
					res2->vRefreeLoc.y = invitor->GetCurLoc().y;
					res2->vRefreeLoc.x = invitor->GetCurLoc().x + 10;
					res2->vRefreeLoc.z = invitor->GetCurLoc().z + 10;
					res2->wOpCode = GU_FREEBATTLE_START_NFY;
					packet2.SetPacketLen(sizeof(sGU_FREEBATTLE_START_NFY));
					app->Send(invitor->GetClientSessionID(), &packet2);
				}
				else
				{
					CNtlPacket packet(sizeof(sGU_FREEBATTLE_CANCEL_NFY));
					sGU_FREEBATTLE_CANCEL_NFY * res = (sGU_FREEBATTLE_CANCEL_NFY *)packet.GetPacketData();
					res->wOpCode = GU_FREEBATTLE_CANCEL_NFY;
					res->wResultCode = resultcode;
					packet.SetPacketLen(sizeof(sGU_FREEBATTLE_CANCEL_NFY));
					app->Send(invitor->GetClientSessionID(), &packet);

					CNtlPacket packet2(sizeof(sGU_FREEBATTLE_CANCEL_NFY));
					sGU_FREEBATTLE_CANCEL_NFY * res2 = (sGU_FREEBATTLE_CANCEL_NFY *)packet2.GetPacketData();
					res2->wOpCode = GU_FREEBATTLE_CANCEL_NFY;
					res2->wResultCode = GAME_SUCCESS;
					packet2.SetPacketLen(sizeof(sGU_FREEBATTLE_CANCEL_NFY));
					app->Send(GetHandle(), &packet2);

					cPlayer->InitFreeBattle();
					invitor->InitFreeBattle();
				}
			}
			else
			{
				g_pFreeBattleManager->DeleteFreeBattle(invitor->GetFreeBattleID());

				CNtlPacket packet(sizeof(sGU_FREEBATTLE_CANCEL_NFY));
				sGU_FREEBATTLE_CANCEL_NFY * res = (sGU_FREEBATTLE_CANCEL_NFY *)packet.GetPacketData();
				res->wOpCode = GU_FREEBATTLE_CANCEL_NFY;
				res->wResultCode = resultcode;
				packet.SetPacketLen( sizeof(sGU_FREEBATTLE_CANCEL_NFY) );
				app->Send( invitor->GetClientSessionID(), &packet );

				CNtlPacket packet2(sizeof(sGU_FREEBATTLE_CANCEL_NFY));
				sGU_FREEBATTLE_CANCEL_NFY * res2 = (sGU_FREEBATTLE_CANCEL_NFY *)packet2.GetPacketData();
				res2->wOpCode = GU_FREEBATTLE_CANCEL_NFY;
				res2->wResultCode = GAME_SUCCESS;
				packet2.SetPacketLen( sizeof(sGU_FREEBATTLE_CANCEL_NFY) );
				app->Send(GetHandle(), &packet2 );
			}
		}
		else
		{
			g_pFreeBattleManager->DeleteFreeBattle(invitor->GetFreeBattleID());

			CNtlPacket packet(sizeof(sGU_FREEBATTLE_CANCEL_NFY));
			sGU_FREEBATTLE_CANCEL_NFY * res = (sGU_FREEBATTLE_CANCEL_NFY *)packet.GetPacketData();
			res->wOpCode = GU_FREEBATTLE_CANCEL_NFY;
			res->wResultCode = GAME_FREEBATTLE_CHALLENGE_ACCEPT_DENIED;
			packet.SetPacketLen( sizeof(sGU_FREEBATTLE_CANCEL_NFY) );
			app->Send( invitor->GetClientSessionID(), &packet );

			CNtlPacket packet2(sizeof(sGU_FREEBATTLE_CANCEL_NFY));
			sGU_FREEBATTLE_CANCEL_NFY * res2 = (sGU_FREEBATTLE_CANCEL_NFY *)packet2.GetPacketData();
			res2->wOpCode = GU_FREEBATTLE_CANCEL_NFY;
			res2->wResultCode = GAME_SUCCESS;
			packet2.SetPacketLen( sizeof(sGU_FREEBATTLE_CANCEL_NFY) );
			app->Send(GetHandle(), &packet2 );
		}
	}
}

void CClientSession::RecvRideOnBusReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_RIDE_ON_BUS_REQ * req = (sUG_RIDE_ON_BUS_REQ*)pPacket->GetPacketData();
	
	WORD wResultCode = GAME_SUCCESS;

	CNpc* pBus = g_pObjectManager->GetNpc(req->hTarget);
	if (pBus && pBus->HasFunction(NPC_FUNC_FLAG_BUS))
	{
		if(pBus->GetCurWorld() == NULL)
			wResultCode = GAME_FAIL;
		else if(pBus->IsSitting() == false && pBus->IsStanding() == false && pBus->IsDestMoving() == false)
			wResultCode = GAME_TARGET_IS_WRONG_STATE;
		else if (cPlayer->GetZeni() < pBus->GetTbldat()->amerchant_Tblidx[0])
			wResultCode = GAME_ZENNY_NOT_ENOUGH;
		else if(cPlayer->IsInRange(pBus, 20.f) == false)
			wResultCode = GAME_TARGET_TOO_FAR;
		else if(cPlayer->GetDragonballScramble())
			wResultCode = SCRAMBLE_CANNOT_RIDE_BUS_WHILE_JOINED;
		else if(cPlayer->GetStateManager()->CanCharStateTransition(CHARSTATE_RIDEON) == false)
			wResultCode = GAME_CHAR_IS_WRONG_STATE;
		else if (cPlayer->GetAspectStateId() != ASPECTSTATE_INVALID && cPlayer->GetAspectStateId() != ASPECTSTATE_VEHICLE)
			wResultCode = GAME_CHAR_IS_WRONG_STATE;
		else
		{
			if (cPlayer->GetAspectStateId() == ASPECTSTATE_VEHICLE)
				cPlayer->EndVehicle(GAME_VEHICLE_END_BY_FORCED);

			if (g_pBusSystem->AddPlayerToBus(req->hTarget, cPlayer))
			{
				cPlayer->UpdateZeni(ZENNY_CHANGE_TYPE_RIDE_ON_BUS, pBus->GetTbldat()->amerchant_Tblidx[0], false);
				cPlayer->SetBusID(req->hTarget);
			}
			else
				wResultCode = GAME_FAIL;
		}
	}
	else wResultCode = GAME_TARGET_NOT_FOUND;

	CNtlPacket packet(sizeof(sGU_RIDE_ON_BUS_RES));
	sGU_RIDE_ON_BUS_RES * res = (sGU_RIDE_ON_BUS_RES *)packet.GetPacketData();
	res->wOpCode = GU_RIDE_ON_BUS_RES;
	res->hTarget = req->hTarget;
	res->wResultCode = wResultCode;
	packet.SetPacketLen(sizeof(sGU_RIDE_ON_BUS_RES));
	g_pApp->Send(GetHandle(), &packet);
}

void CClientSession::RecvRideOffBusReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_RIDE_OFF_BUS_REQ * req = (sUG_RIDE_OFF_BUS_REQ*)pPacket->GetPacketData();

	WORD wResultCode = GAME_SUCCESS;

	if(cPlayer->GetBusID() == INVALID_HOBJECT)
		wResultCode = GAME_FAIL;
	else
	{
		g_pBusSystem->RemovePlayerFromBus(cPlayer, cPlayer->GetBusID(), true);
		cPlayer->SetBusID(INVALID_HOBJECT);
	}

	CNtlPacket packet(sizeof(sGU_RIDE_OFF_BUS_RES));
	sGU_RIDE_OFF_BUS_RES * res = (sGU_RIDE_OFF_BUS_RES *)packet.GetPacketData();
	res->wOpCode = GU_RIDE_OFF_BUS_RES;
	res->wResultCode = wResultCode;
	packet.SetPacketLen(sizeof(sGU_RIDE_OFF_BUS_RES));
	g_pApp->Send(GetHandle(), &packet);
}


void CClientSession::RecvCharKnockdownReleaseNfy(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	//if (cPlayer->GetKnockDownTime() >= NTL_BATTLE_KNOCKDOWN_MIN_WAKEUP_TIME)
	{
		if (cPlayer->GetBuffManager()->CheckAndApplyOtherStun(CHARSTATE_KNOCKDOWN, INVALID_WORD) == false) //check if any previous stuns exist. If yes, then apply them and return true. Otherwise return false.
		{
			CGameServer* app = (CGameServer*)g_pApp;

			DWORD dwAnimationLength = cPlayer->GetAniTbldat()->GetAnimationLength(BTL_DEF_KD_STAND_UP);
			
			//printf("dwAnimationLength: %u \n", dwAnimationLength);

			cPlayer->SetLastAttackTime(app->GetCurTickCount() + dwAnimationLength);
			cPlayer->SendCharStateStanding();
		}
	}
}


void CClientSession::RecvCancelTransformationReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	if(cPlayer->IsStanding() || cPlayer->IsMoving() || cPlayer->IsDestMoving())
		cPlayer->CancelTransformation();
	else
	{
		CNtlPacket packet(sizeof(sGU_TRANSFORM_CANCEL_RES));
		sGU_TRANSFORM_CANCEL_RES * res = (sGU_TRANSFORM_CANCEL_RES*)packet.GetPacketData();
		res->wOpCode = GU_TRANSFORM_CANCEL_RES;
		res->wResultCode = GAME_CHAR_IS_WRONG_STATE;
		g_pApp->Send(cPlayer->GetClientSessionID(), &packet);
	}
}


void CClientSession::RecvSocialSkill(CNtlPacket * pPacket)
 {
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_SOCIAL_ACTION * req = (sUG_SOCIAL_ACTION*)pPacket->GetPacketData();

	if (cPlayer->GetCharStateID() == CHARSTATE_SITTING)
		return;

	if (app->GetCurTickCount() < m_dwLastActionUse + 3000) // only allow every 3 seconds
	{
		return;
	}

	m_dwLastActionUse = app->GetCurTickCount();

	sACTION_TBLDAT* pActionTbldat = (sACTION_TBLDAT*)g_pTableContainer->GetActionTable()->FindData(req->socialActionId);
	if (pActionTbldat == NULL)
	{
		ERR_LOG(LOG_HACK, "Player: %u could not play social-action: %u", cPlayer->GetCharID(), req->socialActionId);
		return;
	}

	if (pActionTbldat->byAction_Type != ACTION_TYPE_SOCIAL_ACTION)
		return;

 	CNtlPacket packet(sizeof(sGU_SOCIAL_ACTION));
	sGU_SOCIAL_ACTION * res = (sGU_SOCIAL_ACTION*)packet.GetPacketData();
	res->wOpCode = GU_SOCIAL_ACTION;
	res->hSubject = cPlayer->GetID();
	res->socialActionId = req->socialActionId;
	packet.SetPacketLen(sizeof(sGU_SOCIAL_ACTION));
	cPlayer->Broadcast(&packet, cPlayer);
 }

 

//-----------------------------------------------------------------//
//-------------------Skill/Item BUFF Drop--------------------------//
//-----------------------------------------------------------------//
void CClientSession::RecvBuffDropReq(CNtlPacket * pPacket)
{
 	sUG_BUFF_DROP_REQ * req = (sUG_BUFF_DROP_REQ*)pPacket->GetPacketData();

	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CNtlPacket packet(sizeof(sGU_BUFF_DROP_RES));
	sGU_BUFF_DROP_RES * res = (sGU_BUFF_DROP_RES*)packet.GetPacketData();
	res->wOpCode = GU_BUFF_DROP_RES;
	res->wResultCode = GAME_SUCCESS;

	CBuff* buff = cPlayer->GetBuffManager()->FindBuff(req->buffIndex);
	if (buff && cPlayer->GetCurWorld())
	{
		if (buff->GetSourceTblidx() == req->tblidx && buff->GetSourceType() == req->bySourceType)
		{
			//anti bug for lp/ep auto recover pot
			if(buff->GetSourceType() == DBO_OBJECT_SOURCE_ITEM)
			{
				if (buff->GetSystemEffectCode(0) == ACTIVE_LP_AUTO_RECOVER || buff->GetSystemEffectCode(0) == ACTIVE_EP_AUTO_RECOVER)
				{
					if (buff->GetRemainTime(0) > 0) //dont allow to remove auto pot while its on cooldown
					{
						res->wResultCode = GAME_FAIL;
					}
					else
					{
						sITEM_TBLDAT * pItemTbldat = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(buff->GetSourceTblidx());
						if (pItemTbldat)
						{
							sUSE_ITEM_TBLDAT * pUseItemTbldat = (sUSE_ITEM_TBLDAT*)g_pTableContainer->GetUseItemTable()->FindData(pItemTbldat->Use_Item_Tblidx);
							if (pUseItemTbldat)
							{
								bool bIsActive = buff->GetBuffInfo()->bActive;
								bool bCanEnableBuff = true;
								BYTE byWorldRuleType = cPlayer->GetCurWorld()->GetTbldat()->byWorldRuleType;

								if (bIsActive == false) //if disabled.. check if can enable it
								{
									if (byWorldRuleType == GAMERULE_CCBATTLEDUNGEON && BIT_FLAG_TEST(pUseItemTbldat->dwUse_Restriction_Rule_Bit_Flag, MAKE_BIT_FLAG(GAMERULE_NORMAL)) == false)
									{
										bCanEnableBuff = false;
									}
									else if (pUseItemTbldat->dwUse_Restriction_Rule_Bit_Flag > 0 && BIT_FLAG_TEST(pUseItemTbldat->dwUse_Restriction_Rule_Bit_Flag, MAKE_BIT_FLAG(byWorldRuleType))) //check if buff is not allowed in current world
									{
										bCanEnableBuff = false;
									}
								}


								if (bCanEnableBuff)
									cPlayer->GetBuffManager()->EnableDisableBuff(req->buffIndex, buff->GetBuffType());
								else
									res->wResultCode = GAME_FAIL;
							}
							else
							{
								res->wResultCode = GAME_FAIL;
							}
						}
						else
						{
							res->wResultCode = GAME_FAIL;
						}
					}
				}
				else if (buff->GetSystemEffectCode(0) == ACTIVE_EXCITATION_MALE && buff->GetSystemEffectCode(1) == ACTIVE_EXCITATION_FEMALE) //check if drop turtle book buff
				{
					if (cPlayer->GetGender() != GENDER_MALE) //only nemkian and female
					{
						//remove all rp and rp balls
						cPlayer->UpdateRpBall(cPlayer->GetMaxRPBall(), false, false);
						cPlayer->UpdateCurRP(cPlayer->GetCharAtt()->GetLastMaxRP(), false, false);
					}

					cPlayer->GetBuffManager()->RemoveBuff(req->buffIndex, buff->GetBuffType(), CBuff::BUFF_REMOVAL_REASON_BY_ITSELF);
				}
				else
				{
					cPlayer->GetBuffManager()->RemoveBuff(req->buffIndex, buff->GetBuffType(), CBuff::BUFF_REMOVAL_REASON_BY_ITSELF);
				}
			}
			else
			{
				if(buff->IsCurseBuff() || buff->IsRevivalAftereffectBuff())
					res->wResultCode = GAME_SKILL_NO_BUFF_TO_DROP_FOUND;
				else
					cPlayer->GetBuffManager()->RemoveBuff(req->buffIndex, buff->GetBuffType(), CBuff::BUFF_REMOVAL_REASON_BY_ITSELF);
			}
		}
		else res->wResultCode = GAME_SKILL_NO_BUFF_TO_DROP_FOUND;
	}
	else res->wResultCode = GAME_SKILL_NO_BUFF_TO_DROP_FOUND;
	

	packet.SetPacketLen(sizeof(sGU_BUFF_DROP_RES));
	g_pApp->Send(GetHandle(), &packet);
 }

//-----------------------------------------------------------//
// START HTB SKILL
//-----------------------------------------------------------//
void CClientSession::RecvHTBStartReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_HTB_START_REQ * req = (sUG_HTB_START_REQ*)pPacket->GetPacketData();

	WORD resultcode = GAME_SKILL_NO_TARGET_APPOINTED;

	if (CCharacter* pTarget = g_pObjectManager->GetChar(req->hTarget))
	{
		if (NTL_HTB_MAX_PC_HAVE_HTB_SKILL <= req->bySkillSlot)
			resultcode = GAME_FAIL;
		else if(pTarget->GetObjType() != OBJTYPE_PC && pTarget->GetObjType() != OBJTYPE_MOB)
			resultcode = GAME_SKILL_INVALID_TARGET_APPOINTED;
		else if (cPlayer->GetCurRPBall() < DBO_HTB_PREREQUISITE_RP_BALL)
			resultcode = GAME_HTB_NOT_ENOUGH_RP_BALL;
		else
		{
			if (CHtbSkill* pHtb = cPlayer->GetHtbSkillManager()->GetHtbSkill(req->bySkillSlot))
			{
				//NTL_PRINT(PRINT_APP,"pHTBSetTblData->wNeed_EP %d ( %d %d %d %d %d %d %d %d %d %d )\n", pHtb->GetTbldat()->wNeed_EP, pHtb->GetTbldat()->aHTBAction[0].skillTblidx, pHtb->GetTbldat()->aHTBAction[1].skillTblidx, pHtb->GetTbldat()->aHTBAction[2].skillTblidx, pHtb->GetTbldat()->aHTBAction[3].skillTblidx, pHtb->GetTbldat()->aHTBAction[4].skillTblidx, pHtb->GetTbldat()->aHTBAction[5].skillTblidx, pHtb->GetTbldat()->aHTBAction[6].skillTblidx, pHtb->GetTbldat()->aHTBAction[7].skillTblidx, pHtb->GetTbldat()->aHTBAction[8].skillTblidx, pHtb->GetTbldat()->aHTBAction[9].skillTblidx);
				//NTL_PRINT(PRINT_APP,"tbldata->byStop_Point %d tbldata->bySetCount %d \n", pHtb->GetTbldat()->byStop_Point, pHtb->GetTbldat()->bySetCount);

				if (pHtb->GetTbldat()->wNeed_EP != INVALID_WORD && cPlayer->GetCurEP() < pHtb->GetTbldat()->wNeed_EP)
					resultcode = GAME_SKILL_NOT_ENOUGH_EP;
				else if (pHtb->GetCoolTimeRemaining() == 0)
				{
					if (cPlayer->IsAttackable(pTarget) == false)
					{
						resultcode = GAME_SKILL_INVALID_TARGET_APPOINTED;
					}
					else if (pTarget->IsValidTarget(0) == false)
					{
						resultcode = GAME_SKILL_INVALID_TARGET_APPOINTED;
					}
					else if (pTarget->GetStateManager()->IsCharCondition(CHARCOND_NULLIFIED_DAMAGE))
					{
						resultcode = GAME_SKILL_INVALID_TARGET_APPOINTED;
					}
					else if (pTarget->IsMonster() && ((CMonster*)pTarget)->GetMobRank() >= MOB_GRADE_BOSS)
					{
						resultcode = GAME_SKILL_INVALID_TARGET_APPOINTED;
					}
					else if (pTarget->GetStateManager()->CanCharStateTransition(CHARSTATE_SANDBAG) == false)
					{
						resultcode = GAME_SKILL_INVALID_TARGET_APPOINTED;
					}
					else if (cPlayer->GetStateManager()->CanCharStateTransition(CHARSTATE_HTB) == false)
					{
						resultcode = GAME_SKILL_INVALID_TARGET_APPOINTED;
					}
					else if (cPlayer->GetStateManager()->GetAspectStateID() == ASPECTSTATE_PURE_MAJIN || cPlayer->GetStateManager()->GetAspectStateID() == ASPECTSTATE_GREAT_NAMEK || cPlayer->GetStateManager()->GetAspectStateID() == ASPECTSTATE_SPINNING_ATTACK
						|| cPlayer->GetStateManager()->GetAspectStateID() == ASPECTSTATE_VEHICLE || cPlayer->GetStateManager()->GetAspectStateID() == ASPECTSTATE_ROLLING_ATTACK)
					{
						resultcode = GAME_SKILL_CANT_USE_HTB_WHEN_TRANSFORMED;
					}
					else
					{
						resultcode = GAME_SUCCESS;

						cPlayer->UpdateBattleCombatMode(true); //go into battle mode

						cPlayer->SetCurrentHtbSkill(req->bySkillSlot);
						cPlayer->UpdateCurEP(pHtb->GetTbldat()->wNeed_EP, false, false);

						if (pTarget->IsPC() == false)
						{
							cPlayer->UpdateRpBall(1, false, false);
							cPlayer->SetHtbUseBalls(1);
						}

						pHtb->SetAttackFailed(false);
						pHtb->SetHitCount(0);
						pHtb->SetCurrentStep(0);
						pHtb->SetTarget(req->hTarget);
						pHtb->SetCoolTimeRemaining(pHtb->GetTbldat()->dwCoolTimeInMilliSecs);


						CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_STATE));
						sGU_UPDATE_CHAR_STATE * res = (sGU_UPDATE_CHAR_STATE*)packet.GetPacketData();
						res->wOpCode = GU_UPDATE_CHAR_STATE;
						res->handle = cPlayer->GetID();
						res->sCharState.sCharStateBase.byStateID = CHARSTATE_HTB;
						cPlayer->GetStateManager()->CopyAspectTo(&res->sCharState.sCharStateBase.aspectState);
						cPlayer->GetCurLoc().CopyTo(res->sCharState.sCharStateBase.vCurLoc);
						cPlayer->GetCurDir().CopyTo(res->sCharState.sCharStateBase.vCurDir);
						res->sCharState.sCharStateBase.bFightMode = cPlayer->GetFightMode();
						res->sCharState.sCharStateBase.bIsValidLoc = true;
						res->sCharState.sCharStateBase.dwConditionFlag = cPlayer->GetConditionState();
						res->sCharState.sCharStateBase.eAirState = AIR_STATE_OFF;
						res->sCharState.sCharStateDetail.sCharStateHTB.hTarget = pTarget->GetID();
						res->sCharState.sCharStateDetail.sCharStateHTB.byStepCount = pHtb->GetTbldat()->bySetCount;
						res->sCharState.sCharStateDetail.sCharStateHTB.byCurStep = 0;
						res->sCharState.sCharStateDetail.sCharStateHTB.HTBId = pHtb->GetHtbID();
						res->sCharState.sCharStateDetail.sCharStateHTB.bIsSuccess = true;
						res->sCharState.sCharStateDetail.sCharStateHTB.byResultCount = pHtb->GetTbldat()->bySetCount;

						CNtlVector vShift(pTarget->GetCurLoc() - cPlayer->GetCurLoc());
						vShift.y = 0.0f;
						vShift.SafeNormalize();

						bool bCanCounterAttack = true;

						if (pTarget->GetCharStateID() == CHARSTATE_GUARD)
						{
							if (((CPlayer*)pTarget)->IsGuardSuccess(DBO_GUARD_TYPE_HTB, bCanCounterAttack))
							{
								for (BYTE i = 0; i < NTL_HTB_MAX_SKILL_COUNT_IN_SET; i++)
								{
									res->sCharState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[i].byStep = i;
									res->sCharState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[i].sSkillResult.byBlockedAction = DBO_GUARD_TYPE_HTB;
									res->sCharState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[i].sSkillResult.byAttackResult = BATTLE_ATTACK_RESULT_BLOCK;
									res->sCharState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[i].sSkillResult.hTarget = req->hTarget;

									vShift.CopyTo(res->sCharState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[i].sSkillResult.vShift);
									pTarget->GetCurLoc().CopyTo(res->sCharState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[i].sSkillResult.vFinalSubjectLoc);
								}

							//	res->sCharState.sCharStateDetail.sCharStateHTB.byStepCount = 0;
							//	res->sCharState.sCharStateDetail.sCharStateHTB.byResultCount = 1;
								res->sCharState.sCharStateDetail.sCharStateHTB.bIsSuccess = false;
							}
						}

						if (res->sCharState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[0].sSkillResult.byAttackResult != BATTLE_ATTACK_RESULT_BLOCK)
						{
							for (BYTE i = 0; i < NTL_HTB_MAX_SKILL_COUNT_IN_SET; i++)
							{
								res->sCharState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[i].byStep = i;
								res->sCharState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[i].sSkillResult.hTarget = req->hTarget;
								res->sCharState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[i].sSkillResult.byAttackResult = BATTLE_ATTACK_RESULT_HIT;
								res->sCharState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[i].sSkillResult.byBlockedAction = 255;
								vShift.CopyTo(res->sCharState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[i].sSkillResult.vShift);
								pTarget->GetCurLoc().CopyTo(res->sCharState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[i].sSkillResult.vFinalSubjectLoc);
								res->sCharState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[i].sSkillResult.effectResult[0].eResultType = DBO_SYSTEM_EFFECT_RESULT_TYPE_GENERAL;

								if (pHtb->GetTbldat()->aHTBAction[i].skillTblidx != INVALID_TBLIDX)
								{
									sSKILL_TBLDAT *pSkillTblData = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData(pHtb->GetTbldat()->aHTBAction[i].skillTblidx);
									res->sCharState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[i].sSkillResult.effectResult[0].eResultType = DBO_SYSTEM_EFFECT_RESULT_TYPE_DD_DOT;

									if (i == pHtb->GetTbldat()->bySetCount - 1)
									{
										res->sCharState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[i].sSkillResult.byAttackResult = BATTLE_ATTACK_RESULT_KNOCKDOWN;
									}


									if (res->sCharState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[i].sSkillResult.byAttackResult != BATTLE_ATTACK_RESULT_KNOCKDOWN)
									{
										if (pSkillTblData->bySkill_Type == NTL_SKILL_TYPE_PHYSICAL) //check if physical dmg
										{
											if (BattleIsCrit(cPlayer->GetCharAtt(), pTarget->GetCharAtt(), true))
												res->sCharState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[i].sSkillResult.byAttackResult = BATTLE_ATTACK_RESULT_CRITICAL_HIT;
										}
										else if (pSkillTblData->bySkill_Type == NTL_SKILL_TYPE_ENERGY) //check if energy dmg
										{
											if (BattleIsCrit(cPlayer->GetCharAtt(), pTarget->GetCharAtt(), false))
												res->sCharState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[i].sSkillResult.byAttackResult = BATTLE_ATTACK_RESULT_CRITICAL_HIT;
										}
									}

									//calculate the damage
									CalcSkillDamage(cPlayer, pTarget, pSkillTblData, 0, (float)pSkillTblData->aSkill_Effect_Value[0], res->sCharState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[i].sSkillResult.effectResult[0].DD_DOT_fDamage,
										res->sCharState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[i].sSkillResult.byAttackResult, res->sCharState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[i].sSkillResult.damageByReflectingCurse,
										&res->sCharState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[i].sSkillResult.effectResult[0].DD_DOT_lpEpRecovered);
								}
								else
									res->sCharState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[i].sSkillResult.effectResult[0].fResultValue = 0;
							}

							pTarget->SetHtbSkillCaster(cPlayer->GetID());
							pTarget->SendCharStateSandbag();
						}

						packet.SetPacketLen(sizeof(sGU_UPDATE_CHAR_STATE));
						cPlayer->GetStateManager()->CopyFrom(&res->sCharState);
						cPlayer->Broadcast(&packet);
					}
				}
				else
				{
					resultcode = GAME_SKILL_CANT_CAST_NOW;
				}
			}
			else
			{
				resultcode = GAME_SKILL_YOU_DONT_HAVE_THE_SKILL;
			}
		}
	}


	CNtlPacket packetEnd(sizeof(sGU_HTB_START_RES));
	sGU_HTB_START_RES * resEnd = (sGU_HTB_START_RES*)packetEnd.GetPacketData();
	resEnd->wOpCode = GU_HTB_START_RES;
	resEnd->wResultCode = resultcode;
	resEnd->bySkillSlot = req->bySkillSlot;
	packetEnd.SetPacketLen(sizeof(sGU_HTB_START_RES));
	g_pApp->Send(GetHandle(), &packetEnd);
}


//-----------------------------------------------------------//
//
//-----------------------------------------------------------//
void CClientSession::RecvHTBRpBallUseReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_HTB_RP_BALL_USE_REQ * req = (sUG_HTB_RP_BALL_USE_REQ*)pPacket->GetPacketData();

	WORD resultcode = GAME_SUCCESS;

	if (req->byRpBallCount > 0 && cPlayer->GetCurRPBall() < req->byRpBallCount)
		resultcode = GAME_HTB_NOT_ENOUGH_RP_BALL;

	CNtlPacket packet(sizeof(sGU_HTB_RP_BALL_USE_RES));
	sGU_HTB_RP_BALL_USE_RES * res = (sGU_HTB_RP_BALL_USE_RES*)packet.GetPacketData();
	res->wOpCode = GU_HTB_RP_BALL_USE_RES;
	res->byRpBallCount = req->byRpBallCount;
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_HTB_RP_BALL_USE_RES));
	g_pApp->Send(GetHandle(), &packet);

	if (resultcode == GAME_SUCCESS)
	{
		cPlayer->UpdateRpBall(req->byRpBallCount, false, false);
		cPlayer->SetHtbUseBalls(req->byRpBallCount);

		CNtlPacket packet2(sizeof(sGU_HTB_RP_BALL_USED_NFY));
		sGU_HTB_RP_BALL_USED_NFY * res2 = (sGU_HTB_RP_BALL_USED_NFY*)packet2.GetPacketData();
		res2->wOpCode = GU_HTB_RP_BALL_USED_NFY;
		res2->hSubject = cPlayer->GetID();
		res2->byRpBallCount = req->byRpBallCount;
		packet2.SetPacketLen(sizeof(sGU_HTB_RP_BALL_USED_NFY));
		cPlayer->Broadcast(&packet2);
	}
}

//-----------------------------------------------------------//
// HTB FORWARD REQ
//-----------------------------------------------------------//
void CClientSession::RecvHTBForwardReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_HTB_FORWARD_REQ * req = (sUG_HTB_FORWARD_REQ*)pPacket->GetPacketData();

	WORD wResultcode = GAME_SUCCESS;
	
	if (cPlayer->GetCharStateID() == CHARSTATE_HTB)
	{
		if(cPlayer->GetCurrentHtbSkill() < NTL_HTB_MAX_PC_HAVE_HTB_SKILL)
		{
			CHtbSkill* pHtb = cPlayer->GetHtbSkillManager()->GetHtbSkill(cPlayer->GetCurrentHtbSkill());
			if (pHtb)
			{
			//	printf("pHtb->GetCurrentStep() %u \n", pHtb->GetCurrentStep());
				if (pHtb->GetCurrentStep() > pHtb->GetTbldat()->bySetCount)
				{
					wResultcode = GAME_HTB_CANT_FORWARD_ANYMORE;
				}
				else
				{
					pHtb->SetCurrentStep(pHtb->GetCurrentStep() + 1);

					CCharacter* pTarget = g_pObjectManager->GetChar(pHtb->GetTarget());
					if (pTarget && pTarget->IsInitialized() && (pTarget->GetCharStateID() == CHARSTATE_SANDBAG || pTarget->GetCharStateID() == CHARSTATE_KNOCKDOWN))
					{
						//decide htb ball winner
						if (pHtb->GetCurrentStep() == pHtb->GetTbldat()->byStop_Point)
						{
							CNtlPacket packet3(sizeof(sGU_HTB_RP_BALL_RESULT_DECIDED_NFY));
							sGU_HTB_RP_BALL_RESULT_DECIDED_NFY * res3 = (sGU_HTB_RP_BALL_RESULT_DECIDED_NFY*)packet3.GetPacketData();
							res3->wOpCode = GU_HTB_RP_BALL_RESULT_DECIDED_NFY;
							res3->hAttacker = cPlayer->GetID();
							res3->hWinner = cPlayer->GetID();
							res3->bHasSubjectRpBallUsed = true;
							res3->bHasTargetRpBallUsed = false;
							res3->bySubjectRpBallUsed = cPlayer->GetHtbUseBalls();
							res3->byTargetRpBallUsed = 0;

							// decide rp winner
							if (pTarget->IsPC())
							{
								res3->byTargetRpBallUsed = ((CPlayer*)pTarget)->GetHtbUseBalls();

								if (res3->byTargetRpBallUsed)
								{
									res3->bHasTargetRpBallUsed = true;

									if (res3->byTargetRpBallUsed >= res3->bySubjectRpBallUsed)
									{
										res3->hWinner = pTarget->GetID();
										pHtb->SetAttackFailed(true);
									}
								}

								pTarget->SendPacket(&packet3);
							}

							cPlayer->SendPacket(&packet3);

							if (res3->hWinner != cPlayer->GetID())
							{
								pHtb->SetHitCount(pHtb->GetTbldat()->byStop_Point);
							
								CNtlPacket packet2(sizeof(sGU_HTB_LAST_STEP_CHANGED_NFY));
								sGU_HTB_LAST_STEP_CHANGED_NFY * res2 = (sGU_HTB_LAST_STEP_CHANGED_NFY*)packet2.GetPacketData();
								res2->wOpCode = GU_HTB_LAST_STEP_CHANGED_NFY;
								res2->byLastStep = pHtb->GetTbldat()->byStop_Point;
								res2->hSubject = cPlayer->GetID();
								packet2.SetPacketLen(sizeof(sGU_HTB_LAST_STEP_CHANGED_NFY));
								cPlayer->Broadcast(&packet2);
							}
							else
							{
								pHtb->SetHitCount(pHtb->GetTbldat()->bySetCount + 1);
							}
						}

						if (pHtb->GetAttackFailed() == false)
						{
							sCHARSTATE curState;
							cPlayer->GetStateManager()->CopyTo(&curState);

							//apply reflect
							if (curState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[pHtb->GetCurrentStep() - 1].sSkillResult.damageByReflectingCurse > 0.f)
							{
								cPlayer->OnSkillAction(pTarget, (int)curState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[pHtb->GetCurrentStep() - 1].sSkillResult.damageByReflectingCurse, 0, BATTLE_ATTACK_RESULT_REFLECTED_DAMAGE, false);
							}

							float fDmg = curState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[pHtb->GetCurrentStep() - 1].sSkillResult.effectResult[0].DD_DOT_fDamage;

							//apply dmg
							if (fDmg > 0.f)
							{
								pTarget->OnSkillAction(cPlayer, (int)fDmg, (DWORD)fDmg, curState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[pHtb->GetCurrentStep() - 1].sSkillResult.byAttackResult, true);
							}

							if (curState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[pHtb->GetCurrentStep() - 1].sSkillResult.byAttackResult == BATTLE_ATTACK_RESULT_KNOCKDOWN)
							{
								pTarget->SendCharStateKnockdown(curState.sCharStateDetail.sCharStateHTB.aHTBSkillResult[0].sSkillResult.vShift);
							}
						}

						//end htb
						if (pHtb->GetCurrentStep() > pHtb->GetTbldat()->bySetCount || (pHtb->GetAttackFailed() && pHtb->GetCurrentStep() == pHtb->GetTbldat()->byStop_Point + 1))
						{
							pHtb->SetHitCount(0);
							pHtb->SetTarget(INVALID_HOBJECT);
							pTarget->SetHtbSkillCaster(cPlayer->GetID());
							cPlayer->SetCurrentHtbSkill(INVALID_BYTE);

							if (pTarget->IsFainting() == false && pTarget->IsKnockedDown() == false)
							{
								pTarget->SendCharStateStanding();
							}

							if (cPlayer->IsFainting() == false) //check if faint. Maybe player faint when receiving reflect dmg
								cPlayer->SendCharStateStanding();
						}
						else
						{
							if (pTarget->IsFainting())
							{
								pHtb->SetHitCount(0);
								pHtb->SetTarget(INVALID_HOBJECT);
								pTarget->SetHtbSkillCaster(cPlayer->GetID());
								cPlayer->SetCurrentHtbSkill(INVALID_BYTE);

								CNtlPacket packet2(sizeof(sGU_HTB_LAST_STEP_CHANGED_NFY));
								sGU_HTB_LAST_STEP_CHANGED_NFY * res2 = (sGU_HTB_LAST_STEP_CHANGED_NFY*)packet2.GetPacketData();
								res2->wOpCode = GU_HTB_LAST_STEP_CHANGED_NFY;
								res2->byLastStep = INVALID_BYTE;
								res2->hSubject = pHtb->GetTarget();
								packet2.SetPacketLen(sizeof(sGU_HTB_LAST_STEP_CHANGED_NFY));
								cPlayer->Broadcast(&packet2);

								cPlayer->SendCharStateStanding();
							}
						}
					}
					else
					{
						pHtb->SetHitCount(0);
						pHtb->SetTarget(INVALID_HOBJECT);

						if(pTarget)
							pTarget->SetHtbSkillCaster(cPlayer->GetID());

						cPlayer->SetCurrentHtbSkill(INVALID_BYTE);

						cPlayer->SendCharStateStanding();

						CNtlPacket packet2(sizeof(sGU_HTB_LAST_STEP_CHANGED_NFY));
						sGU_HTB_LAST_STEP_CHANGED_NFY * res2 = (sGU_HTB_LAST_STEP_CHANGED_NFY*)packet2.GetPacketData();
						res2->wOpCode = GU_HTB_LAST_STEP_CHANGED_NFY;
						res2->byLastStep = 1;
						res2->hSubject = cPlayer->GetID();
						packet2.SetPacketLen(sizeof(sGU_HTB_LAST_STEP_CHANGED_NFY));
						cPlayer->Broadcast(&packet2);
					}
				}
			}
			else wResultcode = GAME_HTB_YOU_HAVE_NO_RELATION_WITH_HTB;
		}
		else wResultcode = GAME_HTB_YOU_HAVE_NO_RELATION_WITH_HTB;
	}

	CNtlPacket packetEnd(sizeof(sGU_HTB_FORWARD_RES));
	sGU_HTB_FORWARD_RES * resEnd = (sGU_HTB_FORWARD_RES*)packetEnd.GetPacketData();
	resEnd->wOpCode = GU_HTB_FORWARD_RES;
	resEnd->wResultCode = wResultcode;
	packetEnd.SetPacketLen(sizeof(sGU_HTB_FORWARD_RES));
	g_pApp->Send(GetHandle(), &packetEnd);
}

//-----------------------------------------------------------//
//	START/STOP CHARGING RP
//-----------------------------------------------------------//
void CClientSession::RecvRpCharge(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_CHARGE * req = (sUG_CHAR_CHARGE*)pPacket->GetPacketData();

	//check if player has skill
	if (cPlayer->GetSkillManager()->FindSkillWithSystemEffectCode(PASSIVE_CHARGE) == NULL)
	{
		ERR_LOG(LOG_USER, "Player %u try to use charge skill but dont have that skill !!!", cPlayer->GetCharID());
		return;
	}
	
	if(req->bCharge) //Start charging
	{
		if (cPlayer->GetStateManager()->IsCharCondition(CHARCOND_CHARGING_BLOCKED))
			return;
		
		cPlayer->GetStateManager()->ChangeCharState(CHARSTATE_CHARGING, NULL, true);
	}
	else //Stop charging
	{
		if (cPlayer->IsCharging()) //Only change state if player is still charging
			cPlayer->SendCharStateStanding();
	}
}
//-----------------------------------------------------------//
//	START/STOP BLOCKING
//-----------------------------------------------------------//
void CClientSession::RecvBlockMode(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_BLOCK_MODE * req = (sUG_CHAR_BLOCK_MODE*)pPacket->GetPacketData();

	if (req->bIsOn)
	{
		if (cPlayer->GetCharStateID() == CHARSTATE_GUARD)
			return;

		cPlayer->ChangeAttackProgress(false);
		cPlayer->SetAttackTarget(INVALID_HOBJECT);

		cPlayer->StartGuard();
	}
	else
	{
		if(cPlayer->GetCharStateID() == CHARSTATE_GUARD)
			cPlayer->SendCharStateStanding();
	}
}

//--------------------------------------------------------------------------------------//
//		RECEIVE SKILL TARGETS
//--------------------------------------------------------------------------------------//
void CClientSession::RecvSkillTargetList(CNtlPacket *pPacket)
{
	sUG_SKILL_TARGET_LIST * req = (sUG_SKILL_TARGET_LIST*)pPacket->GetPacketData();

	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	if (req->byAvatarType == DBO_AVATAR_TYPE_AVATAR)
	{
		if (cPlayer->IsCastingSkill() && cPlayer->GetSkillManager()->IsCastingFinished()) //check if player is casting the skill && casting finished
		{
			CSkillPc* pSkill = (CSkillPc*)cPlayer->GetSkillManager()->FindSkill(cPlayer->GetSkillManager()->GetCurSkillTblidx());
			if (pSkill)
			{
				cPlayer->GetSkillManager()->SetCastingFinished(false);
				cPlayer->GetSkillManager()->SetCurSkillTblidx(INVALID_TBLIDX);

				BYTE byTargetCount = req->byApplyTargetCount;

				if (byTargetCount > pSkill->GetOriginalTableData()->byApply_Target_Max)
					byTargetCount = pSkill->GetOriginalTableData()->byApply_Target_Max;

			//	printf("byTargetCount: %u, req->ahApplyTarget[0]: %u \n", byTargetCount, req->ahApplyTarget[0]);

				pSkill->CastSkill(req->ahApplyTarget[0], byTargetCount, req->ahApplyTarget);
			}
		}
		else
		{
			ERR_LOG(LOG_USER, "HACK-ATTEMPT!! Player is still in casting. Should not receive this packet. Player %u", cPlayer->GetCharID());
		}
	}
}

//-----------------------------------------------------------//
// GMT UPDATES
//-----------------------------------------------------------//
void CClientSession::RecvGmtUpdateReq(CNtlPacket * pPacket)
{
	sUG_GMT_UPDATE_REQ * req = (sUG_GMT_UPDATE_REQ*)pPacket->GetPacketData();
	
	CNtlPacket packet(sizeof(sGU_GMT_UPDATE_RES));
	sGU_GMT_UPDATE_RES * res = (sGU_GMT_UPDATE_RES*)packet.GetPacketData();
	res->wOpCode = GU_GMT_UPDATE_RES;
	res->wResultCode = GAME_SUCCESS;
//	res->sNext = req->sNext;
	packet.SetPacketLen(sizeof(sGU_GMT_UPDATE_RES));
	g_pApp->Send(GetHandle(), &packet);
}

//--------------------------------------------------------------------------------------//
//		ADD WAR FOG INFO (MAP)
//--------------------------------------------------------------------------------------//
void CClientSession::RecvWarFogUpdateReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;
	sUG_WAR_FOG_UPDATE_REQ * req = (sUG_WAR_FOG_UPDATE_REQ*)pPacket->GetPacketData();

	WORD ResultCode = GAME_SUCCESS;
	TBLIDX contentTblidx = INVALID_TBLIDX;

	if (cPlayer->GetCurWorld() == NULL)
		ResultCode = GAME_WORLD_NOT_FOUND;
	else
	{
		CTriggerObject* obj = cPlayer->GetCurWorld()->FindStaticObject(req->hObject);
		if (obj == NULL)
			ResultCode = GAME_COMMON_CANT_FIND_OBJECT;
		else if (cPlayer->IsInRange(obj->GetCurLoc(), 30.0f) == false)
			ResultCode = GAME_TARGET_TOO_FAR;
		else if (cPlayer->CheckWarFog(obj->GetContent()))
			ResultCode = GAME_WARFOG_ARLEADY_ADDED;
		
		else if(cPlayer->AddWarFogFlag(obj->GetContent()) == false) //add warfog and check if success
			ResultCode = GAME_FAIL;
		else
		{
			contentTblidx = obj->GetContent();
		}
	}

	if (ResultCode == GAME_SUCCESS)
	{
		CNtlPacket packetQry(sizeof(sGQ_WAR_FOG_UPDATE_REQ));
		sGQ_WAR_FOG_UPDATE_REQ * resQry = (sGQ_WAR_FOG_UPDATE_REQ*)packetQry.GetPacketData();
		resQry->wOpCode = GQ_WAR_FOG_UPDATE_REQ;
		resQry->charID = cPlayer->GetCharID();
		resQry->contentsTblidx = contentTblidx;
		memcpy(resQry->sInfo.achWarFogFlag, cPlayer->GetWarFogFlag(), sizeof(resQry->sInfo.achWarFogFlag));
		packetQry.SetPacketLen(sizeof(sGQ_WAR_FOG_UPDATE_REQ));
		app->SendTo(app->GetQueryServerSession(), &packetQry);
	}

	CNtlPacket packet(sizeof(sGU_WAR_FOG_UPDATE_RES));
	sGU_WAR_FOG_UPDATE_RES * res = (sGU_WAR_FOG_UPDATE_RES*)packet.GetPacketData();
	res->wOpCode = GU_WAR_FOG_UPDATE_RES;
	res->handle = req->hObject;
	res->wResultCode = ResultCode;
	packet.SetPacketLen(sizeof(sGU_WAR_FOG_UPDATE_RES));;
	app->Send(GetHandle(), &packet);
}

void CClientSession::RecvCharDashKeyBoard(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	// Check if player can move in battle royale (waiting for start period)
	if (g_pBattleRoyaleEvent && !g_pBattleRoyaleEvent->CanPlayerMove(cPlayer))
		return;

	sUG_CHAR_DASH_KEYBOARD * req = (sUG_CHAR_DASH_KEYBOARD*)pPacket->GetPacketData();

	if (cPlayer->GetCurEP() == 0)
	{
		ERR_LOG(LOG_USER, "Player %u try to use dash skill but dont have any EP !!!", cPlayer->GetCharID());
		return;
	}

	CNtlVector vLoc;
	NtlLocationDecompress(&req->vCurLoc, &vLoc.x, &vLoc.y, &vLoc.z);

	if (cPlayer->GetStateManager()->CanCharStateTransition(CHARSTATE_DASH_PASSIVE) == false)
		return;

	CSkill* pSkill = cPlayer->GetSkillManager()->FindSkillWithSystemEffectCode(PASSIVE_DASH);

	if (pSkill == NULL) //check if player has skill
	{
		ERR_LOG(LOG_USER, "Player %u try to use dash skill but dont have that skill !!!", cPlayer->GetCharID());
		this->Disconnect(false);

		return;
	}

	if (IsDashPossible(req->byMoveDirection) && cPlayer->SetCurLoc(vLoc, cPlayer->GetCurWorld()))
	{
		WORD wRequiredEP = pSkill->GetOriginalTableData()->wRequire_EP;
		cPlayer->GetDashPassiveRequiredEP(pSkill, pSkill->GetOriginalTableData()->dwCoolTimeInMilliSecs, wRequiredEP);

		cPlayer->UpdateCurEP(wRequiredEP, false, false); //update ep

		sVECTOR3 sDir;
		NtlDirectionDecompress(&req->vCurDir, &sDir.x, &sDir.y, &sDir.z);
		cPlayer->SetCurDir(sDir);

		// Update movement for battle royale idle detection
		if (cPlayer->GetCurWorld() && cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_RAID && cPlayer->GetCurWorld()->GetTbldat() && cPlayer->GetCurWorld()->GetTbldat()->tblidx == 212000)
		{
			if (g_pBattleRoyaleEvent)
				g_pBattleRoyaleEvent->UpdatePlayerMovement(cPlayer);
		}

		CNtlVector destLoc(vLoc);
		float fDashDist = 0.5f;

		if (req->byMoveDirection == NTL_MOVE_F)
		{
			fDashDist = Dbo_GetMaxDashDistanceForward(cPlayer->GetWalkingSpeed(), cPlayer->GetLastRunSpeed());
			destLoc += cPlayer->GetCurDir() * fDashDist;
		}
		else if (req->byMoveDirection == NTL_MOVE_B)
		{
			fDashDist = Dbo_GetMaxDashDistanceBackwardLeftRight(cPlayer->GetWalkingSpeed(), cPlayer->GetLastRunSpeed());
			destLoc += RotateVector180Degree(&cPlayer->GetCurDir()) * fDashDist;
		}
		else if (req->byMoveDirection == NTL_MOVE_L)
		{
			fDashDist = Dbo_GetMaxDashDistanceBackwardLeftRight(cPlayer->GetWalkingSpeed(), cPlayer->GetLastRunSpeed());
			destLoc += RotateVector90DegreeToLeft(&cPlayer->GetCurDir()) * fDashDist;
		}
		else
		{
			fDashDist = Dbo_GetMaxDashDistanceBackwardLeftRight(cPlayer->GetWalkingSpeed(), cPlayer->GetLastRunSpeed());
			destLoc += RotateVector90DegreeToRight(&cPlayer->GetCurDir()) * fDashDist;
		}

		//set dash state
		CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_STATE));
		sGU_UPDATE_CHAR_STATE * res = (sGU_UPDATE_CHAR_STATE *)packet.GetPacketData();
		res->wOpCode = GU_UPDATE_CHAR_STATE;
		res->handle = cPlayer->GetID();
		res->sCharState.sCharStateBase.byStateID = CHARSTATE_DASH_PASSIVE;
		cPlayer->GetStateManager()->CopyAspectTo(&res->sCharState.sCharStateBase.aspectState);
		cPlayer->GetCurLoc().CopyTo(res->sCharState.sCharStateBase.vCurLoc);
		cPlayer->GetCurDir().CopyTo(res->sCharState.sCharStateBase.vCurDir);
		res->sCharState.sCharStateBase.eAirState = AIR_STATE_OFF;
		res->sCharState.sCharStateBase.bFightMode = cPlayer->GetFightMode();
		res->sCharState.sCharStateBase.bIsValidLoc = true;
		res->sCharState.sCharStateBase.dwConditionFlag = cPlayer->GetConditionState();
		res->sCharState.sCharStateBase.dwStateTime = 0;
		res->sCharState.sCharStateDetail.sCharStateDashPassive.dwTimeStamp = GetTickCount();
		res->sCharState.sCharStateDetail.sCharStateDashPassive.byMoveDirection = req->byMoveDirection;
		res->sCharState.sCharStateDetail.sCharStateDashPassive.byMoveFlag = NTL_MOVE_FLAG_RUN;
		destLoc.CopyTo(res->sCharState.sCharStateDetail.sCharStateDashPassive.vDestLoc);
		cPlayer->GetStateManager()->CopyFrom(&res->sCharState);

		cPlayer->Broadcast(&packet, cPlayer); //send packet no matter what. Dont need to send to myself otherwise there will be issues when getting kd'ed while dashing
	}
	else
	{
		ERR_LOG(LOG_HACK, "Player: %u seems to be location hacking. CurLoc: %f, %f, %f NewLoc: %f %f %f", cPlayer->GetCharID(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().y, cPlayer->GetCurLoc().z, vLoc.x, vLoc.y, vLoc.z);
	}
}

void CClientSession::RecvCharDashMouse(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	// Check if player can move in battle royale (waiting for start period)
	if (g_pBattleRoyaleEvent && !g_pBattleRoyaleEvent->CanPlayerMove(cPlayer))
		return;

	sUG_CHAR_DASH_MOUSE * req = (sUG_CHAR_DASH_MOUSE*)pPacket->GetPacketData();

	if (cPlayer->GetCurEP() == 0)
	{
		ERR_LOG(LOG_USER, "Player %u try to use dash skill but dont have any EP !!!", cPlayer->GetCharID());

		return;
	}

	if (cPlayer->GetStateManager()->CanCharStateTransition(CHARSTATE_DASH_PASSIVE) == false)
		return;

	CSkill* pSkill = cPlayer->GetSkillManager()->FindSkillWithSystemEffectCode(PASSIVE_DASH);

	if (pSkill == NULL) //check if player has skill
	{
		ERR_LOG(LOG_USER, "Player %u try to use dash skill but dont have that skill !!!", cPlayer->GetCharID());
		this->Disconnect(false);
		return;
	}

	if (IsDashPossible(NTL_MOVE_F))
	{
		WORD wRequiredEP = pSkill->GetOriginalTableData()->wRequire_EP;
		cPlayer->GetDashPassiveRequiredEP(pSkill, pSkill->GetOriginalTableData()->dwCoolTimeInMilliSecs, wRequiredEP);

		cPlayer->UpdateCurEP(wRequiredEP, false, false); //update ep

		// Update movement for battle royale idle detection
		if (cPlayer->GetCurWorld() && cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_RAID && cPlayer->GetCurWorld()->GetTbldat() && cPlayer->GetCurWorld()->GetTbldat()->tblidx == 212000)
		{
			if (g_pBattleRoyaleEvent)
				g_pBattleRoyaleEvent->UpdatePlayerMovement(cPlayer);
		}

		CNtlVector vDestLoc;
		NtlLocationDecompress(&req->vDestLoc, &vDestLoc.x, &vDestLoc.y, &vDestLoc.z);

		CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_STATE));
		sGU_UPDATE_CHAR_STATE * res = (sGU_UPDATE_CHAR_STATE *)packet.GetPacketData();
		res->wOpCode = GU_UPDATE_CHAR_STATE;
		res->handle = cPlayer->GetID();
		res->sCharState.sCharStateBase.byStateID = CHARSTATE_DASH_PASSIVE;
		cPlayer->GetStateManager()->CopyAspectTo(&res->sCharState.sCharStateBase.aspectState);
		cPlayer->GetCurLoc().CopyTo(res->sCharState.sCharStateBase.vCurLoc);
		cPlayer->GetCurDir().CopyTo(res->sCharState.sCharStateBase.vCurDir);
		res->sCharState.sCharStateBase.eAirState = AIR_STATE_OFF;
		res->sCharState.sCharStateBase.bFightMode = cPlayer->GetFightMode();
		res->sCharState.sCharStateBase.bIsValidLoc = true;
		res->sCharState.sCharStateBase.dwConditionFlag = cPlayer->GetConditionState();
		res->sCharState.sCharStateBase.dwStateTime = 0;
		res->sCharState.sCharStateDetail.sCharStateDashPassive.dwTimeStamp = GetTickCount();
		res->sCharState.sCharStateDetail.sCharStateDashPassive.byMoveDirection = NTL_MOVE_F;
		res->sCharState.sCharStateDetail.sCharStateDashPassive.byMoveFlag = NTL_MOVE_FLAG_RUN;
		vDestLoc.CopyTo(res->sCharState.sCharStateDetail.sCharStateDashPassive.vDestLoc);
		packet.SetPacketLen(sizeof(sGU_UPDATE_CHAR_STATE));
		cPlayer->GetStateManager()->CopyFrom(&res->sCharState);

		cPlayer->Broadcast(&packet, cPlayer);//send packet no matter what. Dont need to send to myself otherwise there will be issues when getting kd'ed while dashing
	}
}

void CClientSession::RecvCharDashAir(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	// Check if player can move in battle royale (waiting for start period)
	if (g_pBattleRoyaleEvent && !g_pBattleRoyaleEvent->CanPlayerMove(cPlayer))
		return;

	sUG_CHAR_AIR_DASH * req = (sUG_CHAR_AIR_DASH*)pPacket->GetPacketData();

	if (req->byMoveDirection < NTL_MOVE_KEYBOARD_FIRST || req->byMoveDirection > NTL_MOVE_KEYBOARD_LAST)
	{
		ERR_LOG(LOG_USER, "Player %u send wrong movedirection %u", cPlayer->GetCharID(), req->byMoveDirection);
		return;
	}

	if (cPlayer->GetAirState() == AIR_STATE_ON)
	{
		CNtlVector vLoc;
		NtlLocationDecompress(&req->vCurLoc, &vLoc.x, &vLoc.y, &vLoc.z);

		if (cPlayer->SetCurLoc(vLoc, cPlayer->GetCurWorld()))
		{
			sVECTOR3 sDir;
			NtlDirectionDecompress(&req->vCurDir, &sDir.x, &sDir.y, &sDir.z);
			cPlayer->SetCurDir(sDir);

			// Update movement for battle royale idle detection
			if (cPlayer->GetCurWorld() && cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_RAID && cPlayer->GetCurWorld()->GetTbldat() && cPlayer->GetCurWorld()->GetTbldat()->tblidx == 212000)
			{
				g_pBattleRoyaleEvent->UpdatePlayerMovement(cPlayer);
			}

			if (cPlayer->GetMoveFlag() == NTL_MOVE_FLAG_FLY_ACCEL)
			{
				cPlayer->SendCharStateAirDashAccel(req->byMoveDirection);
			}
			else
			{
				cPlayer->SetCanAirAccel(true);
				cPlayer->SendCharStateAirDashAccel(req->byMoveDirection);
				//cPlayer->SetAirDashStartTime(GetTickCount());
				//cPlayer->SendCharStateMoving(req->byMoveDirection, NTL_MOVE_FLAG_FLY_DASH, NTL_MOVE_STATUS_AIR_MOVE);
			}
		}
		else
		{
			ERR_LOG(LOG_HACK, "Player: %u seems to be location hacking. CurLoc: %f, %f, %f NewLoc: %f %f %f", cPlayer->GetCharID(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().y, cPlayer->GetCurLoc().z, vLoc.x, vLoc.y, vLoc.z);
		}
	}
}

//--------------------------------------------------------------------------------------//
//		CHAR AIR ACCEL
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharAirAccel(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_AIR_ACCEL * req = (sUG_CHAR_AIR_ACCEL*)pPacket->GetPacketData();

	if (cPlayer->GetMoveFlag() == NTL_MOVE_FLAG_FLY_DASH && cPlayer->CanAirAccel())
	{
		cPlayer->SendCharStateAirDashAccel(cPlayer->GetMoveDirection());
	}
}

//--------------------------------------------------------------------------------------//
//	NetPY Shop start
//--------------------------------------------------------------------------------------//
void CClientSession::RecvNetPyStartReq(CNtlPacket * pPacket)
{
	CNtlPacket packet(sizeof(sGU_SHOP_NETPYITEM_START_RES));
	sGU_SHOP_NETPYITEM_START_RES* res = (sGU_SHOP_NETPYITEM_START_RES*)packet.GetPacketData();
	res->wOpCode = GU_SHOP_NETPYITEM_START_RES;
	res->wResultCode = GAME_SUCCESS;
	res->byType = 49;
	res->wUnknown = 7;
	res->wUnknown2 = 234;
	packet.SetPacketLen(sizeof(sGU_SHOP_NETPYITEM_START_RES));
	g_pApp->Send(GetHandle(), &packet);
}
//--------------------------------------------------------------------------------------//
//	NetPY Shop end
//--------------------------------------------------------------------------------------//
void CClientSession::RecvNetPyEndReq(CNtlPacket * pPacket)
{
	CNtlPacket packet(sizeof(sGU_SHOP_NETPYITEM_END_RES));
	sGU_SHOP_NETPYITEM_END_RES* res = (sGU_SHOP_NETPYITEM_END_RES*)packet.GetPacketData();
	res->wOpCode = GU_SHOP_NETPYITEM_END_RES;
	res->wResultCode = GAME_SUCCESS;
	packet.SetPacketLen(sizeof(sGU_SHOP_NETPYITEM_END_RES));
	g_pApp->Send(GetHandle(), &packet);
}

//--------------------------------------------------------------------------------------//
//	World map status // packet received when open world map (m button)
//--------------------------------------------------------------------------------------//
void CClientSession::RecvWorldMapStatus(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_WORLD_MAP_STATUS * req = (sUG_WORLD_MAP_STATUS*)pPacket->GetPacketData();

	//printf("bIsWorldMapOpen %i, worldMapZoneId %u \n", req->bIsWorldMapOpen, req->worldMapZoneId);

	if (req->bIsWorldMapOpen)
		g_pBusSystem->AddPlayerSync(cPlayer, req->worldMapZoneId);
	else
		g_pBusSystem->RemovePlayerSync(cPlayer);
}

//--------------------------------------------------------------------------------------//
//	cashitem Shop start
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCashItemStartReq(CNtlPacket * pPacket)
{
	sUG_CASHITEM_START_REQ * req = (sUG_CASHITEM_START_REQ*)pPacket->GetPacketData();

	CNtlPacket packet4(sizeof(sGU_CASHITEM_START_RES));
	sGU_CASHITEM_START_RES* res4 = (sGU_CASHITEM_START_RES*)packet4.GetPacketData();
	res4->wOpCode = GU_CASHITEM_START_RES;
	res4->wResultCode = GAME_SUCCESS;
	packet4.SetPacketLen(sizeof(sGU_CASHITEM_START_RES));
	g_pApp->Send(GetHandle(), &packet4);

}
//--------------------------------------------------------------------------------------//
//	cashitem Shop end
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCashItemEndReq(CNtlPacket * pPacket)
{
	CNtlPacket packet(sizeof(sGU_CASHITEM_END_RES));
	sGU_CASHITEM_END_RES* res = (sGU_CASHITEM_END_RES*)packet.GetPacketData();
	res->wOpCode = GU_CASHITEM_END_RES;
	res->wResultCode = GAME_SUCCESS;
	packet.SetPacketLen(sizeof(sGU_CASHITEM_END_RES));
	g_pApp->Send(GetHandle(), &packet);
}
//--------------------------------------------------------------------------------------//
//	move cash item to inventory
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCashItemMoveReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_CASHITEM_MOVE_REQ * req = (sUG_CASHITEM_MOVE_REQ*)pPacket->GetPacketData();

	WORD resultcode = GAME_SUCCESS;

	sCASHITEM_BRIEF* brief = cPlayer->GetCashShopItem(req->qwProductId);
	if (brief)
	{
		sHLS_ITEM_TBLDAT* hlsitem = (sHLS_ITEM_TBLDAT*)g_pTableContainer->GetHLSItemTable()->FindData(brief->HLSitemTblidx);
		if (hlsitem)
		{
			sITEM_TBLDAT* pItemTbldat = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(hlsitem->itemTblidx);
			if (pItemTbldat)
			{
				std::pair<BYTE, BYTE> pairInv = cPlayer->GetPlayerItemContainer()->GetEmptyInventory();
				if (pairInv.first != INVALID_BYTE && pairInv.second != INVALID_BYTE)
				{
					cPlayer->GetPlayerItemContainer()->AddReservedInventory(pairInv.first, pairInv.second);//mark that place and pos is reserved

					CNtlPacket packetQry(sizeof(sGQ_CASHITEM_MOVE_REQ));
					sGQ_CASHITEM_MOVE_REQ* resQry = (sGQ_CASHITEM_MOVE_REQ*)packetQry.GetPacketData();
					resQry->wOpCode = GQ_CASHITEM_MOVE_REQ;
					resQry->accountId = cPlayer->GetAccountID();
					resQry->charId = cPlayer->GetCharID();
					resQry->handle = cPlayer->GetID();
					resQry->qwProductId = req->qwProductId;

					::ZeroMemory(resQry->sData.awchMaker, sizeof(resQry->sData.awchMaker));
					resQry->sData.byCurrentDurability = pItemTbldat->byDurability;
					resQry->sData.byDurationType = pItemTbldat->byDurationType;
					resQry->sData.nUseStartTime = 0;
					resQry->sData.nUseEndTime = 0;
					if (resQry->sData.byDurationType == eDURATIONTYPE_FLATSUM)
					{
						resQry->sData.nUseStartTime = app->GetTime();
						resQry->sData.nUseEndTime = resQry->sData.nUseStartTime + pItemTbldat->dwUseDurationMax;
					}
					resQry->sData.byGrade = ITEM_GRADE_LEVEL_0;
					resQry->sData.byPlace = pairInv.first;
					resQry->sData.byPos = pairInv.second;
					resQry->sData.byRank = pItemTbldat->byRank;

					// If item is bound AND has no time limit, then make it as egg so it is possible to trade the item.
					if (resQry->sData.nUseStartTime == 0)
						resQry->sData.byRestrictState = ITEM_RESTRICT_STATE_TYPE_SEAL;//(pItemTbldat->byRestrictType > 0) ? ITEM_RESTRICT_STATE_TYPE_SEAL : pItemTbldat->byRestrictType; 

					resQry->sData.byStack = brief->byStackCount;
					resQry->sData.sOptionSet.Init();
					resQry->sData.tblItem = pItemTbldat->tblidx;

					packetQry.SetPacketLen(sizeof(sGQ_CASHITEM_MOVE_REQ));
					app->SendTo(app->GetQueryServerSession(), &packetQry);

					return;
				}
				else resultcode = GAME_ITEM_INVEN_FULL;
			}
			else resultcode = GAME_COMMON_CAN_NOT_FIND_TABLE_DATA;
		}
		else resultcode = GAME_COMMON_CAN_NOT_FIND_TABLE_DATA;
	}
	else resultcode = GAME_CASHITEM_NOT_FOUND;

	CNtlPacket packet(sizeof(sGU_CASHITEM_MOVE_RES));
	sGU_CASHITEM_MOVE_RES* res = (sGU_CASHITEM_MOVE_RES*)packet.GetPacketData();
	res->wOpCode = GU_CASHITEM_MOVE_RES;
	res->qwProductId = req->qwProductId;
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_CASHITEM_MOVE_RES));
	app->Send(GetHandle(), &packet);
}
//--------------------------------------------------------------------------------------//
//	use cash item
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCashItemUseReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_CASHITEM_USE_REQ * req = (sUG_CASHITEM_USE_REQ*)pPacket->GetPacketData();

	WORD resultcode = GAME_SUCCESS;

	sCASHITEM_BRIEF* item = cPlayer->GetCashShopItem(req->qwProductId);
	if (item)
	{
		sHLS_ITEM_TBLDAT* hlsitem = (sHLS_ITEM_TBLDAT*)g_pTableContainer->GetHLSItemTable()->FindData(item->HLSitemTblidx);
		if (hlsitem)
		{
			sITEM_TBLDAT* pItemTbldat = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(hlsitem->itemTblidx);
			if (pItemTbldat)
			{
			//	printf("pItemTbldat->tblidx %u, pItemTbldat->byItem_Type %u, pItemTbldat->byItemGroup %u, pItemTbldat->byCommonPointType %u \n", pItemTbldat->tblidx, pItemTbldat->byItem_Type, pItemTbldat->byItemGroup, pItemTbldat->byCommonPointType);
				if (pItemTbldat->byCommonPointType == 2) //update wagu coin for cash shop
				{
					if (pItemTbldat->byItem_Type == ITEM_TYPE_WAREHOUSE)
					{
						if (cPlayer->GetPlayerItemContainer()->GetItem(CONTAINER_TYPE_BANKSLOT, BANK_TABLE_TYPE_PUBLIC) == NULL)
						{
							CNtlPacket packetQry(sizeof(sGQ_CASHITEM_PUBLIC_BANK_ADD_REQ));
							sGQ_CASHITEM_PUBLIC_BANK_ADD_REQ* resQry = (sGQ_CASHITEM_PUBLIC_BANK_ADD_REQ*)packetQry.GetPacketData();
							resQry->wOpCode = GQ_CASHITEM_PUBLIC_BANK_ADD_REQ;
							resQry->accountID = cPlayer->GetAccountID();
							resQry->charId = cPlayer->GetCharID();
							resQry->handle = cPlayer->GetID();
							resQry->dwProductId = req->qwProductId;
							resQry->hlsItemNo = item->HLSitemTblidx;
							resQry->itemNo = pItemTbldat->tblidx;
							resQry->byPlace = CONTAINER_TYPE_BANKSLOT;
							resQry->byPosition = BANK_TABLE_TYPE_PUBLIC;
							resQry->byRank = pItemTbldat->byRank;
							resQry->byDurationType = pItemTbldat->byDurationType;
							resQry->nUseStartTime = app->GetTime();
							resQry->nUseEndTime = resQry->nUseStartTime + pItemTbldat->dwUseDurationMax;
							packetQry.SetPacketLen(sizeof(sGQ_CASHITEM_PUBLIC_BANK_ADD_REQ));
							app->SendTo(app->GetQueryServerSession(), &packetQry);

							cPlayer->GetPlayerItemContainer()->AddReservedInventory(CONTAINER_TYPE_BANKSLOT, BANK_TABLE_TYPE_PUBLIC);

							return;
						}
						else resultcode = CASHITEM_FAIL_BANK_ALREADY_EXIST;

					}
					else
					{
						sUSE_ITEM_TBLDAT* pUseItemTbldat = (sUSE_ITEM_TBLDAT*)g_pTableContainer->GetUseItemTable()->FindData(pItemTbldat->Use_Item_Tblidx);
						if (pUseItemTbldat)
						{
							if (app->GetChatServerSession())
							{
								CNtlPacket packetQry(sizeof(sGQ_WAGUWAGUMACHINE_COIN_INCREASE_REQ));
								sGQ_WAGUWAGUMACHINE_COIN_INCREASE_REQ* resQry = (sGQ_WAGUWAGUMACHINE_COIN_INCREASE_REQ*)packetQry.GetPacketData();
								resQry->wOpCode = GQ_WAGUWAGUMACHINE_COIN_INCREASE_REQ;
								resQry->handle = cPlayer->GetID();
								resQry->accountId = cPlayer->GetAccountID();
								resQry->characterId = cPlayer->GetCharID();
								resQry->wWaguCoin = (WORD)pUseItemTbldat->aSystem_Effect_Value[0];
								resQry->qwProductId = req->qwProductId;
								packetQry.SetPacketLen(sizeof(sGQ_WAGUWAGUMACHINE_COIN_INCREASE_REQ));
								app->SendTo(app->GetQueryServerSession(), &packetQry);

								return;
							}
							else resultcode = GAME_COMMON_CANT_DO_THAT_FOR_SOME_REASON;
						}
						else resultcode = GAME_CASHITEM_NOT_FOUND;
					}
				}
				else resultcode = GAME_FAIL;
			}
			else resultcode = GAME_CASHITEM_NOT_FOUND;
		}
		else resultcode = GAME_CASHITEM_NOT_FOUND;
	}
	else resultcode = GAME_CASHITEM_NOT_FOUND;

	CNtlPacket packet(sizeof(sGU_CASHITEM_USE_RES));
	sGU_CASHITEM_USE_RES* res = (sGU_CASHITEM_USE_RES*)packet.GetPacketData();
	res->wOpCode = GU_CASHITEM_USE_RES;
	res->wResultCode = resultcode;
	res->qwProductId = req->qwProductId;
	packet.SetPacketLen(sizeof(sGU_CASHITEM_USE_RES));
	app->Send(GetHandle(), &packet);
}

//--------------------------------------------------------------------------------------//
//	cashitem HLS Shop start
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCashItemHlsStartReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CNtlPacket packet(sizeof(sGU_CASHITEM_HLSHOP_START_RES));
	sGU_CASHITEM_HLSHOP_START_RES* res = (sGU_CASHITEM_HLSHOP_START_RES*)packet.GetPacketData();
	res->wOpCode = GU_CASHITEM_HLSHOP_START_RES;
	res->wResultCode = GAME_SUCCESS;
	res->dwRemainAmount = cPlayer->GetItemShopCash();
	packet.SetPacketLen(sizeof(sGU_CASHITEM_HLSHOP_START_RES));
	g_pApp->Send(GetHandle(), &packet);
}
//--------------------------------------------------------------------------------------//
//	cashitem HLS Shop end
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCashItemHlsEndReq(CNtlPacket * pPacket)
{
	CNtlPacket packet(sizeof(sGU_CASHITEM_HLSHOP_END_RES));
	sGU_CASHITEM_HLSHOP_END_RES* res = (sGU_CASHITEM_HLSHOP_END_RES*)packet.GetPacketData();
	res->wOpCode = GU_CASHITEM_HLSHOP_END_RES;
	res->wResultCode = GAME_SUCCESS;
	packet.SetPacketLen(sizeof(sGU_CASHITEM_HLSHOP_END_RES));
	g_pApp->Send(GetHandle(), &packet);
}

//--------------------------------------------------------------------------------------//
//	cashitem HLS Shop refresh
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCashItemHlsRefreshReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	DWORD dwCurTick = GetTickCount();

	//check if can refresh again
	if (cPlayer->GetCashShopRefresh() < dwCurTick)
	{
		CGameServer* app = (CGameServer*)g_pApp;
		cPlayer->SetCashShopRefresh(dwCurTick + NTL_CASHITEM_RELOAD_TIME); // set timer to not allow refresh within next minute

		CNtlPacket packet(sizeof(sGQ_CASHITEM_HLSHOP_REFRESH_REQ));
		sGQ_CASHITEM_HLSHOP_REFRESH_REQ* res = (sGQ_CASHITEM_HLSHOP_REFRESH_REQ*)packet.GetPacketData();
		res->wOpCode = GQ_CASHITEM_HLSHOP_REFRESH_REQ;
		res->accountId = cPlayer->GetAccountID();
		res->charId = cPlayer->GetCharID();
		res->handle = cPlayer->GetID();
		packet.SetPacketLen(sizeof(sGQ_CASHITEM_HLSHOP_REFRESH_REQ));
		app->SendTo(app->GetQueryServerSession(), &packet);
	}
	else
	{
		CNtlPacket packet(sizeof(sGU_CASHITEM_HLSHOP_REFRESH_RES));
		sGU_CASHITEM_HLSHOP_REFRESH_RES* res = (sGU_CASHITEM_HLSHOP_REFRESH_RES*)packet.GetPacketData();
		res->wOpCode = GU_CASHITEM_HLSHOP_REFRESH_RES;
		res->wResultCode = GAME_SUCCESS;
		res->dwRemainAmount = cPlayer->GetItemShopCash();
		packet.SetPacketLen(sizeof(sGU_CASHITEM_HLSHOP_REFRESH_RES));
		g_pApp->Send(GetHandle(), &packet);
	}
}

//--------------------------------------------------------------------------------------//
//	cashitem buy
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCashItemBuyReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_CASHITEM_BUY_REQ * req = (sUG_CASHITEM_BUY_REQ*)pPacket->GetPacketData();

	sHLS_ITEM_TBLDAT* hlsitem = (sHLS_ITEM_TBLDAT*)g_pTableContainer->GetHLSItemTable()->FindData(req->dwIdxHlsTable);

	WORD resultcode = GAME_SUCCESS;

	if (hlsitem && hlsitem->bOnSale == true)
	{
		if (hlsitem->dwCash <= cPlayer->GetItemShopCash())
		{
			BYTE bycount = hlsitem->byStackCount;

			if (hlsitem->wHLSItemType == CASHITEM_TYPE_WAGUCOIN)
				bycount = 1;

			cPlayer->SetCashShopRefresh(GetTickCount() + NTL_CASHITEM_RELOAD_TIME); // set timer to not allow refresh cash shop within next minute

			CNtlPacket packetQry(sizeof(sGQ_CASHITEM_BUY_REQ));
			sGQ_CASHITEM_BUY_REQ* resQry = (sGQ_CASHITEM_BUY_REQ*)packetQry.GetPacketData();
			resQry->wOpCode = GQ_CASHITEM_BUY_REQ;
			resQry->accountId = cPlayer->GetAccountID();
			resQry->characterId = cPlayer->GetCharID();
			resQry->handle = cPlayer->GetID();
			resQry->byCount = bycount;
			resQry->dwPrice = hlsitem->dwCash;
			resQry->HLSitemTblidx = hlsitem->tblidx;
			packetQry.SetPacketLen(sizeof(sGQ_CASHITEM_BUY_REQ));
			app->SendTo(app->GetQueryServerSession(), &packetQry);

			return;
		}
		else resultcode = CASHITEM_FAIL_NEED_MORE_CASH;
	}
	else resultcode = GAME_CASHITEM_NOT_FOUND;

	CNtlPacket packet(sizeof(sGU_CASHITEM_BUY_RES));
	sGU_CASHITEM_BUY_RES * res = (sGU_CASHITEM_BUY_RES *)packet.GetPacketData();
	res->wOpCode = GU_CASHITEM_BUY_RES;
	res->dwRemainAmount = cPlayer->GetItemShopCash();
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_CASHITEM_BUY_RES));
	app->Send(GetHandle(), &packet);
}
//--------------------------------------------------------------------------------------//
//	cashitem SEND GIFT
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCashItemSendGiftReq(CNtlPacket * pPacket)
{
	/*
	
	*/
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_CASHITEM_SEND_GIFT_REQ * req = (sUG_CASHITEM_SEND_GIFT_REQ*)pPacket->GetPacketData();

	sHLS_ITEM_TBLDAT* hlsitem = (sHLS_ITEM_TBLDAT*)g_pTableContainer->GetHLSItemTable()->FindData(req->dwIdxHlsTable);

	WORD resultcode = GAME_SUCCESS;

	char* chname = Ntl_WC2MB(req->wchName);
	std::string charname = chname;

	if (charname.length() < NTL_MIN_SIZE_CHAR_NAME)
		resultcode = CHARACTER_TOO_SHORT_NAME;
	else if (charname.length() > NTL_MAX_SIZE_CHAR_NAME)
		resultcode = CHARACTER_TOO_LONG_NAME;
	//else if (charname.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890") != std::string::npos)
	//	resultcode = CHARACTER_BLOCK_STRING_INCLUDED;

	else if (_wcsicmp(cPlayer->GetCharName(), req->wchName) != 0)
	{
		if (hlsitem && hlsitem->bOnSale)
		{
			if (hlsitem->dwCash <= cPlayer->GetItemShopCash())
			{
				BYTE bycount = hlsitem->byStackCount;

				if (hlsitem->wHLSItemType == CASHITEM_TYPE_WAGUCOIN)
					bycount = 1;


				cPlayer->SetCashShopRefresh(GetTickCount() + NTL_CASHITEM_RELOAD_TIME); // set timer to not allow refresh cash shop within next minute

				CNtlPacket packetQry(sizeof(sGQ_CASHITEM_SEND_GIFT_REQ));
				sGQ_CASHITEM_SEND_GIFT_REQ* resQry = (sGQ_CASHITEM_SEND_GIFT_REQ*)packetQry.GetPacketData();
				resQry->wOpCode = GQ_CASHITEM_SEND_GIFT_REQ;
				resQry->handle = cPlayer->GetID();
				resQry->SenderCharId = cPlayer->GetCharID();
				resQry->SenderAccountId = cPlayer->GetAccountID();
				resQry->SenderServerFarmId = app->GetGsServerId();
				NTL_SAFE_WCSCPY(resQry->wchSenderName, cPlayer->GetCharName());
				NTL_SAFE_WCSCPY(resQry->wchDestName, req->wchName);
				resQry->DestServerFarmId = req->serverFarmId;

				resQry->dwIdxHlsTable = hlsitem->tblidx;
				resQry->byCount = bycount;
				resQry->dwDuration = hlsitem->dwHLSDurationTime;
				resQry->dwPrice = hlsitem->dwCash;
				packetQry.SetPacketLen(sizeof(sGQ_CASHITEM_SEND_GIFT_REQ));
				app->SendTo(app->GetQueryServerSession(), &packetQry);

				return;
			}
			else resultcode = CASHITEM_FAIL_NEED_MORE_CASH;
		}
		else resultcode = GAME_CASHITEM_NOT_FOUND;
	}
	else resultcode = CASHITEM_FAIL_CANT_GIFT_MYSELF;

	Ntl_CleanUpHeapString(chname);

	CNtlPacket packet2(sizeof(sGU_CASHITEM_SEND_GIFT_RES));
	sGU_CASHITEM_SEND_GIFT_RES * res2 = (sGU_CASHITEM_SEND_GIFT_RES *)packet2.GetPacketData();
	res2->wOpCode = GU_CASHITEM_SEND_GIFT_RES;
	res2->dwRemainAmount = cPlayer->GetItemShopCash();
	res2->wResultCode = resultcode;
	res2->dwIdxHlsTable = req->dwIdxHlsTable;
	res2->serverFarmId = req->serverFarmId;
	NTL_SAFE_WCSCPY(res2->wchName, req->wchName);
	packet2.SetPacketLen(sizeof(sGU_CASHITEM_SEND_GIFT_RES));
	app->Send(GetHandle(), &packet2);
}

//--------------------------------------------------------------------------------------//
//		BANK START
//--------------------------------------------------------------------------------------//
void CClientSession::RecvBankStartReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_BANK_START_REQ * req = (sUG_BANK_START_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_BANK_START_RES));
	sGU_BANK_START_RES * res = (sGU_BANK_START_RES *)packet.GetPacketData();
	res->wOpCode = GU_BANK_START_RES;
	res->wResultCode  = GAME_FAIL;
	res->handle = req->handle;

	if (req->handle != INVALID_HOBJECT)
	{
		CNpc* pNpc = g_pObjectManager->GetNpc(req->handle);
		if (pNpc == NULL)	//check if NPC exist
			res->wResultCode = GAME_TARGET_NOT_FOUND;
		else if (cPlayer->IsInRange(pNpc, DBO_DISTANCE_CHECK_TOLERANCE) == false) //check if npc is in range
			res->wResultCode = GAME_TARGET_TOO_FAR;
		else if (pNpc->GetTbldat()->byJob != NPC_JOB_BANKER)	//check if npc is a banke
			res->wResultCode = GAME_TARGET_HAS_DIFFERENT_JOB;
	}
	else if(cPlayer->CanOpenRemoteBank() == false)
		res->wResultCode = GAME_FAIL;

	if (cPlayer->IsUsingBank() == true)	//check if has the bank open
		res->wResultCode = GAME_FAIL;
	else if(cPlayer->IsBankLoaded() == false)	//bank is loaded before this packet is received
		res->wResultCode = GAME_FAIL;
	else
	{
		res->wResultCode = GAME_SUCCESS;
		cPlayer->SetUsingBank(true);
	}

	packet.SetPacketLen( sizeof(sGU_BANK_START_RES) );
	g_pApp->Send(GetHandle(), &packet );

}

//--------------------------------------------------------------------------------------//
//		BANK END
//--------------------------------------------------------------------------------------//
void CClientSession::RecvBankEndReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CNtlPacket packet(sizeof(sGU_BANK_END_RES));
	sGU_BANK_END_RES * res = (sGU_BANK_END_RES *)packet.GetPacketData();
	res->wOpCode = GU_BANK_END_RES;
	res->wResultCode  = GAME_FAIL;

	if (cPlayer->IsUsingBank())
	{
		res->wResultCode = GAME_SUCCESS;
		cPlayer->SetUsingBank(false);
	}

	packet.SetPacketLen( sizeof(sGU_BANK_END_RES) );
	g_pApp->Send(GetHandle(), &packet );
}

//--------------------------------------------------------------------------------------//
//		BANK LOAD
//--------------------------------------------------------------------------------------//
void CClientSession::RecvBankLoadReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_BANK_LOAD_REQ * req = (sUG_BANK_LOAD_REQ*)pPacket->GetPacketData();

	WORD wResultcode = GAME_SUCCESS;

	if (cPlayer->IsUsingBank() == true)	//check if user already use bank
		wResultcode = GAME_FAIL;

	if (cPlayer->IsBankLoaded() || wResultcode != GAME_SUCCESS)
	{
		CNtlPacket packet2(sizeof(sGU_BANK_LOAD_RES));
		sGU_BANK_LOAD_RES * res2 = (sGU_BANK_LOAD_RES *)packet2.GetPacketData();
		res2->wOpCode = GU_BANK_LOAD_RES;
		res2->handle = req->handle;
		res2->wResultCode = wResultcode;
		packet2.SetPacketLen(sizeof(sGU_BANK_LOAD_RES));
		app->Send(GetHandle(), &packet2);
	}
	else
	{
		CNtlPacket packet(sizeof(sGQ_LOAD_PC_BANK_DATA_REQ));
		sGQ_LOAD_PC_BANK_DATA_REQ * res = (sGQ_LOAD_PC_BANK_DATA_REQ *)packet.GetPacketData();
		res->wOpCode = GQ_LOAD_PC_BANK_DATA_REQ;
		res->accountId = cPlayer->GetAccountID();
		res->charId = cPlayer->GetCharID();
		res->handle = cPlayer->GetID();
		res->npchandle = req->handle;
		packet.SetPacketLen(sizeof(sGQ_LOAD_PC_BANK_DATA_REQ));
		app->SendTo(app->GetQueryServerSession(), &packet);
	}
}

//--------------------------------------------------------------------------------------//
//		BANK BUY
//--------------------------------------------------------------------------------------//
void CClientSession::RecvBankBuyReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_BANK_BUY_REQ * req = (sUG_BANK_BUY_REQ*)pPacket->GetPacketData();

	CGameServer* app = (CGameServer*)g_pApp;

	if (req->byPos >= NTL_MAX_MERCHANT_COUNT || req->byMerchantTab >= NTL_MAX_MERCHANT_TAB_COUNT)
		return;

	WORD wResultcode = GAME_SUCCESS;
	

	CNpc* pNpc = g_pObjectManager->GetNpc(req->hNpchandle);
	sMERCHANT_TBLDAT* pMerchantTbldat = NULL;
	sITEM_TBLDAT* pItemTbldat = NULL;


	if (pNpc == NULL)	//check if NPC exist
		wResultcode = GAME_TARGET_NOT_FOUND;

	else if (cPlayer->IsInRange(pNpc, DBO_DISTANCE_CHECK_TOLERANCE) == false) //check if npc is in range
		wResultcode = GAME_TARGET_TOO_FAR;

	else if (pNpc->GetTbldat()->byJob != NPC_JOB_BANKER)	//check if npc is a banker
		wResultcode = GAME_TARGET_HAS_DIFFERENT_JOB;

	else if (cPlayer->IsBankLoaded() == false)	//check if has the bank loaded
		wResultcode = GAME_FAIL;

	else if(cPlayer->GetPlayerItemContainer()->GetItem(CONTAINER_TYPE_BANKSLOT, req->byPos) != NULL)	//check if already has bank
		wResultcode = GAME_BANK_ALREADY_EXIST;
	else if(cPlayer->GetPlayerItemContainer()->IsInventoryReserved(CONTAINER_TYPE_BANKSLOT, req->byPos))
		wResultcode = GAME_BANK_ALREADY_EXIST;

	else
	{
		pMerchantTbldat = (sMERCHANT_TBLDAT*)g_pTableContainer->GetMerchantTable()->FindData(pNpc->GetMerchant(req->byMerchantTab));

		if(pMerchantTbldat == NULL)
			wResultcode = GAME_COMMON_CAN_NOT_FIND_TABLE_DATA;
		else if (pMerchantTbldat->bySell_Type != MERCHANT_SELL_TYPE_BANK)
			wResultcode = GAME_FAIL;
		else
		{
			pItemTbldat = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(pMerchantTbldat->aitem_Tblidx[req->byPos]);
			if (pItemTbldat == NULL)
				wResultcode = GAME_COMMON_CAN_NOT_FIND_TABLE_DATA;

			else if (cPlayer->GetZeni() < pItemTbldat->dwCost)	//check if has enough zeni
				wResultcode = GAME_ZENNY_NOT_ENOUGH;
		}
	}
	
	if (wResultcode == GAME_SUCCESS)
	{
		//reseve dest place/pos to avoid item getting there while moving. Only need to do this when moving to a free slot
		cPlayer->GetPlayerItemContainer()->AddReservedInventory(CONTAINER_TYPE_BANKSLOT, req->byPos);

		CNtlPacket packet2(sizeof(sGQ_BANK_BUY_REQ));
		sGQ_BANK_BUY_REQ * res2 = (sGQ_BANK_BUY_REQ *)packet2.GetPacketData();
		res2->wOpCode = GQ_BANK_BUY_REQ;
		res2->handle = cPlayer->GetID();
		res2->npchandle = req->hNpchandle;
		res2->accountID = cPlayer->GetAccountID();
		res2->byMerchantTab = req->byMerchantTab;
		res2->byMerchantPos = req->byPos;
		res2->dwZenny = pItemTbldat->dwCost;
		res2->itemNo = pItemTbldat->tblidx;
		res2->charId = cPlayer->GetCharID();
		res2->byPlace = CONTAINER_TYPE_BANKSLOT;
		res2->byPosition = req->byPos;
		res2->byRank = pItemTbldat->byRank;
		res2->byDurationType = pItemTbldat->byDurationType;

		if (pItemTbldat->byDurationType == eDURATIONTYPE_FLATSUM)
		{
			res2->nUseStartTime = app->GetTime();
			res2->nUseEndTime = res2->nUseStartTime + pItemTbldat->dwUseDurationMax;
		}

		packet2.SetPacketLen(sizeof(sGQ_BANK_BUY_REQ));
		app->SendTo(app->GetQueryServerSession(), &packet2);
	}
	else
	{
		CNtlPacket packet(sizeof(sGU_BANK_BUY_RES));
		sGU_BANK_BUY_RES * res = (sGU_BANK_BUY_RES *)packet.GetPacketData();
		res->wOpCode = GU_BANK_BUY_RES;
		res->hNpchandle = req->hNpchandle;
		res->wResultCode = wResultcode;
		packet.SetPacketLen(sizeof(sGU_BANK_BUY_RES));
		app->Send(GetHandle(), &packet);
	}
}

//--------------------------------------------------------------------------------------//
//		BANK MONEY
//--------------------------------------------------------------------------------------//
void CClientSession::RecvBankZeniReq(CNtlPacket * pPacket)
{
	WORD result = GAME_FAIL;
	sUG_BANK_ZENNY_REQ * req = (sUG_BANK_ZENNY_REQ*)pPacket->GetPacketData();


}
//--------------------------------------------------------------------------------------//
//		BANK MOVE ITEM
//--------------------------------------------------------------------------------------//
void CClientSession::RecvBankMoveReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_BANK_MOVE_REQ * req = (sUG_BANK_MOVE_REQ*)pPacket->GetPacketData();
	WORD item_move_res = GAME_SUCCESS;

	CNtlPacket pQry(sizeof(sGQ_BANK_MOVE_REQ));
	sGQ_BANK_MOVE_REQ * rQry = (sGQ_BANK_MOVE_REQ *)pQry.GetPacketData();
	rQry->wOpCode = GQ_BANK_MOVE_REQ;
	rQry->hNpcHandle = req->handle;
	rQry->handle = cPlayer->GetID();
	rQry->charId = cPlayer->GetCharID();
	rQry->accountId = cPlayer->GetAccountID();
	rQry->bySrcPlace = req->bySrcPlace;
	rQry->bySrcPos = req->bySrcPos;
	rQry->byDstPlace = req->byDestPlace;
	rQry->byDstPos = req->byDestPos;
	rQry->dstItemId = INVALID_ITEMID;


	CItem* src_item = NULL;
	if (cPlayer->IsUsingBank() == false || cPlayer->IsBankLoaded() == false || cPlayer->GetPlayerItemContainer()->IsUsingGuildBank())
	{
		item_move_res = GAME_FAIL;
		goto END;
	}

	if (cPlayer->GetPlayerItemContainer()->IsInventoryReserved(req->bySrcPlace, req->bySrcPos) || cPlayer->GetPlayerItemContainer()->IsInventoryReserved(req->byDestPlace, req->byDestPos))
	{
		item_move_res = GAME_FAIL;
		goto END;
	}

	src_item = cPlayer->GetPlayerItemContainer()->GetItem(req->bySrcPlace, req->bySrcPos);
	if (src_item)
	{
		//check if source item can be stored in bank
		if (src_item->CanWarehouse(req->byDestPlace == CONTAINER_TYPE_BANK4) == false)
		{
			item_move_res = GAME_ITEM_NOT_GO_THERE;
			goto END;
		}

		if (src_item->IsEquipped() == false && src_item->IsLocked() || src_item->GetOwner() == NULL)
		{
			item_move_res = GAME_ITEM_IS_LOCK;
			goto END;
		}

		CItem* dest_item = cPlayer->GetPlayerItemContainer()->GetItem(req->byDestPlace, req->byDestPos);

		sITEM_TBLDAT* pItemData = src_item->GetTbldat();

		if (dest_item) //check if dest item can be stored in bank
		{
			if (dest_item->CanWarehouse(req->bySrcPlace == CONTAINER_TYPE_BANK4) == false)
			{
				item_move_res = GAME_ITEM_NOT_GO_THERE;
				goto END;
			}
			if (dest_item->IsEquipped() == false && dest_item->IsLocked() || dest_item->GetOwner() == NULL)
			{
				item_move_res = GAME_ITEM_IS_LOCK;
				goto END;
			}
		}

		if (req->bySrcPlace == CONTAINER_TYPE_BAGSLOT) //move bag into bank
		{
			if (req->bySrcPos == BAGSLOT_POSITION_BAGSLOT_POSITION_0) //DONT ALLOW TO MOVE THE FIRST BAG SLOT.
			{
				item_move_res = GAME_ITEM_IS_LOCK;
				goto END;
			}

			if (!cPlayer->GetPlayerItemContainer()->IsBagEmpty(req->bySrcPos)) //check if bag is empty
			{
				item_move_res = GAME_ITEM_IS_LOCK;
				goto END;
			}
		}

		if (req->bySrcPlace == CONTAINER_TYPE_BANKSLOT || req->byDestPlace == CONTAINER_TYPE_BANKSLOT) //dont allow moving anything from/to bankslot
		{
			item_move_res = GAME_ITEM_IS_LOCK;
			goto END;
		}

		//check if can move into dest inventory
		if (IsInvenContainer(req->byDestPlace))
		{
			CItem* pBagItem = cPlayer->GetPlayerItemContainer()->GetActiveBag(req->byDestPlace - 1);
			if (pBagItem)
			{
				if (req->byDestPos >= pBagItem->GetBagSize())
				{
					item_move_res = GAME_ITEM_POSITION_FAIL;
					goto END;
				}

				if (pBagItem->IsExpired())
				{
					item_move_res = GAME_FAIL;
					goto END;
				}
			}
			else
			{
				item_move_res = GAME_ITEM_POSITION_FAIL;
				goto END;
			}
		}

		//check if bank bag pos is valid
		if (IsBankContainer(req->byDestPlace) && req->byDestPos >= NTL_MAX_BANK_ITEM_SLOT)
		{
			item_move_res = GAME_ITEM_POSITION_FAIL;
			goto END;
		}

		if (req->byDestPlace == CONTAINER_TYPE_BAGSLOT) //move bag to bag slot
		{
			if (dest_item) //if bag already on slot then dont allow to switch bags
			{
				item_move_res = GAME_ITEM_IS_LOCK;
				goto END;
			}

			if (!src_item->IsBag()) //check if item is bag
			{
				item_move_res = GAME_ITEM_NOT_GO_THERE;
				goto END;
			}
			else if (dest_item && dest_item->IsBag()) //check if dest item exist and if its a bag
			{
				if (!cPlayer->GetPlayerItemContainer()->IsBagEmpty(req->byDestPos)) //check if dest bag is empty
				{
					item_move_res = GAME_FAIL;
					goto END;
				}
			}
		}

		if (req->bySrcPlace == CONTAINER_TYPE_EQUIP) //unequip item
		{
			if (dest_item == NULL)
			{
				if (cPlayer->UnequipItem(src_item) == false)
					item_move_res = GAME_FAIL;
			}
			else item_move_res = GAME_ITEM_INVEN_FULL;
		}

		if (req->byDestPlace == CONTAINER_TYPE_EQUIP) //check if equip item
		{
			if (cPlayer->GetLevel() < pItemData->byNeed_Min_Level) //check level
				item_move_res = GAME_ITEM_NEED_MORE_LEVEL;
			else if (cPlayer->GetLevel() > pItemData->byNeed_Max_Level)
				item_move_res = GAME_ITEM_TOO_HIGH_LEVEL_TO_USE_ITEM;

			else if (Dbo_CheckClass(cPlayer->GetClass(), pItemData->dwNeed_Class_Bit_Flag) == false) //check class
				item_move_res = GAME_ITEM_CLASS_FAIL;

			else if (BIT_FLAG_TEST(MAKE_BIT_FLAG(cPlayer->GetGender()), pItemData->dwNeed_Gender_Bit_Flag) == false) //check gender
				item_move_res = GAME_ITEM_GENDER_DOESNT_MATCH;

			else if (pItemData->byRace_Special != INVALID_BYTE && pItemData->byRace_Special != cPlayer->GetRace()) //check race
				item_move_res = GAME_CHAR_RACE_FAIL;

			else if (BIT_FLAG_TEST(MAKE_BIT_FLAG(req->byDestPos), pItemData->dwEquip_Slot_Type_Bit_Flag) == false) //check if item can go to that position
				item_move_res = GAME_ITEM_POSITION_FAIL;

			else if (src_item->GetRestrictState() == ITEM_RESTRICT_STATE_TYPE_SEAL)
				item_move_res = GAME_FAIL;

			else if (dest_item && dest_item->GetRestrictState() == ITEM_RESTRICT_STATE_TYPE_SEAL)
				item_move_res = GAME_FAIL;

			if (item_move_res == GAME_SUCCESS)
			{
				cPlayer->EquipItem(src_item, req->byDestPos);

				if (src_item->GetRestrictState() == 0 && src_item->GetTbldat()->byRestrictType > 0) //check if must restrict item
				{
					rQry->bRestrictUpdate = true;

					if (src_item->GetTbldat()->byRestrictType == ITEM_RESTRICT_TYPE_EQUIP)
						rQry->bySrcRestrictState = ITEM_RESTRICT_STATE_TYPE_LIMIT;
					else
						rQry->bySrcRestrictState = GetDefaultRestrictState(src_item->GetTbldat()->byRestrictType, src_item->GetTbldat()->byItem_Type, true);

					src_item->SetRestrictState(rQry->bySrcRestrictState);

					CNtlPacket packetItemUpdate(sizeof(sGU_ITEM_UPDATE));
					sGU_ITEM_UPDATE * resIU = (sGU_ITEM_UPDATE *)packetItemUpdate.GetPacketData();
					resIU->wOpCode = GU_ITEM_UPDATE;
					resIU->handle = src_item->GetID();
					resIU->sItemData = src_item->GetItemData();
					packetItemUpdate.SetPacketLen(sizeof(sGU_ITEM_UPDATE));
					app->Send(GetHandle(), &packetItemUpdate);
				}
			}
		}
		else if (req->byDestPlace == CONTAINER_TYPE_SCOUT) //dont allow to use scouter chips
			item_move_res = GAME_ITEM_NOT_GO_THERE;
		//printf("item_move_res %u, curPos %u, curPlace %u, bySrcPlace %u, bySrcPos %u, byDestPlace %u, byDestPos %u, itemid %zu, GetID %u\n", item_move_res, src_item->GetPos(), src_item->GetPlace(), req->bySrcPlace, req->bySrcPos, req->byDestPlace, req->byDestPos, src_item->GetItemID(), src_item->GetID());
		/*Update item in map and database if move success*/
		if (item_move_res == GAME_SUCCESS)
		{
			src_item->SetLocked(true);

			rQry->srcItemId = src_item->GetItemID();
			rQry->hSrcItem = src_item->GetID();

			if (dest_item)
			{
				dest_item->SetLocked(true);

				rQry->dstItemId = dest_item->GetItemID();
				rQry->hDstItem = dest_item->GetID();

				if (req->byDestPlace == CONTAINER_TYPE_EQUIP)
					dest_item->SetEquipped(false);
			}
			else
			{
				rQry->hDstItem = INVALID_HOBJECT;
				rQry->dstItemId = INVALID_ITEMID;

				//reseve dest place/pos to avoid item getting there while moving. Only need to do this when moving to a free slot
				cPlayer->GetPlayerItemContainer()->AddReservedInventory(req->byDestPlace, req->byDestPos);
			}

			pQry.SetPacketLen(sizeof(sGQ_BANK_MOVE_REQ));
			app->SendTo(app->GetQueryServerSession(), &pQry);

			return;
		}

	}
	else item_move_res = GAME_ITEM_NOT_FOUND;


END:
	CNtlPacket packet(sizeof(sGU_BANK_MOVE_RES));
	sGU_BANK_MOVE_RES * res = (sGU_BANK_MOVE_RES *)packet.GetPacketData();
	res->wOpCode = GU_BANK_MOVE_RES;
	res->handle = req->handle;
	res->bySrcPlace = req->bySrcPlace;
	res->bySrcPos = req->bySrcPos;
	res->byDestPlace = req->byDestPlace;
	res->byDestPos = req->byDestPos;
	res->wResultCode = item_move_res;
	packet.SetPacketLen(sizeof(sGU_BANK_MOVE_RES));
	app->Send(GetHandle(), &packet);
}

//--------------------------------------------------------------------------------------//
//		BANK STACK ITEM
//--------------------------------------------------------------------------------------//
void CClientSession::RecvBankStackReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_BANK_MOVE_STACK_REQ * req = (sUG_BANK_MOVE_STACK_REQ*)pPacket->GetPacketData();
	WORD resultcode = GAME_SUCCESS;

	CNtlPacket packet(sizeof(sGQ_BANK_MOVE_STACK_REQ));
	sGQ_BANK_MOVE_STACK_REQ * res = (sGQ_BANK_MOVE_STACK_REQ *)packet.GetPacketData();
	res->wOpCode = GQ_BANK_MOVE_STACK_REQ;
	res->handle = cPlayer->GetID();
	res->hNpcHandle = req->handle;
	res->charId = cPlayer->GetCharID();
	res->accountID = cPlayer->GetAccountID();
	res->bySrcPlace = req->bySrcPlace;
	res->bySrcPos = req->bySrcPos;
	res->byDstPlace = req->byDestPlace;
	res->byDstPos = req->byDestPos;

	CItem* pSrcItem = NULL;
	CItem* pDestItem = NULL;
	if (req->bySrcPlace == req->byDestPlace && req->bySrcPos == req->byDestPos)
	{
		resultcode = GAME_FAIL;
		goto END;
	}

	if (cPlayer->IsUsingBank() == false || cPlayer->IsBankLoaded() == false || cPlayer->GetPlayerItemContainer()->IsUsingGuildBank())
	{
		resultcode = GAME_FAIL;
		goto END;
	}

	if (cPlayer->GetPlayerItemContainer()->IsInventoryReserved(req->bySrcPlace, req->bySrcPos) || cPlayer->GetPlayerItemContainer()->IsInventoryReserved(req->byDestPlace, req->byDestPos))
	{
		resultcode = GAME_FAIL;
		goto END;
	}

	pSrcItem = cPlayer->GetPlayerItemContainer()->GetItem(req->bySrcPlace, req->bySrcPos);
	pDestItem = cPlayer->GetPlayerItemContainer()->GetItem(req->byDestPlace, req->byDestPos);

	if (pSrcItem == NULL || pSrcItem->GetCount() == 0)
	{
		resultcode = GAME_ITEM_NOT_FOUND;
		goto END;
	}

	if (pSrcItem->IsLocked() || pSrcItem->GetOwner() == NULL)
	{
		resultcode = GAME_ITEM_IS_LOCK;
		goto END;
	}

	if ( (!IsInvenContainer(req->bySrcPlace) && !IsBankContainer(req->byDestPlace)) //only allow to stack items inside inventory<->bank with this packet
		&& (!IsBankContainer(req->bySrcPlace) && !IsInvenContainer(req->byDestPlace))
		)
	{
		resultcode = GAME_ITEM_STACK_FAIL;
		goto END;
	}


	if (pDestItem == NULL)	//UNSTACK ITEM
	{
		if (pSrcItem->GetTbldat()->byMax_Stack == 1 || req->byStackCount == 0) //is the item even stack-able?
		{
			resultcode = GAME_ITEM_STACK_FAIL;
		}
		else if (pSrcItem->GetCount() <= req->byStackCount) //check if has enough stack
		{
			resultcode = GAME_ITEM_STACK_FAIL;
		}
		else
		{
			pSrcItem->SetLocked(true);

			//reseve dest place/pos to avoid item getting there while moving. Only need to do this when moving to a free slot
			cPlayer->GetPlayerItemContainer()->AddReservedInventory(req->byDestPlace, req->byDestPos);

			res->srcItemID = pSrcItem->GetItemID();
			res->dstItemID = INVALID_ITEMID;
			res->hSrcItem = pSrcItem->GetID();
			res->hDstItem = INVALID_HOBJECT;
			res->byStackCount1 = UnsignedSafeDecrease<BYTE>(pSrcItem->GetCount(), req->byStackCount);
			res->byStackCount2 = req->byStackCount;
		}
	}
	else					// STACK ITEM
	{
		if (pDestItem->IsLocked())
		{
			resultcode = GAME_ITEM_IS_LOCK;
		}
		else if (pSrcItem->GetTblidx() != pDestItem->GetTblidx()) //is source and dest item even same?
		{
			resultcode = GAME_ITEM_NOT_SAME;
		}
		else if (pSrcItem->GetTbldat()->byMax_Stack == 1 || pDestItem->GetTbldat()->byMax_Stack == 1) //is item stack-able
		{
			resultcode = GAME_ITEM_STACK_FAIL;
		}
		else if (pSrcItem->GetCount() < req->byStackCount || pSrcItem->GetTbldat()->byMax_Stack == 1 || req->byStackCount == 0) //check: has less than required items? is item possible to stack? is the stack request == 0
		{
			resultcode = GAME_ITEM_STACK_FAIL;
		}
		else if (pDestItem->GetCount() == 0 || pDestItem->GetCount() == pDestItem->GetTbldat()->byMax_Stack) //check if dest item already max stack
		{
			resultcode = GAME_ITEM_STACK_FULL;
		}
		else if (pDestItem->GetRestrictState() != pSrcItem->GetRestrictState())
			resultcode = GAME_ITEM_STACK_FAIL;
		else
		{
			BYTE stackCount1 = pSrcItem->GetCount() - req->byStackCount;
			int stackCount2 = req->byStackCount + pDestItem->GetCount();

			if (stackCount2 > pSrcItem->GetTbldat()->byMax_Stack)
			{
				stackCount1 = stackCount2 - pSrcItem->GetTbldat()->byMax_Stack;
				stackCount2 = (int)pSrcItem->GetTbldat()->byMax_Stack;
			}

			pSrcItem->SetLocked(true);
			pDestItem->SetLocked(true);

			res->srcItemID = pSrcItem->GetItemID();
			res->dstItemID = pDestItem->GetItemID();
			res->hSrcItem = pSrcItem->GetID();
			res->hDstItem = pDestItem->GetID();
			res->byStackCount1 = stackCount1;
			res->byStackCount2 = (BYTE)stackCount2;
		}
	}

	if (resultcode == GAME_SUCCESS)
	{
		packet.SetPacketLen(sizeof(sGQ_BANK_MOVE_STACK_REQ));
		g_pApp->SendTo(app->GetQueryServerSession(), &packet);
	}
	else
	{
	END:
		CNtlPacket packetEnd(sizeof(sGU_BANK_MOVE_STACK_RES));
		sGU_BANK_MOVE_STACK_RES * resEnd = (sGU_BANK_MOVE_STACK_RES *)packetEnd.GetPacketData();
		resEnd->wOpCode = GU_BANK_MOVE_STACK_RES;
		resEnd->wResultCode = resultcode;
		g_pApp->Send(GetHandle(), &packetEnd);
	}
}

//--------------------------------------------------------------------------------------//
//		BANK DELETE ITEM
//--------------------------------------------------------------------------------------//
void CClientSession::RecvBankDeleteReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_BANK_ITEM_DELETE_REQ * req = (sUG_BANK_ITEM_DELETE_REQ*)pPacket->GetPacketData();

	WORD resultcode = GAME_SUCCESS;

	CItem* item = cPlayer->GetPlayerItemContainer()->GetItem(req->byPlace, req->byPos);
	if (item == NULL)
	{
		resultcode = GAME_ITEM_NOT_FOUND;
		ERR_LOG(LOG_USER, "Player %d failed to delete bank item", cPlayer->GetCharID());
	}
	else if (IsBankContainer(req->byPlace) == false)
	{
		resultcode = GAME_FAIL;
	}
	else if (!item->CanDelete())
		resultcode = GAME_ITEM_CANNOT_DELETE;
	else if (cPlayer->IsUsingBank() == false || cPlayer->IsBankLoaded() == false)
	{
		resultcode = GAME_FAIL;
	}

	CNtlPacket packet(sizeof(sGU_BANK_ITEM_DELETE_RES));
	sGU_BANK_ITEM_DELETE_RES * res = (sGU_BANK_ITEM_DELETE_RES *)packet.GetPacketData();
	res->wOpCode = GU_BANK_ITEM_DELETE_RES;
	res->wResultCode = resultcode;
	packet.SetPacketLen( sizeof(sGU_BANK_ITEM_DELETE_RES) );
	g_pApp->Send(GetHandle() , &packet );

	if(resultcode == GAME_SUCCESS)
		item->SetCount(0, false, true);
}


//--------------------------------------------------------------------------------------//
//		REPAIR ITEM
//--------------------------------------------------------------------------------------//
void CClientSession::RecvRepairItemReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_ITEM_REPAIR_REQ * req = (sUG_ITEM_REPAIR_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_ITEM_REPAIR_RES));
	sGU_ITEM_REPAIR_RES * res = (sGU_ITEM_REPAIR_RES *)packet.GetPacketData();
	res->hNpchandle = req->handle;

	if (!cPlayer->HasPrivateShop() && !cPlayer->IsTrading())
	{
		CItem* item = cPlayer->GetPlayerItemContainer()->GetItem(req->byPlace, req->byPos);
		if (item)
		{
			if (!item->IsLocked())
			{
				sITEM_TBLDAT* pItemData = item->GetTbldat();
				if (pItemData->byDurability != INVALID_BYTE && item->GetDurability() < pItemData->byDurability)
				{
					DWORD repairprice = Dbo_GetRepairPay(pItemData->dwCost, pItemData->byDurability, item->GetDurability());
					if (cPlayer->GetZeni() >= repairprice)
					{
						res->wOpCode = GU_ITEM_REPAIR_RES;
						res->dwSpendedZenny = repairprice;
						res->wResultCode = GAME_SUCCESS;
						res->hItem = item->GetID();

						CGameServer* app = (CGameServer*)g_pApp;

						cPlayer->UpdateZeni(ZENNY_CHANGE_TYPE_REPAIR, repairprice, false, false);


						CNtlPacket packet2(sizeof(sGQ_ITEM_REPAIR_REQ));
						sGQ_ITEM_REPAIR_REQ * res2 = (sGQ_ITEM_REPAIR_REQ *)packet2.GetPacketData();
						res2->wOpCode = GQ_ITEM_REPAIR_REQ;
						res2->handle = cPlayer->GetID();
						res2->charId = cPlayer->GetCharID();
						res2->npcHandle = req->handle;
						res2->sItemData.itemID = item->GetItemID();
						res2->sItemData.byPlace = item->GetPlace();
						res2->sItemData.byPosition = item->GetPos();
						res2->sItemData.byDur = pItemData->byDurability;
						res2->dwZenny = repairprice;
						packet2.SetPacketLen(sizeof(sGQ_ITEM_REPAIR_REQ));
						app->SendTo(app->GetQueryServerSession(), &packet2);

						
						item->UpdateDurability(pItemData->byDurability);

						if(item->GetPlace() == CONTAINER_TYPE_EQUIP)
							cPlayer->GetCharAtt()->CalculateAll();
					}
					else res->wResultCode = GAME_ZENNY_NOT_ENOUGH;
				}
				else res->wResultCode = GAME_REPAIR_VALUE_FAIL;
			}
			else res->wResultCode = GAME_ITEM_IS_LOCK;
		}
		else res->wResultCode = GAME_REPAIR_NOT_FOUND;
	}
	else res->wResultCode = GAME_FAIL;

	packet.SetPacketLen( sizeof(sGU_ITEM_REPAIR_RES) );
	g_pApp->Send( GetHandle() , &packet );
}

//--------------------------------------------------------------------------------------//
//		REPAIR EQUIP
//--------------------------------------------------------------------------------------//
void CClientSession::RecvEquipRepairReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_ITEM_EQUIP_REPAIR_REQ * req = (sUG_ITEM_EQUIP_REPAIR_REQ*)pPacket->GetPacketData();
	DWORD repairprice = 0;

	CNtlPacket packet(sizeof(sGU_ITEM_EQUIP_REPAIR_RES));
	sGU_ITEM_EQUIP_REPAIR_RES * res = (sGU_ITEM_EQUIP_REPAIR_RES *)packet.GetPacketData();
	res->wOpCode = GU_ITEM_EQUIP_REPAIR_RES;

	//dont allow repair item while trading or while having private shop
	if (cPlayer->IsTrading() || cPlayer->HasPrivateShop())
	{
		res->wResultCode = GAME_FAIL;
	}
	else
	{
		//Get repair price
		for (int slot = 0; slot < EQUIP_SLOT_TYPE_COUNT; slot++)
		{
			CItem* itemdata = cPlayer->GetPlayerItemContainer()->GetItem(CONTAINER_TYPE_EQUIP, slot);
			if (itemdata)
			{
				sITEM_TBLDAT* pItemData = itemdata->GetTbldat();
				if (itemdata->GetDurability() < pItemData->byDurability)
				{
					repairprice += Dbo_GetRepairPay(pItemData->dwCost, pItemData->byDurability, itemdata->GetDurability());
				}
			}
		}

		//Check gold and repair all equipments
		if (repairprice > 0)
		{
			if (cPlayer->GetZeni() >= repairprice)
			{
				CNtlPacket packet2(sizeof(sGQ_ITEM_EQUIP_REPAIR_REQ));
				sGQ_ITEM_EQUIP_REPAIR_REQ * res2 = (sGQ_ITEM_EQUIP_REPAIR_REQ *)packet2.GetPacketData();
				res2->wOpCode = GQ_ITEM_EQUIP_REPAIR_REQ;
				res2->handle = cPlayer->GetID();
				res2->charId = cPlayer->GetCharID();
				res2->npcHandle = req->handle;

				for (int j = 0; j < EQUIP_SLOT_TYPE_COUNT; j++)
				{
					CItem* pItem = cPlayer->GetPlayerItemContainer()->GetItem(CONTAINER_TYPE_EQUIP, j);
					if (pItem)
					{
						sITEM_TBLDAT* pItemData = pItem->GetTbldat();
						if (pItem->GetDurability() < pItemData->byDurability)
						{
							pItem->UpdateDurability(pItemData->byDurability);

							res->wResultCode = GAME_SUCCESS;
							res2->byCount += 1;
							res2->asItemData[j].itemID = pItem->GetItemID();
							res2->asItemData[j].byPlace = pItem->GetPlace();
							res2->asItemData[j].byPosition = pItem->GetPos();
							res2->asItemData[j].byDur = pItemData->byDurability;
						}
					}
				}

				res2->dwZenny = repairprice;
				packet2.SetPacketLen(sizeof(sGQ_ITEM_EQUIP_REPAIR_REQ));
				app->SendTo(app->GetQueryServerSession(), &packet2);

				cPlayer->UpdateZeni(ZENNY_CHANGE_TYPE_REPAIR, repairprice, false, false);

			}
			else res->wResultCode = GAME_ZENNY_NOT_ENOUGH;
		}
		else res->wResultCode = GAME_REPAIR_NOT_FOUND;
	}


	res->dwSpendedZenny = repairprice;
	res->handle = req->handle;
	packet.SetPacketLen( sizeof(sGU_ITEM_EQUIP_REPAIR_RES) );
	app->Send( GetHandle() , &packet );

	if(res->wResultCode == GAME_SUCCESS)
		cPlayer->GetCharAtt()->CalculateAll();
}

//--------------------------------------------------------------------------------------//
//		SHOP IDENTIFY ITEM
//--------------------------------------------------------------------------------------//
void CClientSession::RecvShopIdentifyItemReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_SHOP_ITEM_IDENTIFY_REQ * req = (sUG_SHOP_ITEM_IDENTIFY_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_SHOP_ITEM_IDENTIFY_RES));
	sGU_SHOP_ITEM_IDENTIFY_RES * res = (sGU_SHOP_ITEM_IDENTIFY_RES *)packet.GetPacketData();
	res->wOpCode = GU_SHOP_ITEM_IDENTIFY_RES;
	res->hNpchandle = req->hNpchandle;
	res->byPlace = req->byPlace;
	res->byPos = req->byPos;
	res->wResultCode = GAME_SUCCESS;
	
	CNpc* pNpc = g_pObjectManager->GetNpc(req->hNpchandle);

	if (pNpc == NULL)	//check if NPC exist
		res->wResultCode = GAME_TARGET_NOT_FOUND;
	else if (cPlayer->IsInRange(pNpc, DBO_DISTANCE_CHECK_TOLERANCE) == false) //check if npc is in range
		res->wResultCode = GAME_TARGET_TOO_FAR;
	else if (cPlayer->IsTrading() || cPlayer->HasPrivateShop())
		res->wResultCode = GAME_FAIL;
	else if (cPlayer->GetZeni() < NTL_SHOP_ITEM_IDENTIFY_ZENNY)
		res->wResultCode = GAME_ZENNY_NOT_ENOUGH;
	else if(IsInvenContainer(req->byPlace) == false)
		res->wResultCode = GAME_FAIL;
	else
	{
		CItem* item = cPlayer->GetPlayerItemContainer()->GetItem(req->byPlace, req->byPos);
		if (item && item->GetCount() > 0)
		{
			if (item->IsLocked() == false)
			{
				item->IdentifiyItem(false);
				memcpy(&res->sItemData, &item->GetItemData(), sizeof(sITEM_DATA));

				cPlayer->UpdateZeni(ZENNY_CHANGE_TYPE_ITEM_IDENTIFY, NTL_SHOP_ITEM_IDENTIFY_ZENNY, false, false);

				CNtlPacket pQry(sizeof(sGQ_ITEM_IDENTIFY_REQ));
				sGQ_ITEM_IDENTIFY_REQ * rQry = (sGQ_ITEM_IDENTIFY_REQ *)pQry.GetPacketData();
				rQry->wOpCode = GQ_ITEM_IDENTIFY_REQ;
				rQry->charId = cPlayer->GetCharID();
				rQry->dwZeni = NTL_SHOP_ITEM_IDENTIFY_ZENNY;
				rQry->itemId = item->GetItemID();
				pQry.SetPacketLen(sizeof(sGQ_ITEM_IDENTIFY_REQ));
				app->SendTo(app->GetQueryServerSession(), &pQry);
			}
			else res->wResultCode = GAME_ITEM_IS_LOCK;
		}
		else res->wResultCode = GAME_ITEM_UNIDENTIFY_FAIL;
	}
	
	packet.SetPacketLen( sizeof(sGU_SHOP_ITEM_IDENTIFY_RES) );
	app->Send(GetHandle(), &packet );
}

//--------------------------------------------------------------------------------------//
//		IDENTIFY ITEM
//--------------------------------------------------------------------------------------//
void CClientSession::RecvIdentifyItemReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_ITEM_IDENTIFY_REQ * req = (sUG_ITEM_IDENTIFY_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_ITEM_IDENTIFY_RES));
	sGU_ITEM_IDENTIFY_RES * res = (sGU_ITEM_IDENTIFY_RES *)packet.GetPacketData();

	res->wOpCode = GU_ITEM_IDENTIFY_RES;

	if(cPlayer->GetZeni() >= NTL_SHOP_ITEM_IDENTIFY_ZENNY)
	{
		CItem* item = cPlayer->GetPlayerItemContainer()->GetItem(req->byPlace, req->byPos);
		if(item && item->GetCount() > 0)
		{
			if (item->IsLocked() == false)
			{
				item->IdentifiyItem(false);
				res->wResultCode = GAME_SUCCESS;
				res->hItemHandle = item->GetID();
				memcpy(&res->sItemData, &item->GetItemData(), sizeof(sITEM_DATA));

				cPlayer->UpdateZeni(ZENNY_CHANGE_TYPE_ITEM_IDENTIFY, NTL_SHOP_ITEM_IDENTIFY_ZENNY, false, false);

				CNtlPacket pQry(sizeof(sGQ_ITEM_IDENTIFY_REQ));
				sGQ_ITEM_IDENTIFY_REQ * rQry = (sGQ_ITEM_IDENTIFY_REQ *)pQry.GetPacketData();
				rQry->wOpCode = GQ_ITEM_IDENTIFY_REQ;
				rQry->charId = cPlayer->GetCharID();
				rQry->dwZeni = cPlayer->GetZeni();
				rQry->itemId = item->GetItemID();
				packet.SetPacketLen(sizeof(sGQ_ITEM_IDENTIFY_REQ));
				app->SendTo(app->GetQueryServerSession(), &pQry);
			}
			else res->wResultCode = GAME_ITEM_IS_LOCK;
		} else res->wResultCode = GAME_ITEM_UNIDENTIFY_FAIL;
	} else res->wResultCode = GAME_ZENNY_NOT_ENOUGH;

	packet.SetPacketLen( sizeof(sGU_ITEM_IDENTIFY_RES) );
	app->Send( GetHandle() , &packet );
}

//--------------------------------------------------------------------------------------//
//		START VEHICLE ENGINE
//--------------------------------------------------------------------------------------//
void CClientSession::RecvVehicleStartEngineReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_VEHICLE_ENGINE_START_REQ * req = (sUG_VEHICLE_ENGINE_START_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_VEHICLE_ENGINE_START_RES));
	sGU_VEHICLE_ENGINE_START_RES * res = (sGU_VEHICLE_ENGINE_START_RES *)packet.GetPacketData();
	res->wOpCode = GU_VEHICLE_ENGINE_START_RES;
	res->wResultCode = GAME_SUCCESS;
	packet.SetPacketLen( sizeof(sGU_VEHICLE_ENGINE_START_RES) );
	g_pApp->Send( GetHandle() , &packet );

	CNtlPacket packet2(sizeof(sGU_VEHICLE_ENGINE_START_NFY));
	sGU_VEHICLE_ENGINE_START_NFY * res2 = (sGU_VEHICLE_ENGINE_START_NFY *)packet2.GetPacketData();
	res2->wOpCode = GU_VEHICLE_ENGINE_START_NFY;
	res2->hDriverHandle = cPlayer->GetID();
	packet2.SetPacketLen( sizeof(sGU_VEHICLE_ENGINE_START_NFY) );
	cPlayer->Broadcast(&packet2, cPlayer);

	cPlayer->SetVehicleEngine(true);
}

//--------------------------------------------------------------------------------------//
//		STOP VEHICLE ENGINE
//--------------------------------------------------------------------------------------//
void CClientSession::RecvVehicleStopEngineReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_VEHICLE_ENGINE_STOP_REQ * req = (sUG_VEHICLE_ENGINE_STOP_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_VEHICLE_ENGINE_STOP_RES));
	sGU_VEHICLE_ENGINE_STOP_RES * res = (sGU_VEHICLE_ENGINE_STOP_RES *)packet.GetPacketData();
	res->wOpCode = GU_VEHICLE_ENGINE_STOP_RES;
	res->wResultCode = GAME_SUCCESS;
	packet.SetPacketLen( sizeof(sGU_VEHICLE_ENGINE_STOP_RES) );
	g_pApp->Send(GetHandle() , &packet );

	CNtlPacket packet2(sizeof(sGU_VEHICLE_ENGINE_STOP_NFY));
	sGU_VEHICLE_ENGINE_STOP_NFY * res2 = (sGU_VEHICLE_ENGINE_STOP_NFY *)packet2.GetPacketData();
	res2->wOpCode = GU_VEHICLE_ENGINE_STOP_NFY;
	res2->hDriverHandle = cPlayer->GetID();
	res2->dwFuelRemain = 0;
	packet2.SetPacketLen( sizeof(sGU_VEHICLE_ENGINE_STOP_NFY) );
	cPlayer->Broadcast(&packet2);

	cPlayer->SetVehicleEngine(false);
}

//--------------------------------------------------------------------------------------//
//		VEHICLE STUNT (JUMP)
//--------------------------------------------------------------------------------------//
void CClientSession::RecvVehicleStuntNfy(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CNtlPacket packet(sizeof(sGU_VEHICLE_STUNT_NFY));
	sGU_VEHICLE_STUNT_NFY * res = (sGU_VEHICLE_STUNT_NFY *)packet.GetPacketData();
	res->wOpCode = GU_VEHICLE_STUNT_NFY;
	res->hDriverHandle = cPlayer->GetID();
	packet.SetPacketLen( sizeof(sGU_VEHICLE_STUNT_NFY) );
	cPlayer->Broadcast(&packet);
}


//--------------------------------------------------------------------------------------//
//	 VEHICLE DIRECT PLAY END
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCancelVehicleDirectPlayNfy(CNtlPacket * pPacket)
{
	//CNtlPacket packet(sizeof(sGU_CHAR_DIRECT_PLAY_CANCEL_NFY));
	//sGU_CHAR_DIRECT_PLAY_CANCEL_NFY * res = (sGU_CHAR_DIRECT_PLAY_CANCEL_NFY *)packet.GetPacketData();
	//res->wOpCode = GU_CHAR_DIRECT_PLAY_CANCEL_NFY;
	//res->hPc = cPlayer->GetID();
	//packet.SetPacketLen(sizeof(sGU_CHAR_DIRECT_PLAY_CANCEL_NFY));
	//cPlayer->Broadcast(&packet);
}


//--------------------------------------------------------------------------------------//
//	 
//--------------------------------------------------------------------------------------//
void CClientSession::RecvEndVehicleReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	if(cPlayer->GetVehicleTblidx() != INVALID_TBLIDX)
		cPlayer->EndVehicle(GAME_SUCCESS);
}


//--------------------------------------------------------------------------------------//
//	 
//--------------------------------------------------------------------------------------//
void CClientSession::RecvVehicleFuelRemoveNfy(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	cPlayer->UpdateVehicleFuel(false);
}

//--------------------------------------------------------------------------------------//
//	 
//--------------------------------------------------------------------------------------//
void CClientSession::RecvVehicleFuelInsertReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_VEHICLE_FUEL_INSERT_REQ * req = (sUG_VEHICLE_FUEL_INSERT_REQ*)pPacket->GetPacketData();

	cPlayer->UpdateVehicleFuel(true, req->byPlace, req->byPos);
}


void CClientSession::RecvCharDirectPlayCancel(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	if (cPlayer->GetParty())
		cPlayer->GetParty()->CancelDirectPlay(cPlayer->GetID());
}

//-------------------------------------------------
//		RECEIVED WHEN DIRECT PLAY END
//-------------------------------------------------
void CClientSession::RecvCharDirectPlayAck(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	cPlayer->EndDirectPlay();
}

//-------------------------------------------------
//     UG_CHAR_LOC_AFTER_KNOCKDOWN_NFY
//-------------------------------------------------
void CClientSession::RecvCharLocAfterKnockdown(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_LOC_AFTER_KNOCKDOWN_NFY * req = (sUG_CHAR_LOC_AFTER_KNOCKDOWN_NFY*)pPacket->GetPacketData();

	//CStateBase* pState = (CStateBase*)cPlayer->GetStateManager()->GetCharState();

	//if (pState && (pState->GetStateID() != CHARSTATE_KNOCKDOWN && pState->GetPrevStateID() != CHARSTATE_KNOCKDOWN))
	//{
	//	ERR_LOG(LOG_GENERAL, "KNOCKDOWN: Player %u current and last state is not CHARSTATE_KNOCKDOWN. Cur State %u, Previous State %u", cPlayer->GetCharID(), cPlayer->GetCharStateID(), pState->GetPrevStateID());

	//	return;
	//}

	CNtlVector vLoc;
	NtlLocationDecompress(&req->vCurLoc, &vLoc.x, &vLoc.y, &vLoc.z);

	if (cPlayer->SetCurLoc(vLoc, cPlayer->GetCurWorld()))
	{
		sVECTOR3 sDir;
		NtlDirectionDecompress(&req->vCurDir, &sDir.x, &sDir.y, &sDir.z);
		cPlayer->SetCurDir(sDir);
	}
	else
	{
		ERR_LOG(LOG_HACK, "Player: %u seems to be location hacking. CurLoc: %f, %f, %f NewLoc: %f %f %f", cPlayer->GetCharID(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().y, cPlayer->GetCurLoc().z, vLoc.x, vLoc.y, vLoc.z);
	}
}
//-------------------------------------------------
//     UG_CHAR_LOC_AFTER_SLIDING_NFY
//-------------------------------------------------
void CClientSession::RecvCharLocAfterSliding(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_LOC_AFTER_SLIDING_NFY * req = (sUG_CHAR_LOC_AFTER_SLIDING_NFY*)pPacket->GetPacketData();

	CNtlVector vLoc;
	NtlLocationDecompress(&req->vCurLoc, &vLoc.x, &vLoc.y, &vLoc.z);

	if (cPlayer->SetCurLoc(vLoc, cPlayer->GetCurWorld()))
	{
		sVECTOR3 sDir;
		NtlDirectionDecompress(&req->vCurDir, &sDir.x, &sDir.y, &sDir.z);
		cPlayer->SetCurDir(sDir);
	}
	else
	{
		ERR_LOG(LOG_HACK, "Player: %u seems to be location hacking. CurLoc: %f, %f, %f NewLoc: %f %f %f", cPlayer->GetCharID(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().y, cPlayer->GetCurLoc().z, vLoc.x, vLoc.y, vLoc.z);
	}
}

//-------------------------------------------------
//     UG_CHAR_LOC_AFTER_PUSH_NFY
//-------------------------------------------------
void CClientSession::RecvCharLocAfterPush(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_LOC_AFTER_PUSH_NFY * req = (sUG_CHAR_LOC_AFTER_PUSH_NFY*)pPacket->GetPacketData();

	CNtlVector vLoc;
	NtlLocationDecompress(&req->vCurLoc, &vLoc.x, &vLoc.y, &vLoc.z);

	if (cPlayer->SetCurLoc(vLoc, cPlayer->GetCurWorld()))
	{
		sVECTOR3 sDir;
		NtlDirectionDecompress(&req->vCurDir, &sDir.x, &sDir.y, &sDir.z);
		cPlayer->SetCurDir(sDir);
	}
	else
	{
		ERR_LOG(LOG_HACK, "Player: %u seems to be location hacking. CurLoc: %f, %f, %f NewLoc: %f %f %f", cPlayer->GetCharID(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().y, cPlayer->GetCurLoc().z, vLoc.x, vLoc.y, vLoc.z);
	}
}


//-------------------------------------------------
//     UG_CROSSFIRE_REQ (Select next target)
//-------------------------------------------------
void CClientSession::RecvCrossfireReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CROSSFIRE_REQ * req = (sUG_CROSSFIRE_REQ*)pPacket->GetPacketData();

	cPlayer->ChangeTarget(req->hAnotherUser);
	
	CNtlPacket packet(sizeof(sGU_CROSSFIRE_RES));
	sGU_CROSSFIRE_RES * res = (sGU_CROSSFIRE_RES*)packet.GetPacketData();
	res->wOpCode = GU_CROSSFIRE_RES;
	res->hTarget = req->hAnotherUser;
	packet.SetPacketLen(sizeof(sGU_CROSSFIRE_RES));
	g_pApp->Send(GetHandle(), &packet);
}

//-------------------------------------------------
//     SEND RESULTCODE TO PLAYER
//-------------------------------------------------
void CClientSession::SendResultcode(WORD wResultcode)
{
	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_RESULTCODE));
	sGU_SYSTEM_DISPLAY_RESULTCODE * res = (sGU_SYSTEM_DISPLAY_RESULTCODE*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_RESULTCODE;
	res->byDisplayType = 0;
	res->wResultCode = wResultcode;
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_RESULTCODE));
	g_pApp->Send(GetHandle(), &packet);
}


//-------------------------------------------------
//     SEND ITEM UPGRADE WORK
//-------------------------------------------------
void CClientSession::RecvItemUpgradeWorkReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;
	
	sUG_ITEM_UPGRADE_WORK_REQ * req = (sUG_ITEM_UPGRADE_WORK_REQ*)pPacket->GetPacketData();
	//printf("req->byCardIndex: %u \n", req->byCardIndex);
	WORD resultcode = GAME_SUCCESS;

	CNtlPacket packet(sizeof(sGU_ITEM_UPGRADE_WORK_RES));
	sGU_ITEM_UPGRADE_WORK_RES * res = (sGU_ITEM_UPGRADE_WORK_RES*)packet.GetPacketData();
	res->wOpCode = GU_ITEM_UPGRADE_WORK_RES;
	res->byItemPlace = req->byItemPlace;
	res->byItemPos = req->byItemPos;
	res->byStonePlace = req->byStonePlace;
	res->byStonePos = req->byStonePos;
	res->byStoneStack = 0;
	res->byCoreStack = 0;

	res->wResultMessageCode = GAME_ITEM_UPGRADE_FAIL;


	CNpc* pNpc = g_pObjectManager->GetNpc(cPlayer->GetFacingHandle());
	if (pNpc == NULL)
		resultcode = GAME_FAIL;
	else if (cPlayer->IsInRange(pNpc, DBO_DISTANCE_CHECK_TOLERANCE) == false)
		resultcode = GAME_TARGET_TOO_FAR;
	else if (pNpc->GetTbldat()->byJob != NPC_JOB_ITEM_UPGRADE)
		resultcode = GAME_TARGET_HAS_DIFFERENT_JOB;
	else if (IsInvenContainer(req->byItemPlace) && IsInvenContainer(req->byStonePlace))
	{
		CItem* equipdata = cPlayer->GetPlayerItemContainer()->GetItem(req->byItemPlace, req->byItemPos);
		CItem* hpstonedata = cPlayer->GetPlayerItemContainer()->GetItem(req->byStonePlace, req->byStonePos);
		CItem* whiteStone = NULL;

		if (equipdata && hpstonedata && hpstonedata->GetCount() > 0 && IsInvenContainer(equipdata->GetPlace()) && IsInvenContainer(hpstonedata->GetPlace()))
		{
			if (equipdata->IsLocked() || hpstonedata->IsLocked())
			{
				resultcode = GAME_ITEM_IS_LOCK;
				goto END;
			}

			sITEM_TBLDAT * pEquipData = equipdata->GetTbldat();
			sITEM_TBLDAT * pStoneData = hpstonedata->GetTbldat();
			sITEM_DATA pItemData = equipdata->GetItemData();

			if (cPlayer->GetZeni() < (pEquipData->byNeed_Min_Level * pEquipData->byRank * equipdata->GetGrade() * 10))
			{
				resultcode = GAME_ZENNY_NOT_ENOUGH;
				goto END;
			}

			if (pEquipData && pStoneData)
			{
				if (equipdata->GetDurationtype() != eDURATIONTYPE_NORMAL)
				{
					resultcode = GAME_ITEM_UPGRADE_FAIL_FOR_DURATION_ITEM;
					goto END;
				}

				if (pEquipData->wScouter_Watt == 555)
				{
					bool pass = false;
					for (int i = 850748; i < 850751 + 1; i++)
					{
						if (pStoneData->tblidx == i)
							pass = true;
					}
					if (pass == false)
					{
						resultcode = GAME_FAIL;
						//goto END;
					}
				}

				printf("equip watt: %u | stone id: %u | resultcode: %u\n", pEquipData->wScouter_Watt, pStoneData->tblidx, resultcode);

				res->byStoneStack = UnsignedSafeDecrease<BYTE>(hpstonedata->GetCount(), 1);

				if (pStoneData->byItem_Type == ITEM_TYPE_DOWNGRADE_STONE_WEAPON || pStoneData->byItem_Type == ITEM_TYPE_DOWNGRADE_STONE_ARMOR)
				{
					if (pEquipData->byNeed_Min_Level > pStoneData->byNeed_Max_Level)
					{
						resultcode = GAME_ITEM_UPGRADE_MUST_USE_PROPER_LEVEL_STONE;
						goto END;
					}

					if (pEquipData->byItem_Type >= ITEM_TYPE_WEAPON_FIRST && pEquipData->byItem_Type <= ITEM_TYPE_WEAPON_LAST)
					{
						if (pStoneData->byItem_Type != ITEM_TYPE_DOWNGRADE_STONE_WEAPON)
						{
							resultcode = GAME_ITEM_UPGRADE_MUST_USE_STONE_WEAPON;
							goto END;
						}
					}
					else if (pEquipData->byItem_Type >= ITEM_TYPE_ARMOR_FIRST && pEquipData->byItem_Type <= ITEM_TYPE_ARMOR_LAST)
					{
						if (pStoneData->byItem_Type != ITEM_TYPE_DOWNGRADE_STONE_ARMOR)
						{
							resultcode = GAME_ITEM_UPGRADE_MUST_USE_STONE_ARMOR;
							goto END;
						}
					}
					else
					{
						resultcode = GAME_ITEM_UPGRADE_WRONG_ITEM_TYPE;
						goto END;
					}

					if (equipdata->GetGrade() == 0)
						resultcode = GAME_ITEM_UPGRADE_ITEM_IS_ZERO_GRADE;
					else
					{
						res->wResultMessageCode = GAME_SUCCESS;
						resultcode = GAME_SUCCESS;
						res->byItemGrade = equipdata->GetGrade() - 1;
					}
				}
				else if (pStoneData->byItem_Type == ITEM_TYPE_UPGRADE_STONE_WEAPON || pStoneData->byItem_Type == ITEM_TYPE_UPGRADE_STONE_ARMOR
					|| pStoneData->byItem_Type == ITEM_TYPE_GREATER_UPGRADE_STONE_WEAPON || pStoneData->byItem_Type == ITEM_TYPE_GREATER_UPGRADE_STONE_ARMOR)
				{
					BYTE byItemTypeGroup = GetItemTypeGroup(pEquipData->byItem_Type);

					if (equipdata->GetGrade() == NTL_ITEM_MAX_GRADE)
					{
						resultcode = GAME_ITEM_UPGRADE_ALREADY_HIGHEST_GRADE;
						goto END;
					}

					//printf("pEquipData->byNeed_Min_Level %u, pStoneData->byNeed_Max_Level %u, byItemTypeGroup %u, pStoneData->byItem_Type %u \n", pEquipData->byNeed_Min_Level, pStoneData->byNeed_Max_Level, byItemTypeGroup, pStoneData->byItem_Type);
					if (pEquipData->byNeed_Min_Level > pStoneData->byNeed_Max_Level)
					{
						resultcode = GAME_ITEM_UPGRADE_MUST_USE_PROPER_LEVEL_STONE;
						goto END;
					}

					//check item type again
					if (byItemTypeGroup == ITEM_TYPE_GROUP_WEAPON)
					{
						if (pStoneData->byItem_Type != ITEM_TYPE_UPGRADE_STONE_WEAPON && pStoneData->byItem_Type != ITEM_TYPE_GREATER_UPGRADE_STONE_WEAPON)
						{
							resultcode = GAME_ITEM_UPGRADE_MUST_USE_STONE_WEAPON;
							goto END;
						}
					}
					else if (byItemTypeGroup == ITEM_TYPE_GROUP_ARMOR)
					{
						if (pStoneData->byItem_Type != ITEM_TYPE_UPGRADE_STONE_ARMOR && pStoneData->byItem_Type != ITEM_TYPE_GREATER_UPGRADE_STONE_ARMOR)
						{
							resultcode = GAME_ITEM_UPGRADE_MUST_USE_STONE_ARMOR;
							goto END;
						}
					}
					else
					{
						resultcode = GAME_ITEM_UPGRADE_WRONG_ITEM_TYPE;
						goto END;
					}

					sITEM_UPGRADE_RATE_NEW_TBLDAT* pItemUpgrade = (sITEM_UPGRADE_RATE_NEW_TBLDAT*)g_pTableContainer->GetItemUpgradeRateNewTable()->GetUpgradeRateNewTbldat(byItemTypeGroup, equipdata->GetGrade());
					if (pItemUpgrade && resultcode == GAME_SUCCESS)
					{
						resultcode = GAME_SUCCESS;

						whiteStone = cPlayer->GetPlayerItemContainer()->GetItem(req->byCorePlace, req->byCorePos);
						bool isProtection = false;
						if (whiteStone && whiteStone->GetCount() > 0 && IsInvenContainer(whiteStone->GetPlace()))
						{
							if (whiteStone->IsLocked())
							{
								resultcode = GAME_ITEM_IS_LOCK;
								goto END;
							}

							//check if its really a white stone
							if (whiteStone->GetTbldat()->byItem_Type != ITEM_TYPE_STONE_CORE)
							{
								resultcode = GAME_ITEM_UPGRADE_CANT_USE_STONE_CORE_WITH_SAFE;
								goto END;
							}
							if (pEquipData->byNeed_Min_Level > whiteStone->GetTbldat()->byNeed_Max_Level)
							{
								resultcode = GAME_ITEM_UPGRADE_MUST_USE_PROPER_LEVEL_CORE_STONE;
								goto END;
							}

							for (TBLIDX i = 850094; i < 850096 + 1; i++)
							{
								//printf("stone: %u \n", m_protection[i]);
								if (whiteStone->GetTblidx() == i) isProtection = true;
							}

							res->byCorePlace = req->byCorePlace;
							res->byCorePos = req->byCorePos;
							res->byCoreStack = UnsignedSafeDecrease<BYTE>(whiteStone->GetCount(), 1);
						}

						BYTE byGradePlus = 1; BYTE byBonus = 0;
						if (pStoneData->byItem_Type == ITEM_TYPE_GREATER_UPGRADE_STONE_WEAPON || pStoneData->byItem_Type == ITEM_TYPE_GREATER_UPGRADE_STONE_ARMOR)
						{
							byGradePlus = (BYTE)RandomRange(1, 3);
						}

						for (TBLIDX i = 850097; i < 850102 + 1; i++)
						{
							if (pStoneData->tblidx == i) byBonus = 10;
						}

						float upgradeProb = 0.0f;
						if (isProtection) upgradeProb = 30.0f + pItemUpgrade->fUpgrade_Success_Basic_Value + byBonus;
						else if (whiteStone) upgradeProb = 15.0f + pItemUpgrade->fUpgrade_Success_Basic_Value + byBonus;

						float successChance = whiteStone ? 100.0f : pItemUpgrade->fUpgrade_Success_Basic_Value + byBonus;
						successChance += (float)app->GetUpgradeRateBonus();
						if (successChance < 0.0f)
							successChance = 0.0f;
						if (successChance > 100.0f)
							successChance = 100.0f;


						if (Dbo_CheckProbabilityF(successChance)) //check if success
						{
							res->wResultMessageCode = GAME_SUCCESS;

							if (equipdata->GetGrade() + byGradePlus > 15)
							{
								res->byItemGrade = NTL_ITEM_MAX_GRADE;
							}
							else 
							{
								res->byItemGrade = equipdata->GetGrade() + byGradePlus;
							}
							equipdata->GenerateOptionSet(byGradePlus);
						}
						//else if (Dbo_CheckProbabilityF(pItemUpgrade->fUpgrade_Destroy_Rate)) //check if destroy
						//{
						//	res->wResultMessageCode = GAME_ITEM_UPGRADE_FAIL_AND_DEL;

						//	if (whiteStone && isProtection) res->byItemGrade = equipdata->GetGrade();
						//	else if (whiteStone && !isProtection) res->byItemGrade = UnsignedSafeDecrease<BYTE>(equipdata->GetGrade(), 2);
						//	else res->byItemGrade = 0;
						//}
						else //item upgrade fail
						{
							res->wResultMessageCode = GAME_ITEM_UPGRADE_FAIL;

							res->byItemGrade = equipdata->GetGrade();
						}
					}
					else resultcode = GAME_ITEM_UPGRADE_WRONG_ITEM_TYPE;
				}
				else
				{
					ERR_LOG(LOG_HACK, "Player %u tried upgrading with invalid stone type %u", cPlayer->GetCharID(), pStoneData->byItem_Type);
					resultcode = GAME_ITEM_UPGRADE_WRONG_ITEM_TYPE;
				}

				//update item grade
				if (resultcode == GAME_SUCCESS)
				{
					if (res->byItemGrade >= 12 && res->byItemGrade > equipdata->GetGrade())
					{
						if (res->byItemGrade == NTL_ITEM_MAX_GRADE)
							ERR_LOG(LOG_HACK, "Player %u upgraded %zu (%u) to +15 from %u . stoneId %zu (%u)", cPlayer->GetCharID(), equipdata->GetItemID(), equipdata->GetTblidx(), equipdata->GetGrade(), hpstonedata->GetItemID(), hpstonedata->GetTblidx());

						CNtlPacket pChat(sizeof(sGT_BROADCASTING_SYSTEM_NFY));
						sGT_BROADCASTING_SYSTEM_NFY * rChat = (sGT_BROADCASTING_SYSTEM_NFY *)pChat.GetPacketData();
						rChat->wOpCode = GT_BROADCASTING_SYSTEM_NFY;
						rChat->byMsgType = DBO_BROADCASTING_MSG_TYPE_ITEMUPGRADE;
						rChat->sData.sItemUpgrade.byGrade = res->byItemGrade;
						rChat->sData.sItemUpgrade.tblidx = equipdata->GetTblidx();
						NTL_WCSCPY_S(rChat->sData.sItemUpgrade.wszName, NTL_MAX_SIZE_CHAR_NAME + 1, cPlayer->GetCharName());
						pChat.SetPacketLen(sizeof(sGT_BROADCASTING_SYSTEM_NFY));
						app->SendTo(app->GetChatServerSession(), &pChat);
					}

					equipdata->SetGrade(res->byItemGrade);

					CNtlPacket pQry(sizeof(sGQ_ITEM_UPGRADE_WORK_REQ));
					sGQ_ITEM_UPGRADE_WORK_REQ * rQry = (sGQ_ITEM_UPGRADE_WORK_REQ *)pQry.GetPacketData();
					rQry->wOpCode = GQ_ITEM_UPGRADE_WORK_REQ;
					rQry->handle = cPlayer->GetID();
					rQry->charId = cPlayer->GetCharID();
					rQry->bIsSuccessful = true;
					rQry->bNeedToDestroyItem = false;
					rQry->bCoreItemUse = (whiteStone == NULL) ? false : true;
					rQry->itemId = equipdata->GetItemID();
					rQry->byItemPlace = equipdata->GetPlace();
					rQry->byItemPos = equipdata->GetPos();
					rQry->byItemGrade = equipdata->GetGrade();

					rQry->stoneId = hpstonedata->GetItemID();
					rQry->byStonePlace = hpstonedata->GetPlace();
					rQry->byStonePos = hpstonedata->GetPos();
					rQry->byStoneStack = res->byStoneStack;

					if (rQry->bCoreItemUse)
					{
						rQry->coreId = whiteStone->GetItemID();
						rQry->byCorePlace = whiteStone->GetPlace();
						rQry->byCorePos = whiteStone->GetPos();
						rQry->byCoreStack = res->byCoreStack;
					}

					pQry.SetPacketLen(sizeof(sGQ_ITEM_UPGRADE_WORK_REQ));
					app->SendTo(app->GetQueryServerSession(), &pQry);

					//delete/update core item
					if (whiteStone)
					{
						whiteStone->SetCount(res->byCoreStack, false, false);
					}
				}

				//Delete/Update stone item
				hpstonedata->SetCount(res->byStoneStack, false, false);
				int upgradeCost = pEquipData->byNeed_Min_Level * pEquipData->byRank * equipdata->GetGrade() * 10;

				cPlayer->UpdateZeni(ZENNY_CHANGE_TYPE_ITEM_ATTRIBUTE_CHANGE, upgradeCost, false, true);
			}
			else resultcode = GAME_ITEM_NOT_FOUND;
		}
		else
		{
			resultcode = GAME_ITEM_UPGRADE_NO_HOIPOI_STONE;
			ERR_LOG(LOG_HACK, "ERROR: User %u dont have equip, hoi poi stone or stone count is 0 !!", cPlayer->GetCharID());
		}
	}
	else
	{
		resultcode = GAME_FAIL;
	}

END:

	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_ITEM_UPGRADE_WORK_RES));
	app->Send(GetHandle(), &packet);
}

//-------------------------------------------------//-------------------------------------------------//-------------------------------------------------
//													   =====			PRIVATE SHOP			=====
//-------------------------------------------------//-------------------------------------------------//-------------------------------------------------
void CClientSession::RecvCreatePrivateShopReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	cPlayer->CreatePrivateShop();
}

void CClientSession::RecvExitPrivateShopReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	cPlayer->ClosePrivateShop();
}

void CClientSession::RecvItemInsertPrivateShopReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PRIVATESHOP_ITEM_INSERT_REQ * req = (sUG_PRIVATESHOP_ITEM_INSERT_REQ*)pPacket->GetPacketData();
	
	if (cPlayer->GetPrivateShop())
		cPlayer->GetPrivateShop()->AddItem(req->byInventoryPlace, req->byInventoryPos, req->byPrivateShopInventorySlotPos);
}

void CClientSession::RecvItemUpdatePrivateShopReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PRIVATESHOP_ITEM_UPDATE_REQ * req = (sUG_PRIVATESHOP_ITEM_UPDATE_REQ*)pPacket->GetPacketData();

	if (cPlayer->GetPrivateShop())
		cPlayer->GetPrivateShop()->UpdatePrice(req->byPrivateShopInventorySlotPos, req->dwZenny);
}

void CClientSession::RecvItemDeletePrivateShopReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PRIVATESHOP_ITEM_DELETE_REQ * req = (sUG_PRIVATESHOP_ITEM_DELETE_REQ*)pPacket->GetPacketData();

	if (cPlayer->GetPrivateShop())
		cPlayer->GetPrivateShop()->DelItem(req->byPrivateShopInventorySlotPos);
}

void CClientSession::RecvOpenPrivateShopReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PRIVATESHOP_OPEN_REQ * req = (sUG_PRIVATESHOP_OPEN_REQ*)pPacket->GetPacketData();

	WORD wResultcode = GAME_FAIL;

	if (cPlayer->GetCharStateID() != CHARSTATE_PRIVATESHOP || cPlayer->GetCombatMode())
	{
		wResultcode = GAME_CHAR_IS_WRONG_STATE;
	}
	else if (cPlayer->GetCurWorld() == NULL || GetNaviEngine()->IsBasicAttributeSet(cPlayer->GetCurWorld()->GetNaviInstanceHandle(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().z, DBO_WORLD_ATTR_BASIC_FORBID_PC_SHOP))
	{
		wResultcode = GAME_PRIVATESHOP_PRIVATESHOP_NOT_CREATE_PLACE;
	}
	else if (cPlayer->GetPrivateShop())
	{
		cPlayer->GetPrivateShop()->OpenShop(req->bIsOwnerEmpty, req->byNoticeSize, req->wcNotice, req->wcPrivateShopName);
		return;
	}
	
	CNtlPacket packet(sizeof(sGU_PRIVATESHOP_OPEN_RES));
	sGU_PRIVATESHOP_OPEN_RES* res = (sGU_PRIVATESHOP_OPEN_RES*)packet.GetPacketData();
	res->wOpCode = GU_PRIVATESHOP_OPEN_RES;
	res->wResultCode = wResultcode;
	packet.SetPacketLen(sizeof(sGU_PRIVATESHOP_OPEN_RES));
	cPlayer->SendPacket(&packet);
}

void CClientSession::RecvEnterPrivateShopReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PRIVATESHOP_ENTER_REQ * req = (sUG_PRIVATESHOP_ENTER_REQ*)pPacket->GetPacketData();

	CPrivateShop* shop = g_pPShopManager->GetPrivateShop(req->hOwner);
	if (shop && req->hOwner != cPlayer->GetID())
		shop->EnterShop(cPlayer);
	else
	{
		CNtlPacket packet(sizeof(sGU_PRIVATESHOP_ENTER_RES));
		sGU_PRIVATESHOP_ENTER_RES* res = (sGU_PRIVATESHOP_ENTER_RES*)packet.GetPacketData();
		res->wOpCode = GU_PRIVATESHOP_ENTER_RES;
		res->wResultCode = GAME_PRIVATESHOP_PRIVATESHOP_NULL;
		packet.SetPacketLen(sizeof(sGU_PRIVATESHOP_ENTER_RES));
		g_pApp->Send(GetHandle(), &packet);
	}
}
void CClientSession::RecvLeavePrivateShopReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PRIVATESHOP_LEAVE_REQ * req = (sUG_PRIVATESHOP_LEAVE_REQ*)pPacket->GetPacketData();

	CPrivateShop* shop = cPlayer->GetVisitPrivateShop();
	if (shop && shop->GetOwner() && shop->GetOwner()->GetID() == req->hOwner && cPlayer->GetID() != req->hOwner)
		shop->LeaveShop(cPlayer);
	else
	{
		CNtlPacket packet(sizeof(sGU_PRIVATESHOP_LEAVE_RES));
		sGU_PRIVATESHOP_LEAVE_RES* res = (sGU_PRIVATESHOP_LEAVE_RES*)packet.GetPacketData();
		res->wOpCode = GU_PRIVATESHOP_LEAVE_RES;
		res->wResultCode = GAME_PRIVATESHOP_PRIVATESHOP_NULL;
		packet.SetPacketLen(sizeof(sGU_PRIVATESHOP_LEAVE_RES));
		g_pApp->Send(GetHandle(), &packet);
	}
}

void CClientSession::RecvSelectItemPrivateShopReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PRIVATESHOP_ITEM_SELECT_REQ * req = (sUG_PRIVATESHOP_ITEM_SELECT_REQ*)pPacket->GetPacketData();
	
	CPrivateShop* shop = cPlayer->GetVisitPrivateShop();
	if (shop && shop->GetOwner() && shop->GetOwner()->GetID() == req->hOwner && cPlayer->GetID() != req->hOwner)
		shop->SelectItem(req->byPrivateShopInventorySlotPos, req->byItemState, cPlayer);
	else
	{
		CNtlPacket packet(sizeof(sGU_PRIVATESHOP_ITEM_SELECT_RES));
		sGU_PRIVATESHOP_ITEM_SELECT_RES* res = (sGU_PRIVATESHOP_ITEM_SELECT_RES*)packet.GetPacketData();
		res->wOpCode = GU_PRIVATESHOP_ITEM_SELECT_RES;
		res->wResultCode = GAME_PRIVATESHOP_PRIVATESHOP_NULL;
		packet.SetPacketLen(sizeof(sGU_PRIVATESHOP_ITEM_SELECT_RES));
		g_pApp->Send(GetHandle(), &packet);
	}
}

void CClientSession::RecvBuyItemPrivateShopReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PRIVATESHOP_ITEM_BUYING_REQ * req = (sUG_PRIVATESHOP_ITEM_BUYING_REQ*)pPacket->GetPacketData();

	CPrivateShop* shop = cPlayer->GetVisitPrivateShop();
	if (shop && shop->GetOwner() && shop->GetOwner()->GetID() == req->hOwner && cPlayer->GetID() != req->hOwner)
		shop->BuyItem(req->byPrivateShopInventorySlotPos, cPlayer);
	else
	{
		CNtlPacket packet(sizeof(sGU_PRIVATESHOP_ITEM_BUYING_RES));
		sGU_PRIVATESHOP_ITEM_BUYING_RES* res = (sGU_PRIVATESHOP_ITEM_BUYING_RES*)packet.GetPacketData();
		res->wOpCode = GU_PRIVATESHOP_ITEM_BUYING_RES;
		res->wResultCode = GAME_PRIVATESHOP_PRIVATESHOP_NULL;
		packet.SetPacketLen(sizeof(sGU_PRIVATESHOP_ITEM_BUYING_RES));
		g_pApp->Send(GetHandle(), &packet);
	}
}


void CClientSession::RecvClosePrivateShopReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	if (cPlayer->GetPrivateShop())
	{
		CNtlPacket packet(sizeof(sGU_PRIVATESHOP_CLOSE_RES));
		sGU_PRIVATESHOP_CLOSE_RES* res = (sGU_PRIVATESHOP_CLOSE_RES*)packet.GetPacketData();
		res->wOpCode = GU_PRIVATESHOP_CLOSE_RES;
		res->wResultCode = GAME_SUCCESS;
		packet.SetPacketLen(sizeof(sGU_PRIVATESHOP_CLOSE_RES));
		g_pApp->Send(GetHandle(), &packet);

		CNtlPacket packet2(sizeof(sGU_PRIVATESHOP_CLOSE_NFY));
		sGU_PRIVATESHOP_CLOSE_NFY* res2 = (sGU_PRIVATESHOP_CLOSE_NFY*)packet2.GetPacketData();
		res2->wOpCode = GU_PRIVATESHOP_CLOSE_NFY;
		res2->hOwner = cPlayer->GetID();
		packet2.SetPacketLen(sizeof(sGU_PRIVATESHOP_CLOSE_NFY));
		cPlayer->Broadcast(&packet2, cPlayer);

		cPlayer->SendCharStatePrivateShop(true, PRIVATESHOP_STATE_CLOSE, const_cast<WCHAR*>(GetEmptyString()));

		cPlayer->GetPrivateShop()->CloseShop();
	}
	else ERR_LOG(LOG_USER, "player %u no private shop found.", cPlayer->GetCharID());
}

void CClientSession::RecvPickUpItemReq(CNtlPacket * pPacket)
{
	sUG_ITEM_PICK_REQ * req = (sUG_ITEM_PICK_REQ*)pPacket->GetPacketData();

	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	WORD wRes = GAME_SUCCESS;

	if (cPlayer->IsTrading())
		wRes = GAME_LOOTING_FAIL;
	else if (cPlayer->HasPrivateShop())
		wRes = GAME_LOOTING_FAIL;
	else if (cPlayer->GetCurWorld() == NULL)
		wRes = GAME_WORLD_NOT_FOUND;
	else if (cPlayer->IsFainting())
		wRes = GAME_CHAR_IS_WRONG_STATE;
	else if (cPlayer->GetAspectStateId() == ASPECTSTATE_SPINNING_ATTACK || cPlayer->GetAspectStateId() == ASPECTSTATE_ROLLING_ATTACK)
		wRes = GAME_CHAR_IS_WRONG_STATE;
	else
	{
		CItemDrop* item = g_pItemManager->FindDrop(req->handle);
		if (item && item->IsInitialized())
		{
			//check if still on ground
			if (item->GetCurWorld() == NULL)
				wRes = GAME_LOOTING_FAIL;

			else if(item->GetLocked())
				wRes = GAME_ITEM_IS_LOCK;

			//check distance
			else if (cPlayer->IsInRange(item, NTL_MAX_LOOTING_DISTANCE) == false)
				wRes = GAME_LOOTING_FAIL;

			//check ownership
			else if (item->IsOwnership(cPlayer) == false)
				wRes = GAME_LOOTING_FAIL;

			else if (item->GetObjType() != OBJTYPE_DROPITEM)
				wRes = GAME_LOOTING_FAIL;

			else
			{
				item->PickUpItem(cPlayer);
				return; //return because we send item_pick_res in PickUpItem function
			}
		}
	}

	CNtlPacket packet(sizeof(sGU_ITEM_PICK_RES));
	sGU_ITEM_PICK_RES * res = (sGU_ITEM_PICK_RES*)packet.GetPacketData();
	res->wOpCode = GU_ITEM_PICK_RES;
	res->wResultCode = wRes;
	packet.SetPacketLen(sizeof(sGU_ITEM_PICK_RES));
	g_pApp->Send(GetHandle(), &packet);
}
//--------------------------------------------------------------------------------------//
//		RENEW ITEM DURATION
//--------------------------------------------------------------------------------------//
void CClientSession::RecvItemDurationRenewReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_DURATION_RENEW_REQ * req = (sUG_DURATION_RENEW_REQ*)pPacket->GetPacketData();
	WORD resultcode = GAME_SUCCESS; 
	
	CItem* pItem = cPlayer->GetPlayerItemContainer()->GetItem(req->hItemHandle);
	if (pItem && pItem->GetCount() > 0 && IsInvenContainer(pItem->GetPlace()))
	{
		if (pItem->IsLocked() == false)
		{
			sITEM_TBLDAT* pItemData = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(req->itemTblidx);
			if (pItemData)
			{
				if (pItemData->bIsCanRenewal)
				{
					DWORD dwPaymentAmount = 0;
					if (pItemData->byCommonPointType == 1) // 1 == TOKEN (NETPY)
					{
						if (cPlayer->GetNetpyTokens() >= pItemData->CommonPoint)
						{
							CNtlPacket packet2(sizeof(sGU_ITEM_UPDATE));
							sGU_ITEM_UPDATE* res2 = (sGU_ITEM_UPDATE*)packet2.GetPacketData();
							res2->wOpCode = GU_ITEM_UPDATE;
							res2->handle = req->hItemHandle;
							memcpy(&res2->sItemData, &pItem->GetItemData(), sizeof(sITEM_DATA));

							if (pItem->IsExpired() == false)
								res2->sItemData.nUseEndTime = pItem->GetItemData().nUseEndTime + pItemData->dwUseDurationMax;
							else
							{
								res2->sItemData.nUseEndTime = app->GetTime() + (DBOTIME)pItemData->dwUseDurationMax;
							}

							packet2.SetPacketLen(sizeof(sGU_ITEM_UPDATE));
							app->Send(GetHandle(), &packet2);

							pItem->SetUseEndTime(res2->sItemData.nUseEndTime);

							dwPaymentAmount = pItemData->CommonPoint;

							cPlayer->UpdateNetpyToken(false, pItemData->CommonPoint, false);
						}
						else resultcode = GAME_NETP_POINT_NOT_ENOUGH;
					}
					if (pItemData->byCommonPointType == 2) // 2 == CASH POINTS
					{
						if (cPlayer->GetItemShopCash() >= pItemData->CommonPoint)
						{
							CNtlPacket packet2(sizeof(sGU_ITEM_UPDATE));
							sGU_ITEM_UPDATE* res2 = (sGU_ITEM_UPDATE*)packet2.GetPacketData();
							res2->wOpCode = GU_ITEM_UPDATE;
							res2->handle = req->hItemHandle;
							memcpy(&res2->sItemData, &pItem->GetItemData(), sizeof(sITEM_DATA));

							if (pItem->IsExpired() == false)
								res2->sItemData.nUseEndTime = pItem->GetItemData().nUseEndTime + pItemData->dwUseDurationMax;
							else
							{
								res2->sItemData.nUseEndTime = app->GetTime() + (DBOTIME)pItemData->dwUseDurationMax;
							}

							packet2.SetPacketLen(sizeof(sGU_ITEM_UPDATE));
							app->Send(GetHandle(), &packet2);

							pItem->SetUseEndTime(res2->sItemData.nUseEndTime);

							dwPaymentAmount = pItemData->CommonPoint;

							cPlayer->SetItemShopCash(cPlayer->GetItemShopCash() - pItemData->CommonPoint);
						}
						else resultcode = GAME_NETP_POINT_NOT_ENOUGH;
					}
					else //zeni
					{
						if (cPlayer->GetZeni() >= pItemData->dwCost)
						{
							CNtlPacket packet2(sizeof(sGU_ITEM_UPDATE));
							sGU_ITEM_UPDATE* res2 = (sGU_ITEM_UPDATE*)packet2.GetPacketData();
							res2->wOpCode = GU_ITEM_UPDATE;
							res2->handle = req->hItemHandle;
							memcpy(&res2->sItemData, &pItem->GetItemData(), sizeof(sITEM_DATA));

							if(pItem->IsExpired() == false)
								res2->sItemData.nUseEndTime = pItem->GetItemData().nUseEndTime + pItemData->dwUseDurationMax;
							else
							{
								res2->sItemData.nUseEndTime = app->GetTime() + (DBOTIME)pItemData->dwUseDurationMax;
							}

							packet2.SetPacketLen(sizeof(sGU_ITEM_UPDATE));
							app->Send(GetHandle(), &packet2);

							pItem->SetUseEndTime(res2->sItemData.nUseEndTime);

							dwPaymentAmount = pItemData->dwCost;

							cPlayer->UpdateZeni(ZENNY_CHANGE_TYPE_SHOP_DURATION_RENEW, pItemData->dwCost, false, false);
						}
						else resultcode = GAME_ZENNY_NOT_ENOUGH;
					}

					if (resultcode == GAME_SUCCESS)
					{
						if (cPlayer->GetPlayerItemContainer()->HasItemInDuration(pItem->GetID()) == false)
							cPlayer->GetPlayerItemContainer()->AddItemDuration(pItem->GetID(), pItem);

						CNtlPacket pQry(sizeof(sGQ_DURATION_RENEW_REQ));
						sGQ_DURATION_RENEW_REQ* rQry = (sGQ_DURATION_RENEW_REQ*)pQry.GetPacketData();
						rQry->wOpCode = GQ_DURATION_RENEW_REQ;
						rQry->charId = cPlayer->GetCharID();
						rQry->accountId = cPlayer->GetAccountID();
						rQry->byPayType = pItemData->byCommonPointType;
						rQry->dwPayAmount = dwPaymentAmount;
						rQry->sData.itemID = pItem->GetItemID();
						rQry->sData.nUseEndTime = pItem->GetUseEndTime();
						pQry.SetPacketLen(sizeof(sGQ_DURATION_RENEW_REQ));
						app->SendTo(app->GetQueryServerSession(), &pQry);
					}
				}
				else  resultcode = GAME_ITEM_CANT_RENEWRAL;
			}
			else resultcode = GAME_ITEM_NOT_FOUND;
		}
		else resultcode = GAME_ITEM_IS_LOCK;
	}
	else resultcode = GAME_ITEM_NOT_FOUND;

	CNtlPacket packet(sizeof(sGU_DURATION_RENEW_RES));
	sGU_DURATION_RENEW_RES* res = (sGU_DURATION_RENEW_RES*)packet.GetPacketData();
	res->wOpCode = GU_DURATION_RENEW_RES;
	res->hItemHandle = req->hItemHandle;
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_DURATION_RENEW_RES));
	app->Send(GetHandle(), &packet);
}


//--------------------------------------------------------------------------------------//
//		UPDATE ITEM BATTLE ATTRIBUTE
//--------------------------------------------------------------------------------------//
void CClientSession::RecvItemChangeBattleAttributeReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_ITEM_CHAGE_BATTLE_ATTRIBUTE_REQ * req = (sUG_ITEM_CHAGE_BATTLE_ATTRIBUTE_REQ*)pPacket->GetPacketData();
	WORD resultcode = GAME_SUCCESS;
	BYTE byBattleattribute = BATTLE_ATTRIBUTE_NONE;
	DWORD dwPrice = 0;
	
	CNpc* pNpc = g_pObjectManager->GetNpc(req->npcHandle);
	if (pNpc)
	{
		if (pNpc->GetTbldat()->byJob == NPC_JOB_ITEM_UPGRADE)
		{
			if (cPlayer->IsInRange(pNpc, DBO_DISTANCE_CHECK_TOLERANCE))
			{
				CItem* pItem = cPlayer->GetPlayerItemContainer()->GetItem(req->byItemPlace, req->byItemPos);
				if (pItem && IsInvenContainer(pItem->GetPlace()))
				{
					if (pItem->IsLocked() == false)
					{
						dwPrice = Dbo_GetChargeItemBattleAttributeChange(pItem->GetRank(), pItem->GetTbldat()->byNeed_Min_Level);

						if (cPlayer->GetZeni() >= dwPrice)
						{
							CItem* additionalItem = cPlayer->GetPlayerItemContainer()->GetItem(req->byAdditialItemPlace, req->byAdditialItemPos);
							if (additionalItem && additionalItem->GetCount() > 0 && additionalItem->GetGrade() == 0 
								&& req->byAdditionalAttribute < BATTLE_ATTRIBUTE_COUNT && additionalItem->IsLocked() == false
								&& IsInvenContainer(additionalItem->GetPlace()))
							{
								if (Dbo_CheckProbability(50))
									byBattleattribute = req->byAdditionalAttribute;
								else
									byBattleattribute = RandomRange(BATTLE_ATTRIBUTE_HONEST, BATTLE_ATTRIBUTE_FUNNY);

								//del additional item
								additionalItem->SetCount(additionalItem->GetCount() - 1, false, true);
							}
							else
							{
								if (Dbo_CheckProbability(20))
									byBattleattribute = RandomRange(BATTLE_ATTRIBUTE_HONEST, BATTLE_ATTRIBUTE_FUNNY);
								else
									byBattleattribute = BATTLE_ATTRIBUTE_NONE;
							}


							pItem->SetBattleAttribute(byBattleattribute);
							cPlayer->UpdateZeni(ZENNY_CHANGE_TYPE_ITEM_ATTRIBUTE_CHANGE, dwPrice, false, false);

							CNtlPacket packetQry(sizeof(sGQ_ITEM_CHANGE_ATTRIBUTE_REQ));
							sGQ_ITEM_CHANGE_ATTRIBUTE_REQ* resQry = (sGQ_ITEM_CHANGE_ATTRIBUTE_REQ*)packetQry.GetPacketData();
							resQry->wOpCode = GQ_ITEM_CHANGE_ATTRIBUTE_REQ;
							resQry->charId = cPlayer->GetCharID();
							resQry->handle = cPlayer->GetID();
							resQry->byBattleAttribute = byBattleattribute;
							resQry->itemId = pItem->GetItemID();
							resQry->dwZeni = dwPrice;
							packetQry.SetPacketLen(sizeof(sGQ_ITEM_CHANGE_ATTRIBUTE_REQ));
							app->SendTo(app->GetQueryServerSession(), &packetQry);

						}
						else resultcode = GAME_ZENNY_NOT_ENOUGH;
					}
					else resultcode = GAME_ITEM_IS_LOCK;
				}
				else resultcode = GAME_ITEM_CHANGE_BATTLE_ATTRIBUTE_NO_SUBJECT_ITEM;
			}
			else resultcode = GAME_TARGET_TOO_FAR;
		}
		else resultcode = GAME_TARGET_HAS_DIFFERENT_JOB;
	}
	else resultcode = GAME_TARGET_NOT_FOUND;

	CNtlPacket packet(sizeof(sGU_ITEM_CHANGE_BATTLE_ATTRIBUTE_RES));
	sGU_ITEM_CHANGE_BATTLE_ATTRIBUTE_RES* res = (sGU_ITEM_CHANGE_BATTLE_ATTRIBUTE_RES*)packet.GetPacketData();
	res->wOpCode = GU_ITEM_CHANGE_BATTLE_ATTRIBUTE_RES;
	res->byItemPlace = req->byItemPlace;
	res->byItemPos = req->byItemPos;
	res->byBattleAttribute = byBattleattribute;
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_ITEM_CHANGE_BATTLE_ATTRIBUTE_RES));
	app->Send(GetHandle(), &packet);
}


//--------------------------------------------------------------------------------------//
//		UPGRADE ITEM BY COUPON
//--------------------------------------------------------------------------------------//
void CClientSession::RecvItemUpgradeByCouponReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_ITEM_UPGRADE_BY_COUPON_REQ * req = (sUG_ITEM_UPGRADE_BY_COUPON_REQ*)pPacket->GetPacketData();

	WORD resultcode = GAME_FAIL;

	
	CItem* pItemEquipment = cPlayer->GetPlayerItemContainer()->GetItem(req->byItemPlace,req->byItemPos);
	if(pItemEquipment && IsInvenContainer(pItemEquipment->GetPlace()))
	{
		if(pItemEquipment->IsLocked()) //check if locked
			resultcode = GAME_ITEM_IS_LOCK;
		else if(pItemEquipment->GetGrade() > 0) // only allow on items with grade 0
			resultcode = GAME_ITEM_UPGRADE_COUPON_GRADE_FAIL;
		else if(pItemEquipment->GetRestrictType() != ITEM_RESTRICT_TYPE_NONE || pItemEquipment->GetRestrictState() != ITEM_RESTRICT_STATE_TYPE_NONE) // Only allow on items without restriction
			resultcode = GAME_ITEM_UPGRADE_FAIL_FOR_DURATION_ITEM;
		else if (pItemEquipment->GetDurationtype() != eDURATIONTYPE_NORMAL) // check if item has duration
			resultcode = GAME_ITEM_UPGRADE_FAIL_FOR_DURATION_ITEM;
		else if(pItemEquipment->NeedToIdentify() == true)
			resultcode = GAME_ITEM_UPGRADE_FAIL_FOR_DURATION_ITEM;
		else
		{
			CItem* pItemCoupon = cPlayer->GetPlayerItemContainer()->GetItem(req->byCouponPlace, req->byCouponPos);
			if (pItemCoupon && IsInvenContainer(pItemCoupon->GetPlace()))
			{
				BYTE byCouponItemType = pItemCoupon->GetTbldat()->byItem_Type;
				BYTE byItemTypeGroup = GetItemTypeGroup(pItemEquipment->GetTbldat()->byItem_Type);

				if (byItemTypeGroup != ITEM_TYPE_GROUP_WEAPON && byItemTypeGroup != ITEM_TYPE_GROUP_ARMOR)
					resultcode = GAME_ITEM_UPGRADE_WRONG_ITEM_TYPE;
				else if (byItemTypeGroup == ITEM_TYPE_GROUP_WEAPON && byCouponItemType != ITEM_TYPE_UPGRADE_COUPON_WEAPON && byCouponItemType != ITEM_TYPE_UPGRADE_COUPON_FULL)
					resultcode = GAME_ITEM_UPGRADE_MUST_USE_STONE_WEAPON;
				else if (byItemTypeGroup == ITEM_TYPE_GROUP_ARMOR && byCouponItemType != ITEM_TYPE_UPGRADE_COUPON_ARMOR && byCouponItemType != ITEM_TYPE_UPGRADE_COUPON_FULL)
					resultcode = GAME_ITEM_UPGRADE_MUST_USE_STONE_ARMOR;
				else if(pItemCoupon->IsLocked()) //check if locked
					resultcode = GAME_ITEM_IS_LOCK;
				else if(pItemCoupon->GetCount() == 0 || pItemCoupon->GetCount() > pItemCoupon->GetTbldat()->byMax_Stack) //check stack count
					resultcode = GAME_ITEM_STACK_FAIL;
				else if(pItemEquipment->GetTbldat()->byNeed_Min_Level > pItemCoupon->GetTbldat()->byNeed_Max_Level)  //check if item level is higher than max coupon level
					resultcode = GAME_ITEM_NOT_INSERT_BEAD_INVALID_LEVEL;
				else if (pItemEquipment->GetTbldat()->byNeed_Min_Level < pItemCoupon->GetTbldat()->byNeed_Min_Level) //check if item level is lower than min. coupon level
					resultcode = GAME_ITEM_NOT_INSERT_BEAD_INVALID_LEVEL;
				else
				{
					resultcode = GAME_SUCCESS;

					CNtlPacket pQry(sizeof(sGQ_ITEM_UPGRADE_BY_COUPON_REQ));
					sGQ_ITEM_UPGRADE_BY_COUPON_REQ * rQry = (sGQ_ITEM_UPGRADE_BY_COUPON_REQ *)pQry.GetPacketData();
					rQry->wOpCode = GQ_ITEM_UPGRADE_BY_COUPON_REQ;
					rQry->handle = cPlayer->GetID();
					rQry->charId = cPlayer->GetCharID();
					rQry->byCouponPlace = req->byCouponPlace;
					rQry->byCouponPos = req->byCouponPos;
					rQry->byGrade = (BYTE)pItemCoupon->GetTbldat()->Charm_Tblidx;
					rQry->byItemPlace = req->byItemPlace;
					rQry->byItemPos = req->byItemPos;
					rQry->byRestrictState = ITEM_RESTRICT_STATE_TYPE_CHARACTER_GET;
					rQry->byDurationType = eDURATIONTYPE_FLATSUM;
					rQry->couponId = pItemCoupon->GetItemID();
					rQry->itemId = pItemEquipment->GetItemID();
					rQry->nUseStartTime = app->GetTime();
					rQry->nUseEndTime = rQry->nUseStartTime + (DBOTIME)pItemCoupon->GetTbldat()->contentsTblidx;
					pQry.SetPacketLen(sizeof(sGQ_ITEM_UPGRADE_BY_COUPON_REQ));
					app->SendTo(app->GetQueryServerSession(), &pQry);

					// lock equipment and coupon
					pItemEquipment->SetLocked(true);
					pItemCoupon->SetLocked(true);
				}
			}
			else resultcode = GAME_ITEM_UPGRADE_COUPON_NOT_FOUND;
		}
	} 
	else resultcode = GAME_ITEM_NOT_FOUND;

	if (resultcode != GAME_SUCCESS)
	{
		CNtlPacket packet(sizeof(sGU_ITEM_UPGRADE_BY_COUPON_RES));
		sGU_ITEM_UPGRADE_BY_COUPON_RES* res = (sGU_ITEM_UPGRADE_BY_COUPON_RES*)packet.GetPacketData();
		res->wOpCode = GU_ITEM_UPGRADE_BY_COUPON_RES;
		res->byItemPlace = req->byItemPlace;
		res->byItemPos = req->byItemPos;
		res->byCouponPlace = req->byCouponPlace;
		res->byCouponPos = req->byCouponPos;
		res->wResultCode = resultcode;
		packet.SetPacketLen(sizeof(sGU_ITEM_UPGRADE_BY_COUPON_RES));
		app->Send(GetHandle(), &packet);
	}
}

//--------------------------------------------------------------------------------------//
//		DROP ITEM INFO
//--------------------------------------------------------------------------------------//
void CClientSession::RecvDropItemInfoReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_DROPITEM_INFO_REQ * req = (sUG_DROPITEM_INFO_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_DROPITEM_INFO_RES));
	sGU_DROPITEM_INFO_RES* res = (sGU_DROPITEM_INFO_RES*)packet.GetPacketData();
	res->wOpCode = GU_DROPITEM_INFO_RES;
	res->dwDialogID = req->dwDialogID;
	res->hItemHandle = req->hItemHandle;
		
	CItemDrop* item = g_pItemManager->FindDrop(req->hItemHandle);
	if(item && item->GetObjType() == OBJTYPE_DROPITEM && item->GetCurWorld())
		memcpy(&res->sItemOptionSet, item->GetOptionSet(), sizeof(sITEM_OPTION_SET));

	packet.SetPacketLen(sizeof(sGU_DROPITEM_INFO_RES));
	g_pApp->Send(GetHandle(), &packet);
}

//--------------------------------------------------------------------------------------//
//		UPDATE SHORTCUT KEY
//--------------------------------------------------------------------------------------//
void CClientSession::RecvShortcutKeyUpdateReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_CHAR_KEY_UPDATE_REQ * req = (sUG_CHAR_KEY_UPDATE_REQ*)pPacket->GetPacketData();
	if (req->byCount > 0 && req->byCount < NTL_SHORTCUT_MAX_COUNT)
	{
		CNtlPacket packet(sizeof(sGU_CHAR_KEY_UPDATE_RES));
		sGU_CHAR_KEY_UPDATE_RES* res = (sGU_CHAR_KEY_UPDATE_RES*)packet.GetPacketData();
		res->wOpCode = GU_CHAR_KEY_UPDATE_RES;
		res->wResultCode = GAME_SUCCESS;
		packet.SetPacketLen(sizeof(sGU_CHAR_KEY_UPDATE_RES));
		app->Send(GetHandle(), &packet);

		CNtlPacket pQry(sizeof(sGQ_CHAR_KEY_UPDATE_REQ));
		sGQ_CHAR_KEY_UPDATE_REQ* rQry = (sGQ_CHAR_KEY_UPDATE_REQ*)pQry.GetPacketData();
		rQry->wOpCode = GQ_CHAR_KEY_UPDATE_REQ;
		rQry->charID = cPlayer->GetCharID();
		rQry->accountID = cPlayer->GetAccountID();
		rQry->byCount = req->byCount;
		memcpy(rQry->asData, req->asData, sizeof(req->asData));
		pQry.SetPacketLen(sizeof(sGQ_CHAR_KEY_UPDATE_REQ));
		app->SendTo(app->GetQueryServerSession(), &pQry);
	}
}

void CClientSession::RecvPerformanceDataNfy(CNtlPacket * pPacket)
{
	sUG_PERFORMANCE_DATA_NFY * req = (sUG_PERFORMANCE_DATA_NFY*)pPacket->GetPacketData();
	
//	printf("%d %d %d %d %d %d %d \n", req->dwValue, req->adwCount[0], req->adwCount[1], req->adwCount[2], req->adwCount[3], req->adwCount[4], req->adwCount[5]);



}

void CClientSession::RecvAttackTargetNfy(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PET_ATTACK_TARGET_NFY * req = (sUG_PET_ATTACK_TARGET_NFY*)pPacket->GetPacketData();

	//printf("%u %u %u %u\n", req->byAvatarType, req->Unknown, req->Unknown2, req->Unknown3);

	if (CSummonPet* pPet = cPlayer->GetSummonPet())
	{
		if(pPet->GetAttackTarget() == cPlayer->GetTargetHandle() || cPlayer->GetTargetHandle() == INVALID_HOBJECT)
			pPet->SetAttackTarget(INVALID_HOBJECT);
		else if (cPlayer->GetTargetHandle() != INVALID_HOBJECT && cPlayer->GetTargetHandle() != cPlayer->GetID())
		{
			pPet->SetAttackTarget(cPlayer->GetTargetHandle());
		}
	}
}

//--------------------------------------------------------------------------------------//
//		SELECT CHARACTER TITLE
//--------------------------------------------------------------------------------------//
void CClientSession::RecvSelectCharTitleReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;
	sUG_CHARTITLE_SELECT_REQ * req = (sUG_CHARTITLE_SELECT_REQ*)pPacket->GetPacketData();
	
	WORD resultcode = GAME_SUCCESS;

	/*	Note:
		- When unset char title, then req->tblIndex is INVALID_TBLIDX.
	*/

	if (req->tblIndex != INVALID_TBLIDX)
	{
		sCHARTITLE_TBLDAT *charTitleTbldat = (sCHARTITLE_TBLDAT *)g_pTableContainer->GetCharTitleTable()->FindData(req->tblIndex);
		if (charTitleTbldat == NULL)
			resultcode = CHARTITLE_NO_HAVE;
		else
		{
			if ((req->tblIndex - 1) / NTL_MAX_CHAR_TITLE_FLAG_COUNT >= NTL_MAX_CHAR_TITLE_COUNT_IN_FLAG)
				resultcode = CHARTITLE_NO_HAVE;
			else if (cPlayer->CheckCharTitle(req->tblIndex - 1) == false) //check if player has title
			{
				resultcode = CHARTITLE_NO_HAVE;
			}
		}
	}

	CNtlPacket packet(sizeof(sGU_CHARTITLE_SELECT_RES));
	sGU_CHARTITLE_SELECT_RES* res = (sGU_CHARTITLE_SELECT_RES*)packet.GetPacketData();
	res->wOpCode = GU_CHARTITLE_SELECT_RES;
	res->tblIndex = req->tblIndex;
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_CHARTITLE_SELECT_RES));
	app->Send(GetHandle(), &packet);


	if(resultcode == GAME_SUCCESS)
	{
		//send to database
		CNtlPacket packet(sizeof(sGQ_CHARTITLE_SELECT_REQ));
		sGQ_CHARTITLE_SELECT_REQ* res = (sGQ_CHARTITLE_SELECT_REQ*)packet.GetPacketData();
		res->wOpCode = GQ_CHARTITLE_SELECT_REQ;
		res->charId = cPlayer->GetCharID();
		res->charTitle = req->tblIndex;
		packet.SetPacketLen(sizeof(sGQ_CHARTITLE_SELECT_REQ));
		app->SendTo(app->GetQueryServerSession(), &packet);

		//send nfy to other players that my title updated
		CNtlPacket packet2(sizeof(sGU_CHARTITLE_SELECT_NFY));
		sGU_CHARTITLE_SELECT_NFY* res2 = (sGU_CHARTITLE_SELECT_NFY*)packet2.GetPacketData();
		res2->wOpCode = GU_CHARTITLE_SELECT_NFY;
		res2->charTitle = req->tblIndex;
		res2->handle = cPlayer->GetID();
		packet2.SetPacketLen(sizeof(sGU_CHARTITLE_SELECT_NFY));
		cPlayer->BroadcastToNeighbor(&packet2);

		//update char id
		cPlayer->SetCharTitleID(req->tblIndex);

		//update attributes
		cPlayer->GetCharAtt()->CalculateAll();
	}
}


void CClientSession::RecvPartySelectStateReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PARTY_SELECT_STATE_REQ * req = (sUG_PARTY_SELECT_STATE_REQ*)pPacket->GetPacketData();

	WORD wResultcode = GAME_SUCCESS;

	if(req->bySelectState == NTL_PARTY_SELECT_TYPE_FIRST || req->bySelectState > NTL_PARTY_SELECT_TYPE_LAST)
		wResultcode = GAME_FAIL;
	else if(cPlayer->GetParty() == NULL || cPlayer->GetPartyID() == INVALID_PARTYID)
		wResultcode = GAME_PARTY_YOU_ARE_NOT_IN_PARTY;
	else if(cPlayer->GetCCBD() == NULL)
		wResultcode = GAME_FAIL;
	else
	{
		cPlayer->GetParty()->PartyMemberSelect(cPlayer->GetID(), req->bySelectState);
	}

	CNtlPacket packet(sizeof(sGU_PARTY_SELECT_STATE_RES));
	sGU_PARTY_SELECT_STATE_RES* res = (sGU_PARTY_SELECT_STATE_RES*)packet.GetPacketData();
	res->wOpCode = GU_PARTY_SELECT_STATE_RES;
	res->bySelectState = req->bySelectState;
	res->wResultCode = wResultcode;
	packet.SetPacketLen(sizeof(sGU_PARTY_SELECT_STATE_RES));
	cPlayer->SendPacket(&packet);
}


//--------------------------------------------------------------------------------------//
//		(CCBD) BATTLE DUNGEON ENTER REQUEST
//--------------------------------------------------------------------------------------//
void CClientSession::RecvBattleDungeonEnterReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_BATTLE_DUNGEON_ENTER_REQ * req = (sUG_BATTLE_DUNGEON_ENTER_REQ*)pPacket->GetPacketData();

	WORD wResultcode = GAME_SUCCESS;

	if (cPlayer->IsGameMaster()) //only allow for gm until its done
	{
		CNpc* pNpc = g_pObjectManager->GetNpc(req->hNpc);
		if (pNpc && cPlayer->IsInRange(pNpc, DBO_DISTANCE_CHECK_TOLERANCE))
		{
			if (pNpc->GetTbldat()->byJob != NPC_JOB_BATTLE_DUNGEON_MANAGER)
				wResultcode = GAME_TARGET_HAS_DIFFERENT_JOB;
			else
			{
				if (cPlayer->GetParty() && cPlayer->GetPartyID() != INVALID_PARTYID)
				{
					if (cPlayer->GetParty()->GetPartyLeaderID() == cPlayer->GetID())
					{
						if (cPlayer->GetParty()->IsEveryoneInLeaderRange(cPlayer, NTL_MAX_RADIUS_OF_VISIBLE_AREA))
						{
							BYTE byBeginStage = 1;
							CItem* pItem = NULL;

							if (req->hItem != INVALID_HOBJECT)
							{
								pItem = cPlayer->GetPlayerItemContainer()->GetItem(req->hItem);
								if (pItem && pItem->GetCount() > 0 && pItem->IsLocked() == false && IsInvenContainer(pItem->GetPlace()))
								{
									if (pItem->GetTbldat()->byItem_Type == ITEM_TYPE_BATTLE_DUNGEON_TICKET)
										byBeginStage = (BYTE)pItem->GetTbldat()->contentsTblidx;
								}
							}

							CBattleDungeon* pDungeon = g_pDungeonManager->CreateBattleDungeon(cPlayer, wResultcode, byBeginStage);
							if (pDungeon == NULL)
								wResultcode = GAME_PARTY_DUNGEON_IS_NOT_CREATED;
							else
							{
								if (pItem && byBeginStage > 1)
									pItem->SetCount(pItem->GetCount() - 1, false, true);
							}
						}
						else wResultcode = GAME_PARTY_MEMBER_IS_TOO_FAR;
					}
					else wResultcode = GAME_COMMON_YOU_ARE_NOT_A_PARTY_LEADER;
				}
				else wResultcode = GAME_COMMON_YOU_ARE_NOT_IN_A_PARTY;
			}
		}
		else wResultcode = GAME_FAIL;
	}
	else wResultcode = GAME_FAIL;

	CNtlPacket packet(sizeof(sGU_BATTLE_DUNGEON_ENTER_RES));
	sGU_BATTLE_DUNGEON_ENTER_RES* res = (sGU_BATTLE_DUNGEON_ENTER_RES*)packet.GetPacketData();
	res->wOpCode = GU_BATTLE_DUNGEON_ENTER_RES;
	res->wResultCode = wResultcode;
	packet.SetPacketLen(sizeof(sGU_BATTLE_DUNGEON_ENTER_RES));
	g_pApp->Send(GetHandle(), &packet);
}

//--------------------------------------------------------------------------------------//
//		(CCBD) BATTLE DUNGEON LEAVE REQUEST
//--------------------------------------------------------------------------------------//
void CClientSession::RecvBattleDungeonLeaveReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_BATTLE_DUNGEON_LEAVE_REQ * req = (sUG_BATTLE_DUNGEON_LEAVE_REQ*)pPacket->GetPacketData();

	WORD wResultcode = GAME_SUCCESS;

	if (cPlayer->GetCCBD())
	{
		CWorld* pWorld = cPlayer->GetCCBD()->GetWorld();
		if (pWorld)
		{
			if (cPlayer->GetParty())
				cPlayer->LeaveParty(); //here we leave party and get teleported out
			else
				cPlayer->StartTeleport(pWorld->GetTbldat()->outWorldLoc, cPlayer->GetCurDir(), pWorld->GetTbldat()->outWorldTblidx, TELEPORT_TYPE_WORLD_MOVE);
		}
		else
			wResultcode = GAME_WORLD_NOT_FOUND;
	}
	else wResultcode = GAME_FAIL;

	CNtlPacket packet(sizeof(sGU_BATTLE_DUNGEON_LEAVE_RES));
	sGU_BATTLE_DUNGEON_LEAVE_RES* res = (sGU_BATTLE_DUNGEON_LEAVE_RES*)packet.GetPacketData();
	res->wOpCode = GU_BATTLE_DUNGEON_LEAVE_RES;
	res->wResultCode = wResultcode;
	g_pApp->Send(GetHandle(), &packet);
}

void CClientSession::RecvItemSealReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_ITEM_SEAL_REQ * req = (sUG_ITEM_SEAL_REQ*)pPacket->GetPacketData();

	WORD wResultcode = GAME_SUCCESS;

	CNpc* pNpc = g_pObjectManager->GetNpc(cPlayer->GetTargetHandle());
	if (pNpc == NULL)
		wResultcode = GAME_FAIL;
	else if (cPlayer->IsInRange(pNpc, DBO_DISTANCE_CHECK_TOLERANCE) == false)
		wResultcode = GAME_TARGET_TOO_FAR;
	else if (pNpc->GetTbldat()->byJob != NPC_JOB_VENDING_MACHINE)
		wResultcode = GAME_TARGET_HAS_DIFFERENT_JOB;
	else if (BIT_FLAG_TEST(pNpc->GetTbldat()->dwFunc_Bit_Flag, NPC_FUNC_FLAG_SUMMON_PET) == false)
		wResultcode = GAME_TARGET_HAS_NOT_FUNCTION;
	else
	{
		CItem* pItem = cPlayer->GetPlayerItemContainer()->GetItem(req->byItemPlace, req->byItemPos);
		if (pItem == NULL)
			wResultcode = GAME_NEEDITEM_NOT_FOUND;
		else if (pItem->IsLocked() || IsInvenContainer(pItem->GetPlace()) == false)
			wResultcode = GAME_ITEM_IS_LOCK;
		else if (IsSealing(pItem->GetRestrictType(), pItem->GetRestrictState(), pItem->GetTbldat()->byEquip_Type) == false)
			wResultcode = GAME_FAIL;
		else
		{
			CItem* pSealItem = cPlayer->GetPlayerItemContainer()->GetItem(req->bySealPlace, req->bySealPos);
			if (pSealItem == NULL || pSealItem->GetCount() == 0 || IsInvenContainer(pSealItem->GetPlace()) == false)
				wResultcode = GAME_NEEDITEM_NOT_FOUND;
			else if (pSealItem->IsLocked())
				wResultcode = GAME_ITEM_IS_LOCK;
			else if (pSealItem->GetTbldat()->byItem_Type != ITEM_TYPE_SEAL)
				wResultcode = GAME_ITEM_TYPE_NOT_MISMATCHED;
			else
			{
				BYTE byRequiredStack = GetRequiredSealItemNum(pItem->GetRank(), pItem->GetGrade());
				//	printf("byRequiredStack %u \n", byRequiredStack);
				if (pSealItem->GetCount() >= byRequiredStack)
				{
					pSealItem->SetLocked(true);
					pItem->SetLocked(true);

					CNtlPacket pQry(sizeof(sGQ_ITEM_SEAL_REQ));
					sGQ_ITEM_SEAL_REQ* rQry = (sGQ_ITEM_SEAL_REQ*)pQry.GetPacketData();
					rQry->wOpCode = GQ_ITEM_SEAL_REQ;
					rQry->handle = cPlayer->GetID();
					rQry->charId = cPlayer->GetCharID();
					rQry->byItemPlace = req->byItemPlace;
					rQry->byItemPos = req->byItemPos;
					rQry->itemId = pItem->GetItemID();
					rQry->byRestrictState = ITEM_RESTRICT_STATE_TYPE_SEAL;
					rQry->bySealPlace = req->bySealPlace;
					rQry->bySealPos = req->bySealPos;
					rQry->SealitemId = pSealItem->GetItemID();
					rQry->bySealStack = byRequiredStack;
					pQry.SetPacketLen(sizeof(sGQ_ITEM_SEAL_REQ));
					app->SendTo(app->GetQueryServerSession(), &pQry);

					return;
				}
				else wResultcode = GAME_ITEM_NOT_ENOUGH;
			}
		}
	}

	CNtlPacket packet(sizeof(sGU_ITEM_SEAL_RES));
	sGU_ITEM_SEAL_RES* res = (sGU_ITEM_SEAL_RES*)packet.GetPacketData();
	res->wOpCode = GU_ITEM_SEAL_RES;
	res->wResultCode = wResultcode;
	res->byItemPlace = req->byItemPlace;
	res->byItemPos = req->byItemPos;
	res->bySealPlace = req->bySealPlace;
	res->bySealPos = req->bySealPos;
	res->byRestrictState = ITEM_RESTRICT_STATE_TYPE_SEAL;
	g_pApp->Send(GetHandle(), &packet);
}

void CClientSession::RecvItemSealExtractReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_ITEM_SEAL_EXTRACT_REQ * req = (sUG_ITEM_SEAL_EXTRACT_REQ*)pPacket->GetPacketData();

	WORD wResultcode = GAME_SUCCESS;

	CItem* pItem = cPlayer->GetPlayerItemContainer()->GetItem(req->byItemPlace, req->byItemPos);
	if (pItem == NULL)
		wResultcode = GAME_NEEDITEM_NOT_FOUND;
	else if (pItem->IsLocked() || IsInvenContainer(pItem->GetPlace()) == false)
		wResultcode = GAME_ITEM_IS_LOCK;
	else if (IsSealItem(pItem->GetRestrictState()) == false)
		wResultcode = GAME_FAIL;
	else
	{	
		pItem->SetLocked(true);

		CNtlPacket pQry(sizeof(sGQ_ITEM_SEAL_EXTRACT_REQ));
		sGQ_ITEM_SEAL_EXTRACT_REQ* rQry = (sGQ_ITEM_SEAL_EXTRACT_REQ*)pQry.GetPacketData();
		rQry->wOpCode = GQ_ITEM_SEAL_EXTRACT_REQ;
		rQry->handle = cPlayer->GetID();
		rQry->charId = cPlayer->GetCharID();
		rQry->byItemPlace = req->byItemPlace;
		rQry->byItemPos = req->byItemPos;
		rQry->itemId = pItem->GetItemID();
		rQry->byRestrictState = GetDefaultRestrictState(pItem->GetTbldat()->byRestrictType, pItem->GetTbldat()->byItem_Type, true);
		pQry.SetPacketLen(sizeof(sGQ_ITEM_SEAL_EXTRACT_REQ));
		app->SendTo(app->GetQueryServerSession(), &pQry);

		return;
	}


	CNtlPacket packet(sizeof(sGU_ITEM_SEAL_EXTRACT_RES));
	sGU_ITEM_SEAL_EXTRACT_RES* res = (sGU_ITEM_SEAL_EXTRACT_RES*)packet.GetPacketData();
	res->wOpCode = GU_ITEM_SEAL_EXTRACT_RES;
	res->wResultCode = wResultcode;
	res->byItemPlace = req->byItemPlace;
	res->byItemPos = req->byItemPos;
	g_pApp->Send(GetHandle(), &packet);
}


//--------------------------------------------------------------------------------------//
//		LOAD AUCTION HOUSE ITEMS
//--------------------------------------------------------------------------------------//
void CClientSession::RecvLoadAuctionHouseReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_TENKAICHIDAISIJYOU_LIST_REQ * req = (sUG_TENKAICHIDAISIJYOU_LIST_REQ*)pPacket->GetPacketData();

	//ERR_LOG(LOG_USER, "Account %u request auction house", cPlayer->GetAccountID());
	//printf("bIsSystem: %i, byTabType: %u, uiPage: %u, bySortType: %u, byClassType:%u, byItemType:%u, byRank:%u, byMinLevel:%u,byMaxLevel:%u \n",
	//	req->bIsSystem, req->byTabType, req->uiPage, req->bySortType, req->byClassType, req->byItemType, req->byRank, req->byMinLevel, req->byMaxLevel);

	CNtlPacket packet(sizeof(sGT_TENKAICHIDAISIJYOU_LIST_REQ));
	sGT_TENKAICHIDAISIJYOU_LIST_REQ * res = (sGT_TENKAICHIDAISIJYOU_LIST_REQ *)packet.GetPacketData();
	res->wOpCode = GT_TENKAICHIDAISIJYOU_LIST_REQ;
	res->accountId = cPlayer->GetAccountID();
	NTL_SAFE_WCSCPY(res->awchItemName, req->awchItemName);
	res->bIsSystem = req->bIsSystem;
//	res->bMyList =
	res->byClass = req->byClassType;
	res->byItemType = req->byItemType;
	res->byMaxLevel = req->byMaxLevel;
	res->byMinLevel = req->byMinLevel;
	res->byRank = req->byRank;
	res->bySortType = req->bySortType;
	res->byTabType = req->byTabType;
	res->charId = cPlayer->GetCharID();
	res->uiPage = req->uiPage;
	packet.SetPacketLen(sizeof(sGT_TENKAICHIDAISIJYOU_LIST_REQ));
	app->SendTo(app->GetChatServerSession(), &packet);
}
//--------------------------------------------------------------------------------------//
//		SELL ITEM AT AUCTON HOUSE
//--------------------------------------------------------------------------------------//
void CClientSession::RecvSellAuctionHouseReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_TENKAICHIDAISIJYOU_SELL_REQ * req = (sUG_TENKAICHIDAISIJYOU_SELL_REQ*)pPacket->GetPacketData();

	DWORD dwSellFee = Dbo_GetAuctionHouseSellFee(req->dwPrice);

	WORD wResult;

	//check if enough zeni
	if (cPlayer->GetLevel() >= NTL_AUCTIONHOUSE_REQUIRED_LV)
	{
		if (cPlayer->GetZeni() >= dwSellFee)
		{
			if (req->dwPrice <= NTL_MAX_USE_ZENI)
			{
				CItem* pItem = cPlayer->GetPlayerItemContainer()->GetItem(req->byPlace, req->byPosition);
				if (pItem && pItem->CanAuctionhouse() && IsInvenContainer(pItem->GetPlace()))
				{
					//check if enough item
					if ((req->byCount <= pItem->GetCount() && pItem->GetCount() > 0 && req->byCount > 0)
						&& (pItem->GetTbldat()->byMax_Stack >= req->byCount))
					{
						if (app->GetChatServerSession())
						{
							sITEM_TBLDAT* pItemData = pItem->GetTbldat();

							cPlayer->UpdateZeni(ZENNY_CHANGE_TYPE_TMP_SELL_FEE, dwSellFee, false, false);

							CNtlPacket packet(sizeof(sGT_TENKAICHIDAISIJYOU_SELL_REQ));
							sGT_TENKAICHIDAISIJYOU_SELL_REQ * res = (sGT_TENKAICHIDAISIJYOU_SELL_REQ *)packet.GetPacketData();
							res->wOpCode = GT_TENKAICHIDAISIJYOU_SELL_REQ;


							CTextTable* pTextTable = (CTextTable*)g_pTableContainer->GetTextAllTable()->GetItemTbl();
							if (pTextTable)
							{
								sTEXT_TBLDAT* pTextTbldat = (sTEXT_TBLDAT*)pTextTable->FindData(pItemData->tblidx * 10);
								if (pTextTbldat)
								{
									NTL_SAFE_WCSCPY(res->awchItemName, pTextTbldat->wstrText.c_str());
								}
								else
								{
									NTL_SAFE_WCSCPY(res->awchItemName, pItemData->wszNameText);
								}
							} 
							else
							{
								NTL_SAFE_WCSCPY(res->awchItemName, pItemData->wszNameText);
							}
							
							NTL_SAFE_WCSCPY(res->awchSeller, cPlayer->GetCharName());
							res->byCount = pItem->GetCount();
							res->byItemLevel = pItemData->byNeed_Min_Level;
							res->byItemType = pItemData->byItem_Type;
							res->byPlace = req->byPlace;
							res->byPosition = req->byPosition;
							res->byTabType = pItemData->byTmpTabType;
							res->charId = cPlayer->GetCharID();
							res->dwClassBitFlag = pItemData->dwNeed_Class_Bit_Flag;
							res->dwFee = dwSellFee;
							res->dwPrice = req->dwPrice;
							res->itemId = pItem->GetItemID();
							res->dwTime = 86400;
							packet.SetPacketLen(sizeof(sGT_TENKAICHIDAISIJYOU_SELL_REQ));
							app->SendTo(app->GetChatServerSession(), &packet);

							pItem->RemoveFromCharacter();
							g_pItemManager->RemoveItem(pItem->GetID()); //delete item from channel because when buy/cancel item from auctionhouse a new will be created
							SAFE_DELETE(pItem);

							return;
						}
						else wResult = GAME_FAIL;
					}
					else wResult = TENKAICHIDAISIJYOU_CANNOT_LACK_OF_ITEM_STACK;
				}
				else wResult = TENKAICHIDAISIJYOU_CANNOT_INVALID_ITEM;
			}
			else wResult = GAME_FAIL;
		}
		else wResult = TENKAICHIDAISIJYOU_CANNOT_SELL_NO_MONEY;
	}
	else wResult = GAME_ITEM_NEED_MORE_LEVEL;


	CNtlPacket packet2(sizeof(sGU_TENKAICHIDAISIJYOU_SELL_RES));
	sGU_TENKAICHIDAISIJYOU_SELL_RES * res2 = (sGU_TENKAICHIDAISIJYOU_SELL_RES *)packet2.GetPacketData();
	res2->wOpCode = GU_TENKAICHIDAISIJYOU_SELL_RES;
	res2->byCount = req->byCount;
	res2->byPlace = req->byPlace;
	res2->byPosition = req->byPosition;
	res2->dwPrice = req->dwPrice;
	res2->wResultCode = wResult;
	res2->dwMoney = dwSellFee;
	packet2.SetPacketLen(sizeof(sGU_TENKAICHIDAISIJYOU_SELL_RES));
	app->Send(GetHandle(), &packet2);
}

//--------------------------------------------------------------------------------------//
//		CANCEL SELL ITEM AT AUCTON HOUSE
//--------------------------------------------------------------------------------------//
void CClientSession::RecvSellCancelAuctionHouseReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_TENKAICHIDAISIJYOU_SELL_CANCEL_REQ * req = (sUG_TENKAICHIDAISIJYOU_SELL_CANCEL_REQ*)pPacket->GetPacketData();
	
	CNtlPacket packet(sizeof(sGT_TENKAICHIDAISIJYOU_SELL_CANCEL_REQ));
	sGT_TENKAICHIDAISIJYOU_SELL_CANCEL_REQ * res = (sGT_TENKAICHIDAISIJYOU_SELL_CANCEL_REQ *)packet.GetPacketData();
	res->wOpCode = GT_TENKAICHIDAISIJYOU_SELL_CANCEL_REQ;
	NTL_WCSCPY_S(res->awchSystem, NTL_MAX_SIZE_CHAR_NAME + 1, cPlayer->GetCharName());
	NTL_WCSCPY_S(res->awchText, 128 + 1, L"Cancel Auction Item");
	res->charId = cPlayer->GetCharID();
	res->itemId = req->itemId;
	res->nItem = req->nItem;
	packet.SetPacketLen(sizeof(sGT_TENKAICHIDAISIJYOU_SELL_CANCEL_REQ));
	app->SendTo(app->GetChatServerSession(), &packet);
}

//--------------------------------------------------------------------------------------//
//		BUY ITEM AT AUCTON HOUSE
//--------------------------------------------------------------------------------------//
void CClientSession::RecvBuyAuctionHouseReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_TENKAICHIDAISIJYOU_BUY_REQ * req = (sUG_TENKAICHIDAISIJYOU_BUY_REQ*)pPacket->GetPacketData();

	if (cPlayer->GetLevel() >= NTL_AUCTIONHOUSE_REQUIRED_LV)
	{
		CNtlPacket packet(sizeof(sGT_TENKAICHIDAISIJYOU_PRICE_INFO_REQ));
		sGT_TENKAICHIDAISIJYOU_PRICE_INFO_REQ * res = (sGT_TENKAICHIDAISIJYOU_PRICE_INFO_REQ *)packet.GetPacketData();
		res->wOpCode = GT_TENKAICHIDAISIJYOU_PRICE_INFO_REQ;
		res->charId = cPlayer->GetCharID();
		res->itemId = req->itemId;
		res->nItem = req->nItem;
		packet.SetPacketLen(sizeof(sGT_TENKAICHIDAISIJYOU_PRICE_INFO_REQ));
		app->SendTo(app->GetChatServerSession(), &packet);
	}
	else
	{
		CNtlPacket packet2(sizeof(sGU_TENKAICHIDAISIJYOU_BUY_RES));
		sGU_TENKAICHIDAISIJYOU_BUY_RES * res2 = (sGU_TENKAICHIDAISIJYOU_BUY_RES *)packet2.GetPacketData();
		res2->wOpCode = GU_TENKAICHIDAISIJYOU_SELL_RES;
		res2->wResultCode = GAME_FAIL;
		packet2.SetPacketLen(sizeof(sGU_TENKAICHIDAISIJYOU_BUY_RES));
		app->Send(GetHandle(), &packet2);
	}
}

void CClientSession::RecvPetAttackToggleNfy(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PET_ATTACK_TOGGLE_NFY * req = (sUG_PET_ATTACK_TOGGLE_NFY*)pPacket->GetPacketData();
	//printf("%u %u %u\n", req->unknown, req->unknown2, req->unknown3);
	if (CSummonPet* pPet = cPlayer->GetSummonPet())
	{
		if (pPet->GetToggleAttack() == false)
			pPet->SetToggleAttack(true);
		else
			pPet->SetToggleAttack(false);
	}
}

void CClientSession::RecvPetSkillToggleNfy(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_PET_SKILL_TOGGLE_NFY * req = (sUG_PET_SKILL_TOGGLE_NFY*)pPacket->GetPacketData();
	//printf("%u %u %i\n", req->byAvatarType, req->skillTblidx, req->bActivate);

	if (req->byAvatarType == DBO_AVATAR_TYPE_SUMMON_PET_1)
	{
		if (CSummonPet* pPet = cPlayer->GetSummonPet())
		{
			pPet->GetSkillManager()->SetCanUseSkill(req->skillTblidx, req->bActivate);
			pPet->GetSkillManager()->SetCurSkillTblidx(req->skillTblidx);

			/*if (GetBot()->HasNearbyPlayer(false))
			{
				if (!GetBot()->GetStateManager()->IsCharCondition(CHARCOND_CONFUSED))
				{
					CBotAiState* pCurState = GetBot()->GetBotController()->GetCurrentState();
					if (pCurState)
					{
						CComplexState* pCurAction = pCurState->GetCurrentAction();
						if (pCurAction && pCurAction->GetControlStateID() == BOTCONTROL_ACTION_LOOK)
							return m_status;
						else
						{
							CSkillManagerBot* pBotSkillManager = (CSkillManagerBot*)GetBot()->GetSkillManager();
							if (!pBotSkillManager->IsSkillUseLock())
							{
								CSkillCondition* pSkillCondition = pBotSkillManager->GetSkill(req->skillTblidx);

								if (pSkillCondition)
								{
									CBotAiAction_SkillUse* pSkillUse = new CBotAiAction_SkillUse(GetBot(), pSkillCondition->GetSkillConditionIdx());
									if (!pCurState->AddSubControlQueue(pSkillUse, true))
									{
										m_status = FAILED;
									}
								}
							}
						}
					}
				}
			}*/
		}
	}
}

//--------------------------------------------------------------------------------------//
//		START GIFT SHOP (BROWN WP BOX AT CLIENT INTERFACE)
//--------------------------------------------------------------------------------------//
void CClientSession::RecvGiftShopStartReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	//Load items
	{
		CNtlPacket packet(sizeof(sGU_GIFT_SHOP_TAB_INFO_NFY));
		sGU_GIFT_SHOP_TAB_INFO_NFY* res = (sGU_GIFT_SHOP_TAB_INFO_NFY*)packet.GetPacketData();
		/*
		BYTE			byTabIndex;
		BYTE			byItemCount;
		WCHAR			wszTabName[NTL_MAX_SIZE_TAB_NAME_IN_UNICODE + 1];
		sSELL_ITEM_INFO	aSellItemInfo[NTL_MAX_SHOPPING_CART];
		*/



		/*
		CMerchantTable* pMerchantItemTable = g_pTableContainer->GetMerchantTable();
		sMERCHANT_TBLDAT* pMerchantData = (sMERCHANT_TBLDAT*)pMerchantItemTable->FindData(1);
		res->wOpCode = GU_GIFT_SHOP_TAB_INFO_NFY;
		res->byTabIndex = 0;
		NTL_WCSCPY_S(res->wszTabName, NTL_MAX_SIZE_TAB_NAME_IN_UNICODE + 1, L"Items");
		res->byItemCount = 0;

		for (int i = 0; i < NTL_MAX_MERCHANT_COUNT + 1; i++)
		{
			if (pMerchantData->aitem_Tblidx[i] != INVALID_TBLIDX)
			{
				res->byItemCount += 1;
				res->aSellItemInfo[i].idxItemTbl = pMerchantData->aitem_Tblidx[i];
				res->aSellItemInfo[i].dwPrice = pMerchantData->adwNeedZenny[i];
			}
		}

		packet.SetPacketLen(sizeof(sGU_GIFT_SHOP_TAB_INFO_NFY));
		g_pApp->Send(GetHandle(), &packet);
		*/
		

		//kuini additional tabs for wagu shop
		/*
		res->wOpCode = GU_GIFT_SHOP_TAB_INFO_NFY;
		res->byTabIndex = 1;
		NTL_WCSCPY_S(res->wszTabName, NTL_MAX_SIZE_TAB_NAME_IN_UNICODE + 1, L"Items 2");
		res->byItemCount = 1;
		res->aSellItemInfo[0].idxItemTbl = 11120090;
		res->aSellItemInfo[0].dwPrice = 50;
		packet.SetPacketLen(sizeof(sGU_GIFT_SHOP_TAB_INFO_NFY));
		g_pApp->Send(GetHandle(), &packet);
		*/
		
	}

	CNtlPacket packetEnd(sizeof(sGU_GIFT_SHOP_START_RES));
	sGU_GIFT_SHOP_START_RES* resEnd = (sGU_GIFT_SHOP_START_RES*)packetEnd.GetPacketData();
	resEnd->wOpCode = GU_GIFT_SHOP_START_RES;
	resEnd->wResultCode = GAME_SUCCESS;
	resEnd->dwGiftPoint = cPlayer->GetWaguPoints();
	resEnd->nShopVersion = 169;
	packetEnd.SetPacketLen(sizeof(sGU_GIFT_SHOP_START_RES));
	g_pApp->Send(GetHandle(), &packetEnd);
}

//--------------------------------------------------------------------------------------//
//		BUY GIFT SHOP ITEM
//--------------------------------------------------------------------------------------//
void CClientSession::RecvGiftShopBuyReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_GIFT_SHOP_BUY_REQ* req = (sUG_GIFT_SHOP_BUY_REQ*)pPacket->GetPacketData();

	CMerchantTable* pMerchantItemTable = g_pTableContainer->GetMerchantTable();
	CItemTable* itemTbl = g_pTableContainer->GetItemTable();

	WORD buy_item_result = GAME_SUCCESS;
	DWORD price = 0;
	
	if (cPlayer->GetPlayerItemContainer()->CountEmptyInventory() < req->byBuyCount)
		buy_item_result = GAME_ITEM_INVEN_FULL;
	else if (req->byBuyCount == 0 || req->byBuyCount > NTL_MAX_BUY_SHOPPING_CART)
		buy_item_result = GAME_FAIL;
	else
	{
		
		for (int i = 0; i < req->byBuyCount; i++) 
		{
			sMERCHANT_TBLDAT* pMerchantData = (sMERCHANT_TBLDAT*)pMerchantItemTable->FindData(1 + req->sBuyData[i].byMerchantTab);
			if (pMerchantData == NULL)
			{
				buy_item_result = GAME_FAIL;
				break;
			}

			if (req->sBuyData[i].byItemPos >= NTL_MAX_MERCHANT_COUNT)
			{
				buy_item_result = GAME_FAIL;
				break;
			}
			
			sITEM_TBLDAT* pItemData = (sITEM_TBLDAT*)itemTbl->FindData(pMerchantData->aitem_Tblidx[req->sBuyData[i].byItemPos]);
			if (pItemData)
			{
				if (req->sBuyData[i].byStack > 0)
				{
					price += pMerchantData->adwNeedZenny[req->sBuyData[i].byItemPos] * req->sBuyData[i].byStack;
				}
				else
				{
					buy_item_result = GAME_ITEM_STACK_FAIL;
					break;
				}
			}
			else
			{
				buy_item_result = GAME_FAIL;
				break;
			}
		}

		if (buy_item_result == GAME_SUCCESS)
		{
			if (cPlayer->GetWaguPoints() >= price)
			{
				cPlayer->UpdateWaguPoints(cPlayer->GetWaguPoints() - price);

				CGameServer* app = (CGameServer*)g_pApp;

				CNtlPacket pQry(sizeof(sGQ_GIFT_SHOP_BUY_REQ));
				sGQ_GIFT_SHOP_BUY_REQ* rQry = (sGQ_GIFT_SHOP_BUY_REQ*)pQry.GetPacketData();
				rQry->wOpCode = GQ_GIFT_SHOP_BUY_REQ;
				rQry->handle = cPlayer->GetID();
				rQry->charId = cPlayer->GetCharID();
				rQry->dwGiftPoint = cPlayer->GetWaguPoints();

				for (int i = 0; i < req->byBuyCount; i++)
				{
					sMERCHANT_TBLDAT* pMerchantData = (sMERCHANT_TBLDAT*)pMerchantItemTable->FindData(1 + req->sBuyData[i].byMerchantTab);
					if (pMerchantData)
					{
						sITEM_TBLDAT* pItemTbldat = (sITEM_TBLDAT*)itemTbl->FindData(pMerchantData->aitem_Tblidx[req->sBuyData[i].byItemPos]);
						if (pItemTbldat)
						{
							std::pair<BYTE, BYTE> pairInv = cPlayer->GetPlayerItemContainer()->GetEmptyInventory();

							cPlayer->GetPlayerItemContainer()->AddReservedInventory(pairInv.first, pairInv.second);

							::ZeroMemory(rQry->sInven[rQry->byBuyCount].awchMaker, sizeof(rQry->sInven[rQry->byBuyCount].awchMaker));
							rQry->sInven[rQry->byBuyCount].byCurrentDurability = pItemTbldat->byDurability;
							rQry->sInven[rQry->byBuyCount].byDurationType = pItemTbldat->byDurationType;
							if (rQry->sInven[rQry->byBuyCount].byDurationType == eDURATIONTYPE_FLATSUM)
							{
								rQry->sInven[rQry->byBuyCount].nUseStartTime = app->GetTime();
								rQry->sInven[rQry->byBuyCount].nUseEndTime = rQry->sInven[rQry->byBuyCount].nUseStartTime + pItemTbldat->dwUseDurationMax;
							}
							rQry->sInven[rQry->byBuyCount].byGrade = ITEM_GRADE_LEVEL_0;
							rQry->sInven[rQry->byBuyCount].byPlace = pairInv.first;
							rQry->sInven[rQry->byBuyCount].byPos = pairInv.second;
							rQry->sInven[rQry->byBuyCount].byRank = pItemTbldat->byRank;
							rQry->sInven[rQry->byBuyCount].byRestrictState = ITEM_RESTRICT_STATE_TYPE_LIMIT;
							rQry->sInven[rQry->byBuyCount].byStack = req->sBuyData[i].byStack;
							rQry->sInven[rQry->byBuyCount].sOptionSet.Init();
							rQry->sInven[rQry->byBuyCount].tblItem = pItemTbldat->tblidx;
							rQry->sInven[rQry->byBuyCount].sOptionSet.aOptionTblidx[0] = pItemTbldat->Item_Option_Tblidx;

							rQry->byBuyCount++;
						}
					}
				}

				if (rQry->byBuyCount > 0)
				{
					pQry.SetPacketLen(sizeof(sGQ_GIFT_SHOP_BUY_REQ));
					app->SendTo(app->GetQueryServerSession(), &pQry);

					return;
				}
				else buy_item_result = GAME_FAIL;
			}
			else buy_item_result = GAME_NETPY_NOT_ENOUGH;
		}
	}

	CNtlPacket packet(sizeof(sGU_GIFT_SHOP_BUY_RES));
	sGU_GIFT_SHOP_BUY_RES* res = (sGU_GIFT_SHOP_BUY_RES*)packet.GetPacketData();
	res->wOpCode = GU_GIFT_SHOP_BUY_RES;
	res->dwGiftPoint = cPlayer->GetWaguPoints();
	res->wResultCode = buy_item_result;
	packet.SetPacketLen(sizeof(sGU_GIFT_SHOP_BUY_RES));
	g_pApp->Send(GetHandle(), &packet);
}
/*
void CClientSession::RecvNetPyBuyReq(CNtlPacket* pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_SHOP_NETPYITEM_BUY_REQ* req = (sUG_SHOP_NETPYITEM_BUY_REQ*)pPacket->GetPacketData();

	CMerchantTable* pMerchantItemTable = g_pTableContainer->GetMerchantTable();
	CItemTable* itemTbl = g_pTableContainer->GetItemTable();

	WORD buy_item_result = GAME_SUCCESS;
	DWORD price = 0;

	//check inventory space
	if (cPlayer->GetPlayerItemContainer()->CountEmptyInventory() < req->byBuyCount)
		buy_item_result = GAME_ITEM_INVEN_FULL;
	else if (req->byBuyCount == 0 || req->byBuyCount > NTL_MAX_BUY_SHOPPING_CART)
		buy_item_result = GAME_FAIL;

	else
	{
		//check if enough token points
		for (int i = 0; i < 1; i++) // only can buy 1 item
		{
			sMERCHANT_TBLDAT* pMerchantData = (sMERCHANT_TBLDAT*)pMerchantItemTable->FindData(1001 + req->sBuyData[i].byMerchantTab);
			if (pMerchantData == NULL)
			{
				buy_item_result = GAME_FAIL;
				break;
			}

			if (req->sBuyData[i].byItemPos >= NTL_MAX_MERCHANT_COUNT)
			{
				buy_item_result = GAME_FAIL;
				break;
			}

			if (pMerchantData->bySell_Type != MERCHANT_SELL_TYPE_NETPY)
			{
				buy_item_result = GAME_FAIL;
				break;
			}

			sITEM_TBLDAT* pItemData = (sITEM_TBLDAT*)itemTbl->FindData(pMerchantData->aitem_Tblidx[req->sBuyData[i].byItemPos]);
			if (pItemData)
			{
				if (req->sBuyData[i].byStack > 0)
				{
					price += pItemData->CommonPoint * req->sBuyData[i].byStack;
				}
				else
				{
					buy_item_result = GAME_ITEM_STACK_FAIL;
					break;
				}
			}
			else
			{
				buy_item_result = GAME_FAIL;
				break;
			}
		}

		if (buy_item_result == GAME_SUCCESS)
		{
			if (cPlayer->GetNetpyTokens() >= price)
			{
				cPlayer->UpdateNetpyToken(false, price, false);

				CGameServer* app = (CGameServer*)g_pApp;

				CNtlPacket pQry(sizeof(sGQ_SHOP_NETPYITEM_BUY_REQ));
				sGQ_SHOP_NETPYITEM_BUY_REQ* rQry = (sGQ_SHOP_NETPYITEM_BUY_REQ*)pQry.GetPacketData();
				rQry->wOpCode = GQ_SHOP_NETPYITEM_BUY_REQ;
				rQry->handle = cPlayer->GetID();
				rQry->charId = cPlayer->GetCharID();
				rQry->netpyPoint = price;

				for (int i = 0; i < 1; i++)
				{
					sMERCHANT_TBLDAT* pMerchantData = (sMERCHANT_TBLDAT*)pMerchantItemTable->FindData(1001 + req->sBuyData[i].byMerchantTab);
					if (pMerchantData)
					{
						if (pMerchantData->bySell_Type == MERCHANT_SELL_TYPE_NETPY)
						{
							sITEM_TBLDAT* pItemTbldat = (sITEM_TBLDAT*)itemTbl->FindData(pMerchantData->aitem_Tblidx[req->sBuyData[i].byItemPos]);
							if (pItemTbldat)
							{
								std::pair<BYTE, BYTE> pairInv = cPlayer->GetPlayerItemContainer()->GetEmptyInventory();

								cPlayer->GetPlayerItemContainer()->AddReservedInventory(pairInv.first, pairInv.second);//mark that place and pos is reserved

								::ZeroMemory(rQry->sInven[rQry->byBuyCount].awchMaker, sizeof(rQry->sInven[rQry->byBuyCount].awchMaker));
								rQry->sInven[rQry->byBuyCount].byCurrentDurability = pItemTbldat->byDurability;
								rQry->sInven[rQry->byBuyCount].byDurationType = pItemTbldat->byDurationType;
								if (rQry->sInven[rQry->byBuyCount].byDurationType == eDURATIONTYPE_FLATSUM)
								{
									rQry->sInven[rQry->byBuyCount].nUseStartTime = app->GetTime();
									rQry->sInven[rQry->byBuyCount].nUseEndTime = rQry->sInven[rQry->byBuyCount].nUseStartTime + pItemTbldat->dwUseDurationMax;
								}
								rQry->sInven[rQry->byBuyCount].byGrade = ITEM_GRADE_LEVEL_0;
								rQry->sInven[rQry->byBuyCount].byPlace = pairInv.first;
								rQry->sInven[rQry->byBuyCount].byPos = pairInv.second;
								rQry->sInven[rQry->byBuyCount].byRank = pItemTbldat->byRank;
								rQry->sInven[rQry->byBuyCount].byRestrictState = ITEM_RESTRICT_STATE_TYPE_LIMIT;
								rQry->sInven[rQry->byBuyCount].byStack = req->sBuyData[i].byStack;
								rQry->sInven[rQry->byBuyCount].sOptionSet.Init();
								rQry->sInven[rQry->byBuyCount].tblItem = pItemTbldat->tblidx;
								rQry->sInven[rQry->byBuyCount].sOptionSet.aOptionTblidx[0] = pItemTbldat->Item_Option_Tblidx;

								rQry->byBuyCount++;
							}
						}
					}
				}

				if (rQry->byBuyCount > 0)
				{
					pQry.SetPacketLen(sizeof(sGQ_SHOP_NETPYITEM_BUY_REQ));
					app->SendTo(app->GetQueryServerSession(), &pQry);

					return;
				}
				else buy_item_result = GAME_FAIL;
			}
			else buy_item_result = GAME_NETPY_NOT_ENOUGH;
		}
	}

	CNtlPacket packet(sizeof(sGU_SHOP_NETPYITEM_BUY_RES));
	sGU_SHOP_NETPYITEM_BUY_RES* res = (sGU_SHOP_NETPYITEM_BUY_RES*)packet.GetPacketData();
	res->wOpCode = GU_SHOP_NETPYITEM_BUY_RES;
	res->wResultCode = buy_item_result;
	packet.SetPacketLen(sizeof(sGU_SHOP_NETPYITEM_BUY_RES));
	g_pApp->Send(GetHandle(), &packet);
}
*/

//--------------------------------------------------------------------------------------//
//		SKILL INIT (RESET SKILLS) Reset skills by NPC. Cost Money
//--------------------------------------------------------------------------------------//
void CClientSession::RecvSkillInitReq(CNtlPacket * pPacket)
{
	if (!cPlayer->IsGameMaster()) return; // by kuini disabled skill reset
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_SKILL_INIT_REQ * req = (sUG_SKILL_INIT_REQ*)pPacket->GetPacketData();
			
	WORD result = GAME_FAIL;
	DWORD reqzeni = Dbo_GetRequiredZennyForNpcSkillInit(cPlayer->GetLevel());

	if (cPlayer->GetAspectStateId() == ASPECTSTATE_INVALID)
	{
		CNpc* pNpc = g_pObjectManager->GetNpc(req->hNpc);
		if (pNpc && pNpc->IsInitialized())
		{
			if (pNpc->IsInRange(cPlayer, DBO_DISTANCE_CHECK_TOLERANCE) == false)
				result = GAME_TARGET_TOO_FAR;
			else if (pNpc->GetTbldat()->byJob != NPC_JOB_SKILL_TRAINER_HFI && pNpc->GetTbldat()->byJob != NPC_JOB_SKILL_TRAINER_HMY && pNpc->GetTbldat()->byJob != NPC_JOB_SKILL_TRAINER_HEN && pNpc->GetTbldat()->byJob != NPC_JOB_SKILL_TRAINER_NFI
				&& pNpc->GetTbldat()->byJob != NPC_JOB_SKILL_TRAINER_NMY && pNpc->GetTbldat()->byJob != NPC_JOB_SKILL_TRAINER_MMI && pNpc->GetTbldat()->byJob != NPC_JOB_SKILL_TRAINER_MWO
				&& pNpc->GetTbldat()->byJob != NPC_JOB_GRAND_SKILL_TRAINER_HFI && pNpc->GetTbldat()->byJob != NPC_JOB_GRAND_SKILL_TRAINER_HMY && pNpc->GetTbldat()->byJob != NPC_JOB_GRAND_SKILL_TRAINER_HEN && pNpc->GetTbldat()->byJob != NPC_JOB_GRAND_SKILL_TRAINER_NFI
				&& pNpc->GetTbldat()->byJob != NPC_JOB_GRAND_SKILL_TRAINER_NMY && pNpc->GetTbldat()->byJob != NPC_JOB_GRAND_SKILL_TRAINER_MMI && pNpc->GetTbldat()->byJob != NPC_JOB_GRAND_SKILL_TRAINER_MWO)
				result = GAME_TARGET_HAS_DIFFERENT_JOB;
			else
			{
				if (UnsignedSafeDecrease<BYTE>((cPlayer->GetLevel() - 1) + cPlayer->GetSkillPointsBought(), (BYTE)cPlayer->GetSkillPoints()) > 0)
				{
					//	if (cPlayer->GetLevel() <= 100)
					//	{
					//		if (cPlayer->GetClass() <= PC_CLASS_WONDER_MAJIN) //only first class can reset skills
					//		{
					if (cPlayer->GetZeni() >= reqzeni)
					{
						result = GAME_SUCCESS;

						CNtlPacket pQry(sizeof(sGQ_SKILL_INIT_REQ));
						sGQ_SKILL_INIT_REQ* rQry = (sGQ_SKILL_INIT_REQ*)pQry.GetPacketData();
						rQry->wOpCode = GQ_SKILL_INIT_REQ;
						rQry->handle = cPlayer->GetID();
						rQry->charId = cPlayer->GetCharID();
						rQry->dwSP = ((DWORD)cPlayer->GetLevel() - 1) + cPlayer->GetSkillPointsBought();
						rQry->dwZenny = reqzeni;
						rQry->bySkillResetMethod = 0;
						pQry.SetPacketLen(sizeof(sGQ_SKILL_INIT_REQ));
						app->SendTo(app->GetQueryServerSession(), &pQry);

						return;
					}
					else result = GAME_ZENNY_NOT_ENOUGH;
					//		}
					//		else result = GAME_SKILL_TOO_HIGH_LEVEL_FOR_SKILL_INIT;
					//	}
					//	else result = GAME_SKILL_TOO_HIGH_LEVEL_FOR_SKILL_INIT;
				}
				else result = GAME_SKILL_NO_SKILL_TO_INIT;
			}
		}
	}
	else result = GAME_SKILL_CANT_USE_WHEN_TRANSFORMED;

	CNtlPacket packet(sizeof(sGU_SKILL_INIT_RES));
	sGU_SKILL_INIT_RES* res = (sGU_SKILL_INIT_RES*)packet.GetPacketData();
	res->wOpCode = GU_SKILL_INIT_RES;
	res->wResultCode = result;
	packet.SetPacketLen(sizeof(sGU_SKILL_INIT_RES));
	app->Send(GetHandle(), &packet);
}

//--------------------------------------------------------------------------------------//
//		SKILL INIT (RESET SKILLS) Reset all skill by Item.
//--------------------------------------------------------------------------------------//
void CClientSession::RecvSkillResetPlusReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_SKILL_RESET_PLUS_REQ * req = (sUG_SKILL_RESET_PLUS_REQ*)pPacket->GetPacketData();
	
	WORD result = GAME_SUCCESS;

	if (cPlayer->GetAspectStateId() == ASPECTSTATE_INVALID)
	{
		CItem* item = cPlayer->GetPlayerItemContainer()->GetItem(req->byPlace, req->byPos);
		if (item && item->GetCount() > 0 && item->GetTbldat()->byItem_Type == ITEM_TYPE_SKILL_POINT_RESET_PLUS 
			&& item->IsLocked() == false && IsInvenContainer(item->GetPlace()))
		{
			if (cPlayer->GetSkillManager()->HasSkillsInCoolDown() == false)
			{
				if (UnsignedSafeDecrease<BYTE>((cPlayer->GetLevel() - 1) + cPlayer->GetSkillPointsBought(), (BYTE)cPlayer->GetSkillPoints()) > 0)
				{
					item->SetLocked(true);

					CNtlPacket pQry(sizeof(sGQ_SKILL_INIT_REQ));
					sGQ_SKILL_INIT_REQ* rQry = (sGQ_SKILL_INIT_REQ*)pQry.GetPacketData();
					rQry->wOpCode = GQ_SKILL_INIT_REQ;
					rQry->handle = cPlayer->GetID();
					rQry->charId = cPlayer->GetCharID();
					rQry->dwSP = ((DWORD)cPlayer->GetLevel() - 1) + cPlayer->GetSkillPointsBought();
					rQry->dwZenny = 0;
					rQry->bySkillResetMethod = 1;
					rQry->itemId = item->GetItemID();
					pQry.SetPacketLen(sizeof(sGQ_SKILL_INIT_REQ));
					app->SendTo(app->GetQueryServerSession(), &pQry);
				}
				else result = GAME_SKILL_NO_SKILL_TO_INIT;
			}
			else result = GAME_SKILL_NOT_READY_TO_BE_CAST;

		}
		else result = GAME_NEEDITEM_NOT_FOUND_INVANTORY;
	}
	else result = GAME_SKILL_CANT_USE_WHEN_TRANSFORMED;

	CNtlPacket packet(sizeof(sGU_SKILL_RESET_PLUS_RES));
	sGU_SKILL_RESET_PLUS_RES* res = (sGU_SKILL_RESET_PLUS_RES*)packet.GetPacketData();
	res->wOpCode = GU_SKILL_RESET_PLUS_RES;
	res->wResultCode = result;
	packet.SetPacketLen(sizeof(sGU_SKILL_RESET_PLUS_RES));
	app->Send(GetHandle(), &packet);
}

//--------------------------------------------------------------------------------------//
//		TELEPORT CONFIRMATION
//--------------------------------------------------------------------------------------//
void CClientSession::RecvTeleportConfirmationReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;
	sUG_TELEPORT_CONFIRM_REQ * req = (sUG_TELEPORT_CONFIRM_REQ*)pPacket->GetPacketData();

	WORD wResultcode = GAME_FAIL;
	CPlayer* pRequestor = NULL;

	// Check if this is a Battle Royale teleport proposal
	bool bIsBattleRoyale = false;
	if (g_pBattleRoyaleEvent && g_pBattleRoyaleEvent->IsEventActive() && cPlayer->HasEventType(EVENT_TELEPORT_PROPOSAL))
	{
		// Check if player is in the proposal sent list
		// For now, assume any teleport proposal during active BR event is BR-related
		// This is safe because BR event sends proposals to all players
		bIsBattleRoyale = true;
	}

	if (req->bTeleport)
	{
		if (!cPlayer->HasEventType(EVENT_TELEPORT_PROPOSAL)) //only allow players to teleport if they have the event
			wResultcode = GAME_CAN_NOT_TELEPORT;

		else if (cPlayer->GetDragonballScrambleBallFlag() > 0)
			wResultcode = GAME_CAN_NOT_TELEPORT;

		else if(cPlayer->IsStunned() || cPlayer->IsTrading() || cPlayer->IsPrivateShop() || cPlayer->IsSleeping() || cPlayer->IsSkillAffecting() 
			|| cPlayer->IsKeepingEffect() || cPlayer->IsSliding() || cPlayer->IsKnockedDown() || cPlayer->IsUsingHtb() || cPlayer->IsSandbag())
			wResultcode = GAME_CAN_NOT_TELEPORT_THIS_STATE;

		else if (req->byTeleportIndex == TELEPORT_TYPE_RANKBATTLE && g_pRankbattleManager->DoesBattleStillExist(cPlayer->GetRankBattleRoomTblidx(), cPlayer->GetRankBattleRoomId()) == false)
		{
			wResultcode = GAME_RANKBATTLE_CANNOT_FIND_OPPONENT;
			cPlayer->LeaveRankBattleQueue();
		}

		else if (cPlayer->GetCurWorld() && (cPlayer->GetCurWorld()->GetRuleType() != GAMERULE_NORMAL && cPlayer->GetCurWorld()->GetRuleType() != GAMERULE_TEINKAICHIBUDOKAI))
		{
			// Allow teleport if it's a battle royale proposal
			if (!bIsBattleRoyale)
				wResultcode = GAME_CAN_NOT_TELEPORT;
		}

		else if (cPlayer->GetTeleportProposalRequestor() != INVALID_HOBJECT) //check if teleport proposal comes from a player
		{
			pRequestor = g_pObjectManager->GetPC(cPlayer->GetTeleportProposalRequestor());
			if (pRequestor && pRequestor->IsInitialized() && pRequestor->IsPC())
			{
				if (pRequestor->GetCurWorld() && pRequestor->GetCurWorld()->GetTbldat()->bDynamic == false) // ONLY ALLOW TO TELEPORT TO TARGETS WHICH ARE NOT IN DYNAMIC MAPS
				{
					wResultcode = GAME_SUCCESS;
					cPlayer->SetTeleportType(TELEPORT_TYPE_PARTY_POPO);
				}
				else wResultcode = GAME_CAN_NOT_TELEPORT;
			}
			else wResultcode = GAME_TARGET_NOT_FOUND;
		}
		else //check if proposal comes from system
		{
			if (cPlayer->GetTeleportProposalType() == TELEPORT_TYPE_DOJO)
			{
				CDojo* pDojo = g_pDojoWarManager->GetDojo(cPlayer->GetGuildID());
				if (pDojo)
				{
					if(pDojo->CanEnterDojo(cPlayer->GetGuildID()))
						wResultcode = GAME_SUCCESS;
					else
						wResultcode = GAME_GUILD_DOJO_SCRAMBLE_MAX_COUNT_OVER;
				}
				else if(CDojo* pDojo = g_pDojoManager->GetDojo(cPlayer->GetGuildID()))
				{
					wResultcode = GAME_SUCCESS;
				}
				else wResultcode = GAME_TARGET_NOT_FOUND;
			}
			else if (cPlayer->GetTeleportProposalType() == TELEPORT_TYPE_MINORMATCH)
			{
				if(g_pBudokaiManager->CanTeleportPrelim(cPlayer) == false)
					wResultcode = GAME_CAN_NOT_TELEPORT;
				else
					wResultcode = GAME_SUCCESS;
			}
			else if (cPlayer->GetTeleportProposalType() == TELEPORT_TYPE_MAJORMATCH)
			{
				if (g_pBudokaiManager->CanTeleportMajorMatch(cPlayer) == false)
					wResultcode = GAME_CAN_NOT_TELEPORT;
				else
					wResultcode = GAME_SUCCESS;
			}
			else if (cPlayer->GetTeleportProposalWorldID() == INVALID_WORLDID)
			{
				// Allow INVALID_WORLDID for Battle Royale (world is created dynamically)
				if (bIsBattleRoyale)
					wResultcode = GAME_SUCCESS;
				else
					wResultcode = GAME_CAN_NOT_TELEPORT;
			}
			else
			{
				wResultcode = GAME_SUCCESS;
			}
		}
	}
	else
	{
		wResultcode = GAME_SUCCESS;
	}

	CNtlPacket packet(sizeof(sGU_TELEPORT_CONFIRM_RES));
	sGU_TELEPORT_CONFIRM_RES* res = (sGU_TELEPORT_CONFIRM_RES*)packet.GetPacketData();
	res->wOpCode = GU_TELEPORT_CONFIRM_RES;
	res->wResultCode = wResultcode; //use GAME_SUCCESS directly when bTeleport is false
	res->bClearInterface = true;
	res->bTeleport = req->bTeleport;
	res->byTeleportIndex = req->byTeleportIndex;
	packet.SetPacketLen(sizeof(sGU_TELEPORT_CONFIRM_RES));
	g_pApp->Send(GetHandle(), &packet);

	g_pEventMgr->RemoveEvents(cPlayer, EVENT_TELEPORT_PROPOSAL);

	if (wResultcode == GAME_SUCCESS)
	{
		if (req->bTeleport) //check if agree to teleport
		{
			if (bIsBattleRoyale && g_pBattleRoyaleEvent)
			{
				ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent: Player %u accepted teleport proposal", cPlayer->GetCharID());
				g_pBattleRoyaleEvent->OnPlayerAcceptedProposal(cPlayer);
				// Get the instance ID that was assigned to this player
				WORLDID instanceID = g_pBattleRoyaleEvent->GetPlayerInstanceID(cPlayer);
				ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent: Player %u assigned to instance %u", cPlayer->GetCharID(), instanceID);
				
				if (instanceID != INVALID_WORLDID)
				{
					if (!g_pTableContainer)
					{
						ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent: TableContainer is null!");
						return;
					}

					sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData(212000);
					if (pWorldTbldat)
					{
						if (!app->GetGameMain() || !app->GetGameMain()->GetWorldManager())
						{
							ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent: WorldManager is null!");
							return;
						}

						CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld(instanceID);
						if (pWorld)
						{
							ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent: Found world instance %u, teleporting player %u", instanceID, cPlayer->GetCharID());
							cPlayer->StartTeleport(pWorldTbldat->vStart1Loc, pWorldTbldat->vStart1Dir, instanceID, cPlayer->GetTeleportProposalType(), INVALID_TBLIDX, false, cPlayer->GetTeleportAnotherServer());
						}
						else
						{
							ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent: World instance %u not found for player %u. Attempting to create...", instanceID, cPlayer->GetCharID());
							// Try to create the world if it doesn't exist
							CWorld* pNewWorld = app->GetGameMain()->GetWorldManager()->CreateWorld(pWorldTbldat);
							if (pNewWorld)
							{
								WORLDID newWorldID = pNewWorld->GetID();
								ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent: Created new world instance %u (expected %u), teleporting player %u", newWorldID, instanceID, cPlayer->GetCharID());
								// Update the player's instance assignment to the newly created world
								g_pBattleRoyaleEvent->OnPlayerAcceptedProposal(cPlayer); // Reassign to new instance
								WORLDID finalInstanceID = g_pBattleRoyaleEvent->GetPlayerInstanceID(cPlayer);
								if (finalInstanceID != INVALID_WORLDID)
								{
									cPlayer->StartTeleport(pWorldTbldat->vStart1Loc, pWorldTbldat->vStart1Dir, finalInstanceID, cPlayer->GetTeleportProposalType(), INVALID_TBLIDX, false, cPlayer->GetTeleportAnotherServer());
								}
								else
								{
									ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent: Failed to reassign player %u to new instance", cPlayer->GetCharID());
								}
							}
							else
							{
								ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent: Failed to create world instance for player %u", cPlayer->GetCharID());
							}
						}
					}
					else
					{
						ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent: World table data not found for world 212000");
					}
				}
				else
				{
					ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent: Failed to get instance ID for player %u", cPlayer->GetCharID());
				}
			}
			else
			{
				cPlayer->StartTeleport(cPlayer->GetTeleportProposalLoc(), cPlayer->GetTeleportProposalDir(), cPlayer->GetTeleportProposalWorldID(), cPlayer->GetTeleportProposalType(), INVALID_TBLIDX, false, cPlayer->GetTeleportAnotherServer());
			}
		}
		else
		{
			if (bIsBattleRoyale && g_pBattleRoyaleEvent)
			{
				g_pBattleRoyaleEvent->OnPlayerDeclinedProposal(cPlayer);
			}
			else if (req->byTeleportIndex == TELEPORT_TYPE_PARTY_POPO && pRequestor) //send to requestor that player declined the teleport request
			{
				CNtlPacket packetDecline(sizeof(sGU_CHAR_PARTY_POPO_REJECT_NFY));
				sGU_CHAR_PARTY_POPO_REJECT_NFY * res2 = (sGU_CHAR_PARTY_POPO_REJECT_NFY *)packetDecline.GetPacketData();
				res2->wOpCode = GU_CHAR_PARTY_POPO_REJECT_NFY;
				res2->handle = cPlayer->GetID();
				packetDecline.SetPacketLen(sizeof(sGU_CHAR_PARTY_POPO_REJECT_NFY));
				pRequestor->SendPacket(&packetDecline);
			}
			else if (req->byTeleportIndex == TELEPORT_TYPE_RANKBATTLE)
			{
				g_pRankbattleManager->CancelTeleport(cPlayer);
			}
		}
	}
	else
	{
		//reset teleport data
		cPlayer->ResetDirectPlay();
		//reset teleport shit
		cPlayer->event_TeleportProposal(); 
	}
}

//--------------------------------------------------------------------------------------//
//		RESET SINGLE SKILL ( decrease level by 1 )
//--------------------------------------------------------------------------------------//
void CClientSession::RecvResetOneSkillReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_SKILL_ONE_RESET_REQ * req = (sUG_SKILL_ONE_RESET_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_SKILL_ONE_RESET_RES));
	sGU_SKILL_ONE_RESET_RES* res = (sGU_SKILL_ONE_RESET_RES*)packet.GetPacketData();
	res->wOpCode = GU_SKILL_ONE_RESET_RES;
	res->skillIndex = req->skillIndex;
	res->wResultCode = GAME_FAIL;

	if (cPlayer->GetAspectStateId() == ASPECTSTATE_INVALID)
	{
		CItem* item = cPlayer->GetPlayerItemContainer()->GetItem(req->byPlace, req->byPos);
		if (item && item->GetCount() > 0 && item->GetTbldat()->byItem_Type == ITEM_TYPE_SKILL_ONE_POINT_RESET && IsInvenContainer(item->GetPlace()))
		{
			if (item->IsLocked() == false)
			{
				CSkillPc* pSkill = (CSkillPc*)cPlayer->GetSkillManager()->GetSkillWithSkillIndex(req->skillIndex);
				if (pSkill)
				{
					sSKILL_TBLDAT* pSkillTbldat = pSkill->GetOriginalTableData();

					if (!cPlayer->GetSkillManager()->CanResetSkill(pSkill)) //check if has any skills which require this skill
					{
						res->wResultCode = GAME_SKILL_ONE_RESET_FAIL;
					}
					else
					{
						res->wResultCode = GAME_SUCCESS;
						res->resetSkillTblidx = pSkillTbldat->tblidx;

						CNtlPacket pQry(sizeof(sGQ_SKILL_ONE_RESET_REQ));
						sGQ_SKILL_ONE_RESET_REQ* rQry = (sGQ_SKILL_ONE_RESET_REQ*)pQry.GetPacketData();
						rQry->wOpCode = GQ_SKILL_ONE_RESET_REQ;
						rQry->charId = cPlayer->GetCharID();
						rQry->skillIndex = pSkill->GetSkillIndex();
						rQry->dwAddSP = 1;
						rQry->itemId = item->GetItemID();

						sSKILL_TBLDAT* pNewSkillTbldat = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData(pSkillTbldat->tblidx - 1);
						if (pNewSkillTbldat)
						{
							res->preSkillTblidx = pNewSkillTbldat->tblidx;
							rQry->newSkillTblidx = pNewSkillTbldat->tblidx;

							pSkill->ChangeOriginalTableData(pNewSkillTbldat);

							bool bCanHaveRp = false;
							for (int i = 0; i < DBO_MAX_RP_BONUS_COUNT_PER_SKILL; i++)
							{
								if (pSkill->GetOriginalTableData()->abyRpEffect[i] == pSkill->GetRpBonusType())
								{
									bCanHaveRp = true;
									break;
								}
							}

							if (bCanHaveRp == false)
							{
								rQry->byRpBonusType = DBO_RP_BONUS_TYPE_INVALID;
								rQry->bRpBonusAuto = false;

								WORD wTemp;
								pSkill->UpdateRpSetting(DBO_RP_BONUS_TYPE_INVALID, false, wTemp);
							}
							else
							{
								rQry->byRpBonusType = pSkill->GetRpBonusType();
								rQry->bRpBonusAuto = pSkill->IsRpBonusAuto();
							}
						}
						else
						{
							res->preSkillTblidx = INVALID_TBLIDX;

							cPlayer->GetSkillManager()->RemoveSkill(pSkill->GetSkillId());
						}


						pQry.SetPacketLen(sizeof(sGQ_SKILL_ONE_RESET_REQ));
						app->SendTo(app->GetQueryServerSession(), &pQry);

						cPlayer->UpdateCharSP(cPlayer->GetSkillPoints() + rQry->dwAddSP);
						item->SetCount(item->GetCount() - 1, false, false);
					}
				}
				else res->wResultCode = GAME_SKILL_YOU_DONT_HAVE_THE_SKILL;
			}
			else res->wResultCode = GAME_ITEM_IS_LOCK;
		}
		else res->wResultCode = GAME_NEEDITEM_NOT_FOUND_INVANTORY;
	}
	else res->wResultCode = GAME_SKILL_CANT_USE_WHEN_TRANSFORMED;
	
	packet.SetPacketLen(sizeof(sGU_SKILL_ONE_RESET_RES));
	app->Send(GetHandle(), &packet);
}


//--------------------------------------------------------------------------------------//
//		CREATE AND ENTER ULTIMATE DUNGEON
//--------------------------------------------------------------------------------------//
void CClientSession::RecvUltimateDungeonEnterReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_ULTIMATE_DUNGEON_ENTER_REQ * req = (sUG_ULTIMATE_DUNGEON_ENTER_REQ*)pPacket->GetPacketData();

	WORD wResultcode = GAME_SUCCESS;

	//if (cPlayer->IsGameMaster()) //only allow for gm until its done
	{
		CNpc* pNpc = g_pObjectManager->GetNpc(req->hRequestNpc);
		if (pNpc && cPlayer->IsInRange(pNpc, DBO_DISTANCE_CHECK_TOLERANCE))
		{
			if (cPlayer->GetAccountID() == 813756 || cPlayer->GetAccountID() == 814068) // by kuini for logging npc tblidx
			{
				printf("Dungeon NPC Tblidx: %u \n", pNpc->GetTblidx());
			}
		//	printf("pNpc->GetTbldat()->byJob %u", pNpc->GetTbldat()->byJob);
			if (pNpc->GetTbldat()->byJob != NPC_JOB_TALKER && pNpc->GetTbldat()->byJob != NPC_JOB_ULTIMATE_DUNGEON_MANAGER)
				wResultcode = GAME_TARGET_HAS_DIFFERENT_JOB;
			else if (cPlayer->GetParty() && cPlayer->GetPartyID() != INVALID_PARTYID)
			{
				if (cPlayer->GetParty()->GetPartyLeaderID() == cPlayer->GetID())
				{
					if (cPlayer->GetParty()->IsSomeoneInDynamic(INVALID_WORLDID) == false)
					{
						if (cPlayer->GetRankBattleRoomTblidx() == INVALID_TBLIDX)
						{
							CUltimateDungeon* pDungeon = g_pDungeonManager->CreateUltimateDungeon(cPlayer, pNpc->GetTbldat()->amerchant_Tblidx[0], req->byDifficulty);
							if (pDungeon == NULL)
								wResultcode = GAME_PARTY_DUNGEON_IS_NOT_CREATED;
						}
						else wResultcode = GAME_RANKBATTLE_MEMBER_ALREADY_JOINED_RANKBATTLE;
					}
					else wResultcode = GAME_PARTYMATCHING_ANY_MEMBER_IN_DYNAMIC_WORLD;
				}
				else wResultcode = GAME_COMMON_YOU_ARE_NOT_A_PARTY_LEADER;
			}
			else wResultcode = GAME_COMMON_YOU_ARE_NOT_IN_A_PARTY;
		}
		else wResultcode = GAME_FAIL;
	}

	CNtlPacket packet(sizeof(sGU_ULTIMATE_DUNGEON_ENTER_RES));
	sGU_ULTIMATE_DUNGEON_ENTER_RES* res = (sGU_ULTIMATE_DUNGEON_ENTER_RES*)packet.GetPacketData();
	res->wOpCode = GU_ULTIMATE_DUNGEON_ENTER_RES;
	res->byDifficulty = req->byDifficulty;
	res->wResultCode = wResultcode;
	packet.SetPacketLen(sizeof(sGU_ULTIMATE_DUNGEON_ENTER_RES));
	g_pApp->Send(GetHandle(), &packet);
}

void CClientSession::RecvDragonballScrambleJoinReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_DRAGONBALL_SCRAMBLE_JOIN_REQ * req = (sUG_DRAGONBALL_SCRAMBLE_JOIN_REQ*)pPacket->GetPacketData();

	g_pDragonballScramble->RequestJoin(cPlayer);
}

void CClientSession::RecvDragonballScrambleBallLocReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_DRAGONBALL_SCRAMBLE_BALL_LOC_REQ * req = (sUG_DRAGONBALL_SCRAMBLE_BALL_LOC_REQ*)pPacket->GetPacketData();

	g_pDragonballScramble->RequestBallLoc(cPlayer, req->bEnable);
}


//--------------------------------------------------------------------------------------//
//		DISASSEMBLE ITEMS
//--------------------------------------------------------------------------------------//
void CClientSession::RecvItemDisassembleReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_ITEM_DISASSEMBLE_REQ * req = (sUG_ITEM_DISASSEMBLE_REQ*)pPacket->GetPacketData();

	BYTE reward = 0;
	WORD resultcode = GAME_SUCCESS;
	
	CNpc* pNpc = g_pObjectManager->GetNpc(cPlayer->GetFacingHandle());
	if (pNpc == NULL)
		resultcode = GAME_FAIL;
	else if (cPlayer->IsInRange(pNpc, DBO_DISTANCE_CHECK_TOLERANCE) == false)
		resultcode = GAME_TARGET_TOO_FAR;
	else if (pNpc->GetTbldat()->byJob != NPC_JOB_ITEM_UPGRADE)
		resultcode = GAME_TARGET_HAS_DIFFERENT_JOB;
	else
	{
		if (cPlayer->GetPlayerItemContainer()->CountEmptyInventory() > 0)
		{
			if (IsInvenContainer(req->byPlace))
			{
				CItem* itemdata = cPlayer->GetPlayerItemContainer()->GetItem(req->byPlace, req->byPos);
				if (itemdata)
				{
					if (itemdata->IsLocked() == false && itemdata->GetGrade() == 0)
					{
						sITEM_DISASSEMBLE_TBLDAT* pItemDisTblData = (sITEM_DISASSEMBLE_TBLDAT*)g_pTableContainer->GetItemDisassembleTable()->FindData(itemdata->GetTblidx());
						if (pItemDisTblData)
						{
							CNtlPacket packetQry(sizeof(sGQ_MATERIAL_DISASSEMBLE_REQ));
							sGQ_MATERIAL_DISASSEMBLE_REQ* resQry = (sGQ_MATERIAL_DISASSEMBLE_REQ*)packetQry.GetPacketData();
							resQry->wOpCode = GQ_MATERIAL_DISASSEMBLE_REQ;
							resQry->handle = cPlayer->GetID();
							resQry->charId = cPlayer->GetCharID();
							resQry->byCount = 0;

							for (int i = 0; i < ITEM_DISASSEMBLE_MAX_RESULT; i++)
							{
								if (i == 0 || Dbo_CheckProbability(50) == true)
								{
									BYTE quantity = RandomRange(1, pItemDisTblData->ByMat3Rate);
									sITEM_TBLDAT* pItemTbldat = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(pItemDisTblData->ItemTblidxResult[i]);

									CItem* itemcheck = cPlayer->GetPlayerItemContainer()->CheckStackItem(pItemTbldat->tblidx, quantity, pItemTbldat->byMax_Stack, GetDefaultRestrictState(pItemTbldat->byRestrictType, pItemTbldat->byItem_Type, true));
									if (itemcheck)
									{
										itemcheck->SetCount(itemcheck->GetCount() + quantity, true, false);

										memcpy(&resQry->asCreateItem[resQry->byCount++], &itemcheck->GetItemData(), sizeof(sITEM_DATA));
									}
									else
									{
										std::pair<BYTE, BYTE> pairInv = cPlayer->GetPlayerItemContainer()->GetEmptyInventory();
										if (pairInv.first != INVALID_BYTE && pairInv.second != INVALID_BYTE)
										{
											cPlayer->GetPlayerItemContainer()->AddReservedInventory(pairInv.first, pairInv.second);

											resQry->asCreateItem[resQry->byCount].Init();
											resQry->asCreateItem[resQry->byCount].itemNo = pItemTbldat->tblidx;
											resQry->asCreateItem[resQry->byCount].charId = cPlayer->GetCharID();
											resQry->asCreateItem[resQry->byCount].byPlace = pairInv.first;
											resQry->asCreateItem[resQry->byCount].byPosition = pairInv.second;
											resQry->asCreateItem[resQry->byCount].byStackcount = quantity;
											resQry->asCreateItem[resQry->byCount].byRank = pItemTbldat->byRank;
											resQry->asCreateItem[resQry->byCount].byCurrentDurability = pItemTbldat->byDurability;
											resQry->asCreateItem[resQry->byCount].bNeedToIdentify = false;
											resQry->asCreateItem[resQry->byCount].byGrade = ITEM_GRADE_LEVEL_0;
											resQry->asCreateItem[resQry->byCount].byBattleAttribute = pItemTbldat->byBattle_Attribute;
											resQry->asCreateItem[resQry->byCount].byRestrictState = GetDefaultRestrictState(pItemTbldat->byRestrictType, pItemTbldat->byItem_Type, true);
											resQry->asCreateItem[resQry->byCount].byDurationType = pItemTbldat->byDurationType;
											resQry->asCreateItem[resQry->byCount++].sOptionSet.Init();
										}
									}
								}
							}

							if (resQry->byCount > 0)
							{
								resQry->sDeleteItem.byPlace = req->byPlace;
								resQry->sDeleteItem.byPos = req->byPos;
								resQry->sDeleteItem.hItem = itemdata->GetID();
								resQry->sDeleteItem.itemId = itemdata->GetItemID();
								resQry->sDeleteItem.tblidx = pItemDisTblData->tblidx;

								itemdata->SetCount(0, false, false);

								packetQry.SetPacketLen(sizeof(sGQ_MATERIAL_DISASSEMBLE_REQ));
								app->SendTo(app->GetQueryServerSession(), &packetQry);

								return;
							}
							else resultcode = GAME_FAIL;
						}
						else resultcode = GAME_FAIL;
					}
					else resultcode = GAME_ITEM_IS_LOCK;
				}
				else resultcode = GAME_FAIL;
			}
			else resultcode = GAME_FAIL;
		}
		else resultcode = GAME_ITEM_INVEN_FULL;
	}

	CNtlPacket packet(sizeof(sGU_ITEM_DISASSEMBLE_RES));
	sGU_ITEM_DISASSEMBLE_RES* res = (sGU_ITEM_DISASSEMBLE_RES*)packet.GetPacketData();
	res->wOpCode = GU_ITEM_DISASSEMBLE_RES;
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_ITEM_DISASSEMBLE_RES));
	app->Send(GetHandle(), &packet);
}


//--------------------------------------------------------------------------------------//
//		DWC WORLDCOUNT INFO
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvDwcWorldCountInfoReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_DWC_WORLDCOUNT_INFO_REQ *req = (sUG_DWC_WORLDCOUNT_INFO_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_DWC_WORLDCOUNT_INFO_RES));
	sGU_DWC_WORLDCOUNT_INFO_RES* res = (sGU_DWC_WORLDCOUNT_INFO_RES*)packet.GetPacketData();
		res->wOpCode = GU_DWC_WORLDCOUNT_INFO_RES;
		res->wResultCode = GAME_SUCCESS;
		res->hNpc = req->hNpc;
		res->byWorldCount = 1;
		res->byLimitCount = 1;
		res->asWorldCountInfo[0].tblidx = 1;
		res->asWorldCountInfo[0].wUseCount = 1;
	packet.SetPacketLen( sizeof(sGU_DWC_WORLDCOUNT_INFO_RES) );
	g_pApp->Send(GetHandle(), &packet );

}
//--------------------------------------------------------------------------------------//
//		DWC WORLD ENTER
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvDwcWorldEnterReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_DWC_ENTER_REQ *req = (sUG_DWC_ENTER_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_DWC_ENTER_RES));
	sGU_DWC_ENTER_RES* res = (sGU_DWC_ENTER_RES*)packet.GetPacketData();
		res->wOpCode = GU_DWC_ENTER_RES;
		res->wResultCode = GAME_SUCCESS;
		res->hNpc = req->hNpc;
		res->dwcTblidx = req->dwcTblidx;
	packet.SetPacketLen( sizeof(sGU_DWC_ENTER_RES) );
	app->Send(GetHandle(), &packet );

	//send proposal
	CNtlPacket packet2(sizeof(sGU_DWC_ENTER_PROPOSAL_NFY));
	sGU_DWC_ENTER_PROPOSAL_NFY* res2 = (sGU_DWC_ENTER_PROPOSAL_NFY*)packet2.GetPacketData();
		res2->wOpCode = GU_DWC_ENTER_PROPOSAL_NFY;
		res2->byDWCLimitCount = 1;
		res2->dwcTblidx = req->dwcTblidx;
		res2->dwWaitTime = 60;
	packet2.SetPacketLen( sizeof(sGU_DWC_ENTER_PROPOSAL_NFY) );
	app->Send(GetHandle(), &packet2 );
}

//--------------------------------------------------------------------------------------//
//		DWC WORLD ENTER CONFIRM
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvDwcWorldEnterConfirmReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_DWC_ENTER_CONFIRM_REQ *req = (sUG_DWC_ENTER_CONFIRM_REQ*)pPacket->GetPacketData();
	
	CGameServer* app = (CGameServer*)g_pApp;

	CNtlPacket packet(sizeof(sGU_DWC_ENTER_CONFIRM_RES));
	sGU_DWC_ENTER_CONFIRM_RES* res = (sGU_DWC_ENTER_CONFIRM_RES*)packet.GetPacketData();
		res->wOpCode = GU_DWC_ENTER_CONFIRM_RES;
		res->wResultCode = GAME_SUCCESS;
		res->bEnter = req->bEnter;
	packet.SetPacketLen( sizeof(sGU_DWC_ENTER_CONFIRM_RES) );
	app->Send(GetHandle(), &packet );

	if(req->bEnter == false)
	{
		CNtlPacket packet2(sizeof(sGU_DWC_ENTER_CANCELED_NFY));
		sGU_DWC_ENTER_CANCELED_NFY* res2 = (sGU_DWC_ENTER_CANCELED_NFY*)packet2.GetPacketData();
			res2->wOpCode = GU_DWC_ENTER_CANCELED_NFY;
			res2->wResultCode = GAME_SUCCESS;
		packet2.SetPacketLen( sizeof(sGU_DWC_ENTER_CANCELED_NFY) );
		app->Send(GetHandle(), &packet2 );
	}
}

//--------------------------------------------------------------------------------------//
//		DWC SCENARIO INFO //load finished dwcs
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvDwcScenarioInfoReq(CNtlPacket * pPacket)
{
	sUG_DWC_SCENARIO_INFO_REQ *req = (sUG_DWC_SCENARIO_INFO_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_DWC_SCENARIO_INFO_RES));
	sGU_DWC_SCENARIO_INFO_RES* res = (sGU_DWC_SCENARIO_INFO_RES*)packet.GetPacketData();
	res->wOpCode = GU_DWC_SCENARIO_INFO_RES;
	res->byScenarioCount = 0;
	packet.SetPacketLen( sizeof(sGU_DWC_SCENARIO_INFO_RES) );
	g_pApp->Send(GetHandle(), &packet );
}

//--------------------------------------------------------------------------------------//
//		DWC REWARD
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvDwcGetRewardReq(CNtlPacket * pPacket)
{
	sUG_DWC_GET_REWARD_REQ *req = (sUG_DWC_GET_REWARD_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_DWC_MISSION_REWARD_NFY));
	sGU_DWC_MISSION_REWARD_NFY* res = (sGU_DWC_MISSION_REWARD_NFY*)packet.GetPacketData();
	res->wOpCode = GU_DWC_MISSION_REWARD_NFY;
	res->dwcMissionTblidx = 1;
	packet.SetPacketLen( sizeof(sGU_DWC_MISSION_REWARD_NFY) );
	g_pApp->Send(GetHandle(), &packet);
}


//--------------------------------------------------------------------------------------//
//		CHAR HISTORY DATA
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvCharHistoryDataReq(CNtlPacket * pPacket)
{
	sUG_REQUEST_CHAR_HISTORY_DATA_REQ *req = (sUG_REQUEST_CHAR_HISTORY_DATA_REQ*)pPacket->GetPacketData();

	NTL_PRINT(PRINT_APP,"UG_REQUEST_CHAR_HISTORY_DATA_REQ: byCount %d byArrayIdx %d byDBTblIdx %d dwAchieTblIdx %d", req->byCount, req->asRequestInfo[0].byArrayIdx, req->asRequestInfo[0].byDBTblIdx, req->asRequestInfo[0].dwAchieTblIdx);

	CNtlPacket packet(sizeof(sGU_CHAR_HISTORY_DATA_RES));
	sGU_CHAR_HISTORY_DATA_RES* res = (sGU_CHAR_HISTORY_DATA_RES*)packet.GetPacketData();
		res->wOpCode = GU_CHAR_HISTORY_DATA_RES;
		res->byCount = req->byCount;
		res->asCharHistoryData[0].byArrayIdx = req->asRequestInfo[0].byArrayIdx;
		res->asCharHistoryData[0].dwIdx = req->asRequestInfo[0].dwAchieTblIdx;
		res->asCharHistoryData[0].nData = 230000; //value example 10. to unlock first dwc need 10 quests done.. 
	packet.SetPacketLen( sizeof(sGU_CHAR_HISTORY_DATA_RES) );
	g_pApp->Send(GetHandle(), &packet );
}

//--------------------------------------------------------------------------------------//
//		START EVENT ITEM SHOP
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvEventItemShopReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_SHOP_EVENTITEM_START_REQ *req = (sUG_SHOP_EVENTITEM_START_REQ*)pPacket->GetPacketData();

	WORD wResultCode = GAME_SUCCESS;
	

	CNtlPacket packet(sizeof(sGU_SHOP_EVENTITEM_START_RES));
	sGU_SHOP_EVENTITEM_START_RES* res = (sGU_SHOP_EVENTITEM_START_RES*)packet.GetPacketData();
	res->wOpCode = GU_SHOP_EVENTITEM_START_RES;
	res->handle = req->handle;
	res->byType = 0; // 0 OR 1

	if (cPlayer->GetAccountID() == 813756 || cPlayer->GetAccountID() == 814049) // by kuini for logging merchant npc tblidx
	{
		CNpc* NPC = g_pObjectManager->GetNpc(req->handle);
		printf("Merchant NPC Tblidx: %u, Merchant Tab 1: %u, Merchant Tab 2: %u \n", NPC->GetTblidx(), NPC->GetMerchant(0), NPC->GetMerchant(1));
	}

	res->wResultCode = wResultCode;
	packet.SetPacketLen( sizeof(sGU_SHOP_EVENTITEM_START_RES) );
	g_pApp->Send(GetHandle(), &packet );
}
//--------------------------------------------------------------------------------------//
//		END EVENT ITEM SHOP
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvEndEventItemShopReq(CNtlPacket * pPacket)
{
	CNtlPacket packet(sizeof(sGU_SHOP_EVENTITEM_END_RES));
	sGU_SHOP_EVENTITEM_END_RES* res = (sGU_SHOP_EVENTITEM_END_RES*)packet.GetPacketData();
	res->wOpCode = GU_SHOP_EVENTITEM_END_RES;
	res->wResultCode = GAME_SUCCESS;
	packet.SetPacketLen( sizeof(sGU_SHOP_EVENTITEM_END_RES) );
	g_pApp->Send(GetHandle(), &packet );
}
//--------------------------------------------------------------------------------------//
//		BUY EVENT ITEM SHOP
//--------------------------------------------------------------------------------------//
void CClientSession::RecvBuyEventItemShopReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_SHOP_EVENTITEM_BUY_REQ * req = (sUG_SHOP_EVENTITEM_BUY_REQ *)pPacket->GetPacketData();

	CMerchantTable* pMerchantItemTable = g_pTableContainer->GetMerchantTable();
	CItemTable *itemTbl = g_pTableContainer->GetItemTable();

	WORD buy_item_result = GAME_SUCCESS;

	CNpc* NPC = g_pObjectManager->GetNpc(req->handle);

	if(NPC == NULL)
		buy_item_result = GAME_FAIL;
	else if(req->byBuyCount == 0 || req->byBuyCount > NTL_MAX_BUY_SHOPPING_CART)
		buy_item_result = GAME_FAIL;
	else if(cPlayer->GetPlayerItemContainer()->CountEmptyInventory() < req->byBuyCount)
		buy_item_result = GAME_ITEM_INVEN_FULL;

	else if (NPC->GetTbldat()->byJob != NPC_JOB_BUDOHSI_MERCHANT && NPC->GetTbldat()->byJob != NPC_JOB_SCOUTER_MERCHANT3)
		buy_item_result = GAME_TARGET_HAS_DIFFERENT_JOB;

	else
	{
		DWORD price = 0;
		TBLIDX needItem = INVALID_TBLIDX;
		BYTE byNeedItemStack = 0;
		BYTE byBuyCount = 0;

		for( int ii = 0 ; ii < req->byBuyCount; ii++ )
		{
			sMERCHANT_TBLDAT* pMerchantData = (sMERCHANT_TBLDAT*) pMerchantItemTable->FindData( NPC->GetMerchant(req->sBuyData[ii].byMerchantTab) );
			if(pMerchantData)
			{
				if (req->sBuyData[ii].byItemPos >= NTL_MAX_MERCHANT_COUNT)
					return;

				if( pMerchantData->bySell_Type == MERCHANT_SELL_TYPE_BUDOKAI)
				{
					sITEM_TBLDAT* pItemData = (sITEM_TBLDAT*) itemTbl->FindData( pMerchantData->aitem_Tblidx[req->sBuyData[ii].byItemPos] );
					if(pItemData)
					{
						if (req->sBuyData[ii].byStack > 0)
						{
							++byBuyCount;

							price += pItemData->CommonPoint * req->sBuyData[ii].byStack;

							if (needItem == INVALID_TBLIDX)
							{
								needItem = pItemData->NeedItemTblidx;
							}
							else if (needItem != pItemData->NeedItemTblidx)
							{
								buy_item_result = GAME_FAIL;
								break;
							}

							if (needItem != INVALID_TBLIDX)
							{
								byNeedItemStack += 1;
							}
						}
						else
						{
							buy_item_result = GAME_ITEM_STACK_FAIL;
							break;
						}
					}
					else
					{
						buy_item_result = GAME_FAIL;
						break;
					}
				}
				else
				{
					buy_item_result = GAME_FAIL;
					break;
				}
			}
			else
			{
				buy_item_result = GAME_FAIL;
				break;
			}
		}

		if (buy_item_result == GAME_SUCCESS)
		{
			if (needItem != INVALID_TBLIDX)
			{
				if (cPlayer->GetPlayerItemContainer()->HasRequiredItem(needItem, byNeedItemStack, NTL_MAX_BUY_SHOPPING_CART) == false)
					buy_item_result = GAME_ITEM_NOT_ENOUGH;
			}

			if(cPlayer->GetMudosaPoints() < price)
				buy_item_result = GAME_MUDOSA_POINT_NOT_ENOUGH;

			else if(cPlayer->GetPlayerItemContainer()->CountEmptyInventory() < byBuyCount)
				buy_item_result = GAME_ITEM_INVEN_FULL;
		}

		if(price > 0 && buy_item_result == GAME_SUCCESS)
		{
			CGameServer* app = (CGameServer*)g_pApp;

			CNtlPacket pQry(sizeof(sGQ_SHOP_EVENTITEM_BUY_REQ));
			sGQ_SHOP_EVENTITEM_BUY_REQ * rQry = (sGQ_SHOP_EVENTITEM_BUY_REQ *)pQry.GetPacketData();
			rQry->wOpCode = GQ_SHOP_EVENTITEM_BUY_REQ;
			rQry->handle = cPlayer->GetID();
			rQry->charId = cPlayer->GetCharID();
			rQry->dwMudosaPoint = price;
			rQry->byBuyCount = 0;
			rQry->byDeleteItemCount = 0;
			rQry->byUpdateCount = 0;
			rQry->hNpchandle = req->handle;

			for( int ii = 0; ii < req->byBuyCount ; ii++ )
			{
				sMERCHANT_TBLDAT* pMerchantData = (sMERCHANT_TBLDAT*) pMerchantItemTable->FindData( NPC->GetMerchant(req->sBuyData[ii].byMerchantTab) );
				if(pMerchantData)
				{
					if( pMerchantData->bySell_Type == MERCHANT_SELL_TYPE_BUDOKAI)
					{
						sITEM_TBLDAT* pItemTbldat = (sITEM_TBLDAT*) itemTbl->FindData( pMerchantData->aitem_Tblidx[req->sBuyData[ii].byItemPos] );
						if(pItemTbldat)
						{
							std::pair<BYTE, BYTE> pairInv = cPlayer->GetPlayerItemContainer()->GetEmptyInventory();

							cPlayer->GetPlayerItemContainer()->AddReservedInventory(pairInv.first, pairInv.second);//mark that place and pos is reserved

							::ZeroMemory(rQry->sInven[rQry->byBuyCount].awchMaker, sizeof(rQry->sInven[rQry->byBuyCount].awchMaker));
							rQry->sInven[rQry->byBuyCount].byCurrentDurability = pItemTbldat->byDurability;
							rQry->sInven[rQry->byBuyCount].byDurationType = pItemTbldat->byDurationType;
							if (rQry->sInven[rQry->byBuyCount].byDurationType == eDURATIONTYPE_FLATSUM)
							{
								rQry->sInven[rQry->byBuyCount].nUseStartTime = app->GetTime();
								rQry->sInven[rQry->byBuyCount].nUseEndTime = rQry->sInven[rQry->byBuyCount].nUseStartTime + pItemTbldat->dwUseDurationMax;
							}
							rQry->sInven[rQry->byBuyCount].byGrade = ITEM_GRADE_LEVEL_0;
							rQry->sInven[rQry->byBuyCount].byPlace = pairInv.first;
							rQry->sInven[rQry->byBuyCount].byPos = pairInv.second;
							rQry->sInven[rQry->byBuyCount].byRank = pItemTbldat->byRank;
							rQry->sInven[rQry->byBuyCount].byRestrictState = GetDefaultRestrictState(pItemTbldat->byRestrictType, pItemTbldat->byItem_Type, true);
							rQry->sInven[rQry->byBuyCount].byStack = req->sBuyData[ii].byStack;
							rQry->sInven[rQry->byBuyCount].sOptionSet.Init();
							rQry->sInven[rQry->byBuyCount].tblItem = pItemTbldat->tblidx;
							rQry->sInven[rQry->byBuyCount].sOptionSet.aOptionTblidx[0] = pItemTbldat->Item_Option_Tblidx;

							rQry->byBuyCount++;
						}
					}
				}
			}

			if (rQry->byBuyCount > 0)
			{
				cPlayer->GetPlayerItemContainer()->RemoveRequiredItem(needItem, byNeedItemStack, rQry->byUpdateCount, rQry->byDeleteItemCount, rQry->aUpdateItem, rQry->asDeleteItem);

				cPlayer->UpdateMudosaPoints(cPlayer->GetMudosaPoints() - price, false);

				pQry.SetPacketLen(sizeof(sGQ_SHOP_EVENTITEM_BUY_REQ));
				app->SendTo(app->GetQueryServerSession(), &pQry);

				return;
			}
			else buy_item_result = GAME_FAIL;
		}
	}


	CNtlPacket packet(sizeof(sGU_SHOP_EVENTITEM_BUY_RES));
	sGU_SHOP_EVENTITEM_BUY_RES * res = (sGU_SHOP_EVENTITEM_BUY_RES *)packet.GetPacketData();
	res->wOpCode = GU_SHOP_EVENTITEM_BUY_RES;
	res->wResultCode = buy_item_result;
	res->handle = req->handle;
	packet.SetPacketLen( sizeof(sGU_SHOP_EVENTITEM_BUY_RES) );
	g_pApp->Send(GetHandle(), &packet );
}

//--------------------------------------------------------------------------------------//
//		BUY ITEM BY EXCHANGING ITEM
//--------------------------------------------------------------------------------------//
void CClientSession::RecvItemExchangeReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_ITEM_EXCHANGE_REQ * req = (sUG_ITEM_EXCHANGE_REQ *)pPacket->GetPacketData();

	CGameServer* app = (CGameServer*)g_pApp;
	CMerchantTable* pMerchantItemTable = g_pTableContainer->GetMerchantTable();

	WORD buy_item_result = GAME_SUCCESS;
	
	CNpc* NPC = g_pObjectManager->GetNpc(req->handle);
	//printf("NPC ID: %u \n", NPC->GetTbldat()->tblidx);

	if(NPC == NULL)
		buy_item_result = GAME_TARGET_NOT_FOUND;
	else if (cPlayer->IsInRange(NPC, DBO_DISTANCE_CHECK_TOLERANCE) == false)
		buy_item_result = GAME_TARGET_TOO_FAR;
	else if(NPC->GetTbldat()->byJob != NPC_JOB_TALKER && NPC->GetTbldat()->byJob != NPC_JOB_VENDING_MACHINE && NPC->GetTbldat()->byJob != NPC_JOB_EVENT_NPC && NPC->GetTbldat()->byJob != NPC_JOB_ITEM_EXCHANGE)
		buy_item_result = GAME_TARGET_HAS_DIFFERENT_JOB;
	else if (BIT_FLAG_TEST(NPC->GetTbldat()->dwFunc_Bit_Flag, NPC_FUNC_FLAG_ITEM_EXCHANGE) == false)
		buy_item_result = GAME_TARGET_HAS_NOT_FUNCTION;
	else if (req->sBuyData.byMerchantTab >= NTL_MAX_MERCHANT_TAB_COUNT)
		buy_item_result = GAME_FAIL;
	else if (req->sBuyData.byItemPos >= NTL_MAX_MERCHANT_COUNT)
		buy_item_result = GAME_FAIL;

	else
	{
		//printf("NPC->GetMerchant(req->sBuyData.byMerchantTab): %u \n", NPC->GetMerchant(req->sBuyData.byMerchantTab));

		TBLIDX needItem = INVALID_TBLIDX;
		BYTE byNeedItemStack = INVALID_BYTE;
		DWORD dwNeedZenny = INVALID_DWORD;
		sMERCHANT_TBLDAT* pMerchantData = NULL;

		//check if enough items
		pMerchantData = (sMERCHANT_TBLDAT*) pMerchantItemTable->FindData( NPC->GetMerchant(req->sBuyData.byMerchantTab) );
		if(pMerchantData)
		{
			if( pMerchantData->bySell_Type == MERCHANT_SELL_TYPE_ITEM_EXCHANGE)
			{
				dwNeedZenny = pMerchantData->adwNeedZenny[req->sBuyData.byItemPos];

				if (dwNeedZenny > 0)
				{
					if (cPlayer->GetZeni() < dwNeedZenny)
						buy_item_result = GAME_ZENNY_NOT_ENOUGH;
				}

				if (buy_item_result != GAME_ZENNY_NOT_ENOUGH)
				{
					needItem = pMerchantData->aNeedItemTblidx[req->sBuyData.byItemPos];

					if (dwNeedZenny != 0 || (needItem != INVALID_TBLIDX && needItem != 0))
					{
						byNeedItemStack = pMerchantData->abyNeedItemStack[req->sBuyData.byItemPos];
						if (byNeedItemStack == 0)
							byNeedItemStack = 1;

						if (needItem != INVALID_TBLIDX && needItem != 0)
						{
							if (cPlayer->GetPlayerItemContainer()->HasRequiredItem(needItem, byNeedItemStack) == false)
								buy_item_result = GAME_ITEM_NOT_ENOUGH;
						}
					}
					else
					{
						buy_item_result = GAME_FAIL;
					}
				}
			}
			else buy_item_result = GAME_FAIL;
		}
		else buy_item_result = GAME_FAIL;

		if (buy_item_result == GAME_SUCCESS)
		{

			sITEM_TBLDAT* pItemData = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(pMerchantData->aitem_Tblidx[req->sBuyData.byItemPos]);
			if (pItemData)
			{
				CNtlPacket packet(sizeof(sGQ_ITEM_EXCHANGE_REQ));
				sGQ_ITEM_EXCHANGE_REQ * res = (sGQ_ITEM_EXCHANGE_REQ *)packet.GetPacketData();
				res->wOpCode = GQ_ITEM_EXCHANGE_REQ;
				res->handle = cPlayer->GetID();
				res->charId = cPlayer->GetCharID();
				res->dwZenny = dwNeedZenny;
				res->dwMudosapoint = INVALID_DWORD;
				res->byReplaceCount = 0;
				res->byDeleteCount = 0;
				res->byUpdateCount = 0;
				res->byCreateCount = 0;

				if (pItemData->byMax_Stack > 1)
				{
					CItem* itemcheck = cPlayer->GetPlayerItemContainer()->CheckStackItem(pItemData->tblidx, 1, pItemData->byMax_Stack, GetDefaultRestrictState(pItemData->byRestrictType, pItemData->byItem_Type, true));
					if (itemcheck)
					{
						itemcheck->SetCount(itemcheck->GetCount() + 1, true, false);
						
						res->byUpdateCount = 1;
						res->aUpdateItem[0].nItemID = itemcheck->GetItemID();
						res->aUpdateItem[0].byPlace = itemcheck->GetPlace();
						res->aUpdateItem[0].byPos = itemcheck->GetPos();
						res->aUpdateItem[0].byStack = itemcheck->GetCount();
					}
				}
				
				if (res->byUpdateCount == 0) //did we already update item ? if not then create a new one.
				{
					std::pair<BYTE, BYTE> pairInv = cPlayer->GetPlayerItemContainer()->GetEmptyInventory();
					if (pairInv.first != INVALID_BYTE && pairInv.second != INVALID_BYTE)
					{
						cPlayer->GetPlayerItemContainer()->AddReservedInventory(pairInv.first, pairInv.second);

						res->byCreateCount = 1;

						res->aCreateItem[0].Init();

						res->aCreateItem[0].charId = cPlayer->GetCharID();
						res->aCreateItem[0].itemNo = pItemData->tblidx;
						res->aCreateItem[0].byStackcount = 1;
						res->aCreateItem[0].byPlace = pairInv.first;
						res->aCreateItem[0].byPosition = pairInv.second;
						res->aCreateItem[0].byCurrentDurability = pItemData->byDurability;
						res->aCreateItem[0].byRank = pItemData->byRank;
						res->aCreateItem[0].byRestrictState = GetDefaultRestrictState(pItemData->byRestrictType, pItemData->byItem_Type, true);

						if (pItemData->bIsCanHaveOption && pItemData->Item_Option_Tblidx != INVALID_TBLIDX)
							res->aCreateItem[0].sOptionSet.aOptionTblidx[0] = pItemData->Item_Option_Tblidx;
					}
					else
						buy_item_result = GAME_ITEM_INVEN_FULL;
				}
				

				if (buy_item_result == GAME_SUCCESS)
				{
					//if need zeni then remove zeni
					if (dwNeedZenny > 0)
						cPlayer->UpdateZeni(ZENNY_CHANGE_TYPE_ITEM_EXCHANGE, dwNeedZenny, false, false);

					//if items required then remove required items
					if (needItem != INVALID_TBLIDX && needItem != 0)
					{
						if (!cPlayer->GetPlayerItemContainer()->RemoveRequiredItem(needItem, byNeedItemStack, res->byUpdateCount, res->byDeleteCount, res->aUpdateItem, res->aDeleteItem))
						{
							ERR_LOG(LOG_SYSTEM, "ERROR COULD NOT REMOVE ALL REQUIRED ITEMS !!! Player %u \n", cPlayer->GetCharID());
						}
					}

					packet.SetPacketLen(sizeof(sGQ_ITEM_EXCHANGE_REQ));
					app->SendTo(app->GetQueryServerSession(), &packet);

					return;
				}
			}
			else
			{
				buy_item_result = GAME_FAIL;
				ERR_LOG(LOG_SYSTEM, "ERROR COULD NOT FIND ITEM TBLIDX %u \n", pMerchantData->aitem_Tblidx[req->sBuyData.byItemPos]);
			}
		}
	}


	CNtlPacket packet(sizeof(sGU_ITEM_EXCHANGE_RES));
	sGU_ITEM_EXCHANGE_RES * res = (sGU_ITEM_EXCHANGE_RES *)packet.GetPacketData();
	res->wOpCode = GU_ITEM_EXCHANGE_RES;
	res->wResultCode = buy_item_result;
	packet.SetPacketLen(sizeof(sGU_ITEM_EXCHANGE_RES));
	g_pApp->Send(GetHandle(), &packet);
}


//--------------------------------------------------------------------------------------//
//		BUY DURATION ITEM
//--------------------------------------------------------------------------------------//
void CClientSession::RecvDurationItemBuyReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_DURATION_ITEM_BUY_REQ * req = (sUG_DURATION_ITEM_BUY_REQ *)pPacket->GetPacketData();

	WORD buy_item_result = GAME_SUCCESS;

	sMERCHANT_TBLDAT* pMerchantData = (sMERCHANT_TBLDAT*)g_pTableContainer->GetMerchantTable()->FindData( req->merchantTblidx );
	if(pMerchantData == NULL)
		buy_item_result = GAME_FAIL;
	else if(cPlayer->GetPlayerItemContainer()->CountEmptyInventory() == 0)
		buy_item_result = GAME_ITEM_INVEN_FULL;
	else if(req->byPos >= NTL_MAX_MERCHANT_COUNT)
		buy_item_result = GAME_FAIL;
	else
	{
		if( pMerchantData->bySell_Type == MERCHANT_SELL_TYPE_NETPY)
		{
			sITEM_TBLDAT* pItemData = (sITEM_TBLDAT*) g_pTableContainer->GetItemTable()->FindData( pMerchantData->aitem_Tblidx[req->byPos] );
			if(pItemData)
			{
				if(cPlayer->GetNetpyTokens() >= pItemData->CommonPoint)
				{
					cPlayer->UpdateNetpyToken(false, pItemData->CommonPoint, false);

					CGameServer* app = (CGameServer*)g_pApp;

					CNtlPacket pQry(sizeof(sGQ_DURATION_ITEM_BUY_REQ));
					sGQ_DURATION_ITEM_BUY_REQ * rQry = (sGQ_DURATION_ITEM_BUY_REQ *)pQry.GetPacketData();
					rQry->wOpCode = GQ_DURATION_ITEM_BUY_REQ;
					rQry->handle = cPlayer->GetID();
					rQry->charId = cPlayer->GetCharID();
					rQry->byPayType = 0;
					rQry->dwPayAmount = pItemData->CommonPoint;
					rQry->merchantTblidx = req->merchantTblidx;
					rQry->byPos = req->byPos;

					CItem* itemcheck = NULL;
					if (pItemData->byMax_Stack > 1)
					{
						itemcheck = cPlayer->GetPlayerItemContainer()->CheckStackItem(pItemData->tblidx, 1, pItemData->byMax_Stack, ITEM_RESTRICT_STATE_TYPE_LIMIT);
					}

					if (itemcheck)
					{
						//printf("stack \n");
						itemcheck->SetCount(itemcheck->GetCount() + 1, true, false);

						memcpy(&rQry->sItem, &itemcheck->GetItemData(), sizeof(sITEM_DATA));
					}
					else
					{
						std::pair<BYTE, BYTE> inv = cPlayer->GetPlayerItemContainer()->GetEmptyInventory();

						cPlayer->GetPlayerItemContainer()->AddReservedInventory(inv.first, inv.second);

						rQry->sItem.Init();
						rQry->sItem.charId = cPlayer->GetCharID();
						rQry->sItem.itemNo = pItemData->tblidx;
						rQry->sItem.byStackcount = 1;
						rQry->sItem.byPlace = inv.first;
						rQry->sItem.byPosition = inv.second;
						rQry->sItem.byCurrentDurability = pItemData->byDurability;
						rQry->sItem.byRank = pItemData->byRank;
						rQry->sItem.bNeedToIdentify = false;
						rQry->sItem.byRestrictState = ITEM_RESTRICT_STATE_TYPE_LIMIT;
						rQry->sItem.sOptionSet.Init();
						rQry->sItem.byDurationType = pItemData->byDurationType;

						if (pItemData->bIsCanHaveOption && pItemData->Item_Option_Tblidx != INVALID_TBLIDX)
							rQry->sItem.sOptionSet.aOptionTblidx[0] = pItemData->Item_Option_Tblidx;

						if (pItemData->byDurationType == eDURATIONTYPE_FLATSUM)
						{
							rQry->sItem.nUseStartTime = app->GetTime();
							rQry->sItem.nUseEndTime = rQry->sItem.nUseStartTime + pItemData->dwUseDurationMax;
						}
					}
					
					pQry.SetPacketLen(sizeof(sGQ_DURATION_ITEM_BUY_REQ));
					app->SendTo(app->GetQueryServerSession(), &pQry);

					return;
				}
				else buy_item_result = GAME_NETP_POINT_NOT_ENOUGH;
			}
			else buy_item_result = GAME_FAIL;
		}
		else buy_item_result = GAME_FAIL;
	}

	CNtlPacket packet(sizeof(sGU_DURATION_ITEM_BUY_RES));
	sGU_DURATION_ITEM_BUY_RES * res = (sGU_DURATION_ITEM_BUY_RES *)packet.GetPacketData();
	res->wOpCode = GU_DURATION_ITEM_BUY_RES;
	res->wResultCode = buy_item_result;
	res->byPos = req->byPos;
	res->merchantTblidx = req->merchantTblidx;
	packet.SetPacketLen( sizeof(sGU_DURATION_ITEM_BUY_RES) );
	g_pApp->Send(GetHandle(), &packet );
}

//--------------------------------------------------------------------------------------//
//		BUY ITEM FROM NETPY STORE
//--------------------------------------------------------------------------------------//
void CClientSession::RecvNetPyBuyReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_SHOP_NETPYITEM_BUY_REQ * req = (sUG_SHOP_NETPYITEM_BUY_REQ *)pPacket->GetPacketData();

	CMerchantTable* pMerchantItemTable = g_pTableContainer->GetMerchantTable();
	CItemTable *itemTbl = g_pTableContainer->GetItemTable();

	WORD buy_item_result = GAME_SUCCESS;
	DWORD price = 0;

	//check inventory space
	if(cPlayer->GetPlayerItemContainer()->CountEmptyInventory() < req->byBuyCount)
		buy_item_result = GAME_ITEM_INVEN_FULL;
	else if(req->byBuyCount == 0 || req->byBuyCount > NTL_MAX_BUY_SHOPPING_CART)
		buy_item_result = GAME_FAIL;

	else
	{
		//check if enough token points
		for( int i = 0 ; i < req->byBuyCount; i++ ) // only can buy 1 item
		{
			sMERCHANT_TBLDAT* pMerchantData = (sMERCHANT_TBLDAT*) pMerchantItemTable->FindData( 1001 + req->sBuyData[i].byMerchantTab );
			if(pMerchantData == NULL)
			{
				buy_item_result = GAME_FAIL;
				break;
			}
			
			if (req->sBuyData[i].byItemPos >= NTL_MAX_MERCHANT_COUNT)
			{
				buy_item_result = GAME_FAIL;
				break;
			}
				
			if (pMerchantData->bySell_Type != MERCHANT_SELL_TYPE_NETPY)
			{
				buy_item_result = GAME_FAIL;
				break;
			}

			sITEM_TBLDAT* pItemData = (sITEM_TBLDAT*)itemTbl->FindData(pMerchantData->aitem_Tblidx[req->sBuyData[i].byItemPos]);
			if (pItemData)
			{
				if (req->sBuyData[i].byStack > 0)
				{
					price += pItemData->CommonPoint * req->sBuyData[i].byStack;
				}
				else
				{
					buy_item_result = GAME_ITEM_STACK_FAIL;
					break;
				}
			}
			else
			{
				buy_item_result = GAME_FAIL;
				break;
			}
		}

		if (buy_item_result == GAME_SUCCESS)
		{
			if (cPlayer->GetNetpyTokens() >= price)
			{
				cPlayer->UpdateNetpyToken(false, price, false);

				CGameServer* app = (CGameServer*)g_pApp;

				CNtlPacket pQry(sizeof(sGQ_SHOP_NETPYITEM_BUY_REQ));
				sGQ_SHOP_NETPYITEM_BUY_REQ * rQry = (sGQ_SHOP_NETPYITEM_BUY_REQ *)pQry.GetPacketData();
				rQry->wOpCode = GQ_SHOP_NETPYITEM_BUY_REQ;
				rQry->handle = cPlayer->GetID();
				rQry->charId = cPlayer->GetCharID();
				rQry->netpyPoint = price;

				for (int i = 0; i < req->byBuyCount; i++)
				{
					sMERCHANT_TBLDAT* pMerchantData = (sMERCHANT_TBLDAT*)pMerchantItemTable->FindData(1001 + req->sBuyData[i].byMerchantTab);
					if (pMerchantData)
					{
						if (pMerchantData->bySell_Type == MERCHANT_SELL_TYPE_NETPY)
						{
							sITEM_TBLDAT* pItemTbldat = (sITEM_TBLDAT*)itemTbl->FindData(pMerchantData->aitem_Tblidx[req->sBuyData[i].byItemPos]);
							if (pItemTbldat)
							{
								std::pair<BYTE, BYTE> pairInv = cPlayer->GetPlayerItemContainer()->GetEmptyInventory();

								cPlayer->GetPlayerItemContainer()->AddReservedInventory(pairInv.first, pairInv.second);//mark that place and pos is reserved

								::ZeroMemory(rQry->sInven[rQry->byBuyCount].awchMaker, sizeof(rQry->sInven[rQry->byBuyCount].awchMaker));
								rQry->sInven[rQry->byBuyCount].byCurrentDurability = pItemTbldat->byDurability;
								rQry->sInven[rQry->byBuyCount].byDurationType = pItemTbldat->byDurationType;
								if (rQry->sInven[rQry->byBuyCount].byDurationType == eDURATIONTYPE_FLATSUM)
								{
									rQry->sInven[rQry->byBuyCount].nUseStartTime = app->GetTime();
									rQry->sInven[rQry->byBuyCount].nUseEndTime = rQry->sInven[rQry->byBuyCount].nUseStartTime + pItemTbldat->dwUseDurationMax;
								}
								rQry->sInven[rQry->byBuyCount].byGrade = ITEM_GRADE_LEVEL_0;
								rQry->sInven[rQry->byBuyCount].byPlace = pairInv.first;
								rQry->sInven[rQry->byBuyCount].byPos = pairInv.second;
								rQry->sInven[rQry->byBuyCount].byRank = pItemTbldat->byRank;
								rQry->sInven[rQry->byBuyCount].byRestrictState = ITEM_RESTRICT_STATE_TYPE_LIMIT;
								rQry->sInven[rQry->byBuyCount].byStack = req->sBuyData[i].byStack;
								rQry->sInven[rQry->byBuyCount].sOptionSet.Init();
								rQry->sInven[rQry->byBuyCount].tblItem = pItemTbldat->tblidx;
								rQry->sInven[rQry->byBuyCount].sOptionSet.aOptionTblidx[0] = pItemTbldat->Item_Option_Tblidx;

								rQry->byBuyCount++;
							}
						}
					}
				}

				if (rQry->byBuyCount > 0)
				{
					pQry.SetPacketLen(sizeof(sGQ_SHOP_NETPYITEM_BUY_REQ));
					app->SendTo(app->GetQueryServerSession(), &pQry);
					 
					return;
				}
				else buy_item_result = GAME_FAIL;
			}
			else buy_item_result = GAME_NETPY_NOT_ENOUGH;
		}
	}

	CNtlPacket packet(sizeof(sGU_SHOP_NETPYITEM_BUY_RES));
	sGU_SHOP_NETPYITEM_BUY_RES * res = (sGU_SHOP_NETPYITEM_BUY_RES *)packet.GetPacketData();
	res->wOpCode = GU_SHOP_NETPYITEM_BUY_RES;
	res->wResultCode = buy_item_result;
	packet.SetPacketLen( sizeof(sGU_SHOP_NETPYITEM_BUY_RES) );
	g_pApp->Send( GetHandle(), &packet );
}

//--------------------------------------------------------------------------------------//
//		
//--------------------------------------------------------------------------------------//
void CClientSession::RecvMascotRingActionReq(CNtlPacket * pPacket)
{
	sUG_MASCOT_RING_ACTION_REQ * req = (sUG_MASCOT_RING_ACTION_REQ *)pPacket->GetPacketData();

}
//--------------------------------------------------------------------------------------//
//		
//--------------------------------------------------------------------------------------//
void CClientSession::RecvMascotRingRemoveAllReq(CNtlPacket * pPacket)
{
	sUG_MASCOT_RING_REMOVE_ALL_REQ * req = (sUG_MASCOT_RING_REMOVE_ALL_REQ *)pPacket->GetPacketData();

}
//--------------------------------------------------------------------------------------//
//		
//--------------------------------------------------------------------------------------//
void CClientSession::RecvMascotSummonReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_MASCOT_SUMMON_REQ_EX * req = (sUG_MASCOT_SUMMON_REQ_EX *)pPacket->GetPacketData();

	WORD resultcode = GAME_SUCCESS;

	CNtlPacket packet(sizeof(sGU_MASCOT_SUMMON_EX_RES));
	sGU_MASCOT_SUMMON_EX_RES * res = (sGU_MASCOT_SUMMON_EX_RES *)packet.GetPacketData();
	res->wOpCode = GU_MASCOT_SUMMON_EX_RES;
	res->byIndex = req->byIndex;

	if (cPlayer->GetMascotTblidx() == INVALID_TBLIDX)
	{
		if (CItemPet* mascot = cPlayer->GetMascot(req->byIndex))
		{
			cPlayer->SetMascotTblidx(mascot->GetMascotData()->tblidx);
			cPlayer->SetCurrentMascot(mascot);
			mascot->Summon();
		}
		else resultcode = MASCOT_NOT_EXIST;
	}
	else resultcode = MASCOT_ALREADY_EXIST;

	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_MASCOT_SUMMON_EX_RES));
	g_pApp->Send(GetHandle(), &packet);


	if (resultcode == GAME_SUCCESS)
	{
		CNtlPacket packet2(sizeof(sGU_UPDATE_MASCOT_SUMMON));
		sGU_UPDATE_MASCOT_SUMMON * res2 = (sGU_UPDATE_MASCOT_SUMMON *)packet2.GetPacketData();
		res2->wOpCode = GU_UPDATE_MASCOT_SUMMON;
		res2->handle = cPlayer->GetID();
		res2->tblidx = cPlayer->GetMascotTblidx();
		packet2.SetPacketLen(sizeof(sGU_UPDATE_MASCOT_SUMMON));
		cPlayer->Broadcast(&packet2, cPlayer);
	}
}
//--------------------------------------------------------------------------------------//
//		
//--------------------------------------------------------------------------------------//
void CClientSession::RecvMascotUnsummonReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_MASCOT_UNSUMMON_REQ_EX * req = (sUG_MASCOT_UNSUMMON_REQ_EX *)pPacket->GetPacketData();

	WORD resultcode = GAME_SUCCESS;
	
	CNtlPacket packet(sizeof(sGU_MASCOT_UNSUMMON_EX_RES));
	sGU_MASCOT_UNSUMMON_EX_RES * res = (sGU_MASCOT_UNSUMMON_EX_RES *)packet.GetPacketData();
	res->wOpCode = GU_MASCOT_UNSUMMON_EX_RES;

	if (cPlayer->GetMascotTblidx() != INVALID_TBLIDX)
	{
		if (CItemPet* mascot = cPlayer->GetCurrentMascot())
		{
			res->byIndex = req->byIndex;

			cPlayer->SetMascotTblidx(INVALID_TBLIDX);
			cPlayer->SetCurrentMascot(NULL);
			mascot->UnSummon();
		}
		else resultcode = MASCOT_WAS_NOT_SUMMONED;
	}
	else resultcode = MASCOT_WAS_NOT_SUMMONED;

	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_MASCOT_UNSUMMON_EX_RES));
	g_pApp->Send(GetHandle(), &packet);

	if (resultcode == GAME_SUCCESS)
	{
		CNtlPacket packet2(sizeof(sGU_UPDATE_MASCOT_SUMMON));
		sGU_UPDATE_MASCOT_SUMMON * res2 = (sGU_UPDATE_MASCOT_SUMMON *)packet2.GetPacketData();
		res2->wOpCode = GU_UPDATE_MASCOT_SUMMON;
		res2->handle = cPlayer->GetID();
		res2->tblidx = cPlayer->GetMascotTblidx();
		packet2.SetPacketLen(sizeof(sGU_UPDATE_MASCOT_SUMMON));
		cPlayer->Broadcast(&packet2, cPlayer);
	}
}
//--------------------------------------------------------------------------------------//
//		
//--------------------------------------------------------------------------------------//
void CClientSession::RecvMascotDelReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_MASCOT_DELETE_REQ_EX * req = (sUG_MASCOT_DELETE_REQ_EX *)pPacket->GetPacketData();

	cPlayer->DeleteMascot(req->byIndex);
}
//--------------------------------------------------------------------------------------//
//		
//--------------------------------------------------------------------------------------//
void CClientSession::RecvMascotRingMaterialDisassembleReq(CNtlPacket * pPacket)
{
	sUG_MASCOT_RING_MATERIAL_DISASSEMBLE_REQ * req = (sUG_MASCOT_RING_MATERIAL_DISASSEMBLE_REQ *)pPacket->GetPacketData();

}
//--------------------------------------------------------------------------------------//
//		
//--------------------------------------------------------------------------------------//
void CClientSession::RecvMascotRingActionEndReq(CNtlPacket * pPacket)
{
	sUG_MASCOT_RING_ACTION_END_REQ * req = (sUG_MASCOT_RING_ACTION_END_REQ *)pPacket->GetPacketData();

	//printf("sUG_MASCOT_RING_ACTION_END_REQ: byRingPartsType=%u\n", req->byRingPartsType);

	CNtlPacket packet(sizeof(sGU_MASCOT_RING_ACTION_END_RES));
	sGU_MASCOT_RING_ACTION_END_RES * res = (sGU_MASCOT_RING_ACTION_END_RES*)packet.GetPacketData();
	res->wOpCode = GU_MASCOT_RING_ACTION_END_RES;
	res->byRingPartsType = req->byRingPartsType;
	packet.SetPacketLen(sizeof(sGU_MASCOT_RING_ACTION_END_RES));
	g_pApp->Send(GetHandle(), &packet);
}
//--------------------------------------------------------------------------------------//
//		fusion(level up) pet
//--------------------------------------------------------------------------------------//
void CClientSession::RecvMascotFusionReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_MASCOT_FUSION_REQ * req = (sUG_MASCOT_FUSION_REQ *)pPacket->GetPacketData();

	cPlayer->FusionMascot(req->byItemPlace, req->byItemPos, req->byMascotLevelUpSlot, req->byMascotOfferingSlot);
}
//--------------------------------------------------------------------------------------//
//		
//--------------------------------------------------------------------------------------//
void CClientSession::RecvMascotSkillAddReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_MASCOT_SKILL_ADD_REQ * req = (sUG_MASCOT_SKILL_ADD_REQ *)pPacket->GetPacketData();
	
	if (cPlayer->GetCurrentMascot())
		cPlayer->GetCurrentMascot()->LearnRandomSkill(req->bySkillSlot);
}
//--------------------------------------------------------------------------------------//
//		
//--------------------------------------------------------------------------------------//
void CClientSession::RecvMascotSkillUpdateReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_MASCOT_SKILL_UPDATE_REQ * req = (sUG_MASCOT_SKILL_UPDATE_REQ *)pPacket->GetPacketData();

	if (cPlayer->GetCurrentMascot())
		cPlayer->GetCurrentMascot()->UpdateSkill(req->bySkillSlot, req->byItemPlace, req->byItemPos);
}
//--------------------------------------------------------------------------------------//
//		
//--------------------------------------------------------------------------------------//
void CClientSession::RecvMascotSkillUpgradeReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_MASCOT_SKILL_UPGRADE_REQ * req = (sUG_MASCOT_SKILL_UPGRADE_REQ *)pPacket->GetPacketData();

	if (cPlayer->GetCurrentMascot())
		cPlayer->GetCurrentMascot()->UpgradeSkill(req->bySkillSlot, req->byItemPlace, req->byItemPos);
}
//--------------------------------------------------------------------------------------//
//		MASCOT USE SKILL
//--------------------------------------------------------------------------------------//
void CClientSession::RecvMascotSkillReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_MASCOT_SKILL_REQ * req = (sUG_MASCOT_SKILL_REQ *)pPacket->GetPacketData();

	WORD wResultcode = GAME_FAIL;
	DWORD dwNewVP = 0;

	sMASCOT_DATA_EX* pData = NULL;

	if (!cPlayer->GetCurrentMascot())
		wResultcode = MASCOT_WAS_NOT_SUMMONED;
	else if (req->bySkillSlot >= DBO_MASCOT_MAX_SKILL_COUNT)
		wResultcode = GAME_SKILL_NOT_FOUND;

	if (wResultcode == GAME_FAIL)
	{
		pData = cPlayer->GetCurrentMascot()->GetMascotData();

		if (!cPlayer->GetCurrentMascot()->IsSkillOnCooldown(req->bySkillSlot))
		{
			sSKILL_TBLDAT* pSkill = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData(pData->skillTblidx[req->bySkillSlot]);
			if (pSkill)
			{
				if (pData->dwCurVP >= pSkill->dwRequire_VP)
				{
				//	printf("mascot skill tblidx %u dwRequire_VP %u byUse_Type %u dwFunction_Bit_Flag %u skill_Effect %u bySkill_Effect_Type %u\n", pSkill->tblidx, pSkill->dwRequire_VP, pSkill->byUse_Type, pSkill->dwFunction_Bit_Flag, pSkill->skill_Effect[0], pSkill->bySkill_Effect_Type[0]);
					sSYSTEM_EFFECT_TBLDAT* pEffect = (sSYSTEM_EFFECT_TBLDAT*)g_pTableContainer->GetSystemEffectTable()->FindData(pSkill->skill_Effect[0]);
					if (pEffect)
					{
						switch (pEffect->effectCode)
						{
							case ACTIVE_AUTO_LOOTING:
							{
								wResultcode = GAME_SUCCESS;

								cPlayer->GetCurrentMascot()->SetCanLoot(true);
							}
							break;
							case ACTIVE_NPC_TELEPORT:
							{
								if (cPlayer->GetCurWorld() && cPlayer->GetCurWorld()->GetTbldat()->bDynamic == false && cPlayer->GetCombatMode() == false && cPlayer->GetDragonballScrambleBallFlag() == 0)
								{
									CGameServer* app = (CGameServer*)g_pApp;

									CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld(req->worldTblidx);
									if (pWorld && pWorld->GetTbldat()->bDynamic == false)
									{
										CTriggerObject* obj = pWorld->FindStaticObjectByIdx(req->targetTblidx);
										if (obj)
										{
											wResultcode = GAME_SUCCESS;

											CNtlVector vDestLoc(obj->GetCurLoc());
											vDestLoc.x += 2.0f;

											cPlayer->StartTeleport(vDestLoc, cPlayer->GetCurDir(), req->worldTblidx, TELEPORT_TYPE_SKILL);

											break;
										}
										else
										{
											CNpc* pNpc = pWorld->FindNpc(req->targetTblidx);
											if (pNpc)
											{
												wResultcode = GAME_SUCCESS;

												CNtlVector vDestLoc(pNpc->GetCurLoc());
												vDestLoc.x += 2.0f;

												cPlayer->StartTeleport(vDestLoc, cPlayer->GetCurDir(), req->worldTblidx, TELEPORT_TYPE_SKILL);

												break;
											}
										}
									}
									else wResultcode = GAME_WORLD_NOT_EXIST;
								}
								else wResultcode = GAME_CAN_NOT_TELEPORT;
							}
							break;
							case ACTIVE_PARTY_SUMMON:
							{
								if (cPlayer->GetParty())
								{
									if (cPlayer->GetCurWorld() && cPlayer->GetCurWorld()->GetTbldat()->bDynamic == false)
									{
										wResultcode = GAME_SUCCESS;
										cPlayer->GetParty()->StartPartyTeleport(cPlayer);
									}
									else wResultcode = GAME_FAIL;
								}
								else wResultcode = GAME_COMMON_YOU_ARE_NOT_IN_A_PARTY;
							}
							break;
							case ACTIVE_GUILD_SUMMON:
							{
								if (cPlayer->GetGuildID() != 0)
								{
									if (cPlayer->GetCurWorld() && cPlayer->GetCurWorld()->GetTbldat()->bDynamic == false)
									{
										wResultcode = GAME_SUCCESS;
										g_pGuildManager->GuildTeleportProposal(cPlayer, cPlayer->GetGuildID());
									}
									else wResultcode = GAME_FAIL;
								}
								else wResultcode = GAME_GUILD_NO_GUILD_FOUND;
							}
							break;
							case PASSIVE_SELF_RESCUE:
							{
								if (cPlayer->IsFainting())
								{
									if (cPlayer->GetCurWorld())
									{
										if (cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_RANKBATTLE || cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_MINORMATCH || cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_MAJORMATCH || cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_FINALMATCH || cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_CCBATTLEDUNGEON)
											wResultcode = GAME_FAIL;
										else
										{
											wResultcode = GAME_SUCCESS;
											cPlayer->Revival(cPlayer->GetCurLoc(), cPlayer->GetWorldID(), REVIVAL_TYPE_CURRENT_POSITION, TELEPORT_TYPE_SKILL);
											cPlayer->UpdateCurLpEp(cPlayer->GetLastMaxLP() * (int)pSkill->aSkill_Effect_Value[0] / 100, cPlayer->GetLastMaxEP() * (WORD)pSkill->aSkill_Effect_Value[1] / 100, true, false);
										}
									}
									else wResultcode = GAME_FAIL;
								}
								else wResultcode = GAME_FAIL;
							}
							break;
							case ACTIVE_REMOTE_SELL:
							{
								wResultcode = GAME_SUCCESS;

								cPlayer->GetCurrentMascot()->SetCanRemoteSell(true);
							}
							break;
							case ACTIVE_REMOTE_WAREHOUSE:
							{
								if (cPlayer->IsUsingBank() == true)	//check if has the bank open
									wResultcode = GAME_FAIL;
								else
								{
									wResultcode = GAME_SUCCESS;

									cPlayer->SetRemoteBankSkillUsed(true);

									if (cPlayer->IsBankLoaded())
									{
										CNtlPacket packet2(sizeof(sGU_BANK_LOAD_RES));
										sGU_BANK_LOAD_RES * res2 = (sGU_BANK_LOAD_RES *)packet2.GetPacketData();
										res2->wOpCode = GU_BANK_LOAD_RES;
										res2->handle = cPlayer->GetID();
										res2->wResultCode = wResultcode;
										packet2.SetPacketLen(sizeof(sGU_BANK_LOAD_RES));
										g_pApp->Send(GetHandle(), &packet2);
									}
									else
									{
										CGameServer* app = (CGameServer*)g_pApp;

										CNtlPacket packet2(sizeof(sGQ_LOAD_PC_BANK_DATA_REQ));
										sGQ_LOAD_PC_BANK_DATA_REQ * res2 = (sGQ_LOAD_PC_BANK_DATA_REQ *)packet2.GetPacketData();
										res2->wOpCode = GQ_LOAD_PC_BANK_DATA_REQ;
										res2->accountId = cPlayer->GetAccountID();
										res2->charId = cPlayer->GetCharID();
										res2->handle = cPlayer->GetID();
										res2->npchandle = INVALID_HOBJECT;
										packet2.SetPacketLen(sizeof(sGQ_LOAD_PC_BANK_DATA_REQ));
										app->SendTo(app->GetQueryServerSession(), &packet2);
									}
								}
							}
							break;
							default:
							{
								ERR_LOG(LOG_USER, "ERROR mascot skill effectcode %u not set. Skill tblidx %u", pEffect->effectCode, pSkill->tblidx);
							}
							break;
						}

						if (wResultcode == GAME_SUCCESS)
						{
							pData->dwCurVP = dwNewVP = pData->dwCurVP - pSkill->dwRequire_VP;

							cPlayer->GetCurrentMascot()->SetSkillCooldown(req->bySkillSlot, pSkill->dwCoolTimeInMilliSecs);
						}

					}
					else
					{
						wResultcode = GAME_SKILL_NOT_FOUND;
						ERR_LOG(LOG_GENERAL, "ERROR effect tbldat not found. Tblidx %u Skill tblidx %u", pSkill->skill_Effect[0], pSkill->tblidx);
					}
				}
				else wResultcode = MASCOT_NOT_ENOUGH_SP;
			}
			else
			{
				wResultcode = GAME_SKILL_NOT_FOUND;
				ERR_LOG(LOG_GENERAL, "ERROR skill tbldat not found. Skill tblidx %u", pData->skillTblidx[req->bySkillSlot]);
			}
		}
		else wResultcode = GAME_SKILL_NOT_READY_TO_BE_CAST;
	}

	CNtlPacket packet(sizeof(sGU_MASCOT_SKILL_RES));
	sGU_MASCOT_SKILL_RES * res = (sGU_MASCOT_SKILL_RES*)packet.GetPacketData();
	res->wOpCode = GU_MASCOT_SKILL_RES;
	res->wResultCode = wResultcode;
	res->bySkillSlot = req->bySkillSlot;
	res->dwNewCurVP = dwNewVP;
	packet.SetPacketLen(sizeof(sGU_MASCOT_SKILL_RES));
	g_pApp->Send(GetHandle(), &packet);
}
//--------------------------------------------------------------------------------------//
//		
//--------------------------------------------------------------------------------------//
void CClientSession::RecvMascotSealSetReq(CNtlPacket * pPacket)
{
	sUG_MASCOT_SEAL_SET_REQ * req = (sUG_MASCOT_SEAL_SET_REQ *)pPacket->GetPacketData();

	WORD wResultCode = GAME_FAIL;

	CItem* pSealItem = cPlayer->GetPlayerItemContainer()->GetItem(req->bySealPlace, req->bySealPos);
	CItemPet* pMascot = cPlayer->GetMascot(req->byMascotPos);

	if (pSealItem == NULL || pSealItem->GetCount() == 0 || IsInvenContainer(pSealItem->GetPlace()) == false)
		wResultCode = GAME_ITEM_NOT_FOUND;
	else if (pSealItem->IsLocked())
		wResultCode = GAME_ITEM_IS_LOCK;
	else if(pSealItem->GetTbldat()->byItem_Type != ITEM_TYPE_EVENT_SEAL)
		wResultCode = GAME_ITEM_CANNOT_EXTRACT_SEAL_INVALID_ITEM;
	else if (pMascot == NULL)
		wResultCode = MASCOT_NOT_EXIST;
	else if (pMascot == cPlayer->GetCurrentMascot())
		wResultCode = GAME_FAIL;
	else if(pMascot->GetTbldat()->byRank > pSealItem->GetCount())
		wResultCode = GAME_ITEM_CANNOT_SEAL_MORE_SEALITEM;
	else if(pMascot->GetTbldat()->tblidx == 6000013 || pMascot->GetTbldat()->tblidx == 6000014 || pMascot->GetTbldat()->tblidx == 6000015 || pMascot->GetTbldat()->tblidx == 6000016 // mascot from shenron
		|| pMascot->GetTbldat()->tblidx == 6000047 || pMascot->GetTbldat()->tblidx == 6000048 || pMascot->GetTbldat()->tblidx == 6000049 || pMascot->GetTbldat()->tblidx == 6000050) // founder mascot
		wResultCode = GAME_FAIL;
	else
	{
		std::pair<BYTE, BYTE> inventorySlot = cPlayer->GetPlayerItemContainer()->GetEmptyInventory();
		if (inventorySlot.first != INVALID_BYTE)
		{
			sITEM_TBLDAT* pItemTbldat = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(pMascot->GetTbldat()->tblidx);
			if (pItemTbldat)
			{
				BYTE bySealStack = pSealItem->GetCount() - pMascot->GetTbldat()->byRank;

				if (cPlayer->OnlyDeleteMascot(req->byMascotPos) == true) //pMascot gets deleted here
				{
					cPlayer->GetPlayerItemContainer()->AddReservedInventory(inventorySlot.first, inventorySlot.second); // reserve inventory slot

					if (bySealStack > 0)
						pSealItem->SetLocked(true);

					//send packet to query server

					CGameServer* app = (CGameServer*)g_pApp;

					CNtlPacket packet2(sizeof(sGQ_MASCOT_SEAL_SET_REQ));
					sGQ_MASCOT_SEAL_SET_REQ * res2 = (sGQ_MASCOT_SEAL_SET_REQ *)packet2.GetPacketData();
					res2->wOpCode = GQ_MASCOT_SEAL_SET_REQ;
					res2->handle = cPlayer->GetID();
					res2->charId = cPlayer->GetCharID();
					res2->byMascotPos = req->byMascotPos;
					res2->bySealPlace = req->bySealPlace;
					res2->bySealPos = req->bySealPos;
					res2->SealitemId = pSealItem->GetItemID();
					res2->bySealStack = pMascot->GetTbldat()->byRank;

					res2->sNewItemData.Init();
					res2->sNewItemData.itemNo = pItemTbldat->tblidx;
					res2->sNewItemData.charId = cPlayer->GetCharID();
					res2->sNewItemData.byPlace = inventorySlot.first;
					res2->sNewItemData.byPosition = inventorySlot.second;
					res2->sNewItemData.byStackcount = 1;
					res2->sNewItemData.byRank = pItemTbldat->byRank;
					res2->sNewItemData.byCurrentDurability = pItemTbldat->byDurability;
					res2->sNewItemData.bNeedToIdentify = false;
					res2->sNewItemData.byGrade = ITEM_GRADE_LEVEL_0;
					res2->sNewItemData.byBattleAttribute = pItemTbldat->byBattle_Attribute;
					res2->sNewItemData.byRestrictState = GetDefaultRestrictState(pItemTbldat->byRestrictType, pItemTbldat->byItem_Type, true);
					res2->sNewItemData.byDurationType = pItemTbldat->byDurationType;

					packet2.SetPacketLen(sizeof(sGQ_MASCOT_SEAL_SET_REQ));
					app->SendTo(app->GetQueryServerSession(), &packet2);


					pSealItem->SetCount(bySealStack, false, false, false); // might delete pSealItem here

					return;
				}
			}
			else wResultCode = GAME_COMMON_CAN_NOT_FIND_TABLE_DATA;
		}
		else wResultCode = GAME_ITEM_INVEN_FULL;
	}


	CNtlPacket packet(sizeof(sGU_MASCOT_SEAL_SET_RES));
	sGU_MASCOT_SEAL_SET_RES * res = (sGU_MASCOT_SEAL_SET_RES*)packet.GetPacketData();
	res->wOpCode = GU_MASCOT_SEAL_SET_RES;
	res->wResultCode = wResultCode;
	packet.SetPacketLen(sizeof(sGU_MASCOT_SEAL_SET_RES));
	g_pApp->Send(GetHandle(), &packet);
}

void CClientSession::RecvMascotSealClearReq(CNtlPacket * pPacket)
{
	sUG_MASCOT_SEAL_CLEAR_REQ * req = (sUG_MASCOT_SEAL_CLEAR_REQ *)pPacket->GetPacketData();

	printf("%u %u \n", req->byItemPlace, req->byItemPos);
}



void CClientSession::RecvMascotAutoLootingReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_MASCOT_AUTO_LOOTING_REQ * req = (sUG_MASCOT_AUTO_LOOTING_REQ *)pPacket->GetPacketData();

	WORD wRes = GAME_SUCCESS;
	int nCount = 0;

	if (cPlayer->IsTrading())
		wRes = GAME_LOOTING_FAIL;
	else if(cPlayer->HasPrivateShop())
		wRes = GAME_LOOTING_FAIL;
	else if (req->byItemCount == 0 || req->byItemCount > DBO_MASCOT_MAX_LOOT)
		wRes = GAME_LOOTING_FAIL;
	else if(cPlayer->GetCurrentMascot() == NULL || cPlayer->GetCurrentMascot()->CanLoot() == false)
		wRes = MASCOT_WAS_NOT_SUMMONED;
	else if(cPlayer->IsFainting())
		wRes = GAME_CHAR_IS_WRONG_STATE;
	else
	{
		cPlayer->GetCurrentMascot()->SetCanLoot(false);

		for (int i = 0; i < req->byItemCount; i++)
		{
			CItemDrop* item = g_pItemManager->FindDrop(req->ahHandle[i]);
			if (item && item->IsInitialized())
			{
				//check if still on ground
				if (item->GetCurWorld() == NULL)
					continue;

				//check distance
				if (cPlayer->IsInRange(item, NTL_MAX_LOOTING_DISTANCE * 2.f) == false)
					continue;

				//check ownership
				if (item->IsOwnership(cPlayer) == false)
					continue;

				switch (item->GetObjType())
				{
					case OBJTYPE_DROPMONEY: item->PickUpZeni(cPlayer); nCount++; break;
					case OBJTYPE_DROPITEM: item->PickUpItem(cPlayer); nCount++; break;

					default: break;
				}
			}
		}
	}

	if(nCount == 0)
		wRes = GAME_LOOTING_FAIL;

	CNtlPacket packet(sizeof(sGU_MASCOT_AUTO_LOOTING_RES));
	sGU_MASCOT_AUTO_LOOTING_RES * res = (sGU_MASCOT_AUTO_LOOTING_RES *)packet.GetPacketData();
	res->wOpCode = GU_MASCOT_AUTO_LOOTING_RES;
	res->wResultCode = wRes;
	packet.SetPacketLen(sizeof(sGU_MASCOT_AUTO_LOOTING_RES));
	g_pApp->Send(GetHandle(), &packet);
}

void CClientSession::RecvMascotRemoteShopSellReq(CNtlPacket * pPacket)
{
	CGameServer* app = (CGameServer*)g_pApp;

	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_MASCOT_REMOTE_SHOP_SELL_REQ * req = (sUG_MASCOT_REMOTE_SHOP_SELL_REQ *)pPacket->GetPacketData();

	WORD resultcode = GAME_SUCCESS;
	DWORD dwZeni = 0;
	TBLIDX itemidx = 0;

	//dont allow sell item while trading or while having private shop
	if (cPlayer->IsTrading() || cPlayer->HasPrivateShop())
	{
		resultcode = GAME_FAIL;
	}
	else if(req->bySellCount == 0 || req->bySellCount > NTL_MAX_SELL_SHOPPING_CART)
		resultcode = GAME_FAIL;
	else if (cPlayer->GetCurrentMascot() == NULL || cPlayer->GetCurrentMascot()->CanRemoteSell() == false)
		resultcode = MASCOT_WAS_NOT_SUMMONED;
	else
	{
		cPlayer->GetCurrentMascot()->SetCanRemoteSell(false);

		CNtlPacket pQry(sizeof(sGQ_SHOP_SELL_REQ));
		sGQ_SHOP_SELL_REQ* rQry = (sGQ_SHOP_SELL_REQ*)pQry.GetPacketData();
		rQry->wOpCode = GQ_SHOP_SELL_REQ;
		rQry->charId = cPlayer->GetCharID();
		rQry->handle = cPlayer->GetID();
		rQry->hNpchandle = INVALID_HOBJECT;

		for (int i = 0; i < req->bySellCount; i++)
		{
			CItem* pItem = cPlayer->GetPlayerItemContainer()->GetItem(req->sSellData[i].byPlace, req->sSellData[i].byPos);
			if (pItem && pItem->GetCount() > 0 && IsInvenContainer(pItem->GetPlace()))
			{
				if (pItem->CanSell())
				{
					if (req->sSellData[i].byStack > 0 && pItem->GetCount() >= req->sSellData[i].byStack)
					{
						itemidx = pItem->GetTblidx();

						rQry->sInven[rQry->bySellCount].byPlace = pItem->GetPlace();
						rQry->sInven[rQry->bySellCount].byPos = pItem->GetPos();
						rQry->sInven[rQry->bySellCount].byStack = req->sSellData[i].byStack;
						rQry->sInven[rQry->bySellCount++].itemID = pItem->GetItemID();

						pItem->SetCount(pItem->GetCount() - req->sSellData[i].byStack, false, false);
						dwZeni += pItem->GetTbldat()->dwSell_Price * req->sSellData[i].byStack;
					}
				}
			}
		}

		if (dwZeni > 0 && cPlayer->CanReceiveZeni(dwZeni))
		{
			DWORD dwFee = (DWORD)(dwZeni * 10.0f / 100.0f); // fee 10%
			dwZeni = UnsignedSafeDecrease<DWORD>(dwZeni, dwFee);
			cPlayer->UpdateZeni(ZENNY_CHANGE_TYPE_ITEM_SELL, dwZeni, true, false);

		//	ERR_LOG(LOG_USER, "Player %u sell items for zeni: %u. Last itemidx: %u", cPlayer->GetCharID(), dwZeni, itemidx);
		}
		else
			dwZeni = 0;

		rQry->dwMoney = dwZeni;
		pQry.SetPacketLen(sizeof(sGQ_SHOP_SELL_REQ));
		app->SendTo(app->GetQueryServerSession(), &pQry);
	}

	CNtlPacket packet(sizeof(sGU_MASCOT_REMOTE_SHOP_SELL_RES));
	sGU_MASCOT_REMOTE_SHOP_SELL_RES * res = (sGU_MASCOT_REMOTE_SHOP_SELL_RES *)packet.GetPacketData();
	res->wOpCode = GU_MASCOT_REMOTE_SHOP_SELL_RES;
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_MASCOT_REMOTE_SHOP_SELL_RES));
	app->Send(GetHandle(), &packet);
}

//--------------------------------------------------------------------------------------//
//		ENTER LAVA FIELD
//--------------------------------------------------------------------------------------//
void CClientSession::RecvLavaEnteredNfy(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	cPlayer->EnterLava();
}

//--------------------------------------------------------------------------------------//
//		LEAVE LAVA FIELD
//--------------------------------------------------------------------------------------//
void CClientSession::RecvLavaLeftNfy(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	cPlayer->LeaveLava();
}

//--------------------------------------------------------------------------------------//
//		ENTER TIME QUEST DUNGEON REQUEST
//--------------------------------------------------------------------------------------//
void CClientSession::RecvTimeQuestEnterReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_TIMEQUEST_ENTER_REQ * req = (sUG_TIMEQUEST_ENTER_REQ *)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_TIMEQUEST_ENTER_RES));
	sGU_TIMEQUEST_ENTER_RES * res = (sGU_TIMEQUEST_ENTER_RES *)packet.GetPacketData();
	res->wOpCode = GU_TIMEQUEST_ENTER_RES;
	res->wResultCode = GAME_SUCCESS;
	res->byDifficult = req->byDifficult;
	res->hTimeQuestNpc = req->hTimeQuestNpc;

	CNpc* pNpc = g_pObjectManager->GetNpc(req->hTimeQuestNpc);
	if (pNpc == NULL)
		res->wResultCode = GAME_TARGET_NOT_FOUND;
	else if (pNpc->IsInRange(cPlayer, DBO_DISTANCE_CHECK_TOLERANCE) == false)
		res->wResultCode = GAME_TARGET_TOO_FAR;
	else if(req->byDifficult >= MAX_TIMEQUEST_DIFFICULTY)
		res->wResultCode = GAME_FAIL;
	else if(pNpc->GetTbldat()->byJob != NPC_JOB_TIMEMACHINE_MERCHANT)
		res->wResultCode = GAME_TARGET_HAS_DIFFERENT_JOB;
	else if(cPlayer->GetPartyID() == INVALID_PARTYID)
		res->wResultCode = GAME_PARTY_YOU_ARE_NOT_IN_PARTY;
	else if (cPlayer->GetParty()->GetPartyLeaderID() != cPlayer->GetID())
		res->wResultCode = GAME_PARTY_ONLY_ALLOWED_TO_PARTY_LEADER;
	else if(cPlayer->GetParty()->IsSomeoneInDynamic(INVALID_WORLDID))
		res->wResultCode = GAME_PARTYMATCHING_ANY_MEMBER_IN_DYNAMIC_WORLD;
	else if(app->IsDojoChannel() == true)
		res->wResultCode = GAME_FAIL;
	else
	{
		res->wResultCode = g_pDungeonManager->CreateTimeQuest(cPlayer, pNpc->GetTbldat()->amerchant_Tblidx[0], req->byDifficult, TIMEQUEST_MODE_PARTY);
	}

	packet.SetPacketLen(sizeof(sGU_TIMEQUEST_ENTER_RES));
	g_pApp->Send(GetHandle(), &packet);
}



//--------------------------------------------------------------------------------------//
//		SORT INVENTORY REQUEST
//--------------------------------------------------------------------------------------//
void CClientSession::RecvInventorySortReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_INVENTORY_SORT_REQ * req = (sUG_INVENTORY_SORT_REQ *)pPacket->GetPacketData();

	/* req->byInventoryType
		- 0 = INVENTORY
		- 1 = INVENTORY
		- 2 = PRIVAE BANK
		- 3 = Guild Bank
	*/

	cPlayer->GetPlayerItemContainer()->SortInventory(req->byInventoryType, req->hNpcHandle);
}


//--------------------------------------------------------------------------------------//
//		GM Command
//--------------------------------------------------------------------------------------//
void CClientSession::RecvServerCommand(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_SERVER_COMMAND * pServerCmd = (sUG_SERVER_COMMAND*)pPacket->GetPacketData();

	gm_read_command(pServerCmd, cPlayer);
}

//--------------------------------------------------------------------------------------//
//		show/hide costume
//--------------------------------------------------------------------------------------//
void CClientSession::RecvInvisibleCostumeUpdateReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_INVISIBLE_COSTUME_UPDATE_REQ * req = (sUG_INVISIBLE_COSTUME_UPDATE_REQ*)pPacket->GetPacketData();

	if (cPlayer->GetCostumeInvisible() != req->bInvisibleCostume)
	{
		cPlayer->SetCostumeInvisible(req->bInvisibleCostume);

		CNtlPacket packet(sizeof(sGU_INVISIBLE_COSTUME_UPDATE_RES));
		sGU_INVISIBLE_COSTUME_UPDATE_RES * res = (sGU_INVISIBLE_COSTUME_UPDATE_RES *)packet.GetPacketData();
		res->wOpCode = GU_INVISIBLE_COSTUME_UPDATE_RES;
		res->bInvisibleCostume = req->bInvisibleCostume;
		res->wResultCode = GAME_SUCCESS;
		packet.SetPacketLen(sizeof(sGU_INVISIBLE_COSTUME_UPDATE_RES));
		app->Send(GetHandle(), &packet);

		// this can crash the client
		// broadcast to others
		CNtlPacket packet2(sizeof(sGU_INVISIBLE_COSTUME_UPDATE_NFY));
		sGU_INVISIBLE_COSTUME_UPDATE_NFY * res2 = (sGU_INVISIBLE_COSTUME_UPDATE_NFY *)packet2.GetPacketData();
		res2->wOpCode = GU_INVISIBLE_COSTUME_UPDATE_NFY;
		res2->bInvisibleCostume = req->bInvisibleCostume;
		res2->hHandle = cPlayer->GetID();
		packet2.SetPacketLen(sizeof(sGU_INVISIBLE_COSTUME_UPDATE_NFY));
		cPlayer->Broadcast(&packet2, cPlayer);


		CNtlPacket pQry(sizeof(sGQ_INVISIBLE_COSTUME_UPDATE_REQ));
		sGQ_INVISIBLE_COSTUME_UPDATE_REQ * qRes = (sGQ_INVISIBLE_COSTUME_UPDATE_REQ *)pQry.GetPacketData();
		qRes->wOpCode = GQ_INVISIBLE_COSTUME_UPDATE_REQ;
		qRes->charId = cPlayer->GetCharID();
		qRes->bInvisibleCostume = req->bInvisibleCostume;
		pQry.SetPacketLen(sizeof(sGQ_INVISIBLE_COSTUME_UPDATE_REQ));
		app->SendTo(app->GetQueryServerSession(), &pQry);
	}
}


void CClientSession::RecvBudokaiJoinInfoReq(CNtlPacket * pPacket)
{
	sUG_BUDOKAI_JOIN_INFO_REQ * req = (sUG_BUDOKAI_JOIN_INFO_REQ*)pPacket->GetPacketData();


}

void CClientSession::RecvBudokaiJoinStateReq(CNtlPacket * pPacket)
{
	sUG_BUDOKAI_JOIN_STATE_REQ * req = (sUG_BUDOKAI_JOIN_STATE_REQ*)pPacket->GetPacketData();

	g_pBudokaiManager->JoinStateReq(cPlayer);
}

void CClientSession::RecvBudokaiMudosaInfoReq(CNtlPacket * pPacket)
{
	sUG_BUDOKAI_MUDOSA_INFO_REQ * req = (sUG_BUDOKAI_MUDOSA_INFO_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_BUDOKAI_MUDOSA_INFO_RES));
	sGU_BUDOKAI_MUDOSA_INFO_RES * res = (sGU_BUDOKAI_MUDOSA_INFO_RES*)packet.GetPacketData();
	res->wOpCode = GU_BUDOKAI_MUDOSA_INFO_RES;
	res->aMudosaInfo[0].wCurrentUserCount = 0;
	NTL_SAFE_WCSCPY(res->aMudosaInfo[0].wszMudosaName, L"Mudosa");
	res->byMudosaCount = 1;
	g_pApp->Send(GetHandle(), &packet);
}

void CClientSession::RecvBudokaiMudosaTeleportReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_BUDOKAI_MUDOSA_TELEPORT_REQ * req = (sUG_BUDOKAI_MUDOSA_TELEPORT_REQ*)pPacket->GetPacketData();
	UNREFERENCED_PARAMETER(req);

	CNtlPacket packet(sizeof(sGU_BUDOKAI_MUDOSA_TELEPORT_RES));
	sGU_BUDOKAI_MUDOSA_TELEPORT_RES * res = (sGU_BUDOKAI_MUDOSA_TELEPORT_RES*)packet.GetPacketData();
	res->wOpCode = GU_BUDOKAI_MUDOSA_TELEPORT_RES;
	res->wResultCode = GAME_SUCCESS;

	sWORLD_TBLDAT* pWorld = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData(200);
	if (pWorld == NULL)
		res->wResultCode = GAME_WORLD_NOT_FOUND;
	else if(cPlayer->GetZeni() < 200000) //200k fee
		res->wResultCode = GAME_ZENNY_NOT_ENOUGH;

	g_pApp->Send(GetHandle(), &packet);
	
	if (res->wResultCode == GAME_SUCCESS)
	{
		cPlayer->UpdateZeni(ZENNY_CHANGE_TYPE_PORTAL_ADD, 200000, false, true);
		cPlayer->StartTeleport(pWorld->vDefaultLoc, cPlayer->GetCurDir(), 200, TELEPORT_TYPE_BUDOKAI);
	}
}

void CClientSession::RecvBudokaiPartyMakerReq(CNtlPacket * pPacket)
{
	sUG_BUDOKAI_PARTY_MAKER_REQ * req = (sUG_BUDOKAI_PARTY_MAKER_REQ*)pPacket->GetPacketData();


}

//-------------------------------------------------
//     SEND BUDOKAI SOCIAL ACTION SKILL
//-------------------------------------------------
void CClientSession::RecvBudokaiSocialAction(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_BUDOKAI_SOCIAL_ACTION * req = (sUG_BUDOKAI_SOCIAL_ACTION*)pPacket->GetPacketData();

	if (cPlayer->GetCharStateID() == CHARSTATE_SITTING)
		return;

	if (app->GetCurTickCount() < m_dwLastActionUse + 4000) // only allow every 4 seconds
	{
		return;
	}

	m_dwLastActionUse = app->GetCurTickCount();

	sACTION_TBLDAT* pActionTbldat = (sACTION_TBLDAT*)g_pTableContainer->GetActionTable()->FindData(req->socialAction);
	if (pActionTbldat == NULL)
	{
		ERR_LOG(LOG_HACK, "Player: %u could not play social-action: %u", cPlayer->GetCharID(), req->socialAction);
		return;
	}

	if (pActionTbldat->byAction_Type != ACTION_TYPE_SOCIAL_ACTION)
		return;

	CNtlPacket packet(sizeof(sGU_BUDOKAI_SOCIAL_ACTION_NFY));
	sGU_BUDOKAI_SOCIAL_ACTION_NFY * res = (sGU_BUDOKAI_SOCIAL_ACTION_NFY*)packet.GetPacketData();
	res->wOpCode = GU_BUDOKAI_SOCIAL_ACTION_NFY;
	res->hSubject = cPlayer->GetID();
	res->socialAction = req->socialAction;
	packet.SetPacketLen(sizeof(sGU_BUDOKAI_SOCIAL_ACTION_NFY));
	cPlayer->Broadcast(&packet, cPlayer);
}

void CClientSession::RecvBudokaiPrizeWinnerNameReq(CNtlPacket * pPacket)
{
	sUG_BUDOKAI_PRIZEWINNER_NAME_REQ * req = (sUG_BUDOKAI_PRIZEWINNER_NAME_REQ*)pPacket->GetPacketData();
	UNREFERENCED_PARAMETER(req);

}

void CClientSession::RecvBudokaiJoinIndividualReq(CNtlPacket * pPacket)
{
	sUG_BUDOKAI_JOIN_INDIVIDUAL_REQ * req = (sUG_BUDOKAI_JOIN_INDIVIDUAL_REQ*)pPacket->GetPacketData();
	UNREFERENCED_PARAMETER(req);

	g_pBudokaiManager->JoinIndividualReq(cPlayer);
}

void CClientSession::RecvBudokaiLeaveIndividualReq(CNtlPacket * pPacket)
{
	sUG_BUDOKAI_LEAVE_INDIVIDUAL_REQ * req = (sUG_BUDOKAI_LEAVE_INDIVIDUAL_REQ*)pPacket->GetPacketData();
	UNREFERENCED_PARAMETER(req);

	g_pBudokaiManager->LeaveIndividualReq(cPlayer);
}

void CClientSession::RecvBudokaiJoinTeamInfoReq(CNtlPacket * pPacket)
{
	sUG_BUDOKAI_JOIN_TEAM_INFO_REQ * req = (sUG_BUDOKAI_JOIN_TEAM_INFO_REQ*)pPacket->GetPacketData();
	
	CNtlPacket packet(sizeof(sGU_BUDOKAI_JOIN_TEAM_INFO_RES));
	sGU_BUDOKAI_JOIN_TEAM_INFO_RES * res = (sGU_BUDOKAI_JOIN_TEAM_INFO_RES *)packet.GetPacketData();
	res->wOpCode = GU_BUDOKAI_JOIN_TEAM_INFO_RES;
	res->wResultCode = GAME_SUCCESS;
	res->byMemberCount = 0;

	CParty* pParty = cPlayer->GetParty();
	if (pParty)
	{
		for (BYTE i = 0; i < pParty->GetPartyMemberCount(); i++)
		{
			CPlayer* pMember = g_pObjectManager->GetPC(pParty->GetMemberInfo(i).hHandle);
			if (pMember)
			{
				res->asPointInfo[res->byMemberCount].byClass = pMember->GetClass();
				NTL_SAFE_WCSCPY(res->asPointInfo[res->byMemberCount].wszName, pMember->GetCharName());
				res->asPointInfo[res->byMemberCount].byLevel = pMember->GetLevel();
				res->asPointInfo[res->byMemberCount].fPoint = pMember->GetRankBattleScoreInfo()->fPoint;

				++res->byMemberCount;
			}
		}
	}
	else res->wResultCode = GAME_PARTY_YOU_ARE_NOT_IN_PARTY;

	packet.SetPacketLen(sizeof(sGU_BUDOKAI_JOIN_TEAM_INFO_RES));
	g_pApp->Send(GetHandle(), &packet);
}

void CClientSession::RecvBudokaiJoinTeamReq(CNtlPacket * pPacket)
{
	sUG_BUDOKAI_JOIN_TEAM_REQ * req = (sUG_BUDOKAI_JOIN_TEAM_REQ*)pPacket->GetPacketData();

	g_pBudokaiManager->JoinTeamReq(cPlayer, req->wszTeamName);
}

void CClientSession::RecvBudokaiLeaveTeamReq(CNtlPacket * pPacket)
{
	sUG_BUDOKAI_LEAVE_TEAM_REQ * req = (sUG_BUDOKAI_LEAVE_TEAM_REQ*)pPacket->GetPacketData();
	UNREFERENCED_PARAMETER(req);

	g_pBudokaiManager->LeaveTeamReq(cPlayer);
}

void CClientSession::RecvBudokaiLeaveTeamMemberReq(CNtlPacket * pPacket)
{
	sUG_BUDOKAI_LEAVE_TEAM_MEMBER_REQ * req = (sUG_BUDOKAI_LEAVE_TEAM_MEMBER_REQ*)pPacket->GetPacketData();
	
	// SEND GAME FAIL BECAUSE WE DO ONLY ALLOW TEAMS WITH FULL PARTY TO JOIN BUDOKAI

	CNtlPacket packet(sizeof(sGU_BUDOKAI_LEAVE_TEAM_MEMBER_RES));
	sGU_BUDOKAI_LEAVE_TEAM_MEMBER_RES * res = (sGU_BUDOKAI_LEAVE_TEAM_MEMBER_RES *)packet.GetPacketData();
	res->wOpCode = GU_BUDOKAI_LEAVE_TEAM_MEMBER_RES;
	res->wResultCode = GAME_FAIL;
	packet.SetPacketLen(sizeof(sGU_BUDOKAI_LEAVE_TEAM_MEMBER_RES));
	g_pApp->Send(GetHandle(), &packet);
}


//--------------------------------------------------------------------------------------//
//		SCS SYSTEM. SEND Q CODE TO CLIENT
//--------------------------------------------------------------------------------------//
void CClientSession::RecvScsCheckStartRes(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_SCS_CHECK_START_RES * req = (sUG_SCS_CHECK_START_RES*)pPacket->GetPacketData();
	printf("RecvScsCheckStartRes: %i\n", req->bRet);

	if (req->bRet == true)
	{
		cPlayer->Remove_event_SCSCheck();
		cPlayer->SetSCSCheckBegin(true);
		cPlayer->SetSCSPreFailCount(0);

		
		g_pScsManager->TestGenerate(cPlayer);
		

	}
	else
	{
		ERR_LOG(LOG_SYSTEM, "RecvScsCheckStartRes RETURN IS FALSE !!!");
		printf("RecvScsCheckStartRes RETURN IS FALSE !!! \n");
	}
}

void CClientSession::RecvScsCheckRes(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_SCS_CHECK_RES * req = (sUG_SCS_CHECK_RES*)pPacket->GetPacketData();
	printf("RecvScsCheckRes: %i %i\n", req->bRet, req->byCount);

	//if we send 15 count in sGU_SCS_CHECK_REQ then we will receive 16 here (+1)
	
	if (req->bRet == true)
	{
		
	}

}

void CClientSession::RecvScsReplyReq(CNtlPacket * pPacket)
{
	sUG_SCS_REPLY_REQ * req = (sUG_SCS_REPLY_REQ*)pPacket->GetPacketData();

	/*
		INFO: data contains numbers from 1-15.
		1 = I_CDE_IMG_01 
		15 = I_CDE_IMG_15
	*/

	for (int i = 0; i < NTL_SCS_IMAGE_REPLY_LENGTH; i++)
	{
		BYTE byte = req->achData[i];
	}
}

void CClientSession::RecvScsRemakeReq(CNtlPacket * pPacket)
{
	sUG_SCS_REMAKE_REQ * req = (sUG_SCS_REMAKE_REQ*)pPacket->GetPacketData();
	

}



//--------------------------------------------------------------------------------------//
//		DOJO CREATE, DELETE & ADD FUNCTION 
//--------------------------------------------------------------------------------------//
void CClientSession::RecvDojoCreateReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_DOJO_CREATE_REQ * req = (sUG_DOJO_CREATE_REQ*)pPacket->GetPacketData();

	CGameServer* app = (CGameServer*)g_pApp;

	WORD resultcode = GAME_SUCCESS;

	CNpc* npc = g_pObjectManager->GetNpc(req->hObject);
	if (npc)
	{
		if (npc->GetTbldat()->byJob == NPC_JOB_DOJO_MANAGER)
		{
			if (cPlayer->IsInRange(npc, DBO_DISTANCE_CHECK_TOLERANCE))
			{
				sDOJO_TBLDAT* dojo = (sDOJO_TBLDAT*)g_pTableContainer->GetDojoTable()->FindData(npc->GetTbldat()->contentsTblidx);
				if (dojo)
				{
					if (app->GetChatServerSession())
					{
						CNtlPacket packet(sizeof(sGT_DOJO_CREATE_REQ));
						sGT_DOJO_CREATE_REQ * res = (sGT_DOJO_CREATE_REQ *)packet.GetPacketData();
						res->wOpCode = GT_DOJO_CREATE_REQ;
						res->charId = cPlayer->GetCharID();
						res->dojoTblidx = dojo->tblidx;
						packet.SetPacketLen(sizeof(sGT_DOJO_CREATE_REQ));
						app->SendTo(app->GetChatServerSession(), &packet);

						return;
					}
					else resultcode = GAME_FAIL;
				}
				else resultcode = GAME_GUILD_DOJO_NOT_FOUND_TABLE;
			}
			else resultcode = GAME_GUILD_DOJO_MANAGER_IS_TOO_FAR;
		}
		else resultcode = GAME_TARGET_HAS_DIFFERENT_JOB;
	}
	else resultcode = GAME_GUILD_NO_DOJO_MANAGER_NPC_FOUND;

	CNtlPacket packet(sizeof(sGU_DOJO_CREATE_RES));
	sGU_DOJO_CREATE_RES * res = (sGU_DOJO_CREATE_RES *)packet.GetPacketData();
	res->wOpCode = GU_DOJO_CREATE_RES;
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_DOJO_CREATE_RES));
	app->Send(GetHandle(), &packet);
}

void CClientSession::RecvDojoDeleteReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_DOJO_DELETE_REQ * req = (sUG_DOJO_DELETE_REQ*)pPacket->GetPacketData();

	WORD resultcode = GAME_SUCCESS;

	CNpc* npc = g_pObjectManager->GetNpc(req->hObject);
	if (npc)
	{
		if (npc->GetTbldat()->byJob == NPC_JOB_DOJO_MANAGER)
		{
			if (cPlayer->IsInRange(npc, DBO_DISTANCE_CHECK_TOLERANCE))
			{
				sDOJO_TBLDAT* dojo = (sDOJO_TBLDAT*)g_pTableContainer->GetDojoTable()->FindData(npc->GetTbldat()->contentsTblidx);
				if (dojo)
				{
					CNtlPacket packet(sizeof(sGT_DOJO_DELETE_REQ));
					sGT_DOJO_DELETE_REQ * res = (sGT_DOJO_DELETE_REQ *)packet.GetPacketData();
					res->wOpCode = GT_DOJO_DELETE_REQ;
					res->charId = cPlayer->GetCharID();
					res->dojoTblidx = dojo->tblidx;
					packet.SetPacketLen(sizeof(sGT_DOJO_DELETE_REQ));
					app->SendTo(app->GetChatServerSession(), &packet);

					return;
				}
				else resultcode = GAME_GUILD_DOJO_NOT_FOUND_TABLE;
			}
			else resultcode = GAME_GUILD_DOJO_MANAGER_IS_TOO_FAR;
		}
		else resultcode = GAME_TARGET_HAS_DIFFERENT_JOB;
	}
	else resultcode = GAME_GUILD_NO_DOJO_MANAGER_NPC_FOUND;

	CNtlPacket packet(sizeof(sGU_DOJO_DELETE_RES));
	sGU_DOJO_DELETE_RES * res = (sGU_DOJO_DELETE_RES *)packet.GetPacketData();
	res->wOpCode = GU_DOJO_DELETE_RES;
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_DOJO_DELETE_RES));
	app->Send(GetHandle(), &packet);
}

void CClientSession::RecvDojoFunctionAddReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_DOJO_FUNCTION_ADD_REQ * req = (sUG_DOJO_FUNCTION_ADD_REQ*)pPacket->GetPacketData();

	WORD resultcode = GAME_SUCCESS;
	DWORD dwNeedZeni = 0;

	CNpc* npc = g_pObjectManager->GetNpc(req->hGuildManagerNpc);
	if (npc)
	{
		if (NtlGetDistance(cPlayer->GetCurLoc(), npc->GetCurLoc()) < 20.0f)
		{
			eDBO_GUILD_FUNCTION eGuildFunction = static_cast<eDBO_GUILD_FUNCTION>(req->byFunction);

			sDOJO_TBLDAT* pDojoTbldat = (sDOJO_TBLDAT*)g_pTableContainer->GetDojoTable()->FindData(npc->GetTbldat()->contentsTblidx);

			if (pDojoTbldat == NULL)
				resultcode = GAME_GUILD_DOJO_NOT_FOUND_TABLE;
			else if (cPlayer->GetGuildID() == 0)
				resultcode = GAME_GUILD_NO_GUILD_FOUND;
			else if (!IsDojoFunction(eGuildFunction))
				resultcode = GAME_GUILD_NON_EXISTING_GUILD_FUNCTION;

			if (resultcode == GAME_SUCCESS)
			{
				dwNeedZeni = Dbo_GetGuildFunctionInfo(eGuildFunction)->dwRequiredZenny;

				if (cPlayer->GetZeni() < dwNeedZeni) //check zeni
					resultcode = GAME_GUILD_NEED_MORE_ZENNY;
			}
		}
		else resultcode = GAME_GUILD_DOJO_MANAGER_IS_TOO_FAR;
	}
	else resultcode = GAME_GUILD_NO_DOJO_MANAGER_NPC_FOUND;

	if (resultcode == GAME_SUCCESS)
	{
		CNtlPacket cPacket(sizeof(sGT_DOJO_FUNCTION_ADD_REQ));
		sGT_DOJO_FUNCTION_ADD_REQ * cRes = (sGT_DOJO_FUNCTION_ADD_REQ *)cPacket.GetPacketData();
		cRes->wOpCode = GT_DOJO_FUNCTION_ADD_REQ;
		cRes->masterCharId = cPlayer->GetCharID();
		cRes->byFunction = req->byFunction;
		cRes->dwZenny = dwNeedZeni;
		cPacket.SetPacketLen(sizeof(sGT_DOJO_FUNCTION_ADD_REQ));
		app->SendTo(app->GetChatServerSession(), &cPacket); //Send to chat server
	}
	else
	{
		CNtlPacket packet(sizeof(sGU_DOJO_FUNCTION_ADD_RES));
		sGU_DOJO_FUNCTION_ADD_RES * res = (sGU_DOJO_FUNCTION_ADD_RES *)packet.GetPacketData();
		res->wOpCode = GU_DOJO_FUNCTION_ADD_RES;
		res->wResultCode = resultcode;
		res->byFunction = req->byFunction;
		packet.SetPacketLen(sizeof(sGU_DOJO_FUNCTION_ADD_RES));
		app->Send(GetHandle(), &packet);
	}
}


void CClientSession::RecvDojoScrambleReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	WORD resultcode = GAME_SUCCESS;
	CGameServer* app = (CGameServer*)g_pApp;

	sUG_DOJO_SCRAMBLE_REQ * req = (sUG_DOJO_SCRAMBLE_REQ*)pPacket->GetPacketData();

	CNpc* pNpc = g_pObjectManager->GetNpc(req->hNpcHandle);

	if (pNpc == NULL)
		resultcode = GAME_GUILD_NO_DOJO_MANAGER_NPC_FOUND;

	//else if (cPlayer->IsGameMaster() == false)
	//	resultcode = GAME_FAIL;

	else if (cPlayer->IsInRange(pNpc, DBO_DISTANCE_CHECK_TOLERANCE) == false)
		resultcode = GAME_GUILD_DOJO_MANAGER_IS_TOO_FAR;
	else if(pNpc->GetTbldat()->byJob != NPC_JOB_DOJO_MANAGER)
		resultcode = GAME_GUILD_NOT_GUILD_MANAGER_NPC;
	else if(cPlayer->GetZeni() < DBO_DOJO_SCRAMBLE_REQUEST_FARE)
		resultcode = GAME_GUILD_DOJO_YOU_ARE_NOT_ENOUGH_ZENNY;
	else if(cPlayer->GetGuildID() == INVALID_GUILDID)
		resultcode = GAME_GUILD_NO_GUILD_FOUND;
	else
	{
		sDOJO_TBLDAT* pDojoTbldat = (sDOJO_TBLDAT*)g_pTableContainer->GetDojoTable()->FindData(pNpc->GetTbldat()->contentsTblidx);
		if (pDojoTbldat)
		{
			CDojo* pDojo = g_pDojoManager->GetDojoWithTblidx(pDojoTbldat->tblidx);
			if (pDojo)
			{
				CNtlPacket packet(sizeof(sGT_DOJO_SCRAMBLE_REQ));
				sGT_DOJO_SCRAMBLE_REQ * res = (sGT_DOJO_SCRAMBLE_REQ *)packet.GetPacketData();
				res->wOpCode = GT_DOJO_SCRAMBLE_REQ;
				res->charId = cPlayer->GetCharID();
				res->dojoTblidx = pDojoTbldat->tblidx;
				res->dwReqZenny = DBO_DOJO_SCRAMBLE_REQUEST_FARE;
				packet.SetPacketLen(sizeof(sGT_DOJO_SCRAMBLE_REQ));
				app->SendTo(app->GetChatServerSession(), &packet);

				return;
			}
			else resultcode = GAME_GUILD_DOJO_NOT_FOUND;
		}
		else resultcode = GAME_GUILD_DOJO_NOT_FOUND_TABLE;
	}
	
	CNtlPacket packet(sizeof(sGU_DOJO_SCRAMBLE_RES));
	sGU_DOJO_SCRAMBLE_RES * res = (sGU_DOJO_SCRAMBLE_RES *)packet.GetPacketData();
	res->wOpCode = GU_DOJO_SCRAMBLE_RES;
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_DOJO_SCRAMBLE_RES));
	app->Send(GetHandle(), &packet);
}


void CClientSession::RecvDojoScrambleResponseReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	WORD resultcode = GAME_SUCCESS;
	CGameServer* app = (CGameServer*)g_pApp;

	sUG_DOJO_SCRAMBLE_RESPONSE_REQ * req = (sUG_DOJO_SCRAMBLE_RESPONSE_REQ*)pPacket->GetPacketData();

	if (cPlayer->GetGuildID() == INVALID_GUILDID)
		resultcode = GAME_GUILD_NO_GUILD_FOUND;
	else if (req->bIsAccept == false && cPlayer->GetZeni() < DBO_DOJO_SCRAMBLE_REJECT_FARE) //check if reject & has enough zeni
		resultcode = GAME_GUILD_DOJO_YOU_ARE_NOT_ENOUGH_ZENNY;
	else
	{
		CNtlPacket packet(sizeof(sGT_DOJO_SCRAMBLE_RESPONSE));
		sGT_DOJO_SCRAMBLE_RESPONSE * res = (sGT_DOJO_SCRAMBLE_RESPONSE *)packet.GetPacketData();
		res->wOpCode = GT_DOJO_SCRAMBLE_RESPONSE;
		res->charId = cPlayer->GetCharID();
		res->bIsAccept = req->bIsAccept;
		packet.SetPacketLen(sizeof(sGT_DOJO_SCRAMBLE_RESPONSE));
		app->SendTo(app->GetChatServerSession(), &packet);

		return;
	}

	CNtlPacket packet(sizeof(sGU_DOJO_SCRAMBLE_RESPONSE_RES));
	sGU_DOJO_SCRAMBLE_RESPONSE_RES * res = (sGU_DOJO_SCRAMBLE_RESPONSE_RES *)packet.GetPacketData();
	res->wOpCode = GU_DOJO_SCRAMBLE_RESPONSE_RES;
	res->wResultCode = resultcode;
	res->bIsRetry = true;
//	res->wszGuildName 
	packet.SetPacketLen(sizeof(sGU_DOJO_SCRAMBLE_RESPONSE_RES));
	app->Send(GetHandle(), &packet);
}


void CClientSession::RecvDojoNpcInfoReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_DOJO_NPC_INFO_REQ * req = (sUG_DOJO_NPC_INFO_REQ*)pPacket->GetPacketData();

	WORD resultcode = GAME_SUCCESS;

	CNpc* npc = g_pObjectManager->GetNpc(req->hNpcHandle);
	if (npc)
	{
		if (npc->GetTbldat()->byJob == NPC_JOB_DOJO_MANAGER)
		{
			if (NtlGetDistance(cPlayer->GetCurLoc(), npc->GetCurLoc()) < 20.0f)
			{
				sDOJO_TBLDAT* dojo = (sDOJO_TBLDAT*)g_pTableContainer->GetDojoTable()->FindData(npc->GetTbldat()->contentsTblidx);
				if (dojo)
				{
					CNtlPacket packet(sizeof(sGT_DOJO_NPC_INFO_REQ));
					sGT_DOJO_NPC_INFO_REQ * res = (sGT_DOJO_NPC_INFO_REQ *)packet.GetPacketData();
					res->wOpCode = GT_DOJO_NPC_INFO_REQ;
					res->charId = cPlayer->GetCharID();
					res->dojoTblidx = dojo->tblidx;
					packet.SetPacketLen(sizeof(sGT_DOJO_NPC_INFO_REQ));
					app->SendTo(app->GetChatServerSession(), &packet);

					return;
				}
				else resultcode = GAME_GUILD_DOJO_NOT_FOUND_TABLE;
			}
			else resultcode = GAME_GUILD_DOJO_MANAGER_IS_TOO_FAR;
		}
		else resultcode = GAME_GUILD_NOT_GUILD_MANAGER_NPC;
	}
	else resultcode = GAME_GUILD_NO_DOJO_MANAGER_NPC_FOUND;

	CNtlPacket packet(sizeof(sGU_DOJO_NPC_INFO_RES));
	sGU_DOJO_NPC_INFO_RES * res = (sGU_DOJO_NPC_INFO_RES *)packet.GetPacketData();
	res->wOpCode = GU_DOJO_NPC_INFO_RES;
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_DOJO_NPC_INFO_RES));
	app->Send(GetHandle(), &packet);
}


//--------------------------------------------------------------------------------------//
//	CREATE ITEM (NEW)
//--------------------------------------------------------------------------------------//
void CClientSession::RecvHoiPoiItemCreateExReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_HOIPOIMIX_ITEM_CREATE_EX_REQ * req = (sUG_HOIPOIMIX_ITEM_CREATE_EX_REQ*)pPacket->GetPacketData();

	WORD resultcode = GAME_SUCCESS;
	DWORD addexp = 0;
	BYTE byMartialNeed = 0;

	CNtlPacket packetQry(sizeof(sGQ_HOIPOIMIX_ITEM_MAKE_REQ));
	sGQ_HOIPOIMIX_ITEM_MAKE_REQ* resQry = (sGQ_HOIPOIMIX_ITEM_MAKE_REQ*)packetQry.GetPacketData();
	resQry->wOpCode = GQ_HOIPOIMIX_ITEM_MAKE_REQ;
	resQry->handle = cPlayer->GetID();
	resQry->charId = cPlayer->GetCharID();
	resQry->objHandle = req->objHandle;
	resQry->recipeTblidx = req->recipeTblidx;
	resQry->bIsNew = false;

	sITEM_RECIPE_TBLDAT * pRecipeTbldat = (sITEM_RECIPE_TBLDAT*)g_pTableContainer->GetItemRecipeTable()->FindData(req->recipeTblidx);
	if (pRecipeTbldat && pRecipeTbldat->bValidityAble)
	{
		if (cPlayer->GetHoiPoiMixLv() >= pRecipeTbldat->byNeedMixLevel) //check required recipe lv
		{
			if (cPlayer->GetZeni() >= pRecipeTbldat->dwNeedMixZenny) //check zeni
			{
				if (cPlayer->GetPlayerItemContainer()->CountEmptyInventory() >= 1)
				{
					//Get martial count
					for (int b = 0; b < DBO_MAX_COUNT_RECIPE_MATERIAL_ITEM; b++)
					{
						if (pRecipeTbldat->asMaterial[b].materialTblidx != INVALID_TBLIDX)
							byMartialNeed++;
					}
					if (req->byMaterialCount != byMartialNeed)
					{
						resultcode = GAME_FAIL;
						ERR_LOG(LOG_SYSTEM, "CRAFTING ERROR. PLAYER %u MISSMATCH MARTIAL COUNT. %u != %u", cPlayer->GetCharID(), req->byMaterialCount, byMartialNeed);
						goto GOTO_END;
					}
					//check required martial
					for (int a = 0; a < DBO_MAX_COUNT_RECIPE_MATERIAL_ITEM; a++)
					{
						if (pRecipeTbldat->asMaterial[a].materialTblidx != INVALID_TBLIDX)
						{
							CItem* pItem = cPlayer->GetPlayerItemContainer()->GetItem(req->aMaterialSlot[a].byPlace, req->aMaterialSlot[a].byPos);
							if (pItem == NULL || pItem->GetTblidx() != pRecipeTbldat->asMaterial[a].materialTblidx || pItem->GetCount() < pRecipeTbldat->asMaterial[a].byMaterialCount || IsInvenContainer(pItem->GetPlace()) == false)
							{
								resultcode = GAME_NEEDITEM_NOT_FOUND;
								goto GOTO_END;
							}
							else if (pItem->IsLocked())
							{
								resultcode = GAME_ITEM_IS_LOCK;
								goto GOTO_END;
							}
						}
					}
					/*
					std::vector<int> vecRandSlot;

					//get random reward
					for (int a = 0; a < DBO_MAX_COUNT_RECIPE_CREATE_ITEM; a++)
					{
						if (pRecipeTbldat->asCreateItemTblidx[a].itemTblidx == INVALID_TBLIDX)
							continue;
						if (Dbo_CheckProbability((int)pRecipeTbldat->asCreateItemTblidx[a].itemRate))
						{
							vecRandSlot.push_back(a);
						}
					}

					int nRandReward = 0;
					if (vecRandSlot.size() > 1)
						nRandReward = vecRandSlot[RandomRange(0, (int)(vecRandSlot.size() - 1))];
					*/
					BYTE nRandReward = INVALID_BYTE;

					if (pRecipeTbldat->asCreateItemTblidx[0].itemTblidx != INVALID_TBLIDX)
					{
						if (Dbo_CheckProbability((int)pRecipeTbldat->asCreateItemTblidx[0].itemRate)) nRandReward = 0;
						else if (pRecipeTbldat->asCreateItemTblidx[1].itemTblidx != INVALID_TBLIDX && nRandReward == INVALID_BYTE)
						{
							if (Dbo_CheckProbability((int)pRecipeTbldat->asCreateItemTblidx[1].itemRate)) nRandReward = 1;
							else
							{
								if (pRecipeTbldat->asCreateItemTblidx[2].itemTblidx != INVALID_TBLIDX)
								{
									if (Dbo_CheckProbability((int)pRecipeTbldat->asCreateItemTblidx[2].itemRate)) nRandReward = 2;
								}
							}
						}
					}
					
					

					sITEM_TBLDAT* pRewardItem = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(pRecipeTbldat->asCreateItemTblidx[nRandReward].itemTblidx);
					if (pRewardItem)
					{
						if (cPlayer->GetLevel() >= pRewardItem->byNeed_Min_Level)
						{
							//check if same tblidx already exist && can stack
							if (pRewardItem->byMax_Stack > 1)
							{
								CItem* itemcheck = cPlayer->GetPlayerItemContainer()->CheckStackItem(pRewardItem->tblidx, 1, pRewardItem->byMax_Stack, GetDefaultRestrictState(pRewardItem->byRestrictType, pRewardItem->byItem_Type, true));
								if (itemcheck)
								{
									itemcheck->SetCount(itemcheck->GetCount() + 1, true, false);
									memcpy(&resQry->sCreateData, &itemcheck->GetItemData(), sizeof(sITEM_DATA));

									goto CREATEITEM_SUCCESS;
								}
							}

							std::pair<BYTE, BYTE> inv = cPlayer->GetPlayerItemContainer()->GetEmptyInventory();

							cPlayer->GetPlayerItemContainer()->AddReservedInventory(inv.first, inv.second);

							resQry->sCreateData.Init();
							resQry->sCreateData.itemNo = pRewardItem->tblidx;
							resQry->sCreateData.charId = cPlayer->GetCharID();
							resQry->sCreateData.byPlace = inv.first;
							resQry->sCreateData.byPosition = inv.second;
							resQry->sCreateData.byStackcount = 1;
							resQry->sCreateData.byRank = pRewardItem->byRank;
							resQry->sCreateData.byCurrentDurability = pRewardItem->byDurability;
							resQry->sCreateData.bNeedToIdentify = false;
							resQry->sCreateData.byGrade = ITEM_GRADE_LEVEL_0;
							resQry->sCreateData.byBattleAttribute = pRewardItem->byBattle_Attribute;
							resQry->sCreateData.byRestrictState = GetDefaultRestrictState(pRewardItem->byRestrictType, pRewardItem->byItem_Type, true);
							resQry->sCreateData.byDurationType = pRewardItem->byDurationType;

							if (pRewardItem->bIsCanHaveOption && pRewardItem->Item_Option_Tblidx != INVALID_TBLIDX)
								resQry->sCreateData.sOptionSet.aOptionTblidx[0] = pRewardItem->Item_Option_Tblidx;

							if (pRewardItem->Item_Option_Tblidx == INVALID_TBLIDX)
								CItem::ChangeOption(INVALID_WORD, pRewardItem, &resQry->sCreateData, pRewardItem->byRank, &resQry->sCreateData.sOptionSet, false);

							resQry->bIsNew = true;
						}
						else
						{
							resultcode = GAME_ITEM_NEED_MORE_LEVEL;
							goto GOTO_END;
						}
					}
					else
					{
						//ERR_LOG(LOG_GENERAL, "ERROR HOIPOI ITEM REWARD %u DOES NOT EXIST \n");
						//resultcode = GAME_ITEM_NOT_FOUND;
						//goto GOTO_END;
						//resultcode = GAME_ITEM_HOIPOIMIX_FARE_RATE_FAIL;
						goto CREATEITEM_SUCCESS;
					}

					CREATEITEM_SUCCESS:

					for (int c = 0; c < byMartialNeed; c++)
					{
						CItem* neededitem = cPlayer->GetPlayerItemContainer()->GetItem(req->aMaterialSlot[c].byPlace, req->aMaterialSlot[c].byPos); //get item
						if (neededitem && IsInvenContainer(neededitem->GetPlace()))
						{
							resQry->asData[c].byPlace = neededitem->GetPlace();
							resQry->asData[c].byPos = neededitem->GetPos();
							resQry->asData[c].byStack = UnsignedSafeDecrease<BYTE>(neededitem->GetCount(), pRecipeTbldat->asMaterial[c].byMaterialCount);
							resQry->asData[c].nItemID = neededitem->GetItemID();

							if (neededitem->GetTblidx() == pRecipeTbldat->asMaterial[c].materialTblidx && neededitem->GetCount() >= pRecipeTbldat->asMaterial[c].byMaterialCount) //check tblidx and count
							{
								neededitem->SetCount(neededitem->GetCount() - pRecipeTbldat->asMaterial[c].byMaterialCount, false, false); //remove count
							}
						}
						else
						{
							ERR_LOG(LOG_USER, "Error: Player %u dont have all martial item. Hacker", cPlayer->GetCharID());
							resultcode = GAME_FAIL;
							goto GOTO_END;
						}
					}

					if (cPlayer->GetLevel() >= cPlayer->GetHoiPoiMixLv())
					{
						if (nRandReward != INVALID_BYTE)
						{
							bool bReceiveExp = Dbo_CheckProbability((int)pRecipeTbldat->asCreateItemTblidx[nRandReward].itemRate);
							resQry->dwExpGained = Dbo_GetHoipoiMixEXP(true, cPlayer->GetHoiPoiMixLv(), pRecipeTbldat->byNeedMixLevel) * app->GetCraftExpRate();

							cPlayer->SetHoiPoiMixExpAndLevel(resQry->dwExpGained);
						}
						
					}
					else resQry->dwExpGained = 0;

					cPlayer->UpdateZeni(ZENNY_CHANGE_TYPE_ITEM_MIX_MAKE, pRecipeTbldat->dwNeedMixZenny, false, false);
					resQry->byCount = byMartialNeed;
					resQry->dwMixExp = cPlayer->GetHoiPoiMixExp();
					resQry->byMixLevel = cPlayer->GetHoiPoiMixLv();
					resQry->dwSpendZenny = pRecipeTbldat->dwNeedMixZenny;
				}
				else resultcode = GAME_ITEM_HOIPOI_CANNOT_MAKE_INVEN_FULL;
			}
			else resultcode = GAME_ITEM_RECIPE_CANNOT_SET_YOU_NEED_MORE_ZENNY;
		}
		else resultcode = GAME_ITEM_RECIPE_LEVEL_MISMATCHED;
	}
	else resultcode = GAME_FAIL;

GOTO_END:

	if (resultcode == GAME_SUCCESS)
	{
		packetQry.SetPacketLen(sizeof(sGQ_HOIPOIMIX_ITEM_MAKE_REQ));
		app->SendTo(app->GetQueryServerSession(), &packetQry);
	}
	else
	{
		CNtlPacket packet(sizeof(sGU_HOIPOIMIX_ITEM_MAKE_EX_RES));
		sGU_HOIPOIMIX_ITEM_MAKE_EX_RES * res = (sGU_HOIPOIMIX_ITEM_MAKE_EX_RES *)packet.GetPacketData();
		res->wOpCode = GU_HOIPOIMIX_ITEM_MAKE_EX_RES;
		res->objHandle = req->objHandle;
		res->recipeTblidx = req->recipeTblidx;
		res->wResultCode = resultcode;
		packet.SetPacketLen(sizeof(sGU_HOIPOIMIX_ITEM_MAKE_EX_RES));
		app->Send(GetHandle(), &packet);
	}
}


//--------------------------------------------------------------------------------------//
//		START TRADE REQUEST
//--------------------------------------------------------------------------------------//
void CClientSession::RecvTradeStartReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_TRADE_START_REQ* req = (sUG_TRADE_START_REQ *)pPacket->GetPacketData();

	CGameServer* app = (CGameServer*)g_pApp;
	WORD resultcode = GAME_SUCCESS;

	//start if target found
	CPlayer* target = g_pObjectManager->GetPC(req->hTarget);
	if (target && target->GetCharID() != cPlayer->GetCharID())
	{
		if(app->GetGsChannel() == DOJO_CHANNEL_INDEX)
			resultcode = GAME_FAIL;
		else if (!target->IsPC() || target->GetCharStateID() != CHARSTATE_STANDING || cPlayer->GetCharStateID() != CHARSTATE_STANDING) //Check if target is pc
			resultcode = GAME_TRADE_TARGET_WRONG_STATE;
		else if (cPlayer->IsInRange(target, DBO_TRADE_REQUEST_RANGE) == false)
			resultcode = GAME_TARGET_TOO_FAR;
		else if (cPlayer->GetTrade() || target->GetTrade()) //check if already trading
			resultcode = GAME_TRADE_ALREADY_USE;
		else if (cPlayer->GetPlayerItemContainer()->CountBags() == 0 || target->GetPlayerItemContainer()->CountBags() == 0)	//check if has bagslot
			resultcode = GAME_CANNOT_TRADE_NO_BAGSLOT;
		else if (cPlayer->GetID() == target->GetID()) //dont allow trading himself
			resultcode = GAME_FAIL;
		else if (cPlayer->IsPvpZone() || target->IsPvpZone()) //dont allow trading while someone in pvp platform
			resultcode = GAME_FAIL;
	}
	else resultcode = GAME_TARGET_NOT_FOUND;


	if (resultcode == GAME_SUCCESS)
	{
		g_pTradeManager->StartTrade(cPlayer, target);
	}
	else
	{
		CNtlPacket packet(sizeof(sGU_TRADE_START_RES));
		sGU_TRADE_START_RES * res = (sGU_TRADE_START_RES *)packet.GetPacketData();
		res->wOpCode = GU_TRADE_START_RES;
		res->handle = cPlayer->GetID();
		res->wResultCode = resultcode;
		packet.SetPacketLen(sizeof(sGU_TRADE_START_RES));
		g_pApp->Send(GetHandle(), &packet);
	}
}

//--------------------------------------------------------------------------------------//
//		CANCEL OR ACCEPT TRADE REQUEST
//--------------------------------------------------------------------------------------//
void CClientSession::RecvTradeOkRes(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_TRADE_OK_RES* req = (sUG_TRADE_OK_RES *)pPacket->GetPacketData();

	CTrade* trade = cPlayer->GetTrade();
	CPlayer* requestor = g_pObjectManager->GetPC(req->handle);

	if (requestor && trade && trade->GetTradeState() == eTRADE_STATE_WAIT_FOR_ACCEPT)
	{
		if (trade->GetCompany() == requestor->GetTrade() && requestor->GetCharID() != cPlayer->GetCharID())
		{
			if (req->byOK == 1) // else req->byOK == 1 == accepted
			{
				ERR_LOG(LOG_GENERAL, "Player %u accepted the trade request from Player %u", cPlayer->GetCharID(), requestor->GetCharID());
				trade->SetState(eTRADE_STATE_TRADING);
				trade->GetCompany()->SetState(eTRADE_STATE_TRADING);

				BYTE byEmpty = cPlayer->GetPlayerItemContainer()->CountEmptyInventory();
				BYTE byReqEmpty = requestor->GetPlayerItemContainer()->CountEmptyInventory();

				//## SEND TO PLAYER
				CNtlPacket packet(sizeof(sGU_TRADE_START_RES));
				sGU_TRADE_START_RES * res = (sGU_TRADE_START_RES *)packet.GetPacketData();
				res->wOpCode = GU_TRADE_START_RES;
				res->wResultCode = GAME_SUCCESS;
				res->byEmptyInven = byReqEmpty;
				res->hTarget = requestor->GetID();
				res->handle = cPlayer->GetID();
				packet.SetPacketLen(sizeof(sGU_TRADE_START_RES));
				app->Send(GetHandle(), &packet);
				//## SEND TO PLAYER END

				//## SEND TO REQUESTOR
				CNtlPacket packet4(sizeof(sGU_TRADE_START_RES));
				sGU_TRADE_START_RES * res4 = (sGU_TRADE_START_RES *)packet4.GetPacketData();
				res4->wOpCode = GU_TRADE_START_RES;
				res4->wResultCode = GAME_SUCCESS;
				res4->byEmptyInven = byEmpty;
				res4->hTarget = cPlayer->GetID();
				res4->handle = requestor->GetID();
				packet4.SetPacketLen(sizeof(sGU_TRADE_START_RES));
				app->Send(requestor->GetClientSessionID(), &packet4);
				//## SEND TO REQUESTOR END
			}
			else
			{
				CNtlPacket packet(sizeof(sGU_TRADE_DENY_RES));
				sGU_TRADE_DENY_RES* res = (sGU_TRADE_DENY_RES *)packet.GetPacketData();
				res->wOpCode = GU_TRADE_DENY_RES;
				res->wResultCode = GAME_TRADE_DENY_USE;
				res->bIsDeny = true;
				packet.SetPacketLen(sizeof(sGU_TRADE_DENY_RES));
				app->Send(requestor->GetClientSessionID(), &packet);

				//delete trade
				if (trade->GetTradeState() == eTRADE_STATE_WAIT_FOR_ACCEPT)
				{
					g_pTradeManager->DestroyTrade(cPlayer->GetTrade());
					g_pTradeManager->DestroyTrade(requestor->GetTrade());
				}
			}
		}
	}
	else //requestor not anymore online so send error
	{
		CNtlPacket packet(sizeof(sGU_TRADE_START_RES));
		sGU_TRADE_START_RES* res = (sGU_TRADE_START_RES *)packet.GetPacketData();
		res->wOpCode = GU_TRADE_START_RES;
		res->wResultCode = GAME_TRADE_TARGET_WRONG_STATE;
		res->handle = cPlayer->GetID();
		res->hTarget = req->handle;
		packet.SetPacketLen(sizeof(sGU_TRADE_START_RES));
		app->Send(GetHandle(), &packet);
	}
}

//--------------------------------------------------------------------------------------//
//		ADD ITEM INTO EXCHANGE WINDOW
//--------------------------------------------------------------------------------------//
void CClientSession::RecvTradeAddReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_TRADE_ADD_REQ* req = (sUG_TRADE_ADD_REQ *)pPacket->GetPacketData();

	WORD resultcode = GAME_SUCCESS;

	CNtlPacket packet(sizeof(sGU_TRADE_ADD_RES));
	sGU_TRADE_ADD_RES* res = (sGU_TRADE_ADD_RES *)packet.GetPacketData();
	res->wOpCode = GU_TRADE_ADD_RES;
	res->byCount = req->byCount;
	res->hItem = req->hItem;

	CTrade* trade = cPlayer->GetTrade();
	if (trade)
	{
		CPlayer* partner = g_pObjectManager->GetPC(req->hTarget);
		if (partner && partner->GetCharID() != cPlayer->GetCharID()) //check if requestor/target still online
		{
			if (trade->GetCompany() == partner->GetTrade())
			{
				if (trade->IsReady() == false && partner->GetTrade()->IsReady() == false && trade->GetTradeState() == eTRADE_STATE_TRADING && partner->GetTrade()->GetTradeState() == eTRADE_STATE_TRADING)
				{
					CItem* pItem = cPlayer->GetPlayerItemContainer()->GetItem(req->hItem);
					if (pItem && pItem->CanTrade() && IsInvenContainer(pItem->GetPlace()))
					{
						if ( (pItem->GetCount() >= req->byCount && pItem->GetCount() > 0 && req->byCount > 0)
							&& (pItem->GetTbldat()->byMax_Stack >= req->byCount)) //check if has enough stack
						{
							trade->AddItem(pItem, req->byCount);
							pItem->SetTrading(true);

							//send to partner
							CNtlPacket packet2(sizeof(sGU_TRADE_ADD_NFY));
							sGU_TRADE_ADD_NFY* res2 = (sGU_TRADE_ADD_NFY *)packet2.GetPacketData();
							res2->wOpCode = GU_TRADE_ADD_NFY;
							res2->byCount = req->byCount;
							res2->hItem = req->hItem;
							memcpy(&res2->sItem, &pItem->GetItemData(), sizeof(sITEM_DATA));
							packet2.SetPacketLen(sizeof(sGU_TRADE_ADD_NFY));
							app->Send(partner->GetClientSessionID(), &packet2);

						}
						else resultcode = GAME_ITEM_STACK_FAIL;
					}
					else resultcode = GAME_TRADE_ITEM_INVALID;
				}
				else resultcode = GAME_FAIL;
			}
			else resultcode = GAME_FAIL;
		}
		else resultcode = GAME_TARGET_NOT_FOUND;
	}
	else resultcode = GAME_TRADE_ALREADY_CLOSE;

	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_TRADE_ADD_RES));
	app->Send(GetHandle(), &packet);
}

//--------------------------------------------------------------------------------------//
//		DELETE ITEM FROM EXCHANGE WINDOW
//--------------------------------------------------------------------------------------//
void CClientSession::RecvTradeDelReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_TRADE_DEL_REQ* req = (sUG_TRADE_DEL_REQ *)pPacket->GetPacketData();

	WORD resultcode = GAME_SUCCESS;

	CNtlPacket packet(sizeof(sGU_TRADE_DEL_RES));
	sGU_TRADE_DEL_RES* res = (sGU_TRADE_DEL_RES *)packet.GetPacketData();
	res->wOpCode = GU_TRADE_DEL_RES;
	res->hItem = req->hItem;

	CTrade* trade = cPlayer->GetTrade();
	if (trade)
	{
		CPlayer* partner = g_pObjectManager->GetPC(req->hTarget);
		if (partner && partner->GetCharID() != cPlayer->GetCharID()) //check if requestor/target still online
		{
			if (trade->GetCompany() == partner->GetTrade())
			{
				if (trade->IsReady() == false && partner->GetTrade()->IsReady() == false && trade->GetTradeState() == eTRADE_STATE_TRADING && partner->GetTrade()->GetTradeState() == eTRADE_STATE_TRADING)
				{
					if (trade->FindItem(req->hItem))
					{
						trade->DelItem(req->hItem); //here we set item trading false and delete the trade item

						//send to partner
						CNtlPacket packet2(sizeof(sGU_TRADE_DEL_NFY));
						sGU_TRADE_DEL_NFY* res2 = (sGU_TRADE_DEL_NFY *)packet2.GetPacketData();
						res2->wOpCode = GU_TRADE_DEL_NFY;
						res2->hItem = req->hItem;
						packet2.SetPacketLen(sizeof(sGU_TRADE_DEL_NFY));
						app->Send(partner->GetClientSessionID(), &packet2);
					}
					else
						resultcode = GAME_TRADE_ITEM_INVALID;
				}
				else resultcode = GAME_FAIL;
			}
			else resultcode = GAME_FAIL;
		}
	}
	else resultcode = GAME_TRADE_ALREADY_CLOSE;

	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_TRADE_DEL_RES));
	app->Send(GetHandle(), &packet);
}


//--------------------------------------------------------------------------------------//
//		UPDATE ITEM
//--------------------------------------------------------------------------------------//
void CClientSession::RecvTradeModifyReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_TRADE_MODIFY_REQ* req = (sUG_TRADE_MODIFY_REQ *)pPacket->GetPacketData();

	WORD resultcode = GAME_SUCCESS;

	CNtlPacket packet(sizeof(sGU_TRADE_MODIFY_RES));
	sGU_TRADE_MODIFY_RES* res = (sGU_TRADE_MODIFY_RES *)packet.GetPacketData();
	res->wOpCode = GU_TRADE_MODIFY_RES;
	res->hItem = req->hItem;
	res->byCount = req->byCount;
	res->hTarget = req->hTarget;

	CTrade* trade = cPlayer->GetTrade();
	if (trade)
	{
		CPlayer* partner = g_pObjectManager->GetPC(req->hTarget);
		if (partner && partner->GetCharID() != cPlayer->GetCharID()) //check if requestor/target still online
		{
			if (trade->GetCompany() == partner->GetTrade())
			{
				if (trade->IsReady() == false && partner->GetTrade()->IsReady() == false && trade->GetTradeState() == eTRADE_STATE_TRADING && partner->GetTrade()->GetTradeState() == eTRADE_STATE_TRADING)
				{
					if (trade->FindItem(req->hItem))
					{
						CItem* realitem = cPlayer->GetPlayerItemContainer()->GetItem(req->hItem);
						if (realitem && IsInvenContainer(realitem->GetPlace()))
						{
							if ((realitem->GetCount() >= req->byCount && realitem->GetCount() > 0 && req->byCount > 0)
								&& (realitem->GetTbldat()->byMax_Stack >= req->byCount))
							{
								trade->ModifyItem(req->hItem, req->byCount);

								//send to partner
								CNtlPacket packet2(sizeof(sGU_TRADE_MODIFY_NFY));
								sGU_TRADE_MODIFY_NFY* res2 = (sGU_TRADE_MODIFY_NFY *)packet2.GetPacketData();
								res2->wOpCode = GU_TRADE_MODIFY_NFY;
								res2->hItem = req->hItem;
								res2->byCount = req->byCount;
								res2->hTarget = cPlayer->GetID();
								packet2.SetPacketLen(sizeof(sGU_TRADE_MODIFY_NFY));
								app->Send(partner->GetClientSessionID(), &packet2);
							}
							else
								resultcode = GAME_ITEM_STACK_FAIL;
						}
						else
							resultcode = GAME_TRADE_ITEM_INVALID;
					}
					else
						resultcode = GAME_TRADE_ITEM_INVALID;
				}
				else resultcode = GAME_FAIL;
			}
			else resultcode = GAME_FAIL;
		}
	}
	else resultcode = GAME_TRADE_ALREADY_CLOSE;

	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_TRADE_MODIFY_RES));
	app->Send(GetHandle(), &packet);
}


//--------------------------------------------------------------------------------------//
//		UPDATE ZENNY
//--------------------------------------------------------------------------------------//
void CClientSession::RecvTradeZeniUpdateReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_TRADE_ZENNY_UPDATE_REQ* req = (sUG_TRADE_ZENNY_UPDATE_REQ *)pPacket->GetPacketData();

	WORD resultcode = GAME_SUCCESS;

	CNtlPacket packet(sizeof(sGU_TRADE_ZENNY_UPDATE_RES));
	sGU_TRADE_ZENNY_UPDATE_RES* res = (sGU_TRADE_ZENNY_UPDATE_RES *)packet.GetPacketData();
	res->wOpCode = GU_TRADE_ZENNY_UPDATE_RES;
	res->dwZenny = req->dwZenny;
	res->hTarget = req->hTarget;

	CTrade* trade = cPlayer->GetTrade();
	if (trade)
	{
		CPlayer* partner = g_pObjectManager->GetPC(req->hTarget);
		if (partner && partner->GetCharID() != cPlayer->GetCharID()) //check if requestor/target still online
		{
			if (trade->GetCompany() == partner->GetTrade())
			{
				if (trade->IsReady() == false && partner->GetTrade()->IsReady() == false && trade->GetTradeState() == eTRADE_STATE_TRADING && partner->GetTrade()->GetTradeState() == eTRADE_STATE_TRADING)
				{
					if (cPlayer->GetZeni() >= req->dwZenny)
					{
						if (partner->CanReceiveZeni(req->dwZenny)) // check if partner can receive that much zeni
						{
							DWORD dwZeniCache = trade->GetZeni();
							trade->UpdateZeni(req->dwZenny);

							if (trade->GetZeni() <= cPlayer->GetZeni() && trade->GetZeni() <= NTL_MAX_USE_ZENI) //hack check
							{
								//send to partner
								CNtlPacket packet2(sizeof(sGU_TRADE_ZENNY_UPDATE_NFY));
								sGU_TRADE_ZENNY_UPDATE_NFY* res2 = (sGU_TRADE_ZENNY_UPDATE_NFY *)packet2.GetPacketData();
								res2->wOpCode = GU_TRADE_ZENNY_UPDATE_NFY;
								res2->dwZenny = req->dwZenny;
								res2->hTarget = cPlayer->GetID();
								packet2.SetPacketLen(sizeof(sGU_TRADE_ZENNY_UPDATE_NFY));
								app->Send(partner->GetClientSessionID(), &packet2);
							}
							else
							{
								trade->UpdateZeni(dwZeniCache);
								resultcode = GAME_ZENNY_NOT_ENOUGH;
							}
						}
						else
						{
							resultcode = GAME_ZENNY_OVER;
						}
					}
					else resultcode = GAME_ZENNY_NOT_ENOUGH;
				}
				else resultcode = GAME_FAIL;
			}
			else resultcode = GAME_FAIL;
		}
	}
	else resultcode = GAME_TRADE_ALREADY_CLOSE;

	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_TRADE_ZENNY_UPDATE_RES));
	app->Send(GetHandle(), &packet);
}


//--------------------------------------------------------------------------------------//
//		CANCEL TRADE
//--------------------------------------------------------------------------------------//
void CClientSession::RecvTradeCancelReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_TRADE_CANCEL_REQ* req = (sUG_TRADE_CANCEL_REQ *)pPacket->GetPacketData();

	if (cPlayer->GetTrade() && cPlayer->GetTrade()->GetTradeState() == eTRADE_STATE_TRADING)
	{
		CNtlPacket packet(sizeof(sGU_TRADE_CANCEL_RES));
		sGU_TRADE_CANCEL_RES* res = (sGU_TRADE_CANCEL_RES *)packet.GetPacketData();
		res->wOpCode = GU_TRADE_CANCEL_RES;
		res->wResultCode = GAME_SUCCESS;
		res->hTarget = req->hTarget;
		packet.SetPacketLen(sizeof(sGU_TRADE_CANCEL_RES));
		app->Send(GetHandle(), &packet);

		CPlayer* partner = cPlayer->GetTrade()->GetCompany()->GetOwner();
		if (partner && partner->GetCharID() != cPlayer->GetCharID())
		{
			CNtlPacket packet2(sizeof(sGU_TRADE_CANCEL_NFY));
			sGU_TRADE_CANCEL_NFY* res2 = (sGU_TRADE_CANCEL_NFY *)packet2.GetPacketData();
			res2->wOpCode = GU_TRADE_CANCEL_NFY;
			res2->wResultCode = GAME_SUCCESS;
			res2->hTarget = cPlayer->GetID();
			packet2.SetPacketLen(sizeof(sGU_TRADE_CANCEL_NFY));
			app->Send(partner->GetClientSessionID(), &packet2);

			g_pTradeManager->DestroyTrade(partner->GetTrade());
		}

		g_pTradeManager->DestroyTrade(cPlayer->GetTrade());
	}
}

//--------------------------------------------------------------------------------------//
//		END TRADE
//--------------------------------------------------------------------------------------//
void CClientSession::RecvTradeEndReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_TRADE_END_REQ* req = (sUG_TRADE_END_REQ *)pPacket->GetPacketData();

	if (cPlayer->GetID() == req->hTarget) //hacker check
	{
		CNtlPacket packet(sizeof(sGU_TRADE_END_RES));
		sGU_TRADE_END_RES* res = (sGU_TRADE_END_RES *)packet.GetPacketData();
		res->wOpCode = GU_TRADE_END_RES;
		res->wResultCode = GAME_FAIL;
		res->hTarget = req->hTarget;
		res->bIsSet = req->bIsSet;
		packet.SetPacketLen(sizeof(sGU_TRADE_END_RES));
		g_pApp->Send(GetHandle(), &packet);
	}
	else if (cPlayer->GetTrade() && cPlayer->GetTrade()->GetTradeState() == eTRADE_STATE_TRADING)
	{
		CTrade* company = cPlayer->GetTrade()->GetCompany();

		if (cPlayer->GetTrade()->IsBlocked() || company->IsBlocked())
		{
			CNtlPacket packet(sizeof(sGU_TRADE_END_RES));
			sGU_TRADE_END_RES* res = (sGU_TRADE_END_RES *)packet.GetPacketData();
			res->wOpCode = GU_TRADE_END_RES;
			res->wResultCode = GAME_TRADE_WRONG_STATE;
			res->hTarget = req->hTarget;
			res->bIsSet = cPlayer->GetTrade()->IsReady();
			packet.SetPacketLen(sizeof(sGU_TRADE_END_RES));
			g_pApp->Send(GetHandle(), &packet);

			return;
		}

		cPlayer->GetTrade()->SetReady(req->bIsSet);

		if (company == NULL)
		{
			
		}
		else if (company->GetOwner()->GetID() == cPlayer->GetID()) //hack check
		{
			CNtlPacket packet(sizeof(sGU_TRADE_END_RES));
			sGU_TRADE_END_RES* res = (sGU_TRADE_END_RES *)packet.GetPacketData();
			res->wOpCode = GU_TRADE_END_RES;
			res->wResultCode = GAME_FAIL;
			res->hTarget = req->hTarget;
			res->bIsSet = req->bIsSet;
			packet.SetPacketLen(sizeof(sGU_TRADE_END_RES));
			g_pApp->Send(GetHandle(), &packet);
		}
		else if (cPlayer->GetTrade()->IsReady() && company->IsReady())
		{
			if (company->GetTradeState() == eTRADE_STATE_TRADING)
			{
				CNtlPacket packet(sizeof(sGU_TRADE_END_RES));
				sGU_TRADE_END_RES* res = (sGU_TRADE_END_RES *)packet.GetPacketData();
				res->wOpCode = GU_TRADE_END_RES;
				res->wResultCode = GAME_SUCCESS;
				res->hTarget = req->hTarget;
				res->bIsSet = req->bIsSet;
				packet.SetPacketLen(sizeof(sGU_TRADE_END_RES));
				g_pApp->Send(GetHandle(), &packet);

				if (!g_pTradeManager->FinishTrade(cPlayer->GetTrade()))
					cPlayer->GetTrade()->SetReady(false);
			}
			else
			{
				ERR_LOG(LOG_USER, "<TRADE>Player: %u trade state is not trading", company->GetOwner()->GetCharID());

				CNtlPacket packet(sizeof(sGU_TRADE_END_RES));
				sGU_TRADE_END_RES* res = (sGU_TRADE_END_RES *)packet.GetPacketData();
				res->wOpCode = GU_TRADE_END_RES;
				res->wResultCode = GAME_FAIL;
				res->hTarget = req->hTarget;
				res->bIsSet = req->bIsSet;
				packet.SetPacketLen(sizeof(sGU_TRADE_END_RES));
				g_pApp->Send(GetHandle(), &packet);
			}
		}
		else
		{
			CPlayer* pPartner = company->GetOwner();

			CNtlPacket packet(sizeof(sGU_TRADE_END_RES));
			sGU_TRADE_END_RES* res = (sGU_TRADE_END_RES *)packet.GetPacketData();
			res->wOpCode = GU_TRADE_END_RES;
			res->wResultCode = GAME_SUCCESS;
			res->hTarget = req->hTarget;
			res->bIsSet = req->bIsSet;
			packet.SetPacketLen(sizeof(sGU_TRADE_END_RES));
			g_pApp->Send(GetHandle(), &packet);

			if (pPartner && pPartner->IsInitialized())
			{
				CNtlPacket packet2(sizeof(sGU_TRADE_END_NFY));
				sGU_TRADE_END_NFY* res2 = (sGU_TRADE_END_NFY *)packet2.GetPacketData();
				res2->wOpCode = GU_TRADE_END_NFY;
				res2->wResultCode = GAME_SUCCESS;
				res2->hTarget = cPlayer->GetID();
				res2->bIsSet = req->bIsSet;
				packet2.SetPacketLen(sizeof(sGU_TRADE_END_NFY));
				g_pApp->Send(pPartner->GetClientSessionID(), &packet2);
			}
		}
	}
	else
	{
		CNtlPacket packet(sizeof(sGU_TRADE_END_RES));
		sGU_TRADE_END_RES* res = (sGU_TRADE_END_RES *)packet.GetPacketData();
		res->wOpCode = GU_TRADE_END_RES;
		res->wResultCode = GAME_TRADE_ALREADY_CLOSE;
		res->hTarget = req->hTarget;
		res->bIsSet = req->bIsSet;
		packet.SetPacketLen(sizeof(sGU_TRADE_END_RES));
		g_pApp->Send(GetHandle(), &packet);
	}
}

//--------------------------------------------------------------------------------------//
//		DENY TRADE
//--------------------------------------------------------------------------------------//
void CClientSession::RecvTradeDenyReq(CNtlPacket * pPacket)
{
	sUG_TRADE_DENY_REQ* req = (sUG_TRADE_DENY_REQ *)pPacket->GetPacketData();



}



//--------------------------------------------------------------------------------------//
//		WHOLE QUEST FUNCTIONS
//--------------------------------------------------------------------------------------//
void CClientSession::RecvTsConfirmReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	WORD resultcode = RESULT_SUCCESS;
	sUG_TS_CONFIRM_STEP_REQ* req = (sUG_TS_CONFIRM_STEP_REQ *)pPacket->GetPacketData();

	//NTL_PRINT(PRINT_APP, "Player %u process quest %u req->tcCurId %u, req->tcNextId %u, byTsType %u", cPlayer->GetCharID(), req->tId, req->tcCurId, req->tcNextId, req->byTsType);
	//printf("req->byEventType %d, req->byTsType %d, req->dwEventData %d, req->dwParam %d %d %d %d, req->tcCurId %d, req->tcNextId %d, req->tId %d \n", req->byEventType, req->byTsType, req->dwEventData, req->adwParam[0], req->adwParam[1], req->adwParam[2], req->adwParam[3], req->tcCurId, req->tcNextId, req->tId);

	NTL_TS_TC_ID nextId = req->tcNextId;

	switch (req->byTsType)
	{
	case TS_TYPE_QUEST_CS:
	{
		CNtlTSTrigger* pTrig = (CNtlTSTrigger*)g_pTriggerManager->FindQuestFromTS(req->tId);
		if (pTrig)
		{
			CNtlTSCont* pCurCont = NULL;

			CQuestProgress* pQuest = NULL;
			sPROGRESS_QUEST_INFO* qinfo = NULL;

			pQuest = cPlayer->GetQuests()->GetQuestProgress(req->tId);
			if (pQuest)
			{
				qinfo = pQuest->GetProgressInfo();
				pCurCont = (CNtlTSCont*)pTrig->GetGroup(qinfo->uData.sQInfoV0.tgExcCGroup)->GetChildCont(qinfo->uData.sQInfoV0.sMainTSP.tcCurId);
			}
			else
				pCurCont = (CNtlTSCont*)pTrig->GetGroup(NTL_TS_MAIN_GROUP_ID)->GetChildCont(START_CONTAINER_ID);

			if (pCurCont)
			{
				//printf("pCurCont->GetEntityType(): %s \n", pCurCont->GetClassNameA());
				switch (pCurCont->GetEntityType())
				{
					case DBO_CONT_TYPE_ID_CONT_GCOND:
					{
						CDboTSContGCond* pCond = (CDboTSContGCond*)pCurCont;
						if (qinfo)
						{
							if (qinfo->uData.sQInfoV0.sMainTSP.tcCurId == pCond->GetID())
							{
								qinfo->uData.sQInfoV0.sMainTSP.tcPreId = pCond->GetID();
								qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pCond->GetYesLinkID();

								if (req->tcNextId == pCond->GetYesLinkID())
								{
									for (int i = 0; i < pCurCont->GetNumOfChildEntity(); i++)
									{
										resultcode = cPlayer->GetQuests()->ProgressTsEntity(pCond->GetChildEntity(i), req->tId, pQuest);
										if (resultcode != RESULT_SUCCESS)
										{
											nextId = pCond->GetNoLinkID();
											qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pCond->GetNoLinkID();
											break;
										}
									}
								}
								else if (req->tcNextId == pCond->GetNoLinkID())
								{
									qinfo->uData.sQInfoV0.sMainTSP.tcCurId = req->tcNextId;
								}
								else
								{
									nextId = pCond->GetNoLinkID();
									qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pCond->GetNoLinkID();
									resultcode = GAME_TS_ERROR_SYSTEM;
								}
							}
							else
							{
								nextId = pCond->GetNoLinkID();
								qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pCond->GetNoLinkID();
								resultcode = GAME_TS_ERROR_SYSTEM;
							}
						}
						else
						{
							nextId = pCond->GetNoLinkID();
							resultcode = GAME_TS_ERROR_SYSTEM;
						}
				}
				break;
				case DBO_CONT_TYPE_ID_CONT_START:
				{
					CDboTSContStart* pStartCont = (CDboTSContStart*)pCurCont;
					if (qinfo == NULL)
					{
						if (cPlayer->GetQuests()->HasBeenClearQuest(req->tId) == false || pTrig->IsRepeatQuest())
						{
							if (cPlayer->GetQuests()->CheckQuestCounter(req->tId))
							{
								if (req->tcNextId == pStartCont->GetYesLinkID())
								{
									nextId = pStartCont->GetYesLinkID();

									for (int i = 0; i < pStartCont->GetNumOfChildEntity(); i++)
									{
										resultcode = cPlayer->GetQuests()->ProgressTsEntity(pStartCont->GetChildEntity(i), req->tId, pQuest);
										if (resultcode != RESULT_SUCCESS)
										{
											nextId = pStartCont->GetNoLinkID();
											break;
										}
									}
								}
								else if (req->tcNextId == pStartCont->GetNoLinkID())
								{
									nextId = pStartCont->GetNoLinkID();
								}
								else
								{
									nextId = pStartCont->GetNoLinkID();
									resultcode = GAME_TS_ERROR_SYSTEM;
								}
							}
							else
							{
								nextId = pStartCont->GetNoLinkID();
								resultcode = GAME_QUEST_COUNT_OVER;
							}
						}
						else
						{
							nextId = pStartCont->GetNoLinkID();
							resultcode = GAME_QUEST_ALREADY_EXIST;
						}

						qinfo = cPlayer->GetQuests()->StartQuest(req->tId, nextId)->GetProgressInfo(); //Start quest even when condition fail
					}
					else
					{
						nextId = pStartCont->GetNoLinkID();
						resultcode = GAME_TS_ERROR_SYSTEM;
					}
				}
				break;
				case DBO_CONT_TYPE_ID_CONT_GACT:
				{
					CDboTSContGAct* pAct = (CDboTSContGAct*)pCurCont;
					if (qinfo)
					{
						if (qinfo->uData.sQInfoV0.sMainTSP.tcCurId == pAct->GetID())
						{
							qinfo->uData.sQInfoV0.sMainTSP.tcPreId = pAct->GetID();
							qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pAct->GetNextLinkID();

							if (req->tcNextId == pAct->GetNextLinkID())
							{
								for (int i = 0; i < pCurCont->GetNumOfChildEntity(); i++)
								{
									resultcode = cPlayer->GetQuests()->ProgressTsEntity(pAct->GetChildEntity(i), req->tId, pQuest);
									if (resultcode != RESULT_SUCCESS)
									{
										qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pAct->GetID();
										nextId = pAct->GetID();
										break;
									}
								}
							}
							else if (req->tcNextId == pAct->GetErrorLinkID())
								qinfo->uData.sQInfoV0.sMainTSP.tcCurId = req->tcNextId;
							else
							{
								nextId = pAct->GetErrorLinkID();
								qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pAct->GetErrorLinkID();
								resultcode = GAME_TS_ERROR_SYSTEM;
							}
						}
						else
						{
							nextId = pAct->GetErrorLinkID();
							qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pAct->GetErrorLinkID();
							resultcode = GAME_TS_ERROR_SYSTEM;
						}
					}
					else
					{
						nextId = pAct->GetErrorLinkID();
						resultcode = GAME_TS_ERROR_SYSTEM;
					}
				}
				break;
				case DBO_CONT_TYPE_ID_CONT_REWARD:
				{
					CDboTSContReward* pReward = (CDboTSContReward*)pCurCont;
					if (qinfo)
					{
						if (qinfo->uData.sQInfoV0.sMainTSP.tcCurId == pReward->GetID())
						{
							qinfo->uData.sQInfoV0.sMainTSP.tcPreId = pReward->GetID();
							qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pReward->GetNextLinkID();

							if (req->tcNextId == pReward->GetNextLinkID())
							{
								for (int i = 0; i < pCurCont->GetNumOfChildEntity(); i++)
								{
									resultcode = cPlayer->GetQuests()->ProgressTsEntity(pReward->GetChildEntity(i), req->tId, pQuest);
									if (resultcode != RESULT_SUCCESS)
									{
										qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pReward->GetID(); //dont set error id.. Set current id so we can try and repeat
										nextId = pReward->GetID();
										break;
									}
								}

								if (resultcode == RESULT_SUCCESS)
								{
									if (pReward->GetRewardContType() == eREWARD_CONTAINER_TYPE_QUEST)
									{
										if (pReward->IsUseTable())
										{
											sQUEST_REWARD_TBLDAT* basereward = (sQUEST_REWARD_TBLDAT*)g_pTableContainer->GetQuestRewardTable()->FindData(pReward->GetRewardTableIndex());
											if (basereward)
											{
												BYTE rewardcount = 0;
												CQuestRewardSelectTable *selrwdtbl = g_pTableContainer->GetQuestRewardSelectTable();

												//Count reward items
												for (int cnt = 0; cnt < QUEST_REWARD_DEF_MAX_CNT; cnt++)
												{
													if (basereward->arsDefRwd[cnt].dwRewardIdx != INVALID_TBLIDX && basereward->arsDefRwd[cnt].byRewardType == eREWARD_TYPE_NORMAL_ITEM)
														rewardcount++;
												}
												for (int cnt = 0; cnt < QUEST_REWARD_SEL_MAX_CNT; cnt++)
												{
													if (basereward->arsSelRwd[cnt].dwRewardIdx != INVALID_TBLIDX && basereward->arsSelRwd[cnt].byRewardType == eREWARD_TYPE_NORMAL_ITEM)
													{
														rewardcount++;
														break; //break because only add 1 sel reward to count
													}
												}

												if (cPlayer->GetPlayerItemContainer()->CountEmptyInventory() >= rewardcount)
												{
													//DEFINED REWARD
													for (int rew = 0; rew < QUEST_REWARD_DEF_MAX_CNT; rew++)
													{
														switch (basereward->arsDefRwd[rew].byRewardType)
														{
														case TABLE_REWARD_TYPE_NORMAL_ITEM:
														{
															if (!g_pItemManager->CreateQuestRewardItem(cPlayer, (TBLIDX)basereward->arsDefRwd[rew].dwRewardIdx, (BYTE)basereward->arsDefRwd[rew].dwRewardVal))
															{
																resultcode = GAME_TS_WARNING_REWARD_FAIL;
																qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pReward->GetID();
																nextId = pReward->GetID();
																break;
															}
														}
														break;
														case TABLE_REWARD_TYPE_EXP:
														{
															if (cPlayer->IsReceiveExpDisabled() == false)
															{
																DWORD dwExp = basereward->arsDefRwd[rew].dwRewardVal + (basereward->arsDefRwd[rew].dwRewardVal * app->GetQuestExpRate() / 100);
																cPlayer->UpdateExp(dwExp, false);
															}
														}
														break;
														case TABLE_REWARD_TYPE_SKILL:
														{
															WORD wTemp;
															sSKILL_TBLDAT* pSkillTbldat = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData((TBLIDX)basereward->arsDefRwd[rew].dwRewardIdx);
															if (pSkillTbldat)
															{
																if (pSkillTbldat->bySkill_Class == NTL_SKILL_CLASS_HTB)
																{
																	cPlayer->GetHtbSkillManager()->LearnHtbSkill(pSkillTbldat->tblidx, wTemp);
																}
																else
																{
																	cPlayer->GetSkillManager()->LearnSkill(pSkillTbldat->tblidx, wTemp, false);

																	if (pSkillTbldat->byPC_Class_Change != INVALID_BYTE && cPlayer->GetClass() >= PC_CLASS_1_FIRST && cPlayer->GetClass() <= PC_CLASS_1_LAST)
																	{
																		if (cPlayer->GetClass() != pSkillTbldat->byPC_Class_Change)
																			cPlayer->UpdateClass(pSkillTbldat->byPC_Class_Change);
																	}
																}
															}
														}
														break;
														case TABLE_REWARD_TYPE_ZENY:
														{
															DWORD dwZeni = basereward->arsDefRwd[rew].dwRewardVal + (basereward->arsDefRwd[rew].dwRewardVal * app->GetQuestMoneyRate() / 100);
															if (cPlayer->CanReceiveZeni(dwZeni))
															{
																cPlayer->UpdateZeni(ZENNY_CHANGE_TYPE_REWARD, dwZeni, true, true);

																ERR_LOG(LOG_USER, "Player: %u receive %u zeni from quest %u", cPlayer->GetCharID(), dwZeni, req->tId);
															}
															else
															{
																ERR_LOG(LOG_USER, "Player %u could not receive zeni %u. Player Zeni %u", cPlayer->GetCharID(), basereward->arsDefRwd[rew].dwRewardVal, cPlayer->GetZeni());
															}
														}
														break;

														case TABLE_REWARD_TYPE_PASSIVE_SKILL:
														{
															sQUEST_REWARD_SELECT_TBLDAT* selrwdtblData = (sQUEST_REWARD_SELECT_TBLDAT*)selrwdtbl->FindData((TBLIDX)basereward->arsDefRwd[rew].dwRewardIdx);
															if (selrwdtblData)
															{
																for (int rewsel = 0; rewsel < DBO_MAX_COUNT_OF_QUEST_REWARD_SELECT; rewsel++)
																{
																	if (selrwdtblData->aRewardSet[rewsel].byRewardType == eREWARD_TYPE_NORMAL_ITEM)
																	{
																		sITEM_TBLDAT* pItemRewardSlTbldat = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(selrwdtblData->aRewardSet[rewsel].dwRewardIdx);
																		if (pItemRewardSlTbldat)
																		{

																			if (Dbo_CheckClass(cPlayer->GetClass(), pItemRewardSlTbldat->dwNeed_Class_Bit_Flag) == true) //check class
																			{
																				if (cPlayer->GetPlayerItemContainer()->CountEmptyInventory() >= 1)
																				{
																					if (!g_pItemManager->CreateQuestRewardItem(cPlayer, (TBLIDX)selrwdtblData->aRewardSet[rewsel].dwRewardIdx, (BYTE)selrwdtblData->aRewardSet[rewsel].dwRewardVal))
																					{
																						resultcode = GAME_TS_WARNING_REWARD_FAIL;
																						qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pReward->GetID();
																						nextId = pReward->GetID();
																						break;
																					}
																					break;
																				}
																				else
																				{
																					resultcode = GAME_TS_WARNING_INVENTORY_IS_FULL;
																					qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pReward->GetID();
																					nextId = pReward->GetID();
																					break;
																				}
																			}
																		}
																		else
																		{
																			resultcode = GAME_TS_WARNING_REWARD_FAIL;
																			qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pReward->GetID();
																			nextId = pReward->GetID();
																			break;
																		}
																	}
																	else if (selrwdtblData->aRewardSet[rewsel].byRewardType == eREWARD_TYPE_SKILL)
																	{
																		sSKILL_TBLDAT* pSkillTbldat = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData(selrwdtblData->aRewardSet[rewsel].dwRewardIdx);
																		if (pSkillTbldat)
																		{
																			if (Dbo_CheckClass(cPlayer->GetClass(), pSkillTbldat->dwPC_Class_Bit_Flag) == true) //check class
																			{
																				WORD wTemp;
																				cPlayer->GetSkillManager()->LearnSkill(pSkillTbldat->tblidx, wTemp, false);
																				break;
																			}
																		}
																		else
																		{
																			resultcode = GAME_TS_WARNING_REWARD_FAIL;
																			qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pReward->GetID();
																			nextId = pReward->GetID();
																			break;
																		}
																	}
																}
															}
															else
															{
																ERR_LOG(LOG_USER, "sQUEST_REWARD_SELECT_TBLDAT is null. Tblidx %u", basereward->arsDefRwd[rew].dwRewardIdx);
																resultcode = GAME_TS_WARNING_REWARD_FAIL;
																qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pReward->GetID();
																nextId = pReward->GetID();
																break;
															}
														}
														break;

														default:
														{
															if (basereward->arsDefRwd[rew].byRewardType != INVALID_TEMP_REWARD_TYPE)
															{
																ERR_LOG(LOG_USER, "Fail to give defined reward. RewardType %u not set.", basereward->arsDefRwd[rew].byRewardType);

																resultcode = GAME_TS_WARNING_REWARD_FAIL;
																qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pReward->GetID();
																nextId = pReward->GetID();
																break;
															}
														}
														break;
														}
													}

													//SELECTED REWARD
													for (int rew = 0; rew < QUEST_REWARD_SEL_MAX_CNT; rew++)
													{
														if (basereward->arsSelRwd[rew].dwRewardIdx != INVALID_TBLIDX)
														{
															//REWARD FROM REWARD_SELECT_TBLDAT (only contain reward for class as example armor)
															if (basereward->arsSelRwd[rew].byRewardType == eREWARD_TYPE_USE_SELECT_REWARD_TABLE)
															{
																sQUEST_REWARD_SELECT_TBLDAT* selrwdtblData = (sQUEST_REWARD_SELECT_TBLDAT*)selrwdtbl->FindData(basereward->arsSelRwd[rew].dwRewardIdx);
																if (selrwdtblData)
																{
																	for (int rewsel = 0; rewsel < DBO_MAX_COUNT_OF_QUEST_REWARD_SELECT; rewsel++)
																	{
																		if (rewsel == req->adwParam[rew])
																		{
																			if (selrwdtblData->aRewardSet[rewsel].byRewardType == eREWARD_TYPE_NORMAL_ITEM)
																			{
																				sITEM_TBLDAT* pItemData = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(selrwdtblData->aRewardSet[rewsel].dwRewardIdx);
																				if (pItemData)
																				{
																					if (Dbo_CheckClass(cPlayer->GetClass(), pItemData->dwNeed_Class_Bit_Flag) == true) //check class
																					{
																						if (!g_pItemManager->CreateQuestRewardItem(cPlayer, (TBLIDX)selrwdtblData->aRewardSet[rewsel].dwRewardIdx, (BYTE)selrwdtblData->aRewardSet[rewsel].dwRewardVal))
																						{
																							resultcode = GAME_TS_WARNING_REWARD_FAIL;
																							qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pReward->GetID();
																							nextId = pReward->GetID();
																							break;
																						}
																					}
																				}
																			}

																			break;
																		}
																	}
																}
																break;
															}
														}
													}

													if (resultcode == RESULT_SUCCESS)
													{
														if (basereward->dwDef_Reward_Zeny > 0)
														{
															DWORD dwZeni = basereward->dwDef_Reward_Zeny + (basereward->dwDef_Reward_Zeny * app->GetQuestMoneyRate() / 100);
															if (cPlayer->CanReceiveZeni(dwZeni))
															{
																cPlayer->UpdateZeni(ZENNY_CHANGE_TYPE_REWARD, dwZeni, true, true);

																ERR_LOG(LOG_USER, "Player: %u receive %u zeni from quest %u", cPlayer->GetCharID(), dwZeni, req->tId);
															}
															else
																ERR_LOG(LOG_USER, "Player %u could not receive zeni %u. Player Zeni %u", cPlayer->GetCharID(), dwZeni, cPlayer->GetZeni());
														}

														if (basereward->dwDef_Reward_EXP > 0)
														{
															if (cPlayer->IsReceiveExpDisabled() == false)
															{
																DWORD dwExp = basereward->dwDef_Reward_EXP + (basereward->dwDef_Reward_EXP * app->GetQuestExpRate() / 100);
																cPlayer->UpdateExp(dwExp, false);
															}
														}
													}
												}
												else
												{
													resultcode = GAME_TS_WARNING_INVENTORY_IS_FULL;
													qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pReward->GetID();
													nextId = pReward->GetID();
												}
											}
											else
											{
												ERR_LOG(LOG_USER, "QUEST %u NO REWARD TBLIDX %d found", req->tId, pReward->GetRewardTableIndex());
												resultcode = GAME_TS_WARNING_INVENTORY_IS_FULL;
												qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pReward->GetID();
												nextId = pReward->GetID();
											}
										}
										else
										{
											BYTE rewardcount = 0;
											bool bselreward = false;

											//Count reward items
											for (int cnt = 0; cnt < QUEST_REWARD_DEF_MAX_CNT; cnt++)
											{
												const sREWARD_INFO& info = pReward->GetDefRewardInfo(cnt, NULL);
												if (info.m_uiIdx != INVALID_TBLIDX && info.m_uiIdx != 0)
													rewardcount++;
											}

											if (rewardcount == 0) //if 0 then SelReward used
											{
												bselreward = true;
												rewardcount = 1;
											}

											if (cPlayer->GetPlayerItemContainer()->CountEmptyInventory() >= rewardcount || bselreward == true)
											{
												if (bselreward == false)
												{
													for (int rew = 0; rew < QUEST_REWARD_DEF_MAX_CNT; rew++)
													{
														const sREWARD_INFO& info = pReward->GetDefRewardInfo(rew, NULL);
														switch (info.m_eType)
														{
														case eREWARD_TYPE_NORMAL_ITEM:
														{
															if (!g_pItemManager->CreateQuestRewardItem(cPlayer, info.m_uiIdx, info.m_nValue))
															{
																resultcode = GAME_TS_WARNING_REWARD_FAIL;
																qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pReward->GetID();
																nextId = pReward->GetID();
																break;
															}
														}
														break;
														case eREWARD_TYPE_EXP:
														{
															if (cPlayer->IsReceiveExpDisabled() == false)
															{
																cPlayer->UpdateExp((DWORD)info.m_nValue, false);
															}
														}
														break;
														case eREWARD_TYPE_SKILL:
														{
															WORD wTemp;
															sSKILL_TBLDAT* pSkillTbldat = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData(info.m_uiIdx);
															if (pSkillTbldat)
															{
																if (pSkillTbldat->bySkill_Class == NTL_SKILL_CLASS_HTB)
																{
																	cPlayer->GetHtbSkillManager()->LearnHtbSkill(pSkillTbldat->tblidx, wTemp);
																}
																else
																{
																	cPlayer->GetSkillManager()->LearnSkill(pSkillTbldat->tblidx, wTemp, false);

																	if (pSkillTbldat->byPC_Class_Change != INVALID_BYTE && cPlayer->GetClass() >= PC_CLASS_1_FIRST && cPlayer->GetClass() <= PC_CLASS_1_LAST)
																	{
																		if (cPlayer->GetClass() != pSkillTbldat->byPC_Class_Change)
																			cPlayer->UpdateClass(pSkillTbldat->byPC_Class_Change);
																	}
																}
															}
														}
														case eREWARD_TYPE_ZENY:
														{
															if (cPlayer->CanReceiveZeni((DWORD)info.m_nValue))
															{
																cPlayer->UpdateZeni(ZENNY_CHANGE_TYPE_REWARD, (DWORD)info.m_nValue, true, true);

																ERR_LOG(LOG_USER, "Player: %u receive %u zeni from quest %u", cPlayer->GetCharID(), (DWORD)info.m_nValue, req->tId);
															}
															else
																ERR_LOG(LOG_USER, "Player %u could not receive zeni %u. Player Zeni %u", cPlayer->GetCharID(), info.m_nValue, cPlayer->GetZeni());
														}
														break;
														case eREWARD_TYPE_CHANGE_ADULT:
														{
															if (!cPlayer->IsAdult())
															{
																cPlayer->UpdateAdult(true);
															}
														}
														break;
														case eREWARD_TYPE_GET_CONVERT_CLASS_RIGHT: //check if can change class
														{
															CNtlPacket packetChangeClass(sizeof(sGU_CHANGE_CLASS_AUTHORITY_CHANGED_NFY));
															sGU_CHANGE_CLASS_AUTHORITY_CHANGED_NFY * res = (sGU_CHANGE_CLASS_AUTHORITY_CHANGED_NFY *)packetChangeClass.GetPacketData();
															res->wOpCode = GU_CHANGE_CLASS_AUTHORITY_CHANGED_NFY;

															if (cPlayer->GetClass() >= PC_CLASS_1_FIRST && cPlayer->GetClass() <= PC_CLASS_1_LAST)
																res->bCanChangeClass = true;
															else
																res->bCanChangeClass = false;

															packetChangeClass.SetPacketLen(sizeof(sGU_CHANGE_CLASS_AUTHORITY_CHANGED_NFY));
															cPlayer->SendPacket(&packetChangeClass);

														}
														break;

														default:
														{
															if (info.m_eType != eREWARD_TYPE_INVALID)
																ERR_LOG(LOG_USER, "Fail to give defined reward. RewardType %u not set.", info.m_eType);
														}
														break;
														}
													}
												}
												else
												{
													for (int rewsel = 0; rewsel < NTL_QUEST_MAX_SELECT_REWARD; rewsel++)
													{
														if (rewsel == req->adwParam[0])
														{
															const sREWARD_INFO& info = pReward->GetSelRewardInfo(rewsel, NULL);
															switch (info.m_eType)
															{
															case eREWARD_TYPE_NORMAL_ITEM:
															{
																if (!g_pItemManager->CreateQuestRewardItem(cPlayer, info.m_uiIdx, info.m_nValue))
																{
																	resultcode = GAME_TS_WARNING_REWARD_FAIL;
																	qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pReward->GetID();
																	nextId = pReward->GetID();
																	break;
																}
															}
															break;

															default:
															{
																if (info.m_eType != eREWARD_TYPE_INVALID)
																	ERR_LOG(LOG_USER, "Fail to give defined reward. RewardType %u not set.", info.m_eType);
															}
															break;
															}

															break;
														}
													}
												}
											}
											else
											{
												resultcode = GAME_TS_WARNING_INVENTORY_IS_FULL;
												qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pReward->GetID();
												nextId = pReward->GetID();
											}
										}
									}
									else // TMQ EXP REWARD ( ALWAYS EXP !! ) TODO: Calculation to gain exp
									{
										if (cPlayer->IsReceiveExpDisabled() == false)
										{
											if (pReward->GetSelRewardInfo(0, NULL).m_nValue > 0)
												cPlayer->UpdateExp((DWORD)pReward->GetSelRewardInfo(0, NULL).m_nValue, false);
											else if (pReward->GetDefRewardInfo(0, NULL).m_nValue > 0)
												cPlayer->UpdateExp((DWORD)pReward->GetDefRewardInfo(0, NULL).m_nValue, false);
										}
									}
								}
							}
							else if (req->tcNextId == pReward->GetCancelLinkID())
								qinfo->uData.sQInfoV0.sMainTSP.tcCurId = req->tcNextId;
							else
							{
								nextId = pReward->GetCancelLinkID();
								qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pReward->GetCancelLinkID();
								resultcode = GAME_TS_ERROR_SYSTEM;
							}
						}
						else
						{
							nextId = pReward->GetCancelLinkID();
							qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pReward->GetCancelLinkID();
							resultcode = GAME_TS_ERROR_SYSTEM;
						}
					}
					else
					{
						nextId = pReward->GetCancelLinkID();
						resultcode = GAME_TS_ERROR_SYSTEM;
					}
				}
				break;
				case DBO_CONT_TYPE_ID_CONT_END:
				{
					CDboTSContEnd* pEnd = (CDboTSContEnd*)pCurCont;
					if (qinfo)
					{
						if (qinfo->uData.sQInfoV0.sMainTSP.tcCurId == pEnd->GetID())
						{
							if (pEnd->GetID() == NTL_TS_EXCEPT_GIVEUP_ID)
								pQuest->RunGroup(NTL_TS_EXCEPT_GIVEUP_ID);

							//When enable this, then some items will get removed after finish quest. Example when finish master class quest, then the item to start other master class quest gets removed.
							//if (pEnd->GetID() == 253)
							//	pQuest->RunGroup(NTL_TS_EXCEPT_GIVEUP_ID);
							
							if (pEnd->GetEndType() == eEND_TYPE_NOT_PROGRESS) //if end quest but it didnt success finish
							{
								if (cPlayer->GetQuests()->CanStoreQuestInDatabase(qinfo->tId)) //only normal quests can be stored in database
								{
									CNtlPacket pQry(sizeof(sGQ_QUEST_PROGRESS_DATA_DELETE_REQ));
									sGQ_QUEST_PROGRESS_DATA_DELETE_REQ * rQry = (sGQ_QUEST_PROGRESS_DATA_DELETE_REQ *)pQry.GetPacketData();
									rQry->wOpCode = GQ_QUEST_PROGRESS_DATA_DELETE_REQ;
									rQry->charId = cPlayer->GetCharID();
									rQry->handle = cPlayer->GetID();
									rQry->questID = req->tId;
									pQry.SetPacketLen(sizeof(sGQ_QUEST_PROGRESS_DATA_DELETE_REQ));
									app->SendTo(app->GetQueryServerSession(), &pQry);
								}
							}
							else if (pEnd->GetEndType() == eEND_TYPE_COMPLETE) //if quest successfuly finished
							{
								qinfo->uData.sQInfoV0.wQState = INVALID_WORD;
								qinfo->uData.sQInfoV0.taQuestInfo = NTL_TS_TA_ID_INVALID;
								qinfo->uData.sQInfoV0.tcQuestInfo = pEnd->GetID();
								qinfo->uData.sQInfoV0.sMainTSP.tcPreId = pEnd->GetID();
								qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pEnd->GetNextLinkID();

								if (cPlayer->GetQuests()->CanStoreQuestInDatabase(qinfo->tId)) //only normal quests can be stored in database
								{
									CNtlPacket pQry(sizeof(sGQ_QUEST_PROGRESS_DATA_CREATE_REQ));
									sGQ_QUEST_PROGRESS_DATA_CREATE_REQ * rQry = (sGQ_QUEST_PROGRESS_DATA_CREATE_REQ *)pQry.GetPacketData();
									rQry->wOpCode = GQ_QUEST_PROGRESS_DATA_CREATE_REQ;
									rQry->charId = cPlayer->GetCharID();
									rQry->handle = cPlayer->GetID();
									memcpy(&rQry->progressInfo, qinfo, sizeof(sPROGRESS_QUEST_INFO));
									rQry->bIsComplete = true;
									pQry.SetPacketLen(sizeof(sGQ_QUEST_PROGRESS_DATA_CREATE_REQ));
									app->SendTo(app->GetQueryServerSession(), &pQry);
								}

								cPlayer->GetQuests()->SetClearQuest(qinfo->tId); //all quests which finish except repeat-able ones should be marked as cleared else TLQ/dungeons wont work correct
							}
						}
						else
						{
							resultcode = GAME_TS_ERROR_SYSTEM;
						}

						cPlayer->GetQuests()->EraseQuest(qinfo->tId);
					}
					else resultcode = GAME_TS_ERROR_TRIGGER_SYSTEM;
				}
				break;
				case DBO_CONT_TYPE_ID_CONT_USERSEL:
				{
					CDboTSContUsrSel* pSel = (CDboTSContUsrSel*)pCurCont;
					if (qinfo)
					{
						if (qinfo->uData.sQInfoV0.sMainTSP.tcCurId == pSel->GetID())
						{
							qinfo->uData.sQInfoV0.sMainTSP.tcPreId = pSel->GetID();

							if (pSel->IsNextLink(req->tcNextId) == true)
							{
								qinfo->uData.sQInfoV0.sMainTSP.tcCurId = req->tcNextId;
							}
							else if (req->tcNextId == pSel->GetCancelLink())
								qinfo->uData.sQInfoV0.sMainTSP.tcCurId = req->tcNextId;
							else
							{
								nextId = pSel->GetCancelLink();
								qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pSel->GetCancelLink();
								resultcode = GAME_TS_ERROR_SYSTEM;
							}
						}
						else
						{
							nextId = pSel->GetCancelLink();
							qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pSel->GetCancelLink();
							resultcode = GAME_TS_ERROR_SYSTEM;
						}
					}
					else
					{
						nextId = pSel->GetCancelLink();
						resultcode = GAME_TS_ERROR_SYSTEM;
					}
				}
				break;
				case DBO_CONT_TYPE_ID_CONT_NARRATION:
				{
					CDboTSContNarration* pCont = (CDboTSContNarration*)pCurCont;
					if (qinfo)
					{
						if (qinfo->uData.sQInfoV0.sMainTSP.tcCurId == pCont->GetID())
						{
							qinfo->uData.sQInfoV0.sMainTSP.tcPreId = pCont->GetID();

							if(req->tcNextId == pCont->GetOkLink())
							{
								qinfo->uData.sQInfoV0.sMainTSP.tcCurId = req->tcNextId;
							}
							else if (req->tcNextId == pCont->GetCancelLink())
								qinfo->uData.sQInfoV0.sMainTSP.tcCurId = req->tcNextId;
							else
							{
								nextId = pCont->GetCancelLink();
								qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pCont->GetCancelLink();
								resultcode = GAME_TS_ERROR_SYSTEM;
							}
						}
						else
						{
							nextId = pCont->GetCancelLink();
							qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pCont->GetCancelLink();
							resultcode = GAME_TS_ERROR_SYSTEM;
						}
					}
					else
					{
						nextId = pCont->GetCancelLink();
						resultcode = GAME_TS_ERROR_SYSTEM;
					}
				}
				break;
				case DBO_CONT_TYPE_ID_CONT_PROPOSAL:
				{
					CDboTSContProposal* pContProposal = (CDboTSContProposal*)pCurCont;
					if (qinfo)
					{
						if (qinfo->uData.sQInfoV0.sMainTSP.tcCurId == pContProposal->GetID())
						{
							qinfo->uData.sQInfoV0.sMainTSP.tcPreId = pContProposal->GetID();

							if (req->tcNextId == pContProposal->GetOkLink())
							{
								qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pContProposal->GetOkLink();
							}
							else if (req->tcNextId == pContProposal->GetCancelLink())
								qinfo->uData.sQInfoV0.sMainTSP.tcCurId = req->tcNextId;
							else
							{
								nextId = pContProposal->GetCancelLink();
								qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pContProposal->GetCancelLink();
								resultcode = GAME_TS_ERROR_SYSTEM;
							}
						}
						else
						{
							nextId = pContProposal->GetCancelLink();
							qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pContProposal->GetCancelLink();
							resultcode = GAME_TS_ERROR_SYSTEM;
						}
					}
					else
					{
						nextId = pContProposal->GetCancelLink();
						resultcode = GAME_TS_ERROR_SYSTEM;
					}
				}
				break;
				case DBO_CONT_TYPE_ID_CONT_SWITCH:
				{
					ERR_LOG(LOG_USER,"QUEST-PROCESS: DBO_CONT_TYPE_ID_CONT_SWITCH - NEED TO DO -");
				}
				break;
				case DBO_CONT_TYPE_ID_CONT_UNIFIED_NARRATION: //just normal npc talk I think
				{
					//ERR_LOG(LOG_USER,"QUEST-PROCESS: DBO_CONT_TYPE_ID_CONT_UNIFIED_NARRATION");
					CDboTSContUnifiedNarration* pUnifiedNar = (CDboTSContUnifiedNarration*)pCurCont;
					if (qinfo)
					{
						//TO-DO: find out about pUnifiedNar->GetLogInLink(); (Maybe used for quest in progres... call it when player login)
					//	NTL_PRINT(PRINT_APP,"pUnifiedNar: GetID %d OkLink %d GetLogInLink %u tcCurId %d tcNextId %d\n", pUnifiedNar->GetID(), pUnifiedNar->GetOkLink(), pUnifiedNar->GetLogInLink(), req->tcCurId, req->tcNextId);

						if (qinfo->uData.sQInfoV0.sMainTSP.tcCurId == pUnifiedNar->GetID())
						{
							qinfo->uData.sQInfoV0.sMainTSP.tcPreId = pUnifiedNar->GetID();

							if (req->tcNextId == pUnifiedNar->GetOkLink())
							{
								qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pUnifiedNar->GetOkLink();
							}
							else if (req->tcNextId == pUnifiedNar->GetCancelLink())
								qinfo->uData.sQInfoV0.sMainTSP.tcCurId = req->tcNextId;
							else
							{
								nextId = pUnifiedNar->GetCancelLink();
								qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pUnifiedNar->GetCancelLink();
								resultcode = GAME_TS_ERROR_SYSTEM;
							}
						}
						else
						{
							nextId = pUnifiedNar->GetCancelLink();
							qinfo->uData.sQInfoV0.sMainTSP.tcCurId = pUnifiedNar->GetCancelLink();
							resultcode = GAME_TS_ERROR_SYSTEM;
						}
					}
					else
					{
						nextId = pUnifiedNar->GetCancelLink();
						resultcode = GAME_TS_ERROR_SYSTEM;
					}
				}
				break;
				}
			}
			else
			{
				if(qinfo)
					ERR_LOG(LOG_USER, "PLAYER %d PROCESS QUEST %u TC (req->tcCurId %u (qinfo curid %u) req->tcNextId %u) GroupID %u FAILED. Cant find pCurCont", cPlayer->GetCharID(), req->tId, req->tcCurId, qinfo->uData.sQInfoV0.sMainTSP.tcCurId, req->tcNextId, qinfo->uData.sQInfoV0.tgExcCGroup);
				else
					ERR_LOG(LOG_USER, "PLAYER %d PROCESS QUEST %u TC (tcCurId %u tcNextId %u) FAILED. Cant find pCurCont", cPlayer->GetCharID(), req->tId, req->tcCurId, req->tcNextId);

				resultcode = GAME_TS_ERROR_RUN_ERROR;
				nextId = 255;
			}
		}
		else
		{
			resultcode = GAME_QUEST_NOT_EXIST;
			nextId = 255;
			ERR_LOG(LOG_USER, "QUEST %d DOESNT EXIST OR NOT LOADED. Player %d", req->tId, cPlayer->GetCharID());
		}
	}
	break;

	case TS_TYPE_PC_TRIGGER_CS:
	{
		CNtlTSTrigger* pTrig = (CNtlTSTrigger*)g_pTriggerManager->FindPcFromTS(req->tId);
		if (pTrig)
		{
			CNtlTSCont* pCurCont = (CNtlTSCont*)pTrig->GetGroup(NTL_TS_MAIN_GROUP_ID)->GetChildCont(req->tcCurId);
			if (pCurCont)
			{
				switch (pCurCont->GetEntityType())
				{
					case DBO_CONT_TYPE_ID_CONT_GCOND:
					{
						CDboTSContGCond* pCond = (CDboTSContGCond*)pCurCont;
						if (pCond)
						{
							for (int i = 0; i < pCurCont->GetNumOfChildEntity(); i++)
							{
								resultcode = cPlayer->GetQuests()->ProgressTsEntity(pCond->GetChildEntity(i), req->tId, NULL, NULL);
								if (resultcode != RESULT_SUCCESS)
								{
									break;
								}
							}
						}
					}
					break;
					case DBO_CONT_TYPE_ID_CONT_START:
					{
						CDboTSContStart* pStartCont = (CDboTSContStart*)pTrig->GetGroup(NTL_TS_MAIN_GROUP_ID)->GetChildCont(START_CONTAINER_ID);
						if (pStartCont)
						{
							for (int i = 0; i < pStartCont->GetNumOfChildEntity(); i++)
							{
								resultcode = cPlayer->GetQuests()->ProgressTsEntity(pStartCont->GetChildEntity(i), req->tId, NULL, NULL);
								if (resultcode != RESULT_SUCCESS)
								{
									break;
								}
							}

							if (resultcode == RESULT_SUCCESS)
							{
								
							}
						}
						else resultcode = GAME_TS_ERROR_TRIGGER_SYSTEM;
					}
					break;
					case DBO_CONT_TYPE_ID_CONT_GACT:
					{
						CDboTSContGAct* pAct = (CDboTSContGAct*)pCurCont;
						if (pAct)
						{
							for (int i = 0; i < pCurCont->GetNumOfChildEntity(); i++)
							{
								resultcode = cPlayer->GetQuests()->ProgressTsEntity(pAct->GetChildEntity(i), req->tId, NULL, NULL);
								if (resultcode != RESULT_SUCCESS)
								{
									break;
								}
							}
						}
					}
					break;
					case DBO_CONT_TYPE_ID_CONT_REWARD:
						break;
					case DBO_CONT_TYPE_ID_CONT_END:
					{
						CDboTSContEnd* pEnd = (CDboTSContEnd*)pCurCont;
						if (pEnd)
						{
							
						}
					}
					break;
					case DBO_CONT_TYPE_ID_CONT_USERSEL:
					{
						CDboTSContUsrSel* pSel = (CDboTSContUsrSel*)pCurCont;
						if (pSel)
						{
						}
						else resultcode = GAME_TS_ERROR_TRIGGER_SYSTEM;
					}
					break;
					case DBO_CONT_TYPE_ID_CONT_NARRATION:
					{
					}break;
					case DBO_CONT_TYPE_ID_CONT_PROPOSAL:
					{
					}break;
					case DBO_CONT_TYPE_ID_CONT_SWITCH:
					{
					}break;
					case DBO_CONT_TYPE_ID_CONT_UNIFIED_NARRATION:
					{
					}break;

					default:
					{
						ERR_LOG(LOG_USER, "PLAYER %d PROCESS PC-TRIGGER %d TC (%d %d) FAILED. Cant find pCurCont->GetClassName() %s", cPlayer->GetCharID(), req->tId, req->tcCurId, req->tcNextId, pCurCont->GetClassName());
						resultcode = GAME_TS_ERROR_NO_IMP_CONT_TYPE;
					}break;
				}
			}
			else
			{
				ERR_LOG(LOG_USER, "PLAYER %d PROCESS PC-TRIGGER %d TC %d FAILED. Cant find pCurCont", cPlayer->GetCharID(), req->tId, req->tcCurId);
				resultcode = GAME_TS_ERROR_NO_IMP_CONT_TYPE;
				nextId = 255;
			}
		}
		else
		{
			resultcode = GAME_TS_ERROR_CANNOT_FIND_TID;
			nextId = 255;
			ERR_LOG(LOG_USER, "PC TRIGGER %d DOESNT EXIST OR NOT LOADED", req->tId);
		}
	}
	break;


	default:
	{
		ERR_LOG(LOG_USER, "UG_TS_CONFIRM_STEP_REQ: Couldnt find TS Type. Player ID %d hacker?", cPlayer->GetCharID());
		resultcode = GAME_TS_ERROR_NO_IMP_CONT_TYPE;
		nextId = 255;
	}
	}

	//ERR_LOG(LOG_USER,"req->byEventType %d, req->byTsType %d, req->dwEventData %d, req->dwParam %d %d %d %d, req->tcCurId %d, req->tcNextId %d, req->tId %d", req->byEventType, req->byTsType, req->dwEventData, req->adwParam[0], req->adwParam[1], req->adwParam[2], req->adwParam[3], req->tcCurId, req->tcNextId, req->tId);

	CNtlPacket packet(sizeof(sGU_TS_CONFIRM_STEP_RES));
	sGU_TS_CONFIRM_STEP_RES * res = (sGU_TS_CONFIRM_STEP_RES *)packet.GetPacketData();
	res->wOpCode = GU_TS_CONFIRM_STEP_RES;
	res->byTsType = req->byTsType;
	res->tcCurId = req->tcCurId;
	res->tcNextId = nextId;
	res->tId = req->tId;
	res->wResultCode = resultcode;
	//res->dwParam
	packet.SetPacketLen(sizeof(sGU_TS_CONFIRM_STEP_RES));
	app->Send(GetHandle(), &packet);
}



//--------------------------------------------------------------------------------------//
//		WHOLE QUEST FUNCTIONS (WHEN START QUEST WITH ITEM)
//--------------------------------------------------------------------------------------//
void CClientSession::RecvTsConfirmForUseItemReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_TS_CONFIRM_STEP_FOR_USE_ITEM_REQ* req = (sUG_TS_CONFIRM_STEP_FOR_USE_ITEM_REQ *)pPacket->GetPacketData();

	CGameServer* app = (CGameServer*)g_pApp;

	NTL_TS_TC_ID nextId = req->tcNextId;
	WORD resultcode = RESULT_SUCCESS;

	CItem* pItem = cPlayer->GetPlayerItemContainer()->GetItem(req->hExcuteItem);
	if (pItem == NULL || pItem->GetCount() == 0 || IsInvenContainer(pItem->GetPlace()) == false)
	{
		resultcode = GAME_NEEDITEM_NOT_FOUND;
	}
	else
	{
		sUSE_ITEM_TBLDAT* pUseItemTbldat = (sUSE_ITEM_TBLDAT*)g_pTableContainer->GetUseItemTable()->FindData(pItem->GetTbldat()->Use_Item_Tblidx);
		if (pUseItemTbldat)
		{
			if((QUESTID)pUseItemTbldat->aSystem_Effect_Value[0] != req->tId && (QUESTID)pUseItemTbldat->aSystem_Effect_Value[1] != req->tId)
				resultcode = RESULT_FAIL;
		}
		else resultcode = GAME_NEEDITEM_NOT_FOUND;
	}

	if (resultcode != RESULT_SUCCESS)
	{
		CNtlPacket packet(sizeof(sGU_TS_CONFIRM_STEP_RES));
		sGU_TS_CONFIRM_STEP_RES * res = (sGU_TS_CONFIRM_STEP_RES *)packet.GetPacketData();
		res->wOpCode = GU_TS_CONFIRM_STEP_RES;
		res->byTsType = req->byTsType;
		res->tId = req->tId;
		res->wResultCode = resultcode;
		packet.SetPacketLen(sizeof(sGU_TS_CONFIRM_STEP_RES));
		app->Send(GetHandle(), &packet);

		return;
	}

	switch (req->byTsType)
	{
		case TS_TYPE_QUEST_CS:
		{
			CNtlTSTrigger* pTrig = (CNtlTSTrigger*)g_pTriggerManager->FindQuestFromTS(req->tId);
			if (pTrig != NULL)
			{
				if (req->tcCurId == START_CONTAINER_ID) //START QUEST DIALOG
				{
					CDboTSContStart* pStartCont = (CDboTSContStart*)pTrig->GetGroup(NTL_TS_MAIN_GROUP_ID)->GetChildCont(START_CONTAINER_ID);
					if (pStartCont)
					{
						if (cPlayer->GetQuests()->GetQuestProgress(req->tId) == NULL)
						{
							if (cPlayer->GetQuests()->HasBeenClearQuest(req->tId) == false || pTrig->IsRepeatQuest())
							{
								if (cPlayer->GetQuests()->CheckQuestCounter(req->tId))
								{
									if (req->tcNextId == pStartCont->GetYesLinkID())
									{
										nextId = pStartCont->GetYesLinkID();

										for (int i = 0; i < pStartCont->GetNumOfChildEntity(); i++)
										{
											resultcode = cPlayer->GetQuests()->ProgressTsEntity(pStartCont->GetChildEntity(i), req->tId, NULL);
											if (resultcode != RESULT_SUCCESS)
											{
												nextId = pStartCont->GetNoLinkID();
												break;
											}
										}
									}
									else if (req->tcNextId == pStartCont->GetNoLinkID())
									{
										nextId = pStartCont->GetNoLinkID();
									}
									else
									{
										nextId = pStartCont->GetNoLinkID();
										resultcode = GAME_TS_ERROR_SYSTEM;
									}
								}
								else
								{
									nextId = pStartCont->GetNoLinkID();
									resultcode = GAME_QUEST_COUNT_OVER;
								}
							}
							else
							{
								nextId = pStartCont->GetNoLinkID();
								resultcode = GAME_QUEST_ALREADY_EXIST;
							}

							cPlayer->GetQuests()->StartQuest(req->tId, nextId)->GetProgressInfo(); //Start quest even when condition fail
						}
						else
						{
							nextId = pStartCont->GetNoLinkID();
							resultcode = GAME_TS_ERROR_SYSTEM;
						}
					}
					else
					{
						nextId = pStartCont->GetNoLinkID();
						resultcode = GAME_TS_ERROR_SYSTEM;
					}
				}
				else
				{
					nextId = 255;
					resultcode = GAME_TS_ERROR_SYSTEM;
				}
			}
			else
			{
				resultcode = RESULT_FAIL;
				nextId = 255;
				ERR_LOG(LOG_USER, "QUEST %d DOESNT EXIST OR NOT LOADED", req->tId);
			}
		}
		break;

		case TS_TYPE_PC_TRIGGER_CS:
		{
			CNtlTSTrigger* pTrig = (CNtlTSTrigger*)g_pTriggerManager->FindPcFromTS(req->tId);
			if (pTrig)
			{
				if (req->tcCurId == 0)
				{
					CDboTSContStart* pStartCont = (CDboTSContStart*)pTrig->GetGroup(NTL_TS_MAIN_GROUP_ID)->GetChildCont(START_CONTAINER_ID);
					if (pStartCont)
					{
						if (cPlayer->GetQuests()->GetQuestProgress(req->tId) == NULL)
						{
							if (cPlayer->GetQuests()->HasBeenClearQuest(req->tId) == false || pTrig->IsRepeatQuest())
							{
								if (cPlayer->GetQuests()->CheckQuestCounter(req->tId))
								{
									if (req->tcNextId == pStartCont->GetYesLinkID())
									{
										nextId = pStartCont->GetYesLinkID();

										for (int i = 0; i < pStartCont->GetNumOfChildEntity(); i++)
										{
											resultcode = cPlayer->GetQuests()->ProgressTsEntity(pStartCont->GetChildEntity(i), req->tId, NULL);
											if (resultcode != RESULT_SUCCESS)
											{
												nextId = pStartCont->GetNoLinkID();
												break;
											}
										}
									}
									else if (req->tcNextId == pStartCont->GetNoLinkID())
									{
										nextId = pStartCont->GetNoLinkID();
									}
									else
									{
										nextId = pStartCont->GetNoLinkID();
										resultcode = GAME_TS_ERROR_SYSTEM;
									}
								}
								else
								{
									nextId = pStartCont->GetNoLinkID();
									resultcode = GAME_QUEST_COUNT_OVER;
								}
							}
							else
							{
								nextId = pStartCont->GetNoLinkID();
								resultcode = GAME_QUEST_ALREADY_EXIST;
							}

							cPlayer->GetQuests()->StartQuest(req->tId, nextId)->GetProgressInfo(); //Start quest even when condition fail
						}
						else
						{
							nextId = pStartCont->GetNoLinkID();
							resultcode = GAME_TS_ERROR_SYSTEM;
						}
					}
					else
					{
						nextId = pStartCont->GetNoLinkID();
						resultcode = GAME_TS_ERROR_TRIGGER_SYSTEM;
					}
				}
				else
				{
					nextId = 255;
					resultcode = GAME_TS_ERROR_TRIGGER_SYSTEM;
				}
			}
			else
			{
				resultcode = GAME_FAIL;
				nextId = 255;
				ERR_LOG(LOG_USER, "PC trigger %d not found", req->tId);
			}
		}
		break;

		case TS_TYPE_OBJECT_TRIGGER_S:
		{
			ERR_LOG(LOG_SYSTEM, "sUG_TS_CONFIRM_STEP_FOR_USE_ITEM_REQ: TS_TYPE_OBJECT_TRIGGER_S");
			nextId = 255;
			resultcode = GAME_FAIL;
		}
		break;

		default:
		{
			ERR_LOG(LOG_USER, "sUG_TS_CONFIRM_STEP_FOR_USE_ITEM_REQ: Couldnt find TS Type. Player ID %d hacker?", cPlayer->GetCharID());
			nextId = 255;
			resultcode = GAME_FAIL;
		}
		break;
	}

	CNtlPacket packet(sizeof(sGU_TS_CONFIRM_STEP_RES));
	sGU_TS_CONFIRM_STEP_RES * res = (sGU_TS_CONFIRM_STEP_RES *)packet.GetPacketData();
	res->wOpCode = GU_TS_CONFIRM_STEP_RES;
	res->byTsType = req->byTsType;
	res->tcCurId = req->tcCurId;
	res->tcNextId = nextId;
	res->tId = req->tId;
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_TS_CONFIRM_STEP_RES));
	app->Send(GetHandle(), &packet);
}



//--------------------------------------------------------------------------------------//
//		QUEST GIVE UP REQUEST
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvQuestGiveUpReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_QUEST_GIVEUP_REQ* req = (sUG_QUEST_GIVEUP_REQ *)pPacket->GetPacketData();
	WORD wRes = GAME_QUEST_NOT_EXIST;

	CQuestProgress* pQuest = cPlayer->GetQuests()->GetQuestProgress(req->tId);
	if (pQuest)
	{
		if (pQuest->GetProgressInfo()->uData.sQInfoV0.sMainTSP.tcCurId <= 101 && pQuest->GetProgressInfo()->uData.sQInfoV0.sMainTSP.tcPreId < 100) //check if reward has not been received yet
		{
			wRes = RESULT_SUCCESS;

			pQuest->RunGroup(NTL_TS_EXCEPT_GIVEUP_ID);	//run give up group

			cPlayer->GetQuests()->EraseQuest(req->tId);

			if (cPlayer->GetQuests()->CanStoreQuestInDatabase(req->tId))  //only normal quests can be stored in database !
			{
				CNtlPacket pQry(sizeof(sGQ_QUEST_PROGRESS_DATA_DELETE_REQ));
				sGQ_QUEST_PROGRESS_DATA_DELETE_REQ * rQry = (sGQ_QUEST_PROGRESS_DATA_DELETE_REQ *)pQry.GetPacketData();
				rQry->wOpCode = GQ_QUEST_PROGRESS_DATA_DELETE_REQ;
				rQry->charId = cPlayer->GetCharID();
				rQry->handle = cPlayer->GetID();
				rQry->questID = req->tId;
				pQry.SetPacketLen(sizeof(sGQ_QUEST_PROGRESS_DATA_DELETE_REQ));
				app->SendTo(app->GetQueryServerSession(), &pQry);
			}
		}
	}


	CNtlPacket packet(sizeof(sGU_QUEST_GIVEUP_RES));
	sGU_QUEST_GIVEUP_RES * res = (sGU_QUEST_GIVEUP_RES *)packet.GetPacketData();
	res->wOpCode = GU_QUEST_GIVEUP_RES;
	res->wResultCode = wRes;
	res->tId = req->tId;
	packet.SetPacketLen(sizeof(sGU_QUEST_GIVEUP_RES));
	app->Send(GetHandle(), &packet);
}

void CClientSession::RecvQuestShare(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_QUEST_SHARE* req = (sUG_QUEST_SHARE *)pPacket->GetPacketData();

	if (cPlayer->GetParty())
	{
		if (cPlayer->GetQuests()->GetQuestProgress(req->tId))
		{
			if (CNtlTSTrigger* pTrigger = g_pTriggerManager->FindQuestFromTS(req->tId))
			{
				if (pTrigger->IsShareQuest())
					cPlayer->GetParty()->ShareQuest(cPlayer->GetID(), req->tId);
			}
		}
	}
}

//--------------------------------------------------------------------------------------//
//		EXECUTE TRIGGER
//--------------------------------------------------------------------------------------//
void CClientSession::RecvTSExcuteTriggerObject(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	if (app->GetGsChannel() == DOJO_CHANNEL_INDEX)
		return;

	sUG_TS_EXCUTE_TRIGGER_OBJECT * req = (sUG_TS_EXCUTE_TRIGGER_OBJECT*)pPacket->GetPacketData();
	//printf("sUG_TS_EXCUTE_TRIGGER_OBJECT: byEvtGenType:%u hSource:%u hTarget:%u uiParam:%u\n", req->byEvtGenType, req->hSource, req->hTarget, req->uiParam);
	if (cPlayer->GetCurWorld())
	{
		CTriggerObject* obj = cPlayer->GetCurWorld()->FindStaticObject(req->hTarget);
		if (obj && BIT_FLAG_TEST(obj->GetSubState(), TOBJECT_SUBSTATE_FLAG_SHOW))
		{
			if (cPlayer->GetAspectStateId() != ASPECTSTATE_SPINNING_ATTACK && cPlayer->GetAspectStateId() != ASPECTSTATE_ROLLING_ATTACK)
			{
				if (obj->IsBeingExecuted() == false && cPlayer->IsInRange(obj->GetCurLoc(), DBO_DISTANCE_CHECK_TOLERANCE))
				{
					if (cPlayer->GetAspectStateId() == ASPECTSTATE_VEHICLE)
						cPlayer->EndVehicle(GAME_VEHICLE_END_BY_FORCED);

					//	printf("excute objtrigger GetTblidx %u, GetMainState %u, GetSubState %u \n", obj->GetTblidx(), obj->GetMainState(), obj->GetSubState());

					CNtlPacket packet(sizeof(sGU_TS_EXCUTE_TRIGGER_OBJECT_RES));
					sGU_TS_EXCUTE_TRIGGER_OBJECT_RES * res = (sGU_TS_EXCUTE_TRIGGER_OBJECT_RES *)packet.GetPacketData();
					res->wOpCode = GU_TS_EXCUTE_TRIGGER_OBJECT_RES;
					res->wResultCode = RESULT_SUCCESS;
					res->hTriggerObject = req->hTarget;
					packet.SetPacketLen(sizeof(sGU_TS_EXCUTE_TRIGGER_OBJECT_RES));
					app->Send(GetHandle(), &packet);

					obj->StartExecuting(cPlayer, false);
					return;
				}
			}
		}

		CNtlPacket packet(sizeof(sGU_TS_EXCUTE_TRIGGER_OBJECT_RES));
		sGU_TS_EXCUTE_TRIGGER_OBJECT_RES * res = (sGU_TS_EXCUTE_TRIGGER_OBJECT_RES *)packet.GetPacketData();
		res->wOpCode = GU_TS_EXCUTE_TRIGGER_OBJECT_RES;
		res->wResultCode = RESULT_FAIL;
		res->hTriggerObject = req->hTarget;
		packet.SetPacketLen(sizeof(sGU_TS_EXCUTE_TRIGGER_OBJECT_RES));
		app->Send(GetHandle(), &packet);
	}
}

//--------------------------------------------------------------------------------------//
//		
//--------------------------------------------------------------------------------------//
void CClientSession::RecvQuestItemMoveReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_QUEST_ITEM_MOVE_REQ * req = (sUG_QUEST_ITEM_MOVE_REQ*)pPacket->GetPacketData();

	cPlayer->GetQuests()->MoveQuestItem(req->bySrcPos, req->byDestPos);
}

//--------------------------------------------------------------------------------------//
//		
//--------------------------------------------------------------------------------------//
void CClientSession::RecvQuestItemDeleteReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_QUEST_ITEM_DELETE_REQ * req = (sUG_QUEST_ITEM_DELETE_REQ*)pPacket->GetPacketData();

	cPlayer->GetQuests()->DeleteQuestItem(req->byDeletePos);
}

//--------------------------------------------------------------------------------------//
//		QUEST OBJECT VISIT
//--------------------------------------------------------------------------------------//
void CClientSession::RecvQuestObjectVisitReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_QUEST_OBJECT_VISIT_REQ * req = (sUG_QUEST_OBJECT_VISIT_REQ*)pPacket->GetPacketData();

	WORD resultcode = GAME_FAIL;
	CNtlTSTrigger* pTrig = (CNtlTSTrigger*)g_pTriggerManager->FindQuestFromTS(req->qId); //get quest data
	sPROGRESS_QUEST_INFO* qinfo = cPlayer->GetQuests()->GetQuestProgressInfo(req->qId); //get quest info
	if (pTrig && qinfo)
	{
		CDboTSContGAct* pAct = (CDboTSContGAct*)pTrig->GetGroup(qinfo->uData.sQInfoV0.tgExcCGroup)->GetChildCont(qinfo->uData.sQInfoV0.sSToCEvtData.tcId);
		if (pAct)
		{
			CNtlTSEntity* pEntity = pAct->FindChildEntity(qinfo->uData.sQInfoV0.sSToCEvtData.taId);
			if (pEntity)
			{
				CDboTSActSToCEvt* pEvt = (CDboTSActSToCEvt*)pEntity;

				for (int i = 0; i < uSTOC_EVT_DATA::MAX_VISIT_EVT; i++)
				{
					if (cPlayer->GetWorldTblidx() == pEvt->GetEvtData().sVisitEvt[i].uiWorldTblIdx)
					{
						if (qinfo->uData.sQInfoV0.sSToCEvtData.m_aUserData[i] == 0)
						{
							if (pEvt->GetEvtData().sVisitEvt[i].byObjType == OBJTYPE_NPC || pEvt->GetEvtData().sVisitEvt[i].byObjType == OBJTYPE_TOBJECT)
							{
								if (pEvt->GetEvtData().sVisitEvt[i].uiObjTblIdx == req->objectTblidx)
								{
									//		ERR_LOG(LOG_USER,"RecvQuestObjectVisitReq byObjType %d", pEvt->GetEvtData().sVisitEvt[i].byObjType);

									resultcode = GAME_SUCCESS;
									qinfo->uData.sQInfoV0.sSToCEvtData.m_aUserData[i] = req->objectTblidx;

									CNtlPacket packet(sizeof(sGU_QUEST_SVREVT_UPDATE_NFY));
									sGU_QUEST_SVREVT_UPDATE_NFY * res = (sGU_QUEST_SVREVT_UPDATE_NFY *)packet.GetPacketData();
									res->wOpCode = GU_QUEST_SVREVT_UPDATE_NFY;
									res->taId = qinfo->uData.sQInfoV0.sSToCEvtData.taId;
									res->tcId = qinfo->uData.sQInfoV0.sSToCEvtData.tcId;
									res->tId = req->qId;
									res->bySlot = i;
									res->bySvrEvtType = eSTOC_EVT_DATA_TYPE_VISIT;
									res->uEvtData.sVisitEvt.bCompleted = true;
									packet.SetPacketLen(sizeof(sGU_QUEST_SVREVT_UPDATE_NFY));
									app->Send(GetHandle(), &packet);

									break;
								}
							}
							else
							{
								ERR_LOG(LOG_USER, "RecvQuestObjectVisitReq: OBJ TYPE %d NOT SET", pEvt->GetEvtData().sVisitEvt[i].byObjType);
							}
						}
					}
				}
			}
		}
	}

	//	ERR_LOG(LOG_USER,"sGU_QUEST_OBJECT_VISIT_RES RESULT CODE %d", resultcode);
	CNtlPacket packet(sizeof(sGU_QUEST_OBJECT_VISIT_RES));
	sGU_QUEST_OBJECT_VISIT_RES* res = (sGU_QUEST_OBJECT_VISIT_RES*)packet.GetPacketData();
	res->byObjType = req->byObjType;
	res->objectTblidx = req->objectTblidx;
	res->qId = req->qId;
	res->worldId = req->worldId;
	res->wOpCode = GU_QUEST_OBJECT_VISIT_RES;
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_QUEST_OBJECT_VISIT_RES));
	app->Send(GetHandle(), &packet);
}

//--------------------------------------------------------------------------------------//
//		UPDATE TRIGGER SYSTEM STATE
//--------------------------------------------------------------------------------------//
void CClientSession::RecvTSUpdateState(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_TS_UPDATE_STATE* req = (sUG_TS_UPDATE_STATE*)pPacket->GetPacketData();

	//printf("RecvTSUpdateState byTsType:%u byType:%u dwParam:%u tId:%u wTSState:%u", req->byTsType, req->byType, req->dwParam, req->tId, req->wTSState);

	sPROGRESS_QUEST_INFO* qinfo = cPlayer->GetQuests()->GetQuestProgressInfo(req->tId); //get quest info
	if (qinfo)
	{
		qinfo->uData.sQInfoV0.wQState = req->wTSState;
		//printf("req->wTSState %u req->tId %u\n", req->wTSState, req->tId);
		//did quest fail when start quest? If quest fails at start then we remove here. If quest fails while quest active then it has failed icon and player must manually remove it.
		if (req->wTSState == eTS_SVR_STATE_ERROR)
		{
			//printf("qinfo->uData.sQInfoV0.sMainTSP.tcPreId %u \n", qinfo->uData.sQInfoV0.sMainTSP.tcPreId);
			if (qinfo->uData.sQInfoV0.sMainTSP.tcPreId == START_CONTAINER_ID || qinfo->uData.sQInfoV0.sMainTSP.tcPreId >= 253)
			{
				cPlayer->GetQuests()->EraseQuest(req->tId);
			}
		}
	}

	CNtlPacket packet(sizeof(sGU_TS_UPDATE_STATE));
	sGU_TS_UPDATE_STATE* res = (sGU_TS_UPDATE_STATE*)packet.GetPacketData();
	res->wOpCode = GU_TS_UPDATE_STATE;
	res->byTsType = req->byTsType;
	res->byType = req->byType;
	res->wTSState = req->wTSState;
	res->tId = req->tId;
	res->dwParam = req->dwParam;
	g_pApp->Send(GetHandle(), &packet);
}

//--------------------------------------------------------------------------------------//
void CClientSession::SendPcTsDialogNfy(TBLIDX dialogTblidx)
{
	CNtlPacket packet(sizeof(sGU_TS_PC_DIALOG_NFY));
	sGU_TS_PC_DIALOG_NFY* res = (sGU_TS_PC_DIALOG_NFY*)packet.GetPacketData();
	res->wOpCode = GU_TS_PC_DIALOG_NFY;
	res->textTblidx = dialogTblidx;
	packet.SetPacketLen(sizeof(sGU_TS_PC_DIALOG_NFY));
	g_pApp->Send(GetHandle(), &packet);
}


//--------------------------------------------------------------------------------------//
//		USE ITEM
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvItemUseReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_ITEM_USE_REQ * req = (sUG_ITEM_USE_REQ*)pPacket->GetPacketData();
	WORD resultcode = GAME_SUCCESS;

	CNtlVector vLoc;
	NtlLocationDecompress(&req->vCurLoc, &vLoc.x, &vLoc.y, &vLoc.z);

	if (cPlayer->SetCurLoc(vLoc, cPlayer->GetCurWorld()))
	{
		sVECTOR3 sDir;
		NtlDirectionDecompress(&req->vCurDir, &sDir.x, &sDir.y, &sDir.z);
		cPlayer->SetCurDir(sDir);

		TBLIDX itemTblidx = INVALID_TBLIDX;
		CItem* pItem = cPlayer->GetPlayerItemContainer()->GetItem(req->byPlace, req->byPos);

		if (pItem && pItem->GetCount() > 0 && IsInvenContainer(pItem->GetPlace()))
		{
			sITEM_TBLDAT * pItemTbldat = pItem->GetTbldat();

			itemTblidx = pItemTbldat->tblidx;

			sUSE_ITEM_TBLDAT * pUseItemTbldat = (sUSE_ITEM_TBLDAT*)g_pTableContainer->GetUseItemTable()->FindData(pItemTbldat->Use_Item_Tblidx);
			if (pUseItemTbldat)
			{
				//printf("useWorldTblidx %u, dwUse_Allow_Rule_Bit_Flag %u \n", pUseItemTbldat->useWorldTblidx, pUseItemTbldat->dwUse_Allow_Rule_Bit_Flag);
				//CHECK IF VALID WORLD
				if(cPlayer->GetCurWorld() == NULL)
					resultcode = GAME_WORLD_NOT_FOUND;
				else if(pItem->IsExpired())
					resultcode = GAME_ITEM_DURATIONTIME_ZERO;
				else if (pUseItemTbldat->useWorldTblidx != INVALID_TBLIDX && cPlayer->GetWorldTblidx() != pUseItemTbldat->useWorldTblidx)
					resultcode = GAME_ITEM_CANT_USE_INVALID_WORLD;
				else if(cPlayer->GetCurWorld()->GetRuleType() == GAMERULE_CCBATTLEDUNGEON && BIT_FLAG_TEST(pUseItemTbldat->dwUse_Restriction_Rule_Bit_Flag, MAKE_BIT_FLAG(GAMERULE_NORMAL)) == false) //items which are allowed inside ccbd are not allowed in normal world
					resultcode = GAME_ITEM_CANT_USE_INVALID_WORLD;
				//CHECK ITEM COOL TIME
				else if (cPlayer->IsItemCooldown(pUseItemTbldat->dwCool_Time_Bit_Flag) == true)
					resultcode = GAME_ITEM_NOT_READY_TO_BE_USED;
				//CHECK MIN LEVEL
				else if (cPlayer->GetLevel() < pItemTbldat->byNeed_Min_Level)
					resultcode = GAME_ITEM_TOO_LOW_LEVEL_TO_USE_ITEM;
				//CHECK MAX LEVEL
				else if (cPlayer->GetLevel() > pItemTbldat->byNeed_Max_Level)
					resultcode = GAME_ITEM_TOO_HIGH_LEVEL_TO_USE_ITEM;
				//CHECK CLASS
				else if (Dbo_CheckClass(cPlayer->GetClass(), pItemTbldat->dwNeed_Class_Bit_Flag) == false)
					resultcode = GAME_ITEM_CLASS_FAIL;
				//CHECK GENDER
				else if (BIT_FLAG_TEST(MAKE_BIT_FLAG(cPlayer->GetGender()), pItemTbldat->dwNeed_Gender_Bit_Flag) == false)
					resultcode = GAME_ITEM_GENDER_DOESNT_MATCH;
				//CHECK RACE
				else if (pItemTbldat->byRace_Special != cPlayer->GetRace() && pItemTbldat->byRace_Special != INVALID_BYTE)
					resultcode = GAME_CHAR_RACE_FAIL;
				//CHECK IF CURRENT WORLD RULE TYPE IS INSIDE USE-RESTRICTION BITFLAG
				else if (pUseItemTbldat->dwUse_Restriction_Rule_Bit_Flag > 0 && BIT_FLAG_TEST(pUseItemTbldat->dwUse_Restriction_Rule_Bit_Flag, MAKE_BIT_FLAG(cPlayer->GetCurWorld()->GetRuleType())))
					resultcode = GAME_ITEM_CANT_USE_INVALID_WORLD;
				//check if require quest
				else if(pUseItemTbldat->RequiredQuestID != INVALID_QUESTID && cPlayer->GetQuests()->GetQuestProgress(pUseItemTbldat->RequiredQuestID) == NULL)
					resultcode = GAME_ITEM_YOU_HAVE_NO_RELATED_QUEST_TO_USE;
				//check if has required state
				else if(IsValidStateToUseItem(pItemTbldat->tblidx, pUseItemTbldat->wNeed_State_Bit_Flag, cPlayer->GetCharStateID(), cPlayer->GetAspectStateId(), cPlayer->GetConditionState(), false, cPlayer->GetMoveStatus() == NTL_MOVE_STATUS_JUMP, cPlayer->GetAirState()) == false)
					resultcode = GAME_CHAR_IS_WRONG_STATE;

				//CHECK IF TRADING OR PRIVATE SHOP
				else if (pItem->IsLocked())
					resultcode = GAME_ITEM_IS_LOCK;
				else if (cPlayer->GetCharStateID() == CHARSTATE_CASTING || cPlayer->GetCharStateID() == CHARSTATE_CASTING_ITEM)
					resultcode = GAME_COMMON_CANT_DO_THAT_IN_CASTING_STATE;
				else if (req->byKeyPlace != INVALID_BYTE && req->byKeyPos != INVALID_BYTE)
				{
					CItem* pKeyItem = cPlayer->GetPlayerItemContainer()->GetItem(req->byKeyPlace, req->byKeyPos);
					if (pKeyItem == NULL || pKeyItem->GetCount() == 0 || IsInvenContainer(pKeyItem->GetPlace()) == false)
						resultcode = GAME_CLOSED_BOX_KEY_NOT_FOUND;
					else if(pKeyItem->IsLocked())
						resultcode = GAME_ITEM_IS_LOCK;
					else
					{
						//if (pItemTbldat->NeedItemTblidx != pKeyItem->GetTblidx()) //dont work
						//	resultcode = GAME_CLOSED_BOX_KEY_MISMATCH;

						resultcode = GAME_CLOSED_BOX_SUCCESS;
					}
				}
				else if (pUseItemTbldat->useWorldTblidx != INVALID_TBLIDX)
				{
					CNtlVector vec(pUseItemTbldat->fUseLoc_X, 0, pUseItemTbldat->fUseLoc_Z);
					if(cPlayer->GetWorldTblidx() != pUseItemTbldat->useWorldTblidx || !cPlayer->IsInRange(vec, pUseItemTbldat->fUseLoc_Radius))
						resultcode = GAME_ITEM_NOT_PROPER_PLACE_TO_USE;
				}
				else if (pItem->GetTblidx() == 19451)
				{
					if (CTriggerObject* pTrigObj = cPlayer->GetCurWorld()->FindStaticObject(req->hTarget))
					{
						if (NtlGetDistance(cPlayer->GetCurLoc(), pTrigObj->GetCurLoc()) > 15.f)
							resultcode = GAME_TARGET_TOO_FAR;
					}
					else
					{
						resultcode = GAME_TARGET_NOT_FOUND;
					}
				}
				//printf("pUseItemTbldat->dwUse_Restriction_Rule_Bit_Flag %u, dwUse_Allow_Rule_Bit_Flag %u \n", pUseItemTbldat->dwUse_Restriction_Rule_Bit_Flag, pUseItemTbldat->dwUse_Allow_Rule_Bit_Flag);
				if (resultcode == GAME_SUCCESS || resultcode == GAME_CLOSED_BOX_SUCCESS)
				{
				//	NTL_PRINT(PRINT_APP, "USE-ITEM: hRefObject %u pItem->GetID() %u item-tblidx %u ItemID %zu. Player %u", req->hRefObject, pItem->GetID(), pItem->GetTblidx(), pItem->GetItemID(), cPlayer->GetCharID());
				//	printf("%u %u %u %u \n", req->byApplyTargetCount, req->ahApplyTarget[0], req->ahApplyTarget[1], req->hTarget);

					BYTE byTargetCount = req->byApplyTargetCount;

					if (byTargetCount > pUseItemTbldat->byApply_Target_Max)
						byTargetCount = pUseItemTbldat->byApply_Target_Max;

					cPlayer->SetItemCast(pItem->GetID());
					pItem->UseItem(req->byKeyPlace, req->byKeyPos, byTargetCount, req->ahApplyTarget, pUseItemTbldat, req->hTarget);
				}
			}
			else resultcode = GAME_COMMON_CAN_NOT_FIND_TABLE_DATA;
		}
		else resultcode = GAME_ITEM_NOT_FOUND;

		CNtlPacket packet(sizeof(sGU_ITEM_USE_RES));
		sGU_ITEM_USE_RES * res = (sGU_ITEM_USE_RES *)packet.GetPacketData();
		res->wOpCode = GU_ITEM_USE_RES;
		res->wResultCode = resultcode;
		res->byPlace = req->byPlace;
		res->byPos = req->byPos;
		res->byKeyPlace = req->byKeyPlace;
		res->byKeyPos = req->byKeyPos;
		res->tblidxItem = itemTblidx;
		packet.SetPacketLen(sizeof(sGU_ITEM_USE_RES));
		g_pApp->Send(GetHandle(), &packet);
	}
	else
	{
		ERR_LOG(LOG_HACK, "Player: %u seems to be location hacking. CurLoc: %f, %f, %f NewLoc: %f %f %f", cPlayer->GetCharID(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().y, cPlayer->GetCurLoc().z, vLoc.x, vLoc.y, vLoc.z);
	}
}



//--------------------------------------------------------------------------------------//
//		LOAD QUICK TELEPORT
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvLoadQuickTeleportReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_QUICK_TELEPORT_LOAD_REQ *req = (sUG_QUICK_TELEPORT_LOAD_REQ*)pPacket->GetPacketData();

	CItem* pItem = cPlayer->GetPlayerItemContainer()->GetItem(req->byPlace, req->byPos);

	if (pItem == NULL || pItem->GetCount() == 0 || pItem->GetTbldat()->byItem_Type != ITEM_TYPE_QUICK_TELEPORT || pItem->IsLocked() || IsInvenContainer(pItem->GetPlace()) == false)
	{
		CNtlPacket packet(sizeof(sGU_QUICK_TELEPORT_LOAD_RES));
		sGU_QUICK_TELEPORT_LOAD_RES* res = (sGU_QUICK_TELEPORT_LOAD_RES*)packet.GetPacketData();
		res->wOpCode = GU_QUICK_TELEPORT_LOAD_RES;
		res->wResultCode = GAME_QUICK_TELEPORT_ITEM_NOT_FOUND;
		packet.SetPacketLen(sizeof(sGU_QUICK_TELEPORT_LOAD_RES));
		g_pApp->Send(GetHandle(), &packet);
	}
	else
	{
		//check if teleport data already loaded. If not we load them.
		if (cPlayer->IsQuickTeleportLoaded() == false)
			cPlayer->LoadQuickTeleportFromDB();
		else
			cPlayer->LoadQuickTeleport();
	}
}

//--------------------------------------------------------------------------------------//
//		UPDATE QUICK TELEPORT
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvUpdateQuickTeleportReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_QUICK_TELEPORT_UPDATE_REQ *req = (sUG_QUICK_TELEPORT_UPDATE_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_QUICK_TELEPORT_UPDATE_RES));
	sGU_QUICK_TELEPORT_UPDATE_RES* res = (sGU_QUICK_TELEPORT_UPDATE_RES*)packet.GetPacketData();
	res->wOpCode = GU_QUICK_TELEPORT_UPDATE_RES;

	CItem* pItem = cPlayer->GetPlayerItemContainer()->GetItem(req->byPlace, req->byPos);

	if (pItem == NULL || pItem->GetCount() == 0 || req->bySlot >= NTL_QUICK_PORTAL_MAX_COUNT || IsInvenContainer(pItem->GetPlace()) == false)
		res->wResultCode = GAME_QUICK_TELEPORT_ITEM_NOT_FOUND;
	else if(pItem->IsLocked())
		res->wOpCode = GAME_ITEM_IS_LOCK;
	else if (pItem->GetTbldat()->byItem_Type != ITEM_TYPE_QUICK_TELEPORT)
		res->wOpCode = GAME_QUICK_TELEPORT_ITEM_NOT_FOUND;
	else if(cPlayer->IsQuickTeleportLoaded() == false)
		res->wResultCode = GAME_FAIL;
	else if (cPlayer->GetCurWorld() == NULL)
		res->wResultCode = GAME_WORLD_NOT_FOUND;
	else
	{
		res->wResultCode = GAME_SUCCESS;
		res->aData.bySlotNum = req->bySlot;
		res->aData.worldTblidx = cPlayer->GetWorldTblidx();
		res->aData.mapNameTblidx = GetNaviEngine()->GetTextAllIndex(cPlayer->GetCurWorld()->GetNaviInstanceHandle(), cPlayer->GetCurLoc().x, cPlayer->GetCurLoc().z);

		cPlayer->GetCurLoc().CopyTo(res->aData.vLoc);

		SYSTEMTIME ti;
		GetLocalTime(&ti);
		res->aData.tSaveTime.day = (BYTE)ti.wDay;
		res->aData.tSaveTime.hour = (BYTE)ti.wHour;
		res->aData.tSaveTime.minute = (BYTE)ti.wMinute;
		res->aData.tSaveTime.month = (BYTE)ti.wMonth;
		res->aData.tSaveTime.second = (BYTE)ti.wSecond;
		res->aData.tSaveTime.year = ti.wYear;

		cPlayer->AddQuickTeleport(&res->aData, req->bySlot);
	}

	packet.SetPacketLen(sizeof(sGU_QUICK_TELEPORT_UPDATE_RES));
	app->Send(GetHandle(), &packet);
}
//--------------------------------------------------------------------------------------//
//		DELETE QUICK TELEPORT
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvDelQuickTeleportReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_QUICK_TELEPORT_DEL_REQ *req = (sUG_QUICK_TELEPORT_DEL_REQ*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_QUICK_TELEPORT_DEL_RES));
	sGU_QUICK_TELEPORT_DEL_RES* res = (sGU_QUICK_TELEPORT_DEL_RES*)packet.GetPacketData();
	res->wResultCode = GU_QUICK_TELEPORT_DEL_RES;
	res->bySlot = req->bySlot;
	
	CItem* pItem = cPlayer->GetPlayerItemContainer()->GetItem(req->byPlace, req->byPos);
	if (pItem && pItem->GetCount() > 0 && req->bySlot < NTL_QUICK_PORTAL_MAX_COUNT && IsInvenContainer(pItem->GetPlace()))
	{
		if(pItem->IsLocked())
			res->wOpCode = GAME_ITEM_IS_LOCK;
		else if (pItem->GetTbldat()->byItem_Type != ITEM_TYPE_QUICK_TELEPORT)
			res->wOpCode = GAME_QUICK_TELEPORT_ITEM_NOT_FOUND;
		else if (cPlayer->IsQuickTeleportLoaded() && cPlayer->DelQuickTeleportData(req->bySlot))
		{
			res->wOpCode = GAME_SUCCESS;

			CNtlPacket pQry(sizeof(sGQ_QUICK_TELEPORT_DEL_REQ));
			sGQ_QUICK_TELEPORT_DEL_REQ * qRes = (sGQ_QUICK_TELEPORT_DEL_REQ *)pQry.GetPacketData();
			qRes->wOpCode = GQ_QUICK_TELEPORT_DEL_REQ;
			qRes->charId = cPlayer->GetCharID();
			qRes->bySlot = req->bySlot;
			pQry.SetPacketLen(sizeof(sGQ_QUICK_TELEPORT_DEL_REQ));
			app->SendTo(app->GetQueryServerSession(), &pQry);
		}
		else res->wOpCode = GAME_FAIL;
	}
	else res->wOpCode = GAME_FAIL;
	
	packet.SetPacketLen(sizeof(sGU_QUICK_TELEPORT_DEL_RES));
	app->Send(GetHandle(), &packet);
}
//--------------------------------------------------------------------------------------//
//		USE QUICK TELEPORT (TELEPORT)
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvUseQuickTeleportReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_QUICK_TELEPORT_USE_REQ *req = (sUG_QUICK_TELEPORT_USE_REQ*)pPacket->GetPacketData();

	WORD rescode = GAME_SUCCESS;

	CItem* pItem = cPlayer->GetPlayerItemContainer()->GetItem(req->byPlace, req->byPos);
	if (pItem && pItem->GetCount() > 0 && req->bySlot < NTL_QUICK_PORTAL_MAX_COUNT && IsInvenContainer(pItem->GetPlace()))
	{
		CNtlVector destLoc;
		WORLDID destWorldID = INVALID_WORLDID;

		if (pItem->IsLocked())
			rescode = GAME_ITEM_IS_LOCK;
		else if (pItem->GetTbldat()->byItem_Type != ITEM_TYPE_QUICK_TELEPORT)
			rescode = GAME_QUICK_TELEPORT_ITEM_NOT_FOUND;
		else if (cPlayer->GetPcIsFreeBattle())
			rescode = GAME_ITEM_CANT_USE_NOW;
		else if (cPlayer->IsQuickTeleportLoaded() == false)
			rescode = GAME_ITEM_CANT_USE_NOW;
		else if (cPlayer->GetCurWorld() == NULL || cPlayer->GetCurWorld()->GetRuleType() != GAMERULE_NORMAL)
			rescode = GAME_ITEM_CANT_USE_NOW;
		else if (cPlayer->GetDragonballScrambleBallFlag() != 0)
			rescode = GAME_CAN_NOT_TELEPORT;
		else
		{
			sQUICK_TELEPORT_INFO* telinfo = cPlayer->GetQuickTeleportData(req->bySlot);

			if (telinfo == NULL)
				rescode = GAME_QUICK_TELEPORT_INFO_NOT_FOUND;
			else
			{
				destLoc.operator=(telinfo->vLoc);
				destWorldID = telinfo->worldTblidx;
			}
		}

		if (rescode == GAME_SUCCESS) //Teleport if everything ok
		{
			if (pItem->GetTbldat()->byDurationType == eDURATIONTYPE_NORMAL)
				pItem->SetCount(pItem->GetCount() - 1, false, true);
			cPlayer->StartTeleport(destLoc, cPlayer->GetCurDir(), destWorldID, TELEPORT_TYPE_QUICK_TELEPORT);
		}
	}
	else rescode = GAME_QUICK_TELEPORT_ITEM_NOT_FOUND;

	CNtlPacket packet(sizeof(sGU_QUICK_TELEPORT_USE_RES));
	sGU_QUICK_TELEPORT_USE_RES* res = (sGU_QUICK_TELEPORT_USE_RES*)packet.GetPacketData();
	res->wOpCode = GU_QUICK_TELEPORT_USE_RES;
	res->wResultCode = rescode;
	res->bySlot = req->bySlot;
	packet.SetPacketLen(sizeof(sGU_QUICK_TELEPORT_USE_RES));
	g_pApp->Send(GetHandle(), &packet);
}


//--------------------------------------------------------------------------------------//
//		REVIVE CHARACTER WITH ITEM
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCrescentPopoRevivalReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_CHAR_CRESCENT_POPO_REVIVAL_REQ * req = (sUG_CHAR_CRESCENT_POPO_REVIVAL_REQ*)pPacket->GetPacketData();
	WORD resultcode = GAME_SUCCESS;

	sUSE_ITEM_TBLDAT * pTableData = NULL;
	CItem* item = NULL;

	if (!cPlayer->IsFainting())
	{
		resultcode = GAME_FAIL;
		ERR_LOG(LOG_GENERAL, "ERROR: USER IS NOT IN FAINT. CHAR ID %u STATE ID %u", cPlayer->GetCharID(), cPlayer->GetCharStateID());
	}
	else if (cPlayer->GetCurWorld() && cPlayer->GetCurWorld()->GetTbldat()->bDynamic)
		resultcode = GAME_FAIL;
	else
	{
		item = cPlayer->GetPlayerItemContainer()->GetItem(req->byPlace, req->byPos);
		if (item == NULL || item->GetCount() == 0 || item->GetTbldat()->byItem_Type != ITEM_TYPE_CRESCENT_POPO || IsInvenContainer(item->GetPlace()) == false)
			resultcode = GAME_ITEM_NOT_FOUND;
		else if(item->IsLocked())
			resultcode = GAME_ITEM_IS_LOCK;
		else
		{
			pTableData = (sUSE_ITEM_TBLDAT*)g_pTableContainer->GetUseItemTable()->FindData(item->GetTbldat()->Use_Item_Tblidx);
			if (pTableData == NULL)
			{
				resultcode = GAME_ITEM_NOT_FOUND;
				ERR_LOG(LOG_USER, "Cant find USE_ITEM_TBLDAT. Player %u", cPlayer->GetCharID());
			}
		}
	}

	CNtlPacket packet(sizeof(sGU_CHAR_CRESCENT_POPO_REVIVAL_RES));
	sGU_CHAR_CRESCENT_POPO_REVIVAL_RES * res = (sGU_CHAR_CRESCENT_POPO_REVIVAL_RES *)packet.GetPacketData();
	res->wOpCode = GU_CHAR_CRESCENT_POPO_REVIVAL_RES;
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_CHAR_CRESCENT_POPO_REVIVAL_RES));
	g_pApp->Send(GetHandle(), &packet);

	if (resultcode == GAME_SUCCESS)
	{
		item->SetCount(item->GetCount() - 1, false, true);

		cPlayer->Revival(cPlayer->GetCurLoc(), cPlayer->GetWorldID(), REVIVAL_TYPE_CURRENT_POSITION);

		cPlayer->UpdateCurLP(cPlayer->GetLastMaxLP() * (int)pTableData->aSystem_Effect_Value[0] / 100, true, false);

		if ((int)pTableData->aSystem_Effect_Value[1] != 0)
			cPlayer->UpdateCurEP((cPlayer->GetLastMaxEP() * (WORD)pTableData->aSystem_Effect_Value[1] / 100), true, false);
	}
}

//--------------------------------------------------------------------------------------//
//		RENAME CHARACTER BY ITEM
//--------------------------------------------------------------------------------------//
void CClientSession::RecvCharRenameReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_CHARACTER_RENAME_REQ * req = (sUG_CHARACTER_RENAME_REQ*)pPacket->GetPacketData();
	WORD resultcode = GAME_SUCCESS;

	char* chname = Ntl_WC2MB(req->awchCharName);
	std::string charname = chname;

	CItem* item = cPlayer->GetPlayerItemContainer()->GetItem(req->byPlace, req->byPos);
	if (item == NULL || item->GetCount() == 0 || item->GetTbldat()->byItem_Type != ITEM_TYPE_CHARACTER_NAME_CHANGE || IsInvenContainer(item->GetPlace()) == false)
		resultcode = GAME_ITEM_NOT_FOUND;
	else if (item->IsLocked())
		resultcode = GAME_ITEM_IS_LOCK;
	else if (charname.length() < NTL_MIN_SIZE_CHAR_NAME)
		resultcode = GAME_CHARACTER_TOO_SHORT_NAME;
	else if (charname.length() > NTL_MAX_SIZE_CHAR_NAME)
		resultcode = GAME_CHARACTER_TOO_LONG_NAME;
	//else if (charname.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890") != std::string::npos)
	//	resultcode = GAME_CHARACTER_NAME_HAS_INVALID_CHARACTER;
	else
	{
		item->SetLocked(true);

		CNtlPacket pQry(sizeof(sGQ_CHARACTER_RENAME_REQ));
		sGQ_CHARACTER_RENAME_REQ * qRes = (sGQ_CHARACTER_RENAME_REQ *)pQry.GetPacketData();
		qRes->wOpCode = GQ_CHARACTER_RENAME_REQ;
		qRes->charId = cPlayer->GetCharID();
		qRes->accountId = cPlayer->GetAccountID();
		qRes->byPlace = req->byPlace;
		qRes->byPos = req->byPos;
		qRes->handle = cPlayer->GetID();
		qRes->itemId = item->GetItemID();
		NTL_SAFE_WCSCPY(qRes->wszCharName, req->awchCharName);
		pQry.SetPacketLen(sizeof(sGQ_CHARACTER_RENAME_REQ));
		app->SendTo(app->GetQueryServerSession(), &pQry);

		Ntl_CleanUpHeapString(chname);

		return;
	}

	Ntl_CleanUpHeapString(chname);

	CNtlPacket packet(sizeof(sGU_CHARACTER_RENAME_RES));
	sGU_CHARACTER_RENAME_RES * res = (sGU_CHARACTER_RENAME_RES *)packet.GetPacketData();
	res->wOpCode = GU_CHARACTER_RENAME_RES;
	res->wResultCode = resultcode;
	res->dwRemainMin = 0;
	packet.SetPacketLen(sizeof(sGU_CHARACTER_RENAME_RES));
	app->Send(GetHandle(), &packet);
}

//--------------------------------------------------------------------------------------//
//		CHANGE ITEM OPTION
//--------------------------------------------------------------------------------------//
void CClientSession::RecvItemChangeOptionReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_ITEM_CHANGE_OPTION_REQ * req = (sUG_ITEM_CHANGE_OPTION_REQ*)pPacket->GetPacketData();
	WORD resultcode = GAME_SUCCESS;

	CItem* item = cPlayer->GetPlayerItemContainer()->GetItem(req->byItemPlace, req->byItemPos);
	CItem* kit = cPlayer->GetPlayerItemContainer()->GetItem(req->byKitPlace, req->byKitPos);
	if (item == NULL || item->GetCount() == 0 || kit == NULL || kit->GetCount() == 0 || IsInvenContainer(item->GetPlace()) == false || IsInvenContainer(kit->GetPlace()) == false)
		resultcode = GAME_ITEM_NOT_FOUND;
	else if(item->IsLocked() || kit->IsLocked())
		resultcode = GAME_ITEM_IS_LOCK;
	else if (kit->GetTbldat()->byItem_Type != ITEM_TYPE_CHANGE_OPTION_KIT)
	{
		resultcode = GAME_ITEM_NOT_FOUND;
		ERR_LOG(LOG_USER, "Player %u hacker. Item is not option kit. Item tblidx %u byItem_Type %u", cPlayer->GetCharID(), kit->GetTblidx(), kit->GetTbldat()->byItem_Type);
	}
	else
	{
		CGameServer* app = (CGameServer*)g_pApp;

		CNtlPacket packet(sizeof(sGQ_ITEM_CHANGE_OPTION_REQ));
		sGQ_ITEM_CHANGE_OPTION_REQ * res = (sGQ_ITEM_CHANGE_OPTION_REQ *)packet.GetPacketData();
		res->wOpCode = GQ_ITEM_CHANGE_OPTION_REQ;

		WORD costumeIndex6 = INVALID_WORD;
		int costumeOption6 = 0;
		WORD costumeIndex7 = INVALID_WORD;
		int costumeOption7 = 0;

		if (item->GetOptionSet().GetRandomOptionIndex(6) != INVALID_WORD && item->GetOptionSet().GetRandomOptionValue(6) > 0)
		{
			costumeIndex6 = item->GetOptionSet().GetRandomOptionIndex(6);
			costumeOption6 = item->GetOptionSet().GetRandomOptionValue(6);
		}
		if (item->GetOptionSet().GetRandomOptionIndex(7) != INVALID_WORD && item->GetOptionSet().GetRandomOptionValue(7) > 0)
		{
			costumeIndex7 = item->GetOptionSet().GetRandomOptionIndex(7);
			costumeOption7 = item->GetOptionSet().GetRandomOptionValue(7);
		}

		switch (kit->GetTblidx())
		{
		case 11120092: case 850004: case 850006: case 850008: case 850010: case 850752: //brown
			if (CItem::ChangeOption(INVALID_WORD, item->GetTbldat(), &item->GetItemData(), item->GetRank(), &res->sItemOptionSet, kit->GetTblidx(), cPlayer->GetGMLevel()) == false)
				resultcode = GAME_FAIL;
			break;
		case 11120093: case 850005: case 850007: case 850009: case 850011: //silver
			if (CItem::ChangeOption(req->wOptionIndex, item->GetTbldat(), &item->GetItemData(), item->GetRank(), &res->sItemOptionSet, kit->GetTblidx(), cPlayer->GetGMLevel()) == false)
				resultcode = GAME_FAIL;
			break;
		}


		if (resultcode == GAME_SUCCESS)
		{
			res->sItemOptionSet.aRandomOption[6].wOptionIndex = costumeIndex6;
			res->sItemOptionSet.aRandomOption[6].optionValue = costumeOption6;
			res->sItemOptionSet.aRandomOption[7].wOptionIndex = costumeIndex7;
			res->sItemOptionSet.aRandomOption[7].optionValue = costumeOption7;

			res->byItemPlace = req->byItemPlace;
			res->byItemPos = req->byItemPos;
			res->byKitPlace = req->byKitPlace;
			res->byKitPos = req->byKitPos;
			res->byKitStack = kit->GetCount();
			res->charId = cPlayer->GetCharID();
			res->handle = cPlayer->GetID();
			res->ItemId = item->GetItemID();
			res->KitItemId = kit->GetItemID();
			packet.SetPacketLen(sizeof(sGQ_ITEM_CHANGE_OPTION_REQ));
			app->SendTo(app->GetQueryServerSession(), &packet);

			kit->SetLocked(true);
			item->SetLocked(true);

			kit->SetCount(kit->GetCount() - 1, false, false);

			return;
		}
	}

	CNtlPacket packet2(sizeof(sGU_ITEM_CHANGE_OPTION_RES));
	sGU_ITEM_CHANGE_OPTION_RES * res2 = (sGU_ITEM_CHANGE_OPTION_RES *)packet2.GetPacketData();
	res2->wOpCode = GU_ITEM_CHANGE_OPTION_RES;
	res2->byItemPlace = req->byItemPlace;
	res2->byItemPos = req->byItemPos;
	res2->byKitPlace = req->byKitPlace;
	res2->byKitPos = req->byKitPos;
	res2->wResultCode = resultcode;
	packet2.SetPacketLen(sizeof(sGU_ITEM_CHANGE_OPTION_RES));
	g_pApp->Send(GetHandle(), &packet2);
}

//--------------------------------------------------------------------------------------//
//		ADD BALL TO DOGI
//--------------------------------------------------------------------------------------//
void CClientSession::RecvItemSocketInsertBeadReq(CNtlPacket * pPacket) // kuini equip dogiball
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_ITEM_SOCKET_INSERT_BEAD_REQ * req = (sUG_ITEM_SOCKET_INSERT_BEAD_REQ*)pPacket->GetPacketData();
	WORD resultcode = GAME_SUCCESS;

	CItem* item = cPlayer->GetPlayerItemContainer()->GetItem(req->byItemPlace, req->byItemPos);
	CItem* bead = cPlayer->GetPlayerItemContainer()->GetItem(req->byBeadPlace, req->byBeadPos);
	if (item == NULL || item->GetCount() == 0 || bead == NULL || bead->GetCount() == 0 || IsInvenContainer(item->GetPlace()) == false || IsInvenContainer(bead->GetPlace()) == false)
		resultcode = GAME_ITEM_NOT_FOUND;
	else if (item->IsLocked() || bead->IsLocked())
		resultcode = GAME_ITEM_IS_LOCK;
	else
	{
		if (bead->GetTbldat()->byItem_Type != ITEM_TYPE_BEAD) //check if its not a dogi ball
		{
			resultcode = GAME_ITEM_NOT_BEAD;
		}
		else if (item->GetRestrictState() == ITEM_RESTRICT_STATE_TYPE_INSERT_BEAD) //check if socket already in use
		{
			resultcode = GAME_ITEM_NOT_SOCKET;
		}
		else if (item->GetTbldat()->byItem_Type != ITEM_TYPE_COSTUME_SET 
			&& item->GetTbldat()->byItem_Type != ITEM_TYPE_COSTUME_HAIR_STYLE 
			&& item->GetTbldat()->byItem_Type != ITEM_TYPE_COSTUME_MASK 
			&& item->GetTbldat()->byItem_Type != ITEM_TYPE_COSTUME_HAIR_ACCESSORY
			&& item->GetTbldat()->byItem_Type != ITEM_TYPE_COSTUME_BACK_ACCESSORY
			&& item->GetTbldat()->byItem_Type != ITEM_TYPE_GLOVE
			&& item->GetTbldat()->byItem_Type != ITEM_TYPE_STAFF
			&& item->GetTbldat()->byItem_Type != ITEM_TYPE_CLAW
			&& item->GetTbldat()->byItem_Type != ITEM_TYPE_AXE
			&& item->GetTbldat()->byItem_Type != ITEM_TYPE_SCROLL
			&& item->GetTbldat()->byItem_Type != ITEM_TYPE_GEM
			&& item->GetTbldat()->byItem_Type != ITEM_TYPE_STICK
			&& item->GetTbldat()->byItem_Type != ITEM_TYPE_SWORD
			&& item->GetTbldat()->byItem_Type != ITEM_TYPE_FAN
			&& item->GetTbldat()->byItem_Type != ITEM_TYPE_WAND
			&& item->GetTbldat()->byItem_Type != ITEM_TYPE_INSTRUMENT
			&& item->GetTbldat()->byItem_Type != ITEM_TYPE_CLUB
			&& item->GetTbldat()->byItem_Type != ITEM_TYPE_DRUM
			&& item->GetTbldat()->byItem_Type != ITEM_TYPE_MASK
			&& item->GetTbldat()->byItem_Type != ITEM_TYPE_JACKET
			&& item->GetTbldat()->byItem_Type != ITEM_TYPE_PANTS
			&& item->GetTbldat()->byItem_Type != ITEM_TYPE_BOOTS
			&& item->GetTbldat()->byItem_Type != ITEM_TYPE_NECKLACE
			&& item->GetTbldat()->byItem_Type != ITEM_TYPE_EARRING
			&& item->GetTbldat()->byItem_Type != ITEM_TYPE_RING) //check if item is a costume(dogi)

		{
			resultcode = GAME_ITEM_CANNOT_INSERT_BEAD_BY_NO_MATCH_ITEM;
			ERR_LOG(LOG_GENERAL, "Error item %u item_type %u is not a costume.", item->GetTblidx(), item->GetTbldat()->byItem_Type);
		}
		else if(BIT_FLAG_TEST(item->GetTbldat()->wFunction_Bit_Flag, ITEM_FUNC_BIT_FLAG_SOCKET_ON) == false)
			resultcode = GAME_ITEM_CANNOT_INSERT_BEAD_BY_NO_MATCH_ITEM;
		else
		{
			if (cPlayer->GetGMLevel() != 10)
			{
				if (Dbo_CheckProbability(20)) //check if fail
					resultcode = GAME_ITEM_INSERT_BEAD_FAIL_AND_DEL;
			}
		}
	//	printf("bead->GetTbldat()->byItem_Type %u item->GetTbldat()->byItem_Type %u \n", bead->GetTbldat()->byItem_Type, item->GetTbldat()->byItem_Type);
	}


	CNtlPacket packet(sizeof(sGU_ITEM_SOCKET_INSERT_BEAD_RES));
	sGU_ITEM_SOCKET_INSERT_BEAD_RES* res = (sGU_ITEM_SOCKET_INSERT_BEAD_RES*)packet.GetPacketData();
	res->wOpCode = GU_ITEM_SOCKET_INSERT_BEAD_RES;
	res->byBeadPlace = req->byBeadPlace;
	res->byBeadPos = req->byBeadPos;
	res->byItemPlace = req->byItemPlace;
	res->byItemPos = req->byItemPos;

	if (resultcode == GAME_ITEM_INSERT_BEAD_FAIL_AND_DEL)
	{
		memcpy(&res->sItemRandomOption[0], &item->GetOptionSet().aRandomOption[6], sizeof(sITEM_RANDOM_OPTION));
		memcpy(&res->sItemRandomOption[1], &item->GetOptionSet().aRandomOption[7], sizeof(sITEM_RANDOM_OPTION));
		res->byRestrictState = item->GetRestrictState();
		res->byDurationType = item->GetDurationtype();
		res->nUseStartTime = 0;
		res->nUseEndTime = 0;

		res->byBeadRemainStack = bead->GetCount() - 1;
		bead->SetCount(res->byBeadRemainStack, false, true);
	}
	else if (resultcode == GAME_SUCCESS)
	{
		resultcode = item->InsertSocketBead(bead); //insert bead aka dogi ball
		if (resultcode == GAME_SUCCESS)
			return;
	}

	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_ITEM_SOCKET_INSERT_BEAD_RES));
	g_pApp->Send(GetHandle(), &packet);
}

//--------------------------------------------------------------------------------------//
//		REMOVE DOGI BALL FROM DOGI SOCKET
//--------------------------------------------------------------------------------------//
void CClientSession::RecvItemSocketDestroyBeadReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_ITEM_SOCKET_DESTROY_BEAD_REQ * req = (sUG_ITEM_SOCKET_DESTROY_BEAD_REQ*)pPacket->GetPacketData();
	WORD resultcode = GAME_ITEM_NOT_FOUND;

	CItem* pItem = cPlayer->GetPlayerItemContainer()->GetItem(req->byItemPlace, req->byItemPos);
	if (pItem && pItem->GetCount() > 0 && IsInvenContainer(pItem->GetPlace()))
	{
		if (pItem->IsLocked())
			resultcode = GAME_ITEM_IS_LOCK;
		else
		{
			pItem->DestroySocketBead(false);
			return;
		}
	}

	CNtlPacket packet(sizeof(sGU_ITEM_SOCKET_DESTROY_BEAD_RES));
	sGU_ITEM_SOCKET_DESTROY_BEAD_RES * res = (sGU_ITEM_SOCKET_DESTROY_BEAD_RES *)packet.GetPacketData();
	res->wOpCode = GU_ITEM_SOCKET_DESTROY_BEAD_RES;
	res->wResultCode = resultcode;
	packet.SetPacketLen(sizeof(sGU_ITEM_SOCKET_DESTROY_BEAD_RES));
	g_pApp->Send(GetHandle(), &packet);
}

void CClientSession::RecvDynamicFieldSystemBossPositionReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_DYNAMIC_FIELD_SYSTEM_BOSS_POSITION_REQ * req = (sUG_DYNAMIC_FIELD_SYSTEM_BOSS_POSITION_REQ*)pPacket->GetPacketData();

	g_pDynamicFieldSystemEvent->GetBosses(cPlayer);
}

void CClientSession::RecvEventRewardLoadReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_EVENT_REWARD_LOAD_REQ * req = (sUG_EVENT_REWARD_LOAD_REQ*)pPacket->GetPacketData();

	WORD wResultCode = GAME_SUCCESS;

	CNpc* pNpc = g_pObjectManager->GetNpc(req->hNpcHandle);
	if(pNpc == NULL || pNpc->IsNPC() == false)
		wResultCode = GAME_TARGET_NOT_FOUND;
	else if(pNpc->IsInRange(cPlayer, DBO_DISTANCE_CHECK_TOLERANCE) == false)
		wResultCode = GAME_TARGET_TOO_FAR;
	else if(pNpc->HasFunction(NPC_FUNC_FLAG_EVENT_NPC) == false)
		wResultCode = GAME_TARGET_HAS_NOT_FUNCTION;

	if (wResultCode == GAME_SUCCESS)
	{
		if (cPlayer->GetLastEventRewardLoad() + 600000 < app->GetCurTickCount()) // 10 Minutes
		{
			cPlayer->SetLastEventRewardLoad(app->GetCurTickCount());

			CNtlPacket packet(sizeof(sGQ_EVENT_REWARD_LOAD_REQ));
			sGQ_EVENT_REWARD_LOAD_REQ * res = (sGQ_EVENT_REWARD_LOAD_REQ *)packet.GetPacketData();
			res->wOpCode = GQ_EVENT_REWARD_LOAD_REQ;
			res->accountId = cPlayer->GetAccountID();
			res->charId = cPlayer->GetCharID();
			res->handle = cPlayer->GetID();
			app->SendTo(app->GetQueryServerSession(), &packet);
		}
		else // load current shit
		{
			cPlayer->LoadExistingEventReward();
		}
	}
	else
	{
		CNtlPacket pRes(sizeof(sGU_EVENT_REWARD_LOAD_RES));
		sGU_EVENT_REWARD_LOAD_RES * rRes = (sGU_EVENT_REWARD_LOAD_RES *)pRes.GetPacketData();
		rRes->wOpCode = GU_EVENT_REWARD_LOAD_RES;
		rRes->wResultCode = wResultCode;
		app->Send(cPlayer->GetClientSessionID(), &pRes);
	}
}

void CClientSession::RecvEventRewardSelectReq(CNtlPacket * pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	sUG_EVENT_REWARD_SELECT_REQ * req = (sUG_EVENT_REWARD_SELECT_REQ*)pPacket->GetPacketData();

	WORD wResultCode = GAME_SUCCESS;

	sEVENT_SYSTEM_TBLDAT* pEventTbldat = NULL;
	sITEM_TBLDAT* pItemTbldat = NULL;

	CNpc* pNpc = g_pObjectManager->GetNpc(req->hNpcHandle);
	if (pNpc == NULL || pNpc->IsNPC() == false)
		wResultCode = GAME_TARGET_NOT_FOUND;
	else if (pNpc->IsInRange(cPlayer, DBO_DISTANCE_CHECK_TOLERANCE) == false)
		wResultCode = GAME_TARGET_TOO_FAR;
	else if (pNpc->HasFunction(NPC_FUNC_FLAG_EVENT_NPC) == false)
		wResultCode = GAME_TARGET_HAS_NOT_FUNCTION;
	else if(cPlayer->HasEventReward(req->eventTblidx, cPlayer->GetCharID()) == false)
		wResultCode = GAME_TARGET_HAS_NOT_FUNCTION;
	else if (cPlayer->GetPlayerItemContainer()->CountEmptyInventory() == 0)
		wResultCode = GAME_ITEM_INVEN_FULL;
	else
	{
		pEventTbldat = (sEVENT_SYSTEM_TBLDAT*)g_pTableContainer->GetEventSystemTable()->FindData(req->eventTblidx);
		if (pEventTbldat)
		{
			pItemTbldat = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(pEventTbldat->aIndex);
			if(pItemTbldat == NULL)
				wResultCode = GAME_ITEM_NOT_FOUND;
		}
		else wResultCode = GAME_COMMON_CAN_NOT_FIND_TABLE_DATA;
	}

	if (wResultCode == GAME_SUCCESS)
	{
		//remove event reward
		cPlayer->EraseEventReward(req->eventTblidx);

		CNtlPacket packet(sizeof(sGQ_EVENT_REWARD_SELECT_REQ));
		sGQ_EVENT_REWARD_SELECT_REQ * res = (sGQ_EVENT_REWARD_SELECT_REQ *)packet.GetPacketData();
		res->wOpCode = GQ_EVENT_REWARD_SELECT_REQ;
		res->eventTblidx = req->eventTblidx;
		res->accountId = cPlayer->GetAccountID();
		res->charId = cPlayer->GetCharID();
		res->handle = cPlayer->GetID();

		std::pair<BYTE, BYTE> inv = cPlayer->GetPlayerItemContainer()->GetEmptyInventory();

		cPlayer->GetPlayerItemContainer()->AddReservedInventory(inv.first, inv.second);

		res->sItem.Init();

		res->sItem.charId = cPlayer->GetCharID();
		res->sItem.itemNo = pItemTbldat->tblidx;
		res->sItem.byStackcount = (BYTE)pEventTbldat->adwSetting[0];
		res->sItem.byPlace = inv.first;
		res->sItem.byPosition = inv.second;
		res->sItem.byCurrentDurability = pItemTbldat->byDurability;
		res->sItem.byRank = pItemTbldat->byRank;
		res->sItem.bNeedToIdentify = false;
		res->sItem.byRestrictState = GetDefaultRestrictState(pItemTbldat->byRestrictType, pItemTbldat->byItem_Type, true);
		res->sItem.sOptionSet.Init();
		res->sItem.byDurationType = pItemTbldat->byDurationType;

		if (pItemTbldat->bIsCanHaveOption && pItemTbldat->Item_Option_Tblidx != INVALID_TBLIDX)
			res->sItem.sOptionSet.aOptionTblidx[0] = pItemTbldat->Item_Option_Tblidx;

		if (pItemTbldat->byDurationType == eDURATIONTYPE_FLATSUM)
		{
			res->sItem.nUseStartTime = app->GetTime();
			res->sItem.nUseEndTime = res->sItem.nUseStartTime + pItemTbldat->dwUseDurationMax;
		}

		app->SendTo(app->GetQueryServerSession(), &packet);
	}
	else
	{
		CNtlPacket pRes(sizeof(sGU_EVENT_REWARD_SELECT_RES));
		sGU_EVENT_REWARD_SELECT_RES * rRes = (sGU_EVENT_REWARD_SELECT_RES *)pRes.GetPacketData();
		rRes->wOpCode = GU_EVENT_REWARD_SELECT_RES;
		rRes->wResultCode = wResultCode;
		app->Send(cPlayer->GetClientSessionID(), &pRes);
	}
}


//--------------------------------------------------------------------------------------//
//		CANCEL SKILL CASTING NFY
//--------------------------------------------------------------------------------------//
void	CClientSession::RecvCancelSkillCastingNfy(CNtlPacket *pPacket)
{
	if (!cPlayer || !cPlayer->IsInitialized())
		return;

	sUG_SKILL_CASTING_CANCELED_NFY * req = (sUG_SKILL_CASTING_CANCELED_NFY*)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sGU_SKILL_CASTING_CANCELED_NFY));
	sGU_SKILL_CASTING_CANCELED_NFY * res = (sGU_SKILL_CASTING_CANCELED_NFY *)packet.GetPacketData();
	res->wOpCode = GU_SKILL_CASTING_CANCELED_NFY;

	if (!cPlayer->IsCastingSkill())
		res->wResultCode = GAME_FAIL;
	else
		cPlayer->SendCharStateStanding();
	
	res->handle = cPlayer->GetID();
	packet.SetPacketLen(sizeof(sGU_SKILL_CASTING_CANCELED_NFY));
	cPlayer->Broadcast(&packet);
}


