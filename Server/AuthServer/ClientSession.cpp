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
	CNtlPacket packet;
	if (PopPacket(&packet))
	{
		int nPacketLen = GetPacketLen((BYTE*)(packet.GetPacketHeader()));
		sNTLPACKETHEADER * pHeader = (sNTLPACKETHEADER *)packet.GetPacketData();
		
		NTL_PRINT(PRINT_APP, "[ClientSession] ProcessPacket: OpCode=0x%04X, Len=%d, Session=%u, IP=%s", 
			pHeader ? pHeader->wOpCode : 0, nPacketLen, GetHandle(), GetRemoteIP());

		if(m_pPacketEncoder && m_pPacketEncoder->RxDecrypt(packet, GetPacketRecvCount() & PACKET_MAX_SEQUENCE) != NTL_SUCCESS)
		{
			NTL_PRINT(PRINT_APP, "[ClientSession] Decryption failed! Session=%u, IP=%s, FailureCount=%d", 
				GetHandle(), GetRemoteIP(), m_nDecryptionFailureCount);
			
			if (ALLOWED_DECRYPTION_FAILURE_COUNT < ++m_nDecryptionFailureCount)
			{
				m_pNetworkRef->RegisterBlockedIp(GetRemoteAddr().GetAddr()); 
				SetStatus(STATUS_CLOSE);

				if (CheckDisconnect(false))
				{
					ERR_LOG(LOG_NETWORK,"Session[%X] : Diconnecting due to too many decryption failure. Local Port[%u], Remote IP[%s]", this, GetLocalPort(), GetRemoteIP());
				}

				m_nDecryptionFailureCount = 0;
			}
		}
		else
		{
			IncreasePacketRecv();
			int rc = OnDispatch(&packet);
			GetRecvBuffer()->IncreasePopPos(GetHeaderSize() + nPacketLen);
			return rc;
		}

		GetRecvBuffer()->IncreasePopPos(GetHeaderSize() + nPacketLen);
	}
	else
	{
		NTL_PRINT(PRINT_APP, "[ClientSession] PopPacket failed! Session=%u, IP=%s - disconnecting", GetHandle(), GetRemoteIP());
		ERR_LOG(LOG_NETWORK, "Session[%X] : Diconnecting due to sending invalid packet. Local Port[%u], Remote IP[%s]", this, GetLocalPort(), GetRemoteIP());
		Disconnect(false);
		return NTL_ERR_NET_PACKET_INVALID;
	}

	return NTL_SUCCESS;
}

int CClientSession::OnDispatch(CNtlPacket * pPacket)
{
	CAuthServer * app = (CAuthServer*)NtlSfxGetApp();

	sNTLPACKETHEADER * pHeader = (sNTLPACKETHEADER *)pPacket->GetPacketData();
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