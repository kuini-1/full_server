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

	NTL_PRINT(PRINT_APP, "[ClientSession] Client connected! Session: %u, IP: %s, Port: %u", GetHandle(), GetRemoteIP(), GetRemotePort());

	//start handshake (with client)
	unsigned char buf[] = { 0x03, 0x00, 0xac, 0x86, 0xf5, 0x74 };
	CNtlPacket packet(buf, 0x06);
	g_pApp->Send(GetHandle(), &packet);

	NTL_PRINT(PRINT_APP, "[ClientSession] Sent handshake packet to Session: %u", GetHandle());

	return CNtlSession::OnAccept();
}

void CClientSession::OnClose()
{
	CAuthServer * app = (CAuthServer*) NtlSfxGetApp();

	NTL_PRINT(PRINT_APP, "[ClientSession] Client disconnected! Session: %u, IP: %s, AccountID: %u", GetHandle(), GetRemoteIP(), this->AccountID ? this->AccountID : 0);

	if(this->AccountID != NULL)
		app->DelPlayer(this->AccountID);
}

int CClientSession::ProcessPacket()
{
	NTL_PRINT(PRINT_APP, "[ClientSession] ProcessPacket called for Session=%u, IP=%s", GetHandle(), GetRemoteIP());
	
	// Call base class to handle packet processing
	int rc = CNtlSession::ProcessPacket();
	
	if (rc != NTL_SUCCESS)
	{
		NTL_PRINT(PRINT_APP, "[ClientSession] ProcessPacket returned error: %d for Session=%u, IP=%s", rc, GetHandle(), GetRemoteIP());
	}
	
	return rc;
}

int CClientSession::OnDispatch(CNtlPacket * pPacket)
{
	CAuthServer * app = (CAuthServer*)NtlSfxGetApp();

	sNTLPACKETHEADER * pHeader = (sNTLPACKETHEADER *)pPacket->GetPacketData();
	// Skip logging OpCode 0x0001 (heartbeat/ping) to reduce log spam; log all other opcodes
	if (pHeader->wOpCode != 0x0001)
		NTL_PRINT(PRINT_APP, "[ClientSession] Received packet OpCode: 0x%04X (Session: %u, IP: %s)", pHeader->wOpCode, GetHandle(), GetRemoteIP());
	switch( pHeader->wOpCode )
	{
		case UA_LOGIN_REQ:
		case UA_LOGIN_REQ_TAIWAN_CT:	{	this->SendCharLogInReq(pPacket, app);	}	break;
		case UA_LOGIN_CREATEUSER_REQ:	{	this->SendCreateUserReq(pPacket, app);	}	break;
		case UA_LOGIN_DISCONNECT_CN_REQ:
		case UA_LOGIN_DISCONNECT_TW_REQ:	{	this->SendLoginDcReq(pPacket, app);	}	break;

		default: {	return CNtlSession::OnDispatch(pPacket);	}break;
	}

	return NTL_SUCCESS;
}