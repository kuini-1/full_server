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
	WORD packetSize = pPacket->GetUsedSize();
	NTL_PRINT(PRINT_APP, "[Login] SendCharLogInReq called (Session %u, IP %s, packet size %u)", GetHandle(), GetRemoteIP(), packetSize);
	
	if (pPacket == NULL || pPacket->GetPacketData() == NULL)
	{
		NTL_PRINT(PRINT_APP, "[Login] ERROR: pPacket or GetPacketData() is NULL (Session %u)", GetHandle());
		return;
	}
	
	// Check packet size - sUA_LOGIN_REQ_TAIWAN_CT should be at least header + some data
	if (packetSize < sizeof(sNTLPACKETHEADER))
	{
		NTL_PRINT(PRINT_APP, "[Login] ERROR: Packet too small (%u < %u) (Session %u)", packetSize, (unsigned)sizeof(sNTLPACKETHEADER), GetHandle());
		return;
	}
	
	// sUA_LOGIN_REQ_TAIWAN_CT inherits from sNTLPACKETHEADER, so it includes the header.
	// GetPacketBuffer() returns the full packet (header + payload), GetPacketData() skips header.
	// So we must use GetPacketBuffer() to cast to the struct that includes header.
	sUA_LOGIN_REQ_TAIWAN_CT * req = (sUA_LOGIN_REQ_TAIWAN_CT *)pPacket->GetPacketBuffer();
	if (req == NULL)
	{
		NTL_PRINT(PRINT_APP, "[Login] ERROR: req pointer is NULL after cast (Session %u)", GetHandle());
		return;
	}
	
	// Struct sizeof() includes padding (157 bytes), but actual packet from client is 87 bytes.
	// Minimum needed: header (2) + username[17] (34) + password[17] (34) = 70 bytes to read username/password.
	// Packet is 87 bytes which is sufficient. Hardcode minimum as 70 bytes (actual packet size is 87).
	const WORD MIN_PACKET_SIZE = 70;  // Header (2) + username array (34) + password array (34) = 70 bytes minimum
	
	NTL_PRINT(PRINT_APP, "[Login] Packet buffer pointer valid, packet size %u, min needed %u, struct sizeof %u (Session %u)", 
		packetSize, MIN_PACKET_SIZE, (unsigned)sizeof(sUA_LOGIN_REQ_TAIWAN_CT), GetHandle());
	
	// Safety check: ensure packet is large enough to read username and password (minimum fields we need)
	if (packetSize < MIN_PACKET_SIZE)
	{
		NTL_PRINT(PRINT_APP, "[Login] ERROR: Packet too small (size %u < min %u) (Session %u)", 
			packetSize, MIN_PACKET_SIZE, GetHandle());
		return;
	}
	
	std::string username;
	char* password = NULL;
	
	// Try to read username - if it crashes here we'll see where
	NTL_PRINT(PRINT_APP, "[Login] About to call Ntl_WC2MB for username (Session %u)", GetHandle());
	char* usernameMB = Ntl_WC2MB(req->awchUserId);
	if (usernameMB == NULL)
	{
		NTL_PRINT(PRINT_APP, "[Login] ERROR: Ntl_WC2MB returned NULL for username (Session %u)", GetHandle());
		return;
	}
	username = std::string(usernameMB);
	delete[] usernameMB;
	NTL_PRINT(PRINT_APP, "[Login] Username converted: '%s' (Session %u)", username.c_str(), GetHandle());
	
	NTL_PRINT(PRINT_APP, "[Login] About to call Ntl_WC2MB for password (Session %u)", GetHandle());
	password = Ntl_WC2MB(req->awchPasswd);
	if (password == NULL)
	{
		NTL_PRINT(PRINT_APP, "[Login] ERROR: Ntl_WC2MB returned NULL for password (Session %u)", GetHandle());
		return;
	}
	NTL_PRINT(PRINT_APP, "[Login] Password converted (Session %u)", GetHandle());

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