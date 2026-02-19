#include "stdafx.h"
#include "ClientSession.h"
#include "CharServer.h"
#include "NtlPacketCM.h"
#include "NtlResultCode.h"
#include "uc_opcodes.h"
#include "Player.h"
#include "PacketEventObj.h"



CClientSession::~CClientSession()
{
}


int CClientSession::OnAccept()
{
	eUserState = NTL_USER_STATE_NONE;
	m_pPlayer = NULL;
	m_bPlayerAddedToMasterServer = false;
	m_bLoginRequestSent = false;

	ERR_LOG(LOG_USER, "[CharServer] Client connection accepted: IP=%s, Port=%u", GetRemoteIP(), GetRemotePort());

	//start handshake (with client)
	unsigned char buf[] = { 0x03, 0x00, 0xac, 0x86, 0xf5, 0x74 };
	CNtlPacket packet(buf, 0x06);
	g_pApp->Send(GetHandle(), &packet);

	return CNtlSession::OnAccept();
}


void CClientSession::OnClose()
{
	CCharServer* app = (CCharServer*)g_pApp;

	//send to master server remove player from char list
	// Handle both complete login (m_pPlayer exists) and partial login (CM_LOGIN_REQ sent but no response yet)
	if(m_pPlayer)
	{
		//check if in login queue
#ifdef USE_LOGIN_QUEUE_SYSTEM
		if (m_pPlayer->CheckQueue())
		{
			g_pQueueManager->RemoveFromQueue(m_pPlayer->GetAccountID());
			m_pPlayer->SetQueue(false);
		}
#endif

		// Send logout request to Master Server if login request was sent
		// This handles both cases:
		// 1. Login completed successfully (m_bPlayerAddedToMasterServer = true)
		// 2. Login request sent but session closed before response (m_bLoginRequestSent = true)
		// Master Server will handle gracefully if player wasn't actually added
		if (m_bPlayerAddedToMasterServer || m_bLoginRequestSent)
		{
			CNtlPacket packet(sizeof(sCM_LOGOUT_REQ));
			sCM_LOGOUT_REQ * res = (sCM_LOGOUT_REQ *)packet.GetPacketData();
			res->wOpCode = CM_LOGOUT_REQ;
			res->accountId = m_pPlayer->GetAccountID();
			packet.SetPacketLen(sizeof(sCM_LOGOUT_REQ));
			app->SendTo(app->m_pMasterServerSession, &packet);
		}

		m_pPlayer->SetSession(NULL);
		m_pPlayer->SetSessionHandle(INVALID_HSESSION);

		g_PlrMgr->RemovePlayer(m_pPlayer);

		m_pPlayer = NULL;
	}
	else if (m_bLoginRequestSent)
	{
		// Edge case: CM_LOGIN_REQ was sent but player object was destroyed before login completed
		// This shouldn't normally happen (player should exist if CM_LOGIN_REQ was sent),
		// but handle it just in case. Master Server will handle timeout cleanup.
		ERR_LOG(LOG_USER, "[CharServer] Session closed with m_bLoginRequestSent=true but no player object - Master Server will handle timeout cleanup");
	}
}


int CClientSession::OnDispatch(CNtlPacket * pPacket)
{
	CCharServer * app = (CCharServer*)g_pApp;

	sNTLPACKETHEADER * pHeader = (sNTLPACKETHEADER *)pPacket->GetPacketData();

	OpcodeHandler<CClientSession> const* opHandle = uc_opcodeTable->LookupOpcode(pHeader->wOpCode);
	if (opHandle)
	{
		if (opHandle->packetProcessing == PROCESS_INPLACE)
			(this->*opHandle->handler)(pPacket);
		else
			app->PostClientPacketEvent(new TPacketEventObj<CClientSession>(this, opHandle->handler, GetHandle(), pPacket, GetUniqueHandle()));

		return NTL_SUCCESS;
	}

	return CNtlSession::OnDispatch(pPacket);
}

