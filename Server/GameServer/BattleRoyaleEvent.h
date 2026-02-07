#ifndef __BATTLE_ROYALE_EVENT_SYSTEM__
#define __BATTLE_ROYALE_EVENT_SYSTEM__

#include "NtlSingleton.h"
#include "NtlSharedType.h"
#include "NtlVector.h"
#include <set>
#include <map>
#include <unordered_map>

class CPlayer;
class CMonster;
class CWorld;

class BattleRoyaleEvent : public CNtlSingleton<BattleRoyaleEvent>
{
public:
	BattleRoyaleEvent();
	virtual ~BattleRoyaleEvent();

	void Init();

	// Event control
	void StartEvent();
	void EndEvent();
	bool IsEventActive() const { return m_bEventActive; }

	// Player callbacks
	void OnPlayerAcceptedProposal(CPlayer* pPlayer);
	void OnPlayerDeclinedProposal(CPlayer* pPlayer);
	void OnPlayerLoaded(CPlayer* pPlayer);
	void OnPlayerDeath(CPlayer* pPlayer);
	void OnPlayerKill(CPlayer* pKiller, CPlayer* pVictim);
	void OnMonsterKill(CMonster* pMonster, CPlayer* pKiller);
	void UpdatePlayerMovement(CPlayer* pPlayer);
	void OnPlayerDisconnect(CPlayer* pPlayer);

	// Instance management
	WORLDID GetAvailableInstance();
	WORLDID GetPlayerInstanceID(CPlayer* pPlayer) const;
	bool IsPvPPhaseActive(WORLDID worldID) const;
	bool IsPlayerInBattleRoyale(CPlayer* pPlayer) const;
	bool CanPlayerMove(CPlayer* pPlayer) const;

	// Main update loop
	void TickProcess(DWORD dwTick);

private:
	// Event state
	bool m_bEventActive;
	DWORD m_dwEventStartTick;
	DWORD m_dwProposalEndTick;

	// Player tracking
	std::set<HOBJECT> m_setProposalSent;
	std::set<HOBJECT> m_setProposalAccepted;
	std::map<HOBJECT, WORLDID> m_mapPlayerInstance;
	std::map<WORLDID, std::set<HOBJECT>> m_mapInstancePlayers;
	std::map<WORLDID, std::set<HOBJECT>> m_mapLoadedPlayers;
	std::map<HOBJECT, BYTE> m_mapOriginalLevels;
	std::map<HOBJECT, CNtlVector> m_mapOriginalLoc;
	std::map<HOBJECT, WORLDID> m_mapOriginalWorld;

	// Instance state per instance
	std::map<WORLDID, DWORD> m_mapInstanceStartTick;
	std::map<WORLDID, bool> m_mapInstanceWaitingForStart;
	std::map<WORLDID, DWORD> m_mapPreparationEndTick;
	std::map<WORLDID, DWORD> m_mapEventEndTick;
	std::map<WORLDID, bool> m_mapPvPPhaseActive;

	// Kill tracking
	std::map<HOBJECT, DWORD> m_mapPlayerKills;
	std::map<HOBJECT, DWORD> m_mapPlayerDeaths;
	std::set<HOBJECT> m_setAlivePlayers;

	// Idle detection
	std::map<HOBJECT, DWORD> m_mapLastMovementTick;

	// Monster spawning
	void SpawnRandomMonsters(WORLDID worldID);
	void CheckIdlePlayers();
	void SendRewards(WORLDID instanceID);
	void CleanupInstance(WORLDID instanceID);
	void RestorePlayer(CPlayer* pPlayer);

	// Constants
	static const TBLIDX BATTLE_ROYALE_WORLD_TBLIDX;
	static const BYTE MAX_PLAYERS_PER_INSTANCE;
	static const WORD PROPOSAL_TIMEOUT_SECONDS;
	static const WORD LOAD_WAIT_SECONDS;
	static const WORD PREPARATION_PHASE_MINUTES;
	static const WORD TOTAL_EVENT_MINUTES;
	static const DWORD IDLE_CHECK_INTERVAL_MS;
	static const DWORD IDLE_THRESHOLD_MS;
};

#define GetBattleRoyaleEvent()		BattleRoyaleEvent::GetInstance()
#define g_pBattleRoyaleEvent		GetBattleRoyaleEvent()

#endif
