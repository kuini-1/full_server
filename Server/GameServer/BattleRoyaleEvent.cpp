#include "stdafx.h"
#include "BattleRoyaleEvent.h"
#include "GameServer.h"
#include "CPlayer.h"
#include "Monster.h"
#include "ObjectManager.h"
#include "WorldManager.h"
#include "World.h"
#include "GameMain.h"
#include "NtlPacketGU.h"
#include "NtlStringW.h"
#include "TableContainerManager.h"
#include "ExpTable.h"
#include "NtlRandom.h"
#include "NtlMail.h"
#include "NtlAdmin.h"
#include "NtlVector.h"

// Static constant definitions
const TBLIDX BattleRoyaleEvent::BATTLE_ROYALE_WORLD_TBLIDX = 212000;
const BYTE BattleRoyaleEvent::MAX_PLAYERS_PER_INSTANCE = 10;
const WORD BattleRoyaleEvent::PROPOSAL_TIMEOUT_SECONDS = 15;
const WORD BattleRoyaleEvent::LOAD_WAIT_SECONDS = 30;
const WORD BattleRoyaleEvent::PREPARATION_PHASE_MINUTES = 5;
const WORD BattleRoyaleEvent::TOTAL_EVENT_MINUTES = 15;
const DWORD BattleRoyaleEvent::IDLE_CHECK_INTERVAL_MS = 10000;
const DWORD BattleRoyaleEvent::IDLE_THRESHOLD_MS = 30000;

BattleRoyaleEvent::BattleRoyaleEvent()
{
	Init();
}

BattleRoyaleEvent::~BattleRoyaleEvent()
{
}

void BattleRoyaleEvent::Init()
{
	m_bEventActive = false;
	m_dwEventStartTick = 0;
	m_dwProposalEndTick = 0;
	m_setProposalSent.clear();
	m_setProposalAccepted.clear();
	m_mapPlayerInstance.clear();
	m_mapInstancePlayers.clear();
	m_mapLoadedPlayers.clear();
	m_mapOriginalLevels.clear();
	m_mapOriginalLoc.clear();
	m_mapOriginalWorld.clear();
	m_mapInstanceStartTick.clear();
	m_mapInstanceWaitingForStart.clear();
	m_mapPreparationEndTick.clear();
	m_mapEventEndTick.clear();
	m_mapPvPPhaseActive.clear();
	m_mapPlayerKills.clear();
	m_mapPlayerDeaths.clear();
	m_setAlivePlayers.clear();
	m_mapLastMovementTick.clear();
}

