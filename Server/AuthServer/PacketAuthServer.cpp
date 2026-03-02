#include "stdafx.h"

#include "PacketAuthServer.h"
#include "AuthServer.h"
#include "md5.h"
#include "NtlService.h"
#include "NtlAdmin.h"


//--------------------------------------------------------------------------------------//
//		Get the account ID and log in to Char Server									//
//--------------------------------------------------------------------------------------//
void CClientSession::SendCharLogInReq(CNtlPacket * pPacket, CAuthServer * app)
{
	printf("[AuthServer] SendCharLogInReq entered, pPacket=%p\n", (void*)pPacket);
	fflush(stdout);
	if (!pPacket || !pPacket->GetPacketData())
	{
		WORD resultcode = AUTH_USER_NOT_FOUND;
		CNtlPacket packet(sizeof(sAU_LOGIN_RES));
		sAU_LOGIN_RES* res = (sAU_LOGIN_RES*)packet.GetPacketData();
		memset(res, 0, sizeof(sAU_LOGIN_RES));
		res->wOpCode = AU_LOGIN_RES;
		res->wResultCode = resultcode;
		res->byServerInfoCount = 0;
		packet.SetPacketLen(sizeof(sAU_LOGIN_RES));
		printf("[AuthServer] Sending AU_LOGIN_RES to client (early: null packet), resultCode=%u\n", (unsigned)resultcode);
		fflush(stdout);
		app->Send(GetHandle(), &packet);
		return;
	}

	sUA_LOGIN_REQ_TAIWAN_CT * req = (sUA_LOGIN_REQ_TAIWAN_CT *)pPacket->GetPacketData();

	char* username_c = Ntl_WC2MB(req->awchUserId);
	if (!username_c)
	{
		WORD resultcode = AUTH_USER_NOT_FOUND;
		CNtlPacket packet(sizeof(sAU_LOGIN_RES));
		sAU_LOGIN_RES* res = (sAU_LOGIN_RES*)packet.GetPacketData();
		memset(res, 0, sizeof(sAU_LOGIN_RES));
		res->wOpCode = AU_LOGIN_RES;
		res->wResultCode = resultcode;
		res->byServerInfoCount = 0;
		packet.SetPacketLen(sizeof(sAU_LOGIN_RES));
		printf("[AuthServer] Sending AU_LOGIN_RES to client (early: username convert fail), resultCode=%u\n", (unsigned)resultcode);
		fflush(stdout);
		app->Send(GetHandle(), &packet);
		return;
	}

	char* password = Ntl_WC2MB(req->awchPasswd);
	if (!password)
	{
		Ntl_CleanUpHeapString(username_c);
		WORD resultcode = AUTH_USER_NOT_FOUND;
		CNtlPacket packet(sizeof(sAU_LOGIN_RES));
		sAU_LOGIN_RES* res = (sAU_LOGIN_RES*)packet.GetPacketData();
		memset(res, 0, sizeof(sAU_LOGIN_RES));
		res->wOpCode = AU_LOGIN_RES;
		res->wResultCode = resultcode;
		res->byServerInfoCount = 0;
		packet.SetPacketLen(sizeof(sAU_LOGIN_RES));
		printf("[AuthServer] Sending AU_LOGIN_RES to client (early: password convert fail), resultCode=%u\n", (unsigned)resultcode);
		fflush(stdout);
		app->Send(GetHandle(), &packet);
		return;
	}

	std::string username(username_c);

	ERR_LOG(LOG_USER, "User %s request connection! req->wLVersion %i, req->wRVersion %i, state %hu, mac %hu\n", username.c_str(), (int)req->wLVersion, (int)req->wRVersion, req->byState, req->abyMacAddress[0]);
	
//	if((int)req->wRVersion == 40 && (int)req->wLVersion == 71) //only allow clients to connect with R version 40 and lversion 71
//	{
		WORD resultcode = AUTH_SUCCESS;
		
		sDBO_SERVER_INFO* pCharServer = g_pServerInfoManager->GetIdlestServerInfo(NTL_SERVER_TYPE_CHARACTER, 0, 0);
		if (pCharServer == NULL)
			resultcode = AUTH_NO_AVAILABLE_CHARACTER_SERVER;
		else if (!pCharServer->bIsOn)
			resultcode = AUTH_NO_AVAILABLE_CHARACTER_SERVER;
			//<Anti Hack>check username size
		else if (username.size() >= NTL_MAX_SIZE_USERID_UNICODE || username.size() < 3)
				resultcode = AUTH_TOO_LONG_ACCOUNT;
			//<Anti Hack>check password size
		else if (strlen(password) >= NTL_MAX_SIZE_USERPW_UNICODE || strlen(password) < 2)
				resultcode = AUTH_TOO_LONG_PASSWORD;
		else if(app->IsAccountTempBlocked(username))
			resultcode = AUTH_USER_TEMPORARY_BLOCK;

		if (resultcode == AUTH_SUCCESS)
		{
			smart_ptr<QueryResult> result = GetAccDB.Query("SELECT AccountID,Password_hash,acc_status,isGm,lastServerFarmId,founder FROM accounts WHERE Username = \"%s\" LIMIT 1", GetAccDB.EscapeString(username).c_str());
			if (result)
			{
				MD5 md;
				char md5pwd[NTL_MAX_SIZE_USERPW_MULTIBYTE_BUFFER];
				NTL_STRCPY_S(md5pwd, NTL_MAX_SIZE_USERPW_MULTIBYTE_BUFFER, md.digestString(password));

				Field* fields = result->Fetch();

				if (0 != NTL_STRICMP(fields[1].GetString(), md5pwd)) //check password
					resultcode = AUTH_WRONG_PASSWORD;
				else
				{
					bool isGm = (fields[3].GetBYTE() > ADMIN_LEVEL_EARLY_ACCESS);
					std::string accstatus = fields[2].GetString();

					if (app->GetDisableConnection() == TRUE) //check if we disable connection
					{
						if (app->GetFounderConnection() == TRUE) //check if we allow founders to connect
						{
							if (fields[5].GetINT() == 0 && isGm == false) //if not founder and not gm then dont allow to connect
								resultcode = AUTH_SERVER_LOCKED;
						}
						else
						{
							if (isGm == false)
								resultcode = AUTH_SERVER_LOCKED;
						}
					}

					if (/*accstatus == "pending" ||*/ accstatus == "block") // pending = need email activate | block = account banned
						resultcode = AUTH_USER_BLOCK;
					else if (pCharServer->dwLoad >= pCharServer->dwMaxLoad && !isGm)
						resultcode = CHARACTER_USER_SHOULD_WAIT_FOR_CONNECT;

					if (resultcode == AUTH_SUCCESS)
					{
						this->AccountID = fields[0].GetUInt32();

						//check if acc already online
						if (app->AddPlayer(this->AccountID, this) == true)
						{
							ERR_LOG(LOG_USER, "%s Auth Success. <Online Check>Sending packet to master server \n", username.c_str());
							printf("[AuthServer] Sending AM_ON_PLAYER_CHECK_REQ to Master, waiting for MA_ON_PLAYER_CHECK_RES\n");
							fflush(stdout);

							//send check req if player online to master server
							CNtlPacket packet(sizeof(sAM_ON_PLAYER_CHECK_REQ));
							sAM_ON_PLAYER_CHECK_REQ * res = (sAM_ON_PLAYER_CHECK_REQ *)packet.GetPacketData();
							res->wOpCode = AM_ON_PLAYER_CHECK_REQ;
							res->accountId = this->AccountID;
							NTL_WCSCPY_S(res->awchUserId, NTL_MAX_SIZE_USERID_UNICODE + 1, req->awchUserId);
							res->bIsGM = isGm;
							res->dwAllowedFunctionForDeveloper = DBO_ALLOWED_FUNC_FOR_DEV_FLAG_HUMAN + DBO_ALLOWED_FUNC_FOR_DEV_FLAG_NAMEK + DBO_ALLOWED_FUNC_FOR_DEV_FLAG_MAJIN;
							res->lastServerFarmId = fields[4].GetBYTE();
							packet.SetPacketLen(sizeof(sAM_ON_PLAYER_CHECK_REQ));
							app->SendTo(app->m_pMasterServerSession, &packet);
						}
						else
						{
							resultcode = AUTH_USER_EXIST_IN_CHARACTER_SERVER;
							app->DelPlayer(this->AccountID);
						}
					}
				}
			}
			else
				resultcode = AUTH_USER_NOT_FOUND;
		}

		if(resultcode != AUTH_SUCCESS)
		{
			ERR_LOG(LOG_SYSTEM, "Session %u, Login trys %u, User %s Connection failed. Resultcode %u \n", GetHandle(), m_byLoginTrys, username.c_str(), resultcode);
			printf("[AuthServer] Sending AU_LOGIN_RES to client (failure path), resultCode=%u\n", (unsigned)resultcode);
			fflush(stdout);

			if(resultcode == AUTH_WRONG_PASSWORD || resultcode == AUTH_USER_NOT_FOUND)
				++m_byLoginTrys;

			CNtlPacket packet(sizeof(sAU_LOGIN_RES));
			sAU_LOGIN_RES* res = (sAU_LOGIN_RES*)packet.GetPacketData();
			memset(res, 0, sizeof(sAU_LOGIN_RES));
			res->wOpCode = AU_LOGIN_RES;
			res->wResultCode = resultcode;
			res->byServerInfoCount = 0;
			packet.SetPacketLen(sizeof(sAU_LOGIN_RES));
			app->Send(GetHandle(), &packet);

			if (m_byLoginTrys >= 5)
			{
				ERR_LOG(LOG_SYSTEM, "Session %u, IP %s, User %s too many login fails. IP blocked for 5 minutes \n", GetHandle(), GetRemoteIP(), username.c_str());

				//app->GetNetwork()->RegisterBlockedIp(GetRemoteAddr().GetAddr(), 270000);
				Disconnect(false);
			}
		}
//	}

	Ntl_CleanUpHeapString(username_c);
	Ntl_CleanUpHeapString(password);
}

