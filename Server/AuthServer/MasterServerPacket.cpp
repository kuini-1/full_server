#include "stdafx.h"
#include <cstddef>
#include "NtlPacketMA.h"
#include "AuthServer.h"
#include "NtlPacketAU.h"
#include "NtlResultCode.h"
#include "NtlPacket.h"
#include "PacketWireLayout.h"

//--------------------------------------------------------------------------------------//
//		ADD SERVER INFO
//--------------------------------------------------------------------------------------//
void CMasterServerSession::RecvServersInfoAdd(CNtlPacket * pPacket, CAuthServer * app)
{
	UNREFERENCED_PARAMETER(app);
	sMA_SERVERS_INFO_ADD * req = (sMA_SERVERS_INFO_ADD*)pPacket->GetPacketData();

	printf("received server info. index %u port %u\n", req->serverInfo.byServerIndex, req->serverInfo.wPortForClient);

	g_pServerInfoManager->RefreshServerInfo(&req->serverInfo);
}

//--------------------------------------------------------------------------------------//
//		PLAYER ONLINE CHECK RESULT. IF OFFLINE THEN SEND LOGIN SUCCESS
//--------------------------------------------------------------------------------------//
void CMasterServerSession::RecvPlayerOnlineCheck(CNtlPacket * pPacket, CAuthServer * app)
{
	sMA_ON_PLAYER_CHECK_RES * req = (sMA_ON_PLAYER_CHECK_RES*)pPacket->GetPacketData();
	WORD resultcode = AUTH_USER_EXIST_IN_CHARACTER_SERVER;
	CClientSession* session = app->FindPlayer(req->accountId);
	if(session != NULL)
	{
		if(req->bIsOnline == false)
		{
			if(session != NULL)
			{
				resultcode = AUTH_SUCCESS;
			}
			else
			{
				resultcode = AUTH_USER_NOT_FOUND;
			}
		}

		if(resultcode == AUTH_SUCCESS)
		{
			sDBO_SERVER_INFO* srvinfo = g_pServerInfoManager->GetIdlestServerInfo(NTL_SERVER_TYPE_CHARACTER, 0, 0);

			if (srvinfo && srvinfo->dwLoad <= srvinfo->dwMaxLoad)
			{
				ERR_LOG(LOG_USER, "Account %u connect success to char server %u, dwLoad %u, dwMaxLoad %u", req->accountId, srvinfo->byServerIndex, srvinfo->dwLoad, srvinfo->dwMaxLoad);
				resultcode = AUTH_SUCCESS;
				srvinfo->dwLoad += 1;
			}
			else if (srvinfo)
				resultcode = CHARACTER_USER_SHOULD_WAIT_FOR_CONNECT;
			else
			{
				printf("[AuthServer] No character server registered in Auth; client will get login failure (AUTH_NO_AVAILABLE_CHARACTER_SERVER).\n");
				resultcode = AUTH_NO_AVAILABLE_CHARACTER_SERVER;
			}
		}

		if (resultcode == AUTH_SUCCESS)
		{
			sDBO_SERVER_INFO* srvinfo = g_pServerInfoManager->GetIdlestServerInfo(NTL_SERVER_TYPE_CHARACTER, 0, 0);
			CNtlPacket packet(sizeof(sAU_LOGIN_RES));
			sAU_LOGIN_RES* res = (sAU_LOGIN_RES*)packet.GetPacketData();
			memset(res, 0, sizeof(sAU_LOGIN_RES));
			res->wOpCode = AU_LOGIN_RES;
			res->wResultCode = AUTH_SUCCESS;
			memcpy(res->awchUserId, req->awchUserId, sizeof(res->awchUserId));
			memcpy(res->abyAuthKey, req->abyAuthKey, sizeof(res->abyAuthKey));
			res->accountId = req->accountId;
			res->lastServerFarmId = req->lastServerFarmId;
			res->dwAllowedFunctionForDeveloper = req->dwAllowedFunctionForDeveloper;
			res->bIsGM = req->bIsGM;
			res->byServerInfoCount = srvinfo ? 1 : 0;
			if (srvinfo && res->byServerInfoCount > 0)
			{
				NTL_STRCPY_S(res->aServerInfo[0].szCharacterServerIP, NTL_MAX_LENGTH_OF_IP + 1, srvinfo->achPublicAddress);
				res->aServerInfo[0].wCharacterServerPortForClient = srvinfo->wPortForClient;
				res->aServerInfo[0].dwLoad = (DWORD)((float)srvinfo->dwLoad / (float)(srvinfo->dwMaxLoad ? srvinfo->dwMaxLoad : 1) * 100.0f);
				res->aServerInfo[0].serverfarmID = srvinfo->serverFarmId;
				res->aServerInfo[0].serverchannelID = srvinfo->byServerChannelIndex;
			}
			packet.SetPacketLen(sizeof(sAU_LOGIN_RES));
			{
				BYTE* buf = packet.GetPacketBuffer();
				WORD len = packet.GetUsedSize();
				unsigned hdr = (unsigned)packet.GetHeaderSize();
				size_t paySize = (len > hdr) ? (size_t)(len - hdr) : 0;
#if !defined(_WIN32)
				unsigned int wirePayload = PacketWire_GetWirePayloadSize(AU_LOGIN_RES);
				printf("[AU_LOGIN_RES hex dump] PacketWire_GetWirePayloadSize=%u opcode=%u\n", wirePayload, (unsigned)AU_LOGIN_RES);
				if (wirePayload != 0)
				{
					unsigned int totalSent = (unsigned int)PACKET_HEADSIZE + wirePayload;
					printf("[AU_LOGIN_RES hex dump] actual sent to client: %u bytes total (wire payload %u)\n", totalSent, wirePayload);
					printf("[AU_LOGIN_RES hex dump] app buffer: total=%u (header=%u payload=%zu)\n", (unsigned)len, hdr, paySize);
				}
				else
#endif
				printf("[AU_LOGIN_RES hex dump] total=%u bytes (header=%u payload=%zu)\n", (unsigned)len, hdr, paySize);
				printf("[AU_LOGIN_RES hex dump] full packet bytes:");
				for (unsigned i = 0; i < len; i++)
					printf(" %02X", buf[i]);
				printf("\n");
				printf("[AU_LOGIN_RES hex dump] szCharacterServerIP='%s' Port=%u byServerInfoCount=%u\n",
					srvinfo ? srvinfo->achPublicAddress : "(none)", srvinfo ? srvinfo->wPortForClient : 0, (unsigned)res->byServerInfoCount);
				if (res->byServerInfoCount > 0 && srvinfo)
					printf("[AuthServer] Client should connect to character server at %s:%u\n", srvinfo->achPublicAddress, (unsigned)srvinfo->wPortForClient);
				printf("[AU_LOGIN_RES hex dump] awchUserId hex:");
				for (unsigned i = 0; i < sizeof(res->awchUserId) / sizeof(WCHAR) && i < 17; i++)
					printf(" %04X", (unsigned)res->awchUserId[i]);
				printf("\n");
			}
			// Sanity check: on this path we must always send AUTH_SUCCESS with at least one server when srvinfo is valid
			if (res->wResultCode != AUTH_SUCCESS)
			{
				ERR_LOG(LOG_USER, "RecvPlayerOnlineCheck: unexpected wResultCode %u (expected AUTH_SUCCESS=%u) for account %u, byServerInfoCount=%u",
					(unsigned)res->wResultCode, (unsigned)AUTH_SUCCESS, (unsigned)req->accountId, (unsigned)res->byServerInfoCount);
			}
			app->SendTo(session, &packet);

				CNtlPacket packet2(sizeof(sAU_COMMERCIAL_SETTING_NFY));
				sAU_COMMERCIAL_SETTING_NFY * res2 = (sAU_COMMERCIAL_SETTING_NFY *)packet2.GetPacketData();
				res2->wOpCode = AU_COMMERCIAL_SETTING_NFY;
				res2->abySetting[0] = 55;
				res2->abySetting[1] = 255;
				res2->abySetting[2] = 255;
				packet2.SetPacketLen(sizeof(sAU_COMMERCIAL_SETTING_NFY));
				app->SendTo(session, &packet2);

				GetAccDB.Execute("UPDATE accounts SET last_login=CURRENT_TIMESTAMP, last_ip='%s' WHERE AccountID = %u LIMIT 1", session->GetRemoteIP(), req->accountId);
				GetLogDB.Execute("INSERT INTO auth_login_log(AccountID, IP) VALUES (%u, '%s')", req->accountId, session->GetRemoteIP());

			return;
		}

		ERR_LOG(LOG_USER, "Account %u connect failed. Resultcode %d", req->accountId, resultcode);

		//IF NOT SUCCESS SEND ERROR MSG
		CNtlPacket packet2(sizeof(sAU_LOGIN_RES));
		sAU_LOGIN_RES* res2 = (sAU_LOGIN_RES*)packet2.GetPacketData();
		memset(res2, 0, sizeof(sAU_LOGIN_RES));
		res2->wOpCode = AU_LOGIN_RES;
		res2->wResultCode = resultcode;
		res2->byServerInfoCount = 0;
		packet2.SetPacketLen(sizeof(sAU_LOGIN_RES));
		app->SendTo(session, &packet2);

		app->DelPlayer(req->accountId);
	}
}

//--------------------------------------------------------------------------------------//
//		UPDATE CHAR SERVER
//--------------------------------------------------------------------------------------//
void CMasterServerSession::RecvServerInfoChangedNfy(CNtlPacket * pPacket, CAuthServer * app)
{
	UNREFERENCED_PARAMETER(app);
	sMA_SERVER_INFO_CHANGED_NFY * req = (sMA_SERVER_INFO_CHANGED_NFY*)pPacket->GetPacketData();
	
	sDBO_SERVER_INFO* pServerInfo = g_pServerInfoManager->GetServerInfo(req->byServerType, req->serverFarmId, req->serverChannelId, req->serverIndex);
	if (pServerInfo)
	{
		pServerInfo->dwLoad = req->dwLoad;
		pServerInfo->byRunningState = req->byRunningState;
		pServerInfo->bIsOn = req->bIsOn;
	}
	else
	{
		ERR_LOG(LOG_SYSTEM, "ServerType %u Index %u Farm %u Channel %u not found.", req->byServerType, req->serverIndex, req->serverFarmId, req->serverChannelId);
	}
}
