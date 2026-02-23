#include "stdafx.h"
#include <cstddef>
#include "NtlPacketMA.h"
#include "AuthServer.h"
#include "NtlPacketAU.h"
#include "NtlResultCode.h"

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
	WORD payLen = pPacket->GetUsedSize() - (WORD)pPacket->GetHeaderSize();
	if (payLen < (WORD)sizeof(sMA_ON_PLAYER_CHECK_RES))
	{
		printf("[MA_ON_PLAYER_CHECK_RES] packet too small: payload=%u, need %zu - skip\n", (unsigned)payLen, sizeof(sMA_ON_PLAYER_CHECK_RES));
		return;
	}
	sMA_ON_PLAYER_CHECK_RES * req = (sMA_ON_PLAYER_CHECK_RES*)pPacket->GetPacketData();
	WORD resultcode = AUTH_USER_EXIST_IN_CHARACTER_SERVER;
	ACCOUNTID accountId = req->accountId;
	bool bIsOnline = req->bIsOnline;
	CClientSession* session = app->FindPlayer(accountId);
	if(session != NULL)
	{
		if(bIsOnline == false)
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

			CNtlPacket packet(sizeof(sAU_LOGIN_RES));
			sAU_LOGIN_RES * res = (sAU_LOGIN_RES *)packet.GetPacketData();
			res->wOpCode = AU_LOGIN_RES;
			NTL_SAFE_WCSCPY(res->awchUserId, req->awchUserId);

			memcpy(res->abyAuthKey, req->abyAuthKey, sizeof(res->abyAuthKey));
			res->dwAllowedFunctionForDeveloper = req->dwAllowedFunctionForDeveloper;
			res->accountId = accountId;
			if (srvinfo)
			{
				DWORD dwLoad = srvinfo->dwLoad;
				DWORD dwMaxLoad = srvinfo->dwMaxLoad;
				if (dwMaxLoad == 0)
					dwMaxLoad = 1;
				if (dwLoad <= dwMaxLoad)
				{
					ERR_LOG(LOG_USER, "Account %u connect success to char server %u, dwLoad %u, dwMaxLoad %u", accountId, srvinfo->byServerIndex, dwLoad, dwMaxLoad);

					resultcode = AUTH_SUCCESS;
					NTL_STRCPY_S(res->aServerInfo[0].szCharacterServerIP, NTL_MAX_LENGTH_OF_IP + 1, srvinfo->achPublicAddress);
					res->aServerInfo[0].wCharacterServerPortForClient = srvinfo->wPortForClient;
					res->aServerInfo[0].dwLoad = (DWORD)((float)dwLoad / (float)dwMaxLoad * 100.0f);
					res->aServerInfo[0].serverfarmID = srvinfo->serverFarmId;
					res->aServerInfo[0].serverchannelID = srvinfo->byServerChannelIndex;
					res->byServerInfoCount = 1;

					srvinfo->dwLoad += 1;
				}
				else resultcode = CHARACTER_USER_SHOULD_WAIT_FOR_CONNECT;
			}
			else resultcode = AUTH_NO_AVAILABLE_CHARACTER_SERVER;

			res->wResultCode = resultcode;
			res->lastServerFarmId = req->lastServerFarmId;
			res->bIsGM = req->bIsGM;

			if (resultcode == AUTH_SUCCESS)
			{
				packet.SetPacketLen(sizeof(sAU_LOGIN_RES));
				printf("[AU_LOGIN_RES] sizeof(sAU_LOGIN_RES)=%zu (expect 795 for client)\n", (size_t)sizeof(sAU_LOGIN_RES));
				{
					BYTE * buf = packet.GetPacketBuffer();
					WORD len = packet.GetUsedSize();
					sAU_LOGIN_RES * dbg = (sAU_LOGIN_RES *)packet.GetPacketData();
					unsigned hdr = (unsigned)packet.GetHeaderSize();
					unsigned pay = (unsigned)(len - hdr);
					printf("[AU_LOGIN_RES hex dump] total=%u bytes (header=%u payload=%u)\n", (unsigned)len, hdr, pay);
					printf("[AU_LOGIN_RES hex dump] szCharacterServerIP='%s' Port=%u byServerInfoCount=%u\n",
						dbg->aServerInfo[0].szCharacterServerIP, dbg->aServerInfo[0].wCharacterServerPortForClient, dbg->byServerInfoCount);
					printf("[AU_LOGIN_RES hex dump] offsetof aServerInfo=%zu sizeof(sSERVER_INFO)=%zu\n",
						offsetof(sAU_LOGIN_RES, aServerInfo), sizeof(sSERVER_INFO));
					printf("[AU_LOGIN_RES hex dump] raw hex first 140 (header+payload start):");
					for (unsigned i = 0; i < 140 && i < len; i++)
						printf(" %02x", buf[i]);
					printf("\n[AU_LOGIN_RES hex dump] raw hex bytes %zu-%zu (first sSERVER_INFO, client expects ~54-127):",
						(unsigned)(hdr + offsetof(sAU_LOGIN_RES, aServerInfo)), (unsigned)(hdr + offsetof(sAU_LOGIN_RES, aServerInfo) + sizeof(sSERVER_INFO)));
					unsigned start = hdr + (unsigned)offsetof(sAU_LOGIN_RES, aServerInfo);
					for (unsigned i = start; i < start + sizeof(sSERVER_INFO) && i < len; i++)
						printf(" %02x", buf[i]);
					printf("\n");
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

				GetAccDB.Execute("UPDATE accounts SET last_login=CURRENT_TIMESTAMP, last_ip='%s' WHERE AccountID = %u LIMIT 1", session->GetRemoteIP(), accountId);
				GetLogDB.Execute("INSERT INTO auth_login_log(AccountID, IP) VALUES (%u, '%s')", accountId, session->GetRemoteIP());

				return;
			}
		}

		ERR_LOG(LOG_USER, "Account %u connect failed. Resultcode %d", accountId, resultcode);

		//IF NOT SUCCESS SEND ERROR MSG
		CNtlPacket packet2(sizeof(sAU_LOGIN_RES));
		sAU_LOGIN_RES * res2 = (sAU_LOGIN_RES *)packet2.GetPacketData();
		memset(packet2.GetPacketData(), 0, sizeof(sAU_LOGIN_RES));
		res2->wOpCode = AU_LOGIN_RES;
		res2->wResultCode = resultcode;
		packet2.SetPacketLen(sizeof(sAU_LOGIN_RES));
		app->SendTo(session, &packet2);

		app->DelPlayer(accountId);
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
