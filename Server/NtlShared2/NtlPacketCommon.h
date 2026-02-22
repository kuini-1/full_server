#pragma once


#pragma warning(disable:4819) // vs2005 codepage bug disable

#include "NtlSharedType.h"
#include "NtlSharedDef.h"
#include "NtlSharedCommon.h"

#include <string>


#pragma warning(disable : 4328)



//------------------------------------------------------------------
//
//------------------------------------------------------------------

//#pragma pack(push, 1)  /* push current alignment to stack */
//struct sNTLPACKETHEADER
//{
//	sNTLPACKETHEADER(WORD wGivenOpCode) :
//		wOpCode(wGivenOpCode) {}
//	
//	BYTE	null;
//	WORD	packetSequence;
//	QWORD	checksumm;
//	WORD	wOpCode;
//};
//#pragma pack(pop)   /* restore original alignment from stack */

struct sNTLPACKETHEADER
{
	WORD			wOpCode;
};

struct sWEB_ONLINE_PLAYER
{
	ACCOUNTID		accountId;
	CHARACTERID		charId;
	WCHAR			awchCharName[NTL_MAX_SIZE_CHAR_NAME + 1];
	SERVERFARMID	serverFarmId;
	SERVERCHANNELID	serverChannelId;
};

struct sWEB_SERVER_ENTRY
{
	BYTE			byServerType;
	SERVERFARMID	serverFarmId;
	SERVERCHANNELID	serverChannelId;
	SERVERINDEX		serverIndex;
	bool			bIsOn;
	BYTE			byRunningState;
	DWORD			dwLoad;
	DWORD			dwMaxLoad;
	WORD			wPortForClient;
	char			achPublicAddress[NTL_MAX_LENGTH_OF_IP + 1];
};

struct sWEB_GAME_FARM_INFO
{
	SERVERFARMID	serverFarmId;
	BYTE			byServerStatus;
	DWORD			dwLoad;
	DWORD			dwMaxLoad;
	WCHAR			wszGameServerFarmName[NTL_MAX_SIZE_SERVER_FARM_NAME_UNICODE + 1];
};

struct sWEB_GAME_CHANNEL_INFO
{
	SERVERFARMID	serverFarmId;
	SERVERCHANNELID	byServerChannelIndex;
	BYTE			byServerStatus;
	DWORD			dwLoad;
	DWORD			dwMaxLoad;
	bool			bIsVisible;
	bool			bIsScrambleChannel;
	WCHAR			wszServerChannelName[NTL_MAX_SIZE_SERVER_CHANNEL_NAME_UNICODE + 1];
};

struct sWEB_CHANNEL_COUNT
{
	SERVERFARMID	serverFarmId;
	SERVERCHANNELID	serverChannelId;
	DWORD			dwCount;
};

struct sWEB_FARM_COUNT
{
	SERVERFARMID	serverFarmId;
	DWORD			dwCount;
};

struct sWEB_ONLINE_ACCOUNT
{
	ACCOUNTID		accountId;
	SERVERFARMID	serverFarmId;
	SERVERCHANNELID	serverChannelId;
};

struct sWEB_CHANNEL_FLAG
{
	SERVERFARMID	serverFarmId;
	SERVERCHANNELID	serverChannelId;
	bool			bFlag;
};


//------------------------------------------------------------------
//
//------------------------------------------------------------------
/* Use first-member wOpCode instead of inheritance so GCC applies __attribute__((packed)); layout matches sNTLPACKETHEADER at offset 0. */
#define BEGIN_PROTOCOL(opcode)						\
struct s##opcode {									\
	WORD wOpCode;

#define END_PROTOCOL()	};

#define END_PROTOCOL_PACKED()	} NTL_STRUCT_PACKED;

//------------------------------------------------------------------
//
//------------------------------------------------------------------
#define BEGIN_PROTOCOL_IDENTITY( opcode, identity )	\
typedef s##identity s##opcode						\

#define END_PROTOCOL_IDENTITY()	;
//------------------------------------------------------------------


//------------------------------------------------------------------
#define DECLARE_PACKET_NAME( opcode )	{ #opcode }
//------------------------------------------------------------------