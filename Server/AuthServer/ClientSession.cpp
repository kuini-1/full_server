#include "stdafx.h"
#include "ClientSession.h"
#include "AuthServer.h"

#include "NtlPacketUA.h"
#include "NtlPacketAU.h"
#include "NtlResultCode.h"


CClientSession::~CClientSession()
{
}


int CClientSession::OnAccept()
{
	this->AccountID = NULL;
	m_byLoginTrys = 0;

	//start handshake (with client)
	unsigned char buf[] = { 0x03, 0x00, 0xac, 0x86, 0xf5, 0x74 };
	CNtlPacket packet(buf, 0x06);
	g_pApp->Send(GetHandle(), &packet);

	return CNtlSession::OnAccept();
}

void CClientSession::OnClose()
{
	CAuthServer * app = (CAuthServer*) NtlSfxGetApp();

	if(this->AccountID != NULL)
		app->DelPlayer(this->AccountID);
}

int CClientSession::ProcessPacket()
{
	return CNtlSession::ProcessPacket();
}

int CClientSession::OnDispatch(CNtlPacket * pPacket)
{
	CAuthServer * app = (CAuthServer*)NtlSfxGetApp();

	sNTLPACKETHEADER * pHeader = (sNTLPACKETHEADER *)pPacket->GetPacketData();

	switch( pHeader->wOpCode )
	{
		case UA_LOGIN_REQ:
		case UA_LOGIN_REQ_TAIWAN_CT:	{	this->SendCharLogInReq(pPacket, app);	}	break;
		case UA_LOGIN_CREATEUSER_REQ:	{	this->SendCreateUserReq(pPacket, app);	}	break;
		case UA_LOGIN_DISCONNECT_CN_REQ:
		case UA_LOGIN_DISCONNECT_TW_REQ:
		{
			const char* reqName = (pHeader->wOpCode == UA_LOGIN_DISCONNECT_CN_REQ) ? "UA_LOGIN_DISCONNECT_CN_REQ" : "UA_LOGIN_DISCONNECT_TW_REQ";
			printf("[AuthServer] Received %s (opcode=%u, size=%u): client leaving Auth to connect to Char server. AccountID=%u, IP=%s\n",
				reqName, (unsigned)pHeader->wOpCode, pPacket->GetPacketLen(), this->AccountID ? this->AccountID : 0, GetRemoteIP());
			this->SendLoginDcReq(pPacket, app);
		}	break;

		default: 
		{
			// System packets (SYS_ALIVE=1, SYS_AUTH_RES=4) are handled by base class, not errors
			// Log only Character Server packets (UC_* opcodes 2000-2999) which indicate client didn't disconnect properly
			if (pHeader->wOpCode >= 2000 && pHeader->wOpCode < 3000)
			{
				printf("[AuthServer] ERROR: Received Character Server packet opcode %u (size %u) from client AccountID=%u, IP=%s. Client should have disconnected after AU_LOGIN_RES.\n", 
					pHeader->wOpCode, pPacket->GetPacketLen(), this->AccountID ? this->AccountID : 0, GetRemoteIP());
			}
			return CNtlSession::OnDispatch(pPacket);
		}break;
	}

	return NTL_SUCCESS;
}
