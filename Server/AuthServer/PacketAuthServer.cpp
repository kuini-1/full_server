#include "stdafx.h"
#include "Util/NtlPortable.h"
#include <stddef.h> // for offsetof

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
	NTL_PRINT(PRINT_APP, "[Login] SendCharLogInReq called (Session %u, IP %s, packet size %u)", GetHandle(), GetRemoteIP(), pPacket->GetUsedSize());
	
	// Original Windows code uses GetPacketData() - match original behavior
	sUA_LOGIN_REQ_TAIWAN_CT * req = (sUA_LOGIN_REQ_TAIWAN_CT *)pPacket->GetPacketData();
	NTL_PRINT(PRINT_APP, "[Login] Cast to struct complete, req=%p (Session %u)", req, GetHandle());
	
	// Fix memory leak: Ntl_WC2MB returns char* that must be freed with delete[]
	// Note: On Linux, wcstombs may fail if locale is not set or WCHAR data is invalid
	char* usernameMB = Ntl_WC2MB(req->awchUserId);
	if (usernameMB == NULL)
	{
		NTL_PRINT(PRINT_APP, "[Login] ERROR: Ntl_WC2MB(username) returned NULL - wcstombs conversion failed. First WCHAR bytes: 0x%04X 0x%04X (Session %u)", 
			req->awchUserId[0], req->awchUserId[1], GetHandle());
		return;
	}
	std::string username = std::string(usernameMB);
	delete[] usernameMB;
	
	char* password = Ntl_WC2MB(req->awchPasswd);
	if (password == NULL)
	{
		NTL_PRINT(PRINT_APP, "[Login] ERROR: Ntl_WC2MB(password) returned NULL - wcstombs conversion failed (Session %u)", GetHandle());
		return;
	}

	ERR_LOG(LOG_USER, "User %s request connection! req->wLVersion %i, req->wRVersion %i, state %hu, mac %hu\n", username.c_str(), (int)req->wLVersion, (int)req->wRVersion, req->byState, req->abyMacAddress[0]);
	NTL_PRINT(PRINT_APP, "[Login] User %s login request (Session %u, IP %s)", username.c_str(), GetHandle(), GetRemoteIP());
	
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
				snprintf(md5pwd, NTL_MAX_SIZE_USERPW_MULTIBYTE_BUFFER, "%s", md.digestString(password));

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

						// MasterServer must be connected for login success path (it sends AU_LOGIN_RES to client)
						if (app->m_pMasterServerSession == NULL)
						{
							resultcode = AUTH_NO_AVAILABLE_CHARACTER_SERVER;
							ERR_LOG(LOG_SYSTEM, "Login: MasterServer not connected. User %s will get failure response. Start MasterServer and connect Auth to it.", username.c_str());
							NTL_PRINT(PRINT_APP, "[Login] User %s: MasterServer not connected, sending failure (Session %u)", username.c_str(), GetHandle());
						}
						//check if acc already online
						else if (app->AddPlayer(this->AccountID, this) == true)
						{
							ERR_LOG(LOG_USER, "%s Auth Success. <Online Check>Sending packet to master server \n", username.c_str());
							NTL_PRINT(PRINT_APP, "[Login] User %s: Auth success, sending online check to MasterServer (Session %u, AccountID %u)", username.c_str(), GetHandle(), this->AccountID);

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

			if(resultcode == AUTH_WRONG_PASSWORD || resultcode == AUTH_USER_NOT_FOUND)
				++m_byLoginTrys;

			CNtlPacket packet(sizeof(sAU_LOGIN_RES));
			sAU_LOGIN_RES * res = (sAU_LOGIN_RES *)packet.GetPacketData();
			res->wOpCode = AU_LOGIN_RES;
			res->wResultCode = resultcode;
			packet.SetPacketLen(sizeof(sAU_LOGIN_RES));
			int sendRc = app->Send(GetHandle(), &packet);
			ERR_LOG(LOG_USER, "Login failed: sent AU_LOGIN_RES to client Session %u, resultcode %u, Send rc=%d", GetHandle(), resultcode, sendRc);
			NTL_PRINT(PRINT_APP, "[Login] User %s: Login failed, sent AU_LOGIN_RES to client (Session %u, resultcode %u, Send rc=%d)", username.c_str(), GetHandle(), resultcode, sendRc);

			if (m_byLoginTrys >= 5)
			{
				ERR_LOG(LOG_SYSTEM, "Session %u, IP %s, User %s too many login fails. IP blocked for 5 minutes \n", GetHandle(), GetRemoteIP(), username.c_str());

				//app->GetNetwork()->RegisterBlockedIp(GetRemoteAddr().GetAddr(), 270000);
				Disconnect(false);
			}
		}
//	}

	username.erase();
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
	app->SendTo(this, &packet);
}


//--------------------------------------------------------------------------------------//
//		Create user ( CHINA CLIENT )									
//--------------------------------------------------------------------------------------//
void CClientSession::SendCreateUserReq(CNtlPacket * pPacket, CAuthServer * app)
{
	// Struct inherits from sNTLPACKETHEADER, so use GetPacketBuffer() not GetPacketData()
	sUA_LOGIN_CREATEUSER_REQ * req = (sUA_LOGIN_CREATEUSER_REQ *)pPacket->GetPacketBuffer();

	CNtlPacket packet(sizeof(sAU_LOGIN_CREATEUSER_RES));
	sAU_LOGIN_CREATEUSER_RES * res = (sAU_LOGIN_CREATEUSER_RES *)packet.GetPacketData();
	res->wOpCode = AU_LOGIN_CREATEUSER_RES;
	res->wResultCode = AUTH_SUCCESS;
	NTL_WCSCPY_S(res->awchUserId, NTL_MAX_SIZE_USERID_UNICODE + 1, req->awchUserId);
	packet.SetPacketLen(sizeof(sAU_LOGIN_CREATEUSER_RES));
	app->SendTo(this, &packet);
}