void BattleRoyaleEvent::StartEvent()
{
	if (m_bEventActive)
		return;

	CGameServer* app = (CGameServer*)g_pApp;
	if (!app)
	{
		ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent::StartEvent: CGameServer is null!");
		return;
	}

	ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent::StartEvent called");

	m_bEventActive = true;
	m_dwEventStartTick = app->GetCurTickCount();
	m_dwProposalEndTick = m_dwEventStartTick + (PROPOSAL_TIMEOUT_SECONDS * 1000);

	// Send teleport proposals to all online players
	if (!g_pObjectManager)
	{
		ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent::StartEvent: ObjectManager is null!");
		m_bEventActive = false; // Reset state
		return;
	}

	if (!g_pTableContainer)
	{
		ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent::StartEvent: TableContainer is null!");
		m_bEventActive = false; // Reset state
		return;
	}

	// Get world table data once
	sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData(BATTLE_ROYALE_WORLD_TBLIDX);
	if (!pWorldTbldat)
	{
		ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent::StartEvent: World table data not found for world %u! Make sure world 212000 exists in your world table.", BATTLE_ROYALE_WORLD_TBLIDX);
		m_bEventActive = false; // Reset state
		return;
	}

	// Create initial instance for proposals (we'll assign players to instances when they accept)
	if (!app->GetGameMain() || !app->GetGameMain()->GetWorldManager())
	{
		ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent::StartEvent: WorldManager is null!");
		m_bEventActive = false;
		return;
	}

	CWorld* pInitialWorld = app->GetGameMain()->GetWorldManager()->CreateWorld(pWorldTbldat);
	if (!pInitialWorld)
	{
		ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent::StartEvent: Failed to create initial world instance!");
		m_bEventActive = false;
		return;
	}

	WORLDID initialWorldID = pInitialWorld->GetID();
	m_mapInstancePlayers[initialWorldID] = std::set<HOBJECT>();
	m_mapLoadedPlayers[initialWorldID] = std::set<HOBJECT>();

	ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent::StartEvent: Created initial instance %u", initialWorldID);

	const boost::unordered_map<CHARACTERID, CPlayer*>& playerMap = g_pObjectManager->GetPlayerMap();
	DWORD dwProposalsSent = 0;
	DWORD dwPlayersSkipped = 0;
	for (auto it = playerMap.begin(); it != playerMap.end(); ++it)
	{
		CPlayer* pPlayer = it->second;
		if (pPlayer && pPlayer->IsInitialized() && !pPlayer->IsFainting())
		{
			// Check if player can receive teleport proposal
			if (!pPlayer->GetCurWorld())
			{
				dwPlayersSkipped++;
				continue;
			}

			if (pPlayer->GetCurWorld()->GetTbldat() && pPlayer->GetCurWorld()->GetTbldat()->bDynamic)
			{
				dwPlayersSkipped++;
				continue;
			}

			// Store original location
			m_mapOriginalLoc[pPlayer->GetID()] = pPlayer->GetCurLoc();
			m_mapOriginalWorld[pPlayer->GetID()] = pPlayer->GetWorldID();

			// Send teleport proposal with initial world ID (players will be reassigned to instances when they accept)
			if (pPlayer->StartTeleportProposal(NULL, PROPOSAL_TIMEOUT_SECONDS, TELEPORT_TYPE_COMMAND, 0, BATTLE_ROYALE_WORLD_TBLIDX, initialWorldID, pWorldTbldat->vStart1Loc, pWorldTbldat->vStart1Dir))
			{
				m_setProposalSent.insert(pPlayer->GetID());
				dwProposalsSent++;
			}
			else
			{
				ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent: Failed to send teleport proposal to player %u (may already have a proposal or invalid state)", pPlayer->GetCharID());
				dwPlayersSkipped++;
			}
		}
	}

	ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent::StartEvent: Sent %u teleport proposals, skipped %u players", dwProposalsSent, dwPlayersSkipped);

	ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent::StartEvent: Sent %u teleport proposals to players", dwProposalsSent);

	if (dwProposalsSent == 0)
	{
		ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent::StartEvent: No proposals were sent! Check if world %u exists in table and players are online.", BATTLE_ROYALE_WORLD_TBLIDX);
		m_bEventActive = false; // Reset state if no proposals sent
		return;
	}

	// Send announcement
	CNtlStringW msg;
	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
	WCHAR wszFormatBuf[256];
	WCharTLiteralToWCHAR(L"Battle Royale Event Started! Accept the teleport proposal to join!", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
	res->wMessageLengthInUnicode = (WORD)msg.Format(wszFormatBuf);
	res->byDisplayType = SERVER_TEXT_EMERGENCY;
	NTL_SAFE_WCSCPY(res->awchMessage, msg.c_str());
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	g_pObjectManager->SendPacketToAll(&packet);
}

void BattleRoyaleEvent::OnPlayerAcceptedProposal(CPlayer* pPlayer)
{
	if (!pPlayer || !pPlayer->IsInitialized())
		return;

	if (m_setProposalAccepted.find(pPlayer->GetID()) != m_setProposalAccepted.end())
		return; // Already accepted

	m_setProposalAccepted.insert(pPlayer->GetID());

	// Get or create instance
	WORLDID instanceID = GetAvailableInstance();
	if (instanceID == INVALID_WORLDID)
	{
		ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent: Failed to create/get instance for player %u", pPlayer->GetCharID());
		return;
	}

	m_mapPlayerInstance[pPlayer->GetID()] = instanceID;
	m_mapInstancePlayers[instanceID].insert(pPlayer->GetID());

	// Store original level
	m_mapOriginalLevels[pPlayer->GetID()] = pPlayer->GetLevel();

	// Set level to 1
	CGameServer* app = (CGameServer*)g_pApp;
	if (!app)
		return;

	sEXP_TBLDAT* ExpData = (sEXP_TBLDAT*)g_pTableContainer->GetExpTable()->FindData(1);
	if (ExpData)
	{
		BYTE curlv = pPlayer->GetLevel();
		CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_LEVEL));
		sGU_UPDATE_CHAR_LEVEL* res = (sGU_UPDATE_CHAR_LEVEL*)packet.GetPacketData();
		res->wOpCode = GU_UPDATE_CHAR_LEVEL;
		res->byCurLevel = 1;
		res->byPrevLevel = curlv;
		res->dwMaxExpInThisLevel = ExpData->dwNeed_Exp;
		res->handle = pPlayer->GetID();
		packet.SetPacketLen(sizeof(sGU_UPDATE_CHAR_LEVEL));
		pPlayer->Broadcast(&packet);

		// Set level to 1 and max EXP properly using LevelUp
		// LevelUp sets player_data.dwMaxExpInThisLevel internally, which is needed for leveling to work
		pPlayer->SetLevel(0); // Set to 0 first so LevelUp can properly set max EXP
		pPlayer->SetExp(0); // Set EXP to 0
		pPlayer->LevelUp(0, 1); // Level up from 0 to 1 - this sets player_data.dwMaxExpInThisLevel correctly
		
		pPlayer->UpdateCharSP(0);
		pPlayer->GetCharAtt()->CalculateAll();
		
		pPlayer->UpdateCurLpEp(pPlayer->GetLastMaxLP(), pPlayer->GetLastMaxEP(), true, false);
		
		// Send EXP update packet to client
		CNtlPacket expPacket(sizeof(sGU_UPDATE_CHAR_EXP));
		sGU_UPDATE_CHAR_EXP* expRes = (sGU_UPDATE_CHAR_EXP*)expPacket.GetPacketData();
		expRes->wOpCode = GU_UPDATE_CHAR_EXP;
		expRes->handle = pPlayer->GetID();
		expRes->dwCurExp = 0;
		expRes->dwAcquisitionExp = 0;
		expRes->dwIncreasedExp = 0;
		expRes->dwBonusExp = 0;
		expPacket.SetPacketLen(sizeof(sGU_UPDATE_CHAR_EXP));
		pPlayer->SendPacket(&expPacket);
		
		ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent: Set player %u to level 1, EXP 0, MaxEXP %u", pPlayer->GetCharID(), ExpData->dwNeed_Exp);
	}

	// Initialize instance if this is the first player
	if (m_mapInstancePlayers[instanceID].size() == 1)
	{
		m_mapInstanceStartTick[instanceID] = app->GetCurTickCount();
		m_mapInstanceWaitingForStart[instanceID] = true;
		m_mapPvPPhaseActive[instanceID] = false;
	}

	// Add to alive players
	m_setAlivePlayers.insert(pPlayer->GetID());
	m_mapPlayerKills[pPlayer->GetID()] = 0;
	m_mapPlayerDeaths[pPlayer->GetID()] = 0;
}