//--------------------------------------------------------------------------------------//
//		Disconnect from Auth Server
//--------------------------------------------------------------------------------------//
void CClientSession::SendLoginDcReq(CNtlPacket * pPacket, CAuthServer * app) 
{
	UNREFERENCED_PARAMETER(pPacket);

	CNtlPacket packet(sizeof(sAU_LOGIN_DISCONNECT_RES));
	sAU_LOGIN_DISCONNECT_RES * res = (sAU_LOGIN_DISCONNECT_RES *)packet.GetPacketData();
	res->wOpCode = AU_LOGIN_DISCONNECT_RES;
	packet.SetPacketLen(sizeof(sAU_LOGIN_DISCONNECT_RES));
	printf("[AuthServer] Sending AU_LOGIN_DISCONNECT_RES to client; client should connect to Char server next (IP/port were in AU_LOGIN_RES).\n");
	app->SendTo(this, &packet);
}


//--------------------------------------------------------------------------------------//
//		Create user ( CHINA CLIENT )									
//--------------------------------------------------------------------------------------//
void CClientSession::SendCreateUserReq(CNtlPacket * pPacket, CAuthServer * app)
{
	sUA_LOGIN_CREATEUSER_REQ * req = (sUA_LOGIN_CREATEUSER_REQ *)pPacket->GetPacketData();

	CNtlPacket packet(sizeof(sAU_LOGIN_CREATEUSER_RES));
	sAU_LOGIN_CREATEUSER_RES * res = (sAU_LOGIN_CREATEUSER_RES *)packet.GetPacketData();
	res->wOpCode = AU_LOGIN_CREATEUSER_RES;
	res->wResultCode = AUTH_SUCCESS;
	NTL_WCSCPY_S(res->awchUserId, NTL_MAX_SIZE_USERID_UNICODE + 1, req->awchUserId);
	packet.SetPacketLen(sizeof(sAU_LOGIN_CREATEUSER_RES));
	app->SendTo(this, &packet);
}
