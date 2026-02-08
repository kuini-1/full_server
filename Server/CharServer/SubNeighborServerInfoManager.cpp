#include "stdafx.h"
#include "CharServer.h"
#include "NtlPacketCU.h"
#include "NtlPacketCM.h"


CSubNeighborServerInfoManager::CSubNeighborServerInfoManager()
{
	m_pEventHolder = new EventableObjectHolder(-1);
}

CSubNeighborServerInfoManager::~CSubNeighborServerInfoManager()
{
}


void CSubNeighborServerInfoManager::TickProcess(DWORD dwTickDiff)
{
	m_pEventHolder->Update(dwTickDiff);
}

void CSubNeighborServerInfoManager::StartEvents()
{
	g_pEventMgr->AddEvent(this, &CSubNeighborServerInfoManager::OnEvent_ServerAliveUpdate, SERVER_EVENT_KEEP_ALIVE, 4000, INVALID_DWORD, 0);
}

void CSubNeighborServerInfoManager::OnEvent_ServerAliveUpdate()
{
	CCharServer* app = (CCharServer*)g_pApp;

	//NTL_PRINT(PRINT_APP, "send event to master");

	CNtlPacket packet(sizeof(sCM_PING_RES));
	sCM_PING_RES * res = (sCM_PING_RES *)packet.GetPacketData();
	res->wOpCode = CM_PING_RES;
	res->dwTick = GetTickCount();
	packet.SetPacketLen(sizeof(sCM_PING_RES));
	app->SendTo(app->m_pMasterServerSession, &packet);
}


void CSubNeighborServerInfoManager::LoadServerList(HSESSION hSession)
{
	for (std::map<SERVERFARMID, sSERVER_FARM_INFO *>::iterator it = m_mapGameServerFarmInfo.begin(); it != m_mapGameServerFarmInfo.end(); it++)
	{
		sSERVER_FARM_INFO* pFarm = it->second;
		if (pFarm)
		{
			CNtlPacket packet(sizeof(sCU_SERVER_FARM_INFO_NFY));
			sCU_SERVER_FARM_INFO_NFY * res = (sCU_SERVER_FARM_INFO_NFY *)packet.GetPacketData();
			res->wOpCode = CU_SERVER_FARM_INFO_NFY;
			res->serverFarmInfo.serverFarmId = pFarm->serverFarmId;
			NTL_WCSCPY_S(res->serverFarmInfo.wszGameServerFarmName, NTL_MAX_SIZE_SERVER_FARM_NAME_UNICODE + 1, pFarm->wszGameServerFarmName);
			res->serverFarmInfo.byServerStatus = pFarm->byServerStatus;
			res->serverFarmInfo.dwLoad = DWORD((float)pFarm->dwLoad / (float)pFarm->dwMaxLoad * 100.f);
			res->serverFarmInfo.dwMaxLoad = 100;
			packet.SetPacketLen(sizeof(sCU_SERVER_FARM_INFO_NFY));
			g_pApp->Send(hSession, &packet);
		}
	}
}