void BattleRoyaleEvent::OnPlayerDeclinedProposal(CPlayer* pPlayer)
{
	if (!pPlayer)
		return;

	// Remove from tracking if needed
	m_setProposalSent.erase(pPlayer->GetID());
	m_mapOriginalLoc.erase(pPlayer->GetID());
	m_mapOriginalWorld.erase(pPlayer->GetID());
}

void BattleRoyaleEvent::OnPlayerLoaded(CPlayer* pPlayer)
{
	if (!pPlayer || !pPlayer->IsInitialized())
		return;

	if (m_mapPlayerInstance.find(pPlayer->GetID()) == m_mapPlayerInstance.end())
		return; // Not in battle royale

	WORLDID instanceID = m_mapPlayerInstance[pPlayer->GetID()];
	m_mapLoadedPlayers[instanceID].insert(pPlayer->GetID());

	// Update movement tick
	CGameServer* app = (CGameServer*)g_pApp;
	if (app)
		m_mapLastMovementTick[pPlayer->GetID()] = app->GetCurTickCount();
}

void BattleRoyaleEvent::OnPlayerDeath(CPlayer* pPlayer)
{
	if (!pPlayer || !pPlayer->IsInitialized())
		return;

	if (m_mapPlayerInstance.find(pPlayer->GetID()) == m_mapPlayerInstance.end())
		return; // Not in battle royale

	m_setAlivePlayers.erase(pPlayer->GetID());
	m_mapPlayerDeaths[pPlayer->GetID()]++;

	WORLDID instanceID = m_mapPlayerInstance[pPlayer->GetID()];

	// Count alive players in this instance only
	DWORD dwAliveInInstance = 0;
	for (HOBJECT hPlayer : m_mapInstancePlayers[instanceID])
	{
		if (m_setAlivePlayers.find(hPlayer) != m_setAlivePlayers.end())
			dwAliveInInstance++;
	}

	// Check win condition for this instance
	if (dwAliveInInstance <= 1)
	{
		// Only one or zero players left - end event for this instance
		SendRewards(instanceID);
		CleanupInstance(instanceID);
	}
}

void BattleRoyaleEvent::OnPlayerKill(CPlayer* pKiller, CPlayer* pVictim)
{
	if (!pKiller || !pVictim)
		return;

	if (m_mapPlayerInstance.find(pKiller->GetID()) == m_mapPlayerInstance.end())
		return; // Not in battle royale

	m_mapPlayerKills[pKiller->GetID()]++;
}

void BattleRoyaleEvent::OnMonsterKill(CMonster* pMonster, CPlayer* pKiller)
{
	if (!pMonster || !pKiller || !pKiller->IsInitialized())
		return;

	if (m_mapPlayerInstance.find(pKiller->GetID()) == m_mapPlayerInstance.end())
		return; // Not in battle royale

	WORLDID instanceID = m_mapPlayerInstance[pKiller->GetID()];
	if (m_mapPvPPhaseActive.find(instanceID) == m_mapPvPPhaseActive.end() || !m_mapPvPPhaseActive[instanceID])
	{
		// Still in preparation phase - grant EXP (100 exp per kill)
		pKiller->UpdateExp(300, true);
	}
}

void BattleRoyaleEvent::UpdatePlayerMovement(CPlayer* pPlayer)
{
	if (!pPlayer)
		return;

	if (m_mapPlayerInstance.find(pPlayer->GetID()) == m_mapPlayerInstance.end())
		return; // Not in battle royale

	CGameServer* app = (CGameServer*)g_pApp;
	if (!app)
		return;

	m_mapLastMovementTick[pPlayer->GetID()] = app->GetCurTickCount();
}

