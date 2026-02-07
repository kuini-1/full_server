#pragma once

#include "NtlPacketCommon.h"
#include "NtlCSArchitecture.h"

enum eOPCODE_UA
{
	UA_OPCODE_BEGIN = 100,

	UA_LOGIN_REQ,
	UA_LOGIN_REQ_KOREA_CJ,
	UA_LOGIN_REQ_TAIWAN_CT,

	UA_LOGIN_CREATEUSER_REQ,
	UA_LOGIN_DISCONNECT_CN_REQ,

	UA_LOGIN_DISCONNECT_TW_REQ,

	UA_OPCODE_DUMMY,
	UA_OPCODE_END = UA_OPCODE_DUMMY - 1
};


//------------------------------------------------------------------
//
//------------------------------------------------------------------
const char * NtlGetPacketName_UA(WORD wOpCode);
//------------------------------------------------------------------

#pragma pack(1)

//------------------------------------------------------------------
BEGIN_PROTOCOL(UA_LOGIN_REQ)
	WCHAR		awchUserId[NTL_MAX_SIZE_USERID_UNICODE + 1];
	WCHAR		awchPasswd[NTL_MAX_SIZE_USERPW_UNICODE + 1];
	DWORD		dwAuthPatternData;
	WORD		wLVersion;
	WORD		wRVersion;
	BYTE		abyMacAddress[DBO_MAX_ADAPTER_ADDRESS_LENGTH];
	BYTE		byState;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(UA_LOGIN_REQ_KOREA_CJ)
	DWORD		dwAuthPatternData;
	WORD		wLVersion;
	WORD		wRVersion;
	BYTE		abyMacAddress[DBO_MAX_ADAPTER_ADDRESS_LENGTH];
	WORD		wCpCookieLength;
	char		szCpCookie[NTL_MAX_SIZE_CP_COOKIE + 1];
	bool		bIsNew;
	BYTE		abyAuthKey[NTL_MAX_SIZE_AUTH_KEY];
	ACCOUNTID	accountId;
	SERVERFARMID	lastServerFarmId;
	WCHAR		awchUserId[NTL_MAX_SIZE_USERID_UNICODE + 1];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(UA_LOGIN_REQ_TAIWAN_CT)
	WCHAR		awchUserId[NTL_MAX_SIZE_USERID_UNICODE + 1];
	WCHAR		awchPasswd[NTL_MAX_SIZE_USERPW_UNICODE + 1];
	DWORD		dwAuthPatternData;
	WORD		wLVersion;
	WORD		wRVersion;
	BYTE		abyMacAddress[DBO_MAX_ADAPTER_ADDRESS_LENGTH];
	BYTE		byState;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(UA_LOGIN_CREATEUSER_REQ)
	WCHAR		awchUserId[NTL_MAX_SIZE_USERID_UNICODE + 1];
	WCHAR		awchPasswd[NTL_MAX_SIZE_USERPW_UNICODE + 1];
	DWORD		dwCodePage;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(UA_LOGIN_DISCONNECT_CN_REQ)
	WCHAR		awchUserId[NTL_MAX_SIZE_USERID_UNICODE + 1];
	bool		bIsEnteringCharacterServer;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(UA_LOGIN_DISCONNECT_TW_REQ)
WCHAR		awchUserId[NTL_MAX_SIZE_USERID_UNICODE + 1];
bool		bIsEnteringCharacterServer;
END_PROTOCOL()
//------------------------------------------------------------------
#pragma pack()