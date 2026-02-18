#include "stdafx.h"
#include "MasterServerSession.h"
#include "CharServer.h"
#include "NtlPacketCM.h"
#include "mc_opcodes.h"
#include "PacketEventObj.h"


int CMasterServerSession::OnConnect()
{
	CCharServer * app = (CCharServer*) NtlSfxGetApp();
	NTL_PRINT(PRINT_APP, "CONNECTED TO MASTER SERVER \n");

	app->SetMasterServerSession( this );

	//send server data to master server
	CNtlPacket packet(sizeof(sCM_NOTIFY_SERVER_BEGIN));
	sCM_NOTIFY_SERVER_BEGIN * res = (sCM_NOTIFY_SERVER_BEGIN *)packet.GetPacketData();
	res->wOpCode = CM_NOTIFY_SERVER_BEGIN;
	res->sServerInfo.bIsOn = true;
	res->sServerInfo.byRunningState = DBO_SERVER_RUNNING_STATE_RUNNING;
	res->sServerInfo.dwLoad = 0;
	res->sServerInfo.dwMaxLoad = DWORD((float)app->m_config.nMaxConnection * 0.95f); //set max connections to 95% limit
	res->sServerInfo.wPortForClient = app->m_config.wClientAcceptPort;
	snprintf(res->sServerInfo.achPublicAddress, NTL_MAX_LENGTH_OF_IP + 1, "%s", app->m_config.strPublicClientAcceptAddr.c_str());
	
	// Log what IP we're registering with
	NTL_PRINT(PRINT_APP, "[CharServer] Registering with Master Server: PublicAddress='%s', Port=%u, ServerID=%u", 
		res->sServerInfo.achPublicAddress, res->sServerInfo.wPortForClient, app->m_config.byServerID);
	if (strcmp(res->sServerInfo.achPublicAddress, "0.0.0.0") == 0 || strlen(res->sServerInfo.achPublicAddress) == 0)
	{
		ERR_LOG(LOG_SYSTEM, "WARNING: Character Server PublicAddress is 0.0.0.0 or empty! Clients will not be able to connect. Check config file PublicAddress setting.");
		NTL_PRINT(PRINT_APP, "[CharServer] ERROR: PublicAddress is invalid! Check Character Server config file [Char Server] PublicAddress setting.");
	}
	
	res->sServerInfo.byServerType = NTL_SERVER_TYPE_CHARACTER;
	res->sServerInfo.byServerIndex = app->m_config.byServerID;
	packet.SetPacketLen( sizeof(sCM_NOTIFY_SERVER_BEGIN) );
	app->Send(GetHandle(), &packet);


	return CNtlSession::OnConnect();
}


void CMasterServerSession::OnClose()
{
	CCharServer * app = (CCharServer*) NtlSfxGetApp();
	NTL_PRINT(PRINT_APP, "DISCONNECTED FROM MASTER SERVER \n");

	g_PlrMgr->DisconnectAll();

	app->SetMasterServerSession(NULL);
}


int CMasterServerSession::OnDispatch(CNtlPacket * pPacket)
{
	CCharServer * app = (CCharServer*)g_pApp;

	sNTLPACKETHEADER * pHeader = (sNTLPACKETHEADER *)pPacket->GetPacketData();

	OpcodeHandler<CMasterServerSession> const* opHandle = mc_opcodeTable->LookupOpcode(pHeader->wOpCode);
	if (opHandle)
	{
		if (opHandle->packetProcessing == PROCESS_INPLACE)
			(this->*opHandle->handler)(pPacket);
		else
			app->PostClientPacketEvent(new TPacketEventObj<CMasterServerSession>(this, opHandle->handler, GetHandle(), pPacket, GetUniqueHandle()));
	}
	else
	{
		return CNtlSession::OnDispatch(pPacket);
	}

	return NTL_SUCCESS;
}