WORLDID BattleRoyaleEvent::GetAvailableInstance()
{
	CGameServer* app = (CGameServer*)g_pApp;
	if (!app || !app->GetGameMain() || !app->GetGameMain()->GetWorldManager())
		return INVALID_WORLDID;

	CWorldManager* pWorldManager = app->GetGameMain()->GetWorldManager();

	// Find an instance with less than MAX_PLAYERS_PER_INSTANCE
	// Also verify the world actually exists in WorldManager
	std::vector<WORLDID> instancesToRemove;
	for (auto it = m_mapInstancePlayers.begin(); it != m_mapInstancePlayers.end(); ++it)
	{
		if (it->second.size() < MAX_PLAYERS_PER_INSTANCE)
		{
			// Verify the world instance still exists
			CWorld* pWorld = pWorldManager->FindWorld(it->first);
			if (pWorld)
			{
				ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent: Found available instance %u with %u players", it->first, it->second.size());
				return it->first;
			}
			else
			{
				ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent: Instance %u in map but not found in WorldManager, marking for removal", it->first);
				instancesToRemove.push_back(it->first);
			}
		}
	}

	// Remove invalid instances
	for (WORLDID instanceID : instancesToRemove)
	{
		m_mapInstancePlayers.erase(instanceID);
		m_mapLoadedPlayers.erase(instanceID);
		m_mapInstanceStartTick.erase(instanceID);
		m_mapInstanceWaitingForStart.erase(instanceID);
		m_mapPreparationEndTick.erase(instanceID);
		m_mapEventEndTick.erase(instanceID);
		m_mapPvPPhaseActive.erase(instanceID);
	}

	// Create new instance
	sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData(BATTLE_ROYALE_WORLD_TBLIDX);
	if (pWorldTbldat)
	{
		CWorld* pWorld = pWorldManager->CreateWorld(pWorldTbldat);
		if (pWorld)
		{
			WORLDID newInstanceID = pWorld->GetID();
			m_mapInstancePlayers[newInstanceID] = std::set<HOBJECT>();
			m_mapLoadedPlayers[newInstanceID] = std::set<HOBJECT>();
			ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent: Created new instance %u", newInstanceID);
			return newInstanceID;
		}
		else
		{
			ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent: Failed to create new world instance");
		}
	}
	else
	{
		ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent: World table data not found for world %u", BATTLE_ROYALE_WORLD_TBLIDX);
	}

	return INVALID_WORLDID;
}

bool BattleRoyaleEvent::IsPvPPhaseActive(WORLDID worldID) const
{
	auto it = m_mapPvPPhaseActive.find(worldID);
	if (it != m_mapPvPPhaseActive.end())
		return it->second;
	return false;
}

WORLDID BattleRoyaleEvent::GetPlayerInstanceID(CPlayer* pPlayer) const
{
	if (!pPlayer)
		return INVALID_WORLDID;
	auto it = m_mapPlayerInstance.find(pPlayer->GetID());
	if (it != m_mapPlayerInstance.end())
		return it->second;
	return INVALID_WORLDID;
}

bool BattleRoyaleEvent::IsPlayerInBattleRoyale(CPlayer* pPlayer) const
{
	if (!pPlayer)
		return false;
	return m_mapPlayerInstance.find(pPlayer->GetID()) != m_mapPlayerInstance.end();
}

bool BattleRoyaleEvent::CanPlayerMove(CPlayer* pPlayer) const
{
	if (!pPlayer)
		return true;

	if (!m_bEventActive)
		return true; // Allow movement if event not active

	auto it = m_mapPlayerInstance.find(pPlayer->GetID());
	if (it == m_mapPlayerInstance.end())
		return true; // Not in battle royale

	WORLDID instanceID = it->second;
	auto itWaiting = m_mapInstanceWaitingForStart.find(instanceID);
	if (itWaiting == m_mapInstanceWaitingForStart.end())
		return true; // Instance not waiting

	if (itWaiting->second)
		return false; // Still waiting for start

	return true; // Can move
}

