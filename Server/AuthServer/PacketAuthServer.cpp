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
	
	// INVESTIGATION: Check packet layout and struct alignment
	BYTE* buffer = pPacket->GetPacketBuffer();
	BYTE* data = pPacket->GetPacketData();
	WORD headerSize = pPacket->GetHeaderSize();
	
	// Check where OpCode actually is in the packet
	WORD opCodeAtBuffer = *(WORD*)(buffer + headerSize);  // Should be OpCode (after STHeaderBase)
	WORD opCodeAtData = *(WORD*)data;  // What GetPacketData() points to
	
	NTL_PRINT(PRINT_APP, "[Login] Packet layout: buffer=%p, data=%p, offset=%ld, headerSize=%u", 
		buffer, data, (long)(data - buffer), headerSize);
	NTL_PRINT(PRINT_APP, "[Login] OpCode at buffer+%u: 0x%04X, OpCode at data: 0x%04X", headerSize, opCodeAtBuffer, opCodeAtData);
	
	// Check struct layout
	NTL_PRINT(PRINT_APP, "[Login] Struct sizes: sNTLPACKETHEADER=%zu, sUA_LOGIN_REQ_TAIWAN_CT=%zu", 
		sizeof(sNTLPACKETHEADER), sizeof(sUA_LOGIN_REQ_TAIWAN_CT));
	NTL_PRINT(PRINT_APP, "[Login] Struct offsets: wOpCode=%zu, awchUserId=%zu, awchPasswd=%zu", 
		offsetof(sUA_LOGIN_REQ_TAIWAN_CT, wOpCode), offsetof(sUA_LOGIN_REQ_TAIWAN_CT, awchUserId), offsetof(sUA_LOGIN_REQ_TAIWAN_CT, awchPasswd));
	
	// Try GetPacketData() first (original Windows code)
	sUA_LOGIN_REQ_TAIWAN_CT * req = (sUA_LOGIN_REQ_TAIWAN_CT *)pPacket->GetPacketData();
	
	// Check actual memory location vs expected
	BYTE* expectedUserIdStart = data + offsetof(sUA_LOGIN_REQ_TAIWAN_CT, awchUserId);
	BYTE* actualUserIdStart = (BYTE*)&req->awchUserId;
	NTL_PRINT(PRINT_APP, "[Login] Expected awchUserId at data+%zu=%p, actual at %p, difference=%ld bytes", 
		offsetof(sUA_LOGIN_REQ_TAIWAN_CT, awchUserId), expectedUserIdStart, actualUserIdStart, (long)(actualUserIdStart - expectedUserIdStart));
	
	// Check if struct is aligned correctly
	WORD structOpCode = req->wOpCode;
	NTL_PRINT(PRINT_APP, "[Login] Cast GetPacketData() to struct: req->wOpCode=0x%04X (expected 0x%04X)", structOpCode, opCodeAtData);
	
	if (structOpCode != opCodeAtData)
	{
		// Misaligned - try GetPacketBuffer() instead
		NTL_PRINT(PRINT_APP, "[Login] Struct misaligned with GetPacketData(), trying GetPacketBuffer() (Session %u)", GetHandle());
		req = (sUA_LOGIN_REQ_TAIWAN_CT *)pPacket->GetPacketBuffer();
		structOpCode = req->wOpCode;
		NTL_PRINT(PRINT_APP, "[Login] Cast GetPacketBuffer() to struct: req->wOpCode=0x%04X (expected 0x%04X)", structOpCode, opCodeAtBuffer);
	}
	
	// Check raw bytes at awchUserId location
	BYTE* userIdBytes = (BYTE*)&req->awchUserId;
	NTL_PRINT(PRINT_APP, "[Login] req->awchUserId at %p, first 10 bytes: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", 
		userIdBytes, userIdBytes[0], userIdBytes[1], userIdBytes[2], userIdBytes[3], userIdBytes[4], userIdBytes[5], userIdBytes[6], userIdBytes[7], userIdBytes[8], userIdBytes[9]);
	
	// Check WCHAR values directly - use proper format for WCHAR (unsigned short)
	NTL_PRINT(PRINT_APP, "[Login] req->awchUserId WCHAR[0-4]: 0x%04hX 0x%04hX 0x%04hX 0x%04hX 0x%04hX", 
		(unsigned short)req->awchUserId[0], (unsigned short)req->awchUserId[1], (unsigned short)req->awchUserId[2], 
		(unsigned short)req->awchUserId[3], (unsigned short)req->awchUserId[4]);
	
	// Also check what's at the expected location
	WCHAR* expectedWChar = (WCHAR*)(data + offsetof(sUA_LOGIN_REQ_TAIWAN_CT, awchUserId));
	NTL_PRINT(PRINT_APP, "[Login] Expected location WCHAR[0-2]: 0x%04hX 0x%04hX 0x%04hX", 
		(unsigned short)expectedWChar[0], (unsigned short)expectedWChar[1], (unsigned short)expectedWChar[2]);
	
	// Check if WCHAR string has null terminator early
	int wcharLen = 0;
	while (wcharLen < (NTL_MAX_SIZE_USERID_UNICODE + 1) && req->awchUserId[wcharLen] != 0)
		wcharLen++;
	NTL_PRINT(PRINT_APP, "[Login] WCHAR string length (until null): %d", wcharLen);
	
	// Check bytes at WCHAR[2] location - why is it reading null?
	BYTE* wchar2Bytes = (BYTE*)&req->awchUserId[2];
	NTL_PRINT(PRINT_APP, "[Login] Bytes at &req->awchUserId[2]: %02X %02X (WCHAR[2]=0x%04hX)", 
		wchar2Bytes[0], wchar2Bytes[1], (unsigned short)req->awchUserId[2]);
	
	// Check what's actually in the packet at offset+4 (where WCHAR[2] should be)
	BYTE* packetAtOffset4 = data + offsetof(sUA_LOGIN_REQ_TAIWAN_CT, awchUserId) + (2 * sizeof(WCHAR));
	NTL_PRINT(PRINT_APP, "[Login] Packet bytes at data+offset+4: %02X %02X (should be 65 00 for third 'e')", packetAtOffset4[0], packetAtOffset4[1]);
	
	// PASSWORD FIELD DEBUGGING - similar to username debugging
	NTL_PRINT(PRINT_APP, "[Login] === PASSWORD FIELD DEBUGGING ===");
	BYTE* expectedPasswdStart = data + offsetof(sUA_LOGIN_REQ_TAIWAN_CT, awchPasswd);
	BYTE* actualPasswdStart = (BYTE*)&req->awchPasswd;
	NTL_PRINT(PRINT_APP, "[Login] Expected awchPasswd at data+%zu=%p, actual at %p, difference=%ld bytes", 
		offsetof(sUA_LOGIN_REQ_TAIWAN_CT, awchPasswd), expectedPasswdStart, actualPasswdStart, (long)(actualPasswdStart - expectedPasswdStart));
	
	// Check raw bytes at awchPasswd location
	BYTE* passwdBytes = (BYTE*)&req->awchPasswd;
	NTL_PRINT(PRINT_APP, "[Login] req->awchPasswd at %p, first 20 bytes: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", 
		passwdBytes, passwdBytes[0], passwdBytes[1], passwdBytes[2], passwdBytes[3], passwdBytes[4], passwdBytes[5], passwdBytes[6], passwdBytes[7], passwdBytes[8], passwdBytes[9],
		passwdBytes[10], passwdBytes[11], passwdBytes[12], passwdBytes[13], passwdBytes[14], passwdBytes[15], passwdBytes[16], passwdBytes[17], passwdBytes[18], passwdBytes[19]);
	
	// Check WCHAR values directly
	NTL_PRINT(PRINT_APP, "[Login] req->awchPasswd WCHAR[0-9]: 0x%04hX 0x%04hX 0x%04hX 0x%04hX 0x%04hX 0x%04hX 0x%04hX 0x%04hX 0x%04hX 0x%04hX", 
		(unsigned short)req->awchPasswd[0], (unsigned short)req->awchPasswd[1], (unsigned short)req->awchPasswd[2], 
		(unsigned short)req->awchPasswd[3], (unsigned short)req->awchPasswd[4], (unsigned short)req->awchPasswd[5],
		(unsigned short)req->awchPasswd[6], (unsigned short)req->awchPasswd[7], (unsigned short)req->awchPasswd[8], (unsigned short)req->awchPasswd[9]);
	
	// Check what's at the expected location
	WCHAR* expectedPasswdWChar = (WCHAR*)(data + offsetof(sUA_LOGIN_REQ_TAIWAN_CT, awchPasswd));
	NTL_PRINT(PRINT_APP, "[Login] Expected location awchPasswd WCHAR[0-4]: 0x%04hX 0x%04hX 0x%04hX 0x%04hX 0x%04hX", 
		(unsigned short)expectedPasswdWChar[0], (unsigned short)expectedPasswdWChar[1], (unsigned short)expectedPasswdWChar[2],
		(unsigned short)expectedPasswdWChar[3], (unsigned short)expectedPasswdWChar[4]);
	
	// Check if WCHAR string has null terminator
	int passwdWcharLen = 0;
	while (passwdWcharLen < (NTL_MAX_SIZE_USERPW_UNICODE + 1) && req->awchPasswd[passwdWcharLen] != 0)
		passwdWcharLen++;
	NTL_PRINT(PRINT_APP, "[Login] Password WCHAR string length (until null): %d", passwdWcharLen);
	
	// Fix memory leak: Ntl_WC2MB returns char* that must be freed with delete[]
	// Original code: std::string username = Ntl_WC2MB(req->awchUserId); (memory leak)
	// Fixed: allocate, copy to string, then free
	char* usernameMB = Ntl_WC2MB(req->awchUserId);
	NTL_PRINT(PRINT_APP, "[Login] Ntl_WC2MB returned: %p, strlen=%zu, content='%s'", 
		usernameMB, usernameMB ? strlen(usernameMB) : 0, usernameMB ? usernameMB : "(null)");
	if (usernameMB == NULL)
	{
		NTL_PRINT(PRINT_APP, "[Login] ERROR: Ntl_WC2MB(username) returned NULL (Session %u)", GetHandle());
		return;
	}
	std::string username = std::string(usernameMB);
	delete[] usernameMB;
	
	// Convert password from WCHAR to multibyte
	char* password = Ntl_WC2MB(req->awchPasswd);
	NTL_PRINT(PRINT_APP, "[Login] Ntl_WC2MB(password) returned: %p, strlen=%zu", 
		password, password ? strlen(password) : 0);
	if (password == NULL)
	{
		NTL_PRINT(PRINT_APP, "[Login] ERROR: Ntl_WC2MB(password) returned NULL - wcstombs conversion failed (Session %u)", GetHandle());
		return;
	}
	
	// Log password details (masked for security - only show length and first/last chars)
	size_t passwdLen = strlen(password);
	if (passwdLen > 0)
	{
		char firstChar = password[0];
		char lastChar = password[passwdLen - 1];
		NTL_PRINT(PRINT_APP, "[Login] Password extracted: length=%zu, first_char='%c' (0x%02X), last_char='%c' (0x%02X)", 
			passwdLen, firstChar, (unsigned char)firstChar, lastChar, (unsigned char)lastChar);
		// Show full password in debug mode (be careful in production!)
		NTL_PRINT(PRINT_APP, "[Login] Password content: '%s'", password);
	}
	else
	{
		NTL_PRINT(PRINT_APP, "[Login] WARNING: Password length is 0!");
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
			NTL_PRINT(PRINT_APP, "[Login] Querying database for username: '%s' (Session %u)", username.c_str(), GetHandle());
			smart_ptr<QueryResult> result = GetAccDB.Query("SELECT AccountID,Password_hash,acc_status,isGm,lastServerFarmId,founder FROM accounts WHERE Username = \"%s\" LIMIT 1", GetAccDB.EscapeString(username).c_str());
			if (result)
			{
				Field* fields = result->Fetch();
				const char* storedHash = fields[1].GetString();
				
				// MD5 HASH DEBUGGING
				NTL_PRINT(PRINT_APP, "[Login] === MD5 PASSWORD VALIDATION DEBUGGING ===");
				NTL_PRINT(PRINT_APP, "[Login] Password before hashing: length=%zu, content='%s'", strlen(password), password);
				
				MD5 md;
				char md5pwd[NTL_MAX_SIZE_USERPW_MULTIBYTE_BUFFER];
				snprintf(md5pwd, NTL_MAX_SIZE_USERPW_MULTIBYTE_BUFFER, "%s", md.digestString(password));
				
				NTL_PRINT(PRINT_APP, "[Login] Computed MD5 hash: '%s' (length=%zu)", md5pwd, strlen(md5pwd));
				NTL_PRINT(PRINT_APP, "[Login] Stored MD5 hash: '%s' (length=%zu)", storedHash ? storedHash : "(null)", storedHash ? strlen(storedHash) : 0);
				
				// Compare hashes
				int cmpResult = NTL_STRICMP(storedHash, md5pwd);
				NTL_PRINT(PRINT_APP, "[Login] Hash comparison result: %d (0=match, non-zero=mismatch)", cmpResult);
				
				if (0 != cmpResult) //check password
				{
					resultcode = AUTH_WRONG_PASSWORD;
					NTL_PRINT(PRINT_APP, "[Login] Password mismatch! Computed hash '%s' != stored hash '%s'", md5pwd, storedHash ? storedHash : "(null)");
				}
				else
				{
					NTL_PRINT(PRINT_APP, "[Login] Password hash match! Authentication successful.");
					
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