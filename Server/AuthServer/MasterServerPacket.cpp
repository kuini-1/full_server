#include "stdafx.h"
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
	sMA_ON_PLAYER_CHECK_RES * req = (sMA_ON_PLAYER_CHECK_RES*)pPacket->GetPacketData();
	NTL_PRINT(PRINT_APP, "[Login] RecvPlayerOnlineCheck: Received MA_ON_PLAYER_CHECK_RES for AccountID %u, bIsOnline=%d", req->accountId, req->bIsOnline ? 1 : 0);
	
	WORD resultcode = AUTH_USER_EXIST_IN_CHARACTER_SERVER;
	CClientSession* session = app->FindPlayer(req->accountId);
	if(session != NULL)
	{
		NTL_PRINT(PRINT_APP, "[Login] RecvPlayerOnlineCheck: Found session for AccountID %u", req->accountId);
		if(req->bIsOnline == false)
		{
			if(session != NULL)
			{
				resultcode = AUTH_SUCCESS;
				NTL_PRINT(PRINT_APP, "[Login] RecvPlayerOnlineCheck: Player is offline, proceeding with login");
			}
			else
			{
				resultcode = AUTH_USER_NOT_FOUND;
				NTL_PRINT(PRINT_APP, "[Login] RecvPlayerOnlineCheck: ERROR - session is NULL");
			}
		}
		else
		{
			NTL_PRINT(PRINT_APP, "[Login] RecvPlayerOnlineCheck: Player is already online, login failed");
		}

		if(resultcode == AUTH_SUCCESS)
		{
			sDBO_SERVER_INFO* srvinfo = g_pServerInfoManager->GetIdlestServerInfo(NTL_SERVER_TYPE_CHARACTER, 0, 0);
			NTL_PRINT(PRINT_APP, "[Login] RecvPlayerOnlineCheck: Looking for Character Server, srvinfo=%p", srvinfo);

			CNtlPacket packet(sizeof(sAU_LOGIN_RES));
			sAU_LOGIN_RES * res = (sAU_LOGIN_RES *)packet.GetPacketData();
			res->wOpCode = AU_LOGIN_RES;
			NTL_SAFE_WCSCPY(res->awchUserId, req->awchUserId);

			memcpy(res->abyAuthKey, req->abyAuthKey, sizeof(res->abyAuthKey));
			res->dwAllowedFunctionForDeveloper = req->dwAllowedFunctionForDeveloper;
			res->accountId = req->accountId;
			if (srvinfo)
			{
				NTL_PRINT(PRINT_APP, "[Login] RecvPlayerOnlineCheck: Found Character Server: Index=%u, IP=%s, Port=%u, Load=%u/%u, IsOn=%d", 
					srvinfo->byServerIndex, srvinfo->achPublicAddress, srvinfo->wPortForClient, 
					srvinfo->dwLoad, srvinfo->dwMaxLoad, srvinfo->bIsOn ? 1 : 0);
				
				if (srvinfo->bIsOn == false)
				{
					NTL_PRINT(PRINT_APP, "[Login] RecvPlayerOnlineCheck: Character Server is OFF, cannot connect");
					resultcode = AUTH_NO_AVAILABLE_CHARACTER_SERVER;
				}
				else if (srvinfo->dwLoad <= srvinfo->dwMaxLoad)
				{
					ERR_LOG(LOG_USER, "Account %u connect success to char server %u, dwLoad %u, dwMaxLoad %u", req->accountId, srvinfo->byServerIndex, srvinfo->dwLoad, srvinfo->dwMaxLoad);
					NTL_PRINT(PRINT_APP, "[Login] RecvPlayerOnlineCheck: Character Server available, preparing login response");

					resultcode = AUTH_SUCCESS;
					snprintf(res->aServerInfo[0].szCharacterServerIP, NTL_MAX_LENGTH_OF_IP + 1, "%s", srvinfo->achPublicAddress);
					res->aServerInfo[0].wCharacterServerPortForClient = srvinfo->wPortForClient;
					res->aServerInfo[0].dwLoad = (DWORD)((float)srvinfo->dwLoad / (float)srvinfo->dwMaxLoad * 100.0f);
					res->aServerInfo[0].serverfarmID = srvinfo->serverFarmId;
					res->aServerInfo[0].serverchannelID = srvinfo->byServerChannelIndex;
					res->byServerInfoCount = 1;

					NTL_PRINT(PRINT_APP, "[Login] RecvPlayerOnlineCheck: Sending Character Server info to client: IP=%s, Port=%u, FarmID=%u, ChannelID=%u", 
						res->aServerInfo[0].szCharacterServerIP, res->aServerInfo[0].wCharacterServerPortForClient,
						res->aServerInfo[0].serverfarmID, res->aServerInfo[0].serverchannelID);

					//update load
					srvinfo->dwLoad += 1;
				}
				else 
				{
					NTL_PRINT(PRINT_APP, "[Login] RecvPlayerOnlineCheck: Character Server is full: Load=%u, MaxLoad=%u", srvinfo->dwLoad, srvinfo->dwMaxLoad);
					resultcode = CHARACTER_USER_SHOULD_WAIT_FOR_CONNECT;
				}
			}
			else 
			{
				NTL_PRINT(PRINT_APP, "[Login] RecvPlayerOnlineCheck: ERROR - No Character Server found!");
				resultcode = AUTH_NO_AVAILABLE_CHARACTER_SERVER;
			}

			res->wResultCode = resultcode;
			res->lastServerFarmId = req->lastServerFarmId;
			res->bIsGM = req->bIsGM;

			if (resultcode == AUTH_SUCCESS)
			{
				packet.SetPacketLen(sizeof(sAU_LOGIN_RES));
				int rc = app->SendTo(session, &packet);
				ERR_LOG(LOG_USER, "Login success: sent AU_LOGIN_RES to client Session %u, Account %u, SendTo rc=%d", session->GetHandle(), req->accountId, rc);
				NTL_PRINT(PRINT_APP, "[Login] Login success: sent AU_LOGIN_RES to client (Session %u, Account %u, SendTo rc=%d, ResultCode=%u)", session->GetHandle(), req->accountId, rc, resultcode);
				NTL_PRINT(PRINT_APP, "[Login] Login response details: Character Server IP=%s, Port=%u, ServerCount=%u", 
					res->aServerInfo[0].szCharacterServerIP, res->aServerInfo[0].wCharacterServerPortForClient, res->byServerInfoCount);

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
		}

		ERR_LOG(LOG_USER, "Account %u connect failed. Resultcode %d", req->accountId, resultcode);
		NTL_PRINT(PRINT_APP, "[Login] RecvPlayerOnlineCheck: Login failed for AccountID %u, resultcode=%u", req->accountId, resultcode);

		//IF NOT SUCCESS SEND ERROR MSG
		CNtlPacket packet2(sizeof(sAU_LOGIN_RES));
		sAU_LOGIN_RES * res2 = (sAU_LOGIN_RES *)packet2.GetPacketData();
		res2->wOpCode = AU_LOGIN_RES;
		res2->wResultCode = resultcode;
		packet2.SetPacketLen(sizeof(sAU_LOGIN_RES));
		int rc2 = app->SendTo(session, &packet2);
		ERR_LOG(LOG_USER, "Login failed (from master check): sent AU_LOGIN_RES to client Session %u, resultcode %d, SendTo rc=%d", session->GetHandle(), resultcode, rc2);
		NTL_PRINT(PRINT_APP, "[Login] Login failed (from master check): sent AU_LOGIN_RES to client (Session %u, resultcode %d, SendTo rc=%d)", session->GetHandle(), resultcode, rc2);

		app->DelPlayer(req->accountId);
	}
	else
	{
		NTL_PRINT(PRINT_APP, "[Login] RecvPlayerOnlineCheck: ERROR - Session not found for AccountID %u", req->accountId);
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