void BattleRoyaleEvent::TickProcess(DWORD dwTick)
{
	if (!m_bEventActive)
		return;

	CGameServer* app = (CGameServer*)g_pApp;
	if (!app)
		return;

	// Check proposal timeout
	if (dwTick >= m_dwProposalEndTick && m_dwProposalEndTick > 0)
	{
		// Proposal phase ended - remove players who didn't accept
		std::set<HOBJECT> toRemove;
		for (auto it = m_setProposalSent.begin(); it != m_setProposalSent.end(); ++it)
		{
			if (m_setProposalAccepted.find(*it) == m_setProposalAccepted.end())
			{
				toRemove.insert(*it);
			}
		}
		for (auto it = toRemove.begin(); it != toRemove.end(); ++it)
		{
			m_setProposalSent.erase(*it);
			m_mapOriginalLoc.erase(*it);
			m_mapOriginalWorld.erase(*it);
		}
		m_dwProposalEndTick = 0;
	}

	// Process each instance
	std::vector<WORLDID> instancesToCleanup;
	for (auto it = m_mapInstancePlayers.begin(); it != m_mapInstancePlayers.end(); ++it)
	{
		WORLDID instanceID = it->first;
		bool bShouldCleanup = false;

		// Check if waiting for players to load
		auto itWaiting = m_mapInstanceWaitingForStart.find(instanceID);
		if (itWaiting != m_mapInstanceWaitingForStart.end() && itWaiting->second)
		{
			auto itStartTick = m_mapInstanceStartTick.find(instanceID);
			if (itStartTick == m_mapInstanceStartTick.end())
				continue;

			DWORD instanceStartTick = itStartTick->second;
			bool allLoaded = (m_mapLoadedPlayers[instanceID].size() == m_mapInstancePlayers[instanceID].size());
			bool timeoutReached = (dwTick >= instanceStartTick + (LOAD_WAIT_SECONDS * 1000));

			if (allLoaded || timeoutReached)
			{
				// Start preparation phase
				m_mapInstanceWaitingForStart[instanceID] = false;
				m_mapPreparationEndTick[instanceID] = dwTick + (PREPARATION_PHASE_MINUTES * 60 * 1000);
				m_mapEventEndTick[instanceID] = dwTick + (TOTAL_EVENT_MINUTES * 60 * 1000);

				// Spawn monsters
				SpawnRandomMonsters(instanceID);

				// Send message to players in this instance
				CNtlStringW msg;
				CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
				sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
				res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
				WCHAR wszFormatBuf[256];
				WCharTLiteralToWCHAR(L"Battle Royale Preparation Phase Started! Kill monsters to level up!", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				res->wMessageLengthInUnicode = (WORD)msg.Format(wszFormatBuf);
				res->byDisplayType = SERVER_TEXT_SYSNOTICE;
				NTL_SAFE_WCSCPY(res->awchMessage, msg.c_str());
				packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));

				if (g_pObjectManager)
				{
					for (HOBJECT hPlayer : m_mapInstancePlayers[instanceID])
					{
						CPlayer* pPlayer = g_pObjectManager->GetPC(hPlayer);
						if (pPlayer && pPlayer->IsInitialized())
							pPlayer->SendPacket(&packet);
					}
				}
			}
		}
		else
		{
			// Check preparation phase end
			auto itPvP = m_mapPvPPhaseActive.find(instanceID);
			auto itPrepEnd = m_mapPreparationEndTick.find(instanceID);
			if (itPrepEnd != m_mapPreparationEndTick.end() && 
				(itPvP == m_mapPvPPhaseActive.end() || !itPvP->second) && 
				dwTick >= itPrepEnd->second)
			{
				// Start PvP phase
				m_mapPvPPhaseActive[instanceID] = true;

				// Send message to players
				CNtlStringW msg;
				CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
				sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
				res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
				WCHAR wszFormatBuf[256];
				WCharTLiteralToWCHAR(L"PvP Phase Started! Eliminate all other players!", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
				res->wMessageLengthInUnicode = (WORD)msg.Format(wszFormatBuf);
				res->byDisplayType = SERVER_TEXT_EMERGENCY;
				NTL_SAFE_WCSCPY(res->awchMessage, msg.c_str());
				packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));

				if (g_pObjectManager)
				{
					for (HOBJECT hPlayer : m_mapInstancePlayers[instanceID])
					{
						CPlayer* pPlayer = g_pObjectManager->GetPC(hPlayer);
						if (pPlayer && pPlayer->IsInitialized())
							pPlayer->SendPacket(&packet);
					}
				}
			}

			// Check event end
			auto itEventEnd = m_mapEventEndTick.find(instanceID);
			if (itEventEnd != m_mapEventEndTick.end() && dwTick >= itEventEnd->second)
			{
				bShouldCleanup = true;
			}
		}

		// Check idle players
		CheckIdlePlayers();

		if (bShouldCleanup)
		{
			instancesToCleanup.push_back(instanceID);
		}
	}

		// Cleanup finished instances
		for (WORLDID instanceID : instancesToCleanup)
		{
			SendRewards(instanceID);
			CleanupInstance(instanceID);
		}

	// Check if all instances are done
	if (m_mapInstancePlayers.empty())
	{
		EndEvent();
	}
}

