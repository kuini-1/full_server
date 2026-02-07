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
		case UA_LOGIN_DISCONNECT_TW_REQ:	{	this->SendLoginDcReq(pPacket, app);	}	break;

		default: {	return CNtlSession::OnDispatch(pPacket);	}break;
	}

	return NTL_SUCCESS;
}