void BattleRoyaleEvent::SpawnRandomMonsters(WORLDID worldID)
{
	CGameServer* app = (CGameServer*)g_pApp;
	if (!app || !app->GetGameMain() || !app->GetGameMain()->GetWorldManager())
		return;

	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld(worldID);
	if (!pWorld || !pWorld->GetTbldat())
		return;

	// Get world bounds (simplified - spawn in a reasonable area)
	CNtlVector centerLoc = pWorld->GetTbldat()->vStart1Loc;
	
	// Spawn many monsters randomly across the map
	int monsterCount = 400; // Increased spawn count for more action
	for (int i = 0; i < monsterCount; i++)
	{
		// Use mob ID 5515116 for battle royale
		TBLIDX mobTblidx = 5515116;
		sMOB_TBLDAT* pMOBTblData = (sMOB_TBLDAT*)g_pTableContainer->GetMobTable()->FindData(mobTblidx);
		if (pMOBTblData)
		{
			sVECTOR3 spawnloc;
			spawnloc.x = centerLoc.x + RandomRangeF(-500.0f, 500.0f);
			spawnloc.y = centerLoc.y;
			spawnloc.z = centerLoc.z + RandomRangeF(-500.0f, 500.0f);

			sVECTOR3 spawndir;
			spawndir.x = RandomRangeF(-1.0f, 1.0f);
			spawndir.y = 0.0f;
			spawndir.z = RandomRangeF(-1.0f, 1.0f);
			
			CNtlVector spawndirVec;
			spawndirVec.x = spawndir.x;
			spawndirVec.y = spawndir.y;
			spawndirVec.z = spawndir.z;
			if (spawndirVec.SafeNormalize())
			{
				spawndir.x = spawndirVec.x;
				spawndir.y = spawndirVec.y;
				spawndir.z = spawndirVec.z;
			}

			sSPAWN_TBLDAT sMobSpawn;
			sMobSpawn.vSpawn_Dir.CopyFrom(spawndir);
			sMobSpawn.vSpawn_Loc.CopyFrom(spawnloc);
			sMobSpawn.dwParty_Index = INVALID_DWORD;
			sMobSpawn.byMove_Range = 30;
			sMobSpawn.bySpawn_Move_Type = SPAWN_MOVE_WANDER;
			sMobSpawn.bySpawn_Loc_Range = 30;
			sMobSpawn.byWander_Range = 30;
			sMobSpawn.path_Table_Index = INVALID_TBLIDX;
			sMobSpawn.playScript = INVALID_TBLIDX;
			sMobSpawn.playScriptScene = INVALID_TBLIDX;
			sMobSpawn.aiScript = INVALID_TBLIDX;
			sMobSpawn.aiScriptScene = INVALID_TBLIDX;
			sMobSpawn.actionPatternTblidx = 1;

			if (g_pObjectManager)
			{
				CMonster* pMob = (CMonster*)g_pObjectManager->CreateCharacter(OBJTYPE_MOB);
				if (pMob)
				{
					// Set stats BEFORE CreateDataAndSpawn so they're applied correctly
					// We need to modify the mob table data temporarily or set after spawn
					if (pMob->CreateDataAndSpawn(worldID, pMOBTblData, &sMobSpawn, false, 0))
					{
						// Override mob stats for battle royale: 1000 LP, 40 defense, level 5, 100 EXP
						pMob->SetLevel(5);
						pMob->SetEffectiveLevel(5);
						pMob->SetExp(300);
						
						// Recalculate attributes with new level
						if (pMob->GetCharAtt())
						{
							// Recalculate base attributes with level 5
							pMob->GetCharAtt()->CalculateBaseAtt();
							
							// Directly set the attribute values we want
							sAVATAR_ATTRIBUTE* pAttr = pMob->GetCharAtt()->GetAvatarAttributePointer();
							if (pAttr)
							{
								// Set base values directly
								pAttr->lastMaxLp = 1000;
								pAttr->wLastPhysicalDefence = 40;
								pAttr->wLastEnergyDefence = 40;
								
								// Recalculate last attributes (applies buffs, etc.)
								pMob->GetCharAtt()->CalculateAll();
								
								// Update current LP to max
								DWORD maxLP = pMob->GetLastMaxLP();
								pMob->UpdateCurLP(maxLP, false, false);
								
								ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent: Set mob %u to level %u, LP %u, EXP %u (baseMaxLp=%d, lastMaxLp=%d)", 
									pMob->GetTblidx(), pMob->GetLevel(), maxLP, pMob->GetExp(), pAttr->baseMaxLp, pAttr->lastMaxLp);
							}
						}
					}
				}
			}
		}
	}
}

void BattleRoyaleEvent::CheckIdlePlayers()
{
	CGameServer* app = (CGameServer*)g_pApp;
	DWORD dwCurrentTick = app->GetCurTickCount();

	for (auto it = m_mapLastMovementTick.begin(); it != m_mapLastMovementTick.end(); ++it)
	{
		HOBJECT hPlayer = it->first;
		DWORD dwLastMovement = it->second;

		if (dwCurrentTick - dwLastMovement > IDLE_THRESHOLD_MS)
		{
			CPlayer* pPlayer = g_pObjectManager->GetPC(hPlayer);
			if (pPlayer && pPlayer->IsInitialized() && m_setAlivePlayers.find(hPlayer) != m_setAlivePlayers.end())
			{
				// Apply idle debuff (you may need to configure the debuff skill ID)
				// For now, we'll skip this as we need to know the debuff skill ID
				// TODO: Apply idle debuff skill
			}
		}
	}
}

void BattleRoyaleEvent::SendRewards(WORLDID instanceID)
{
	// Find last player alive and player with most kills in this instance
	HOBJECT hWinner = INVALID_HOBJECT;
	HOBJECT hTopKiller = INVALID_HOBJECT;
	DWORD dwMaxKills = 0;

	// Find winner (last alive in this instance)
	for (HOBJECT hPlayer : m_mapInstancePlayers[instanceID])
	{
		if (m_setAlivePlayers.find(hPlayer) != m_setAlivePlayers.end())
		{
			hWinner = hPlayer;
			break;
		}
	}

	// Find top killer in this instance
	for (HOBJECT hPlayer : m_mapInstancePlayers[instanceID])
	{
		auto it = m_mapPlayerKills.find(hPlayer);
		if (it != m_mapPlayerKills.end() && it->second > dwMaxKills)
		{
			dwMaxKills = it->second;
			hTopKiller = hPlayer;
		}
	}

	// Send mail to winner
	if (hWinner != INVALID_HOBJECT && g_pObjectManager)
	{
		CPlayer* pWinner = g_pObjectManager->GetPC(hWinner);
		if (pWinner && pWinner->IsInitialized())
		{
			sMAIL_NEW_PROFILE mail;
			memset(&mail, 0, sizeof(sMAIL_NEW_PROFILE));
			mail.mailID = INVALID_MAILID; // Will be assigned by query server
			mail.byMailType = eMAIL_TYPE_BASIC;
			mail.bySenderType = eMAIL_SENDER_TYPE_BASIC;
			NTL_WCSCPY_S(mail.wszFromName, NTL_MAX_SIZE_CHAR_NAME + 1, L"Battle Royale System");
			mail.bIsAccept = false;
			mail.bIsRead = false;
			mail.bIsLock = false;
			mail.dwZenny = 0;
			mail.byExpired = 10; // 10 days
			// tCreateTime is initialized by memset
			mail.endTime = 0;

			// TODO: Add reward items to mail if needed
			pWinner->AddMail(&mail);
		}
	}

	// Send mail to top killer
	if (hTopKiller != INVALID_HOBJECT && hTopKiller != hWinner && g_pObjectManager)
	{
		CPlayer* pTopKiller = g_pObjectManager->GetPC(hTopKiller);
		if (pTopKiller && pTopKiller->IsInitialized())
		{
			sMAIL_NEW_PROFILE mail;
			memset(&mail, 0, sizeof(sMAIL_NEW_PROFILE));
			mail.mailID = INVALID_MAILID;
			mail.byMailType = eMAIL_TYPE_BASIC;
			mail.bySenderType = eMAIL_SENDER_TYPE_BASIC;
			NTL_WCSCPY_S(mail.wszFromName, NTL_MAX_SIZE_CHAR_NAME + 1, L"Battle Royale System");
			mail.bIsAccept = false;
			mail.bIsRead = false;
			mail.bIsLock = false;
			mail.dwZenny = 0;
			mail.byExpired = 10; // 10 days
			// tCreateTime is initialized by memset
			mail.endTime = 0;

			// TODO: Add reward items to mail if needed
			pTopKiller->AddMail(&mail);
		}
	}
}

void BattleRoyaleEvent::CleanupInstance(WORLDID instanceID)
{
	// Restore all players in this instance
	std::set<HOBJECT> playersToRestore = m_mapInstancePlayers[instanceID];
	if (g_pObjectManager)
	{
		for (HOBJECT hPlayer : playersToRestore)
		{
			CPlayer* pPlayer = g_pObjectManager->GetPC(hPlayer);
			if (pPlayer && pPlayer->IsInitialized())
			{
				RestorePlayer(pPlayer);
			}
		}
	}

	// Clean up instance data
	m_mapInstancePlayers.erase(instanceID);
	m_mapLoadedPlayers.erase(instanceID);
	m_mapInstanceStartTick.erase(instanceID);
	m_mapInstanceWaitingForStart.erase(instanceID);
	m_mapPreparationEndTick.erase(instanceID);
	m_mapEventEndTick.erase(instanceID);
	m_mapPvPPhaseActive.erase(instanceID);

	// Remove players from tracking
	for (HOBJECT hPlayer : playersToRestore)
	{
		m_mapPlayerInstance.erase(hPlayer);
		m_mapOriginalLevels.erase(hPlayer);
		m_mapOriginalLoc.erase(hPlayer);
		m_mapOriginalWorld.erase(hPlayer);
		m_mapPlayerKills.erase(hPlayer);
		m_mapPlayerDeaths.erase(hPlayer);
		m_setAlivePlayers.erase(hPlayer);
		m_mapLastMovementTick.erase(hPlayer);
	}
}

void BattleRoyaleEvent::OnPlayerDisconnect(CPlayer* pPlayer)
{
	if (!pPlayer)
		return;

	HOBJECT hPlayer = pPlayer->GetID();
	
	// Check if player is in battle royale
	auto it = m_mapPlayerInstance.find(hPlayer);
	if (it == m_mapPlayerInstance.end())
		return; // Not in battle royale

	WORLDID instanceID = it->second;

	// Remove from alive players
	m_setAlivePlayers.erase(hPlayer);

	// Remove from instance player list
	auto itInstance = m_mapInstancePlayers.find(instanceID);
	if (itInstance != m_mapInstancePlayers.end())
	{
		itInstance->second.erase(hPlayer);
		
		// If instance becomes empty or has only disconnected players, clean it up
		if (itInstance->second.empty())
		{
			CleanupInstance(instanceID);
		}
		else
		{
			// Check if only one player remains alive in this instance
			DWORD dwAliveInInstance = 0;
			for (HOBJECT hP : itInstance->second)
			{
				if (m_setAlivePlayers.find(hP) != m_setAlivePlayers.end())
					dwAliveInInstance++;
			}

			if (dwAliveInInstance <= 1)
			{
				// End event for this instance
				SendRewards(instanceID);
				CleanupInstance(instanceID);
			}
		}
	}

	// Clean up player tracking data
	m_mapPlayerInstance.erase(hPlayer);
	m_setProposalSent.erase(hPlayer);
	m_setProposalAccepted.erase(hPlayer);
	m_mapOriginalLevels.erase(hPlayer);
	m_mapOriginalLoc.erase(hPlayer);
	m_mapOriginalWorld.erase(hPlayer);
	m_mapPlayerKills.erase(hPlayer);
	m_mapPlayerDeaths.erase(hPlayer);
	m_mapLastMovementTick.erase(hPlayer);
	m_mapLoadedPlayers[instanceID].erase(hPlayer);

	// Restore player if they're still in the world (teleport them back)
	if (pPlayer->IsInitialized() && pPlayer->GetCurWorld())
	{
		RestorePlayer(pPlayer);
	}
}

void BattleRoyaleEvent::RestorePlayer(CPlayer* pPlayer)
{
	if (!pPlayer || !pPlayer->IsInitialized())
		return;

	// Restore level
	if (m_mapOriginalLevels.find(pPlayer->GetID()) != m_mapOriginalLevels.end())
	{
		BYTE originalLevel = m_mapOriginalLevels[pPlayer->GetID()];
		CGameServer* app = (CGameServer*)g_pApp;
		if (!app)
			return;

		sEXP_TBLDAT* ExpData = (sEXP_TBLDAT*)g_pTableContainer->GetExpTable()->FindData(originalLevel);
		if (ExpData)
		{
			BYTE curlv = pPlayer->GetLevel();
			CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_LEVEL));
			sGU_UPDATE_CHAR_LEVEL* res = (sGU_UPDATE_CHAR_LEVEL*)packet.GetPacketData();
			res->wOpCode = GU_UPDATE_CHAR_LEVEL;
			res->byCurLevel = originalLevel;
			res->byPrevLevel = curlv;
			res->dwMaxExpInThisLevel = ExpData->dwNeed_Exp;
			res->handle = pPlayer->GetID();
			packet.SetPacketLen(sizeof(sGU_UPDATE_CHAR_LEVEL));
			pPlayer->Broadcast(&packet);

			pPlayer->SetLevel(originalLevel);
			pPlayer->UpdateCharSP((pPlayer->GetLevel() - 1) + pPlayer->GetSkillPointsBought());
			pPlayer->GetCharAtt()->CalculateAll();
			pPlayer->UpdateCurLpEp(pPlayer->GetLastMaxLP(), pPlayer->GetLastMaxEP(), true, false);
		}
	}

	// Teleport back to original world
	if (m_mapOriginalWorld.find(pPlayer->GetID()) != m_mapOriginalWorld.end() &&
		m_mapOriginalLoc.find(pPlayer->GetID()) != m_mapOriginalLoc.end())
	{
		WORLDID originalWorldID = m_mapOriginalWorld[pPlayer->GetID()];
		CNtlVector originalLoc = m_mapOriginalLoc[pPlayer->GetID()];
		pPlayer->StartTeleport(originalLoc, pPlayer->GetCurDir(), originalWorldID, TELEPORT_TYPE_COMMAND);
	}
}

void BattleRoyaleEvent::EndEvent()
{
	// Cleanup all instances
	std::vector<WORLDID> instancesToCleanup;
	for (auto it = m_mapInstancePlayers.begin(); it != m_mapInstancePlayers.end(); ++it)
	{
		instancesToCleanup.push_back(it->first);
	}

	for (WORLDID instanceID : instancesToCleanup)
	{
		CleanupInstance(instanceID);
	}

	// Reset state
	Init();
}
