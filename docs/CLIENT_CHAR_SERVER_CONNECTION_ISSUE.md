# Client Character Server Connection Issue - Long Term Memory

**Date Created:** 2026-02-19  
**Status:** IN PROGRESS - Client receives AU_LOGIN_RES but does not connect to Character Server

## Problem Statement

**Symptom:** Client successfully logs into Auth Server, receives `AU_LOGIN_RES` with Character Server IP/Port (`172.28.112.18:20310`), disconnects from Auth Server correctly, but **never attempts to connect to Character Server**.

**Evidence:**
- Auth Server logs show: `[AuthServer] Sending Character Server info to client: IP='172.28.112.18', Port=20310`
- Client disconnects from Auth Server: `[AuthServer] Client disconnecting: AccountID=814049`
- Character Server is listening: `AcceptorThread [0.0.0.0:20310]` and `ss -tuln | grep 20310` shows port is active
- **NO** `[PostAccept] AcceptEx SUCCEEDED!` or `[Acceptor] OnAccepted` messages in Character Server logs
- Network connectivity verified: `Test-NetConnection -ComputerName 172.28.112.18 -Port 20310` succeeds

**Critical Fact:** Windows server code works 100%. Linux port has this issue. Therefore, the problem is in Linux-specific code changes.

## Root Cause Hypothesis

The client is not parsing `AU_LOGIN_RES` correctly on Linux, likely due to:
1. **WCHAR handling mismatch** - Packet structure or WCHAR array initialization differs from Windows
2. **Packet structure mismatch** - Field offsets, padding, or initialization differs
3. **String encoding issue** - IP address string or WCHAR userId encoding differs

## Verified Working Components

1. ✅ **MD5 hash calculation** - Fixed UINT4 size issue, passwords validate correctly
2. ✅ **Login authentication** - Client can login successfully, receives `AU_LOGIN_RES` with `ResultCode=100`
3. ✅ **Master Server communication** - Auth Server queries Master Server for player online status
4. ✅ **Character Server registration** - Character Server registers with Master Server successfully
5. ✅ **Character Server listening** - Port 20310 is active and accepting connections
6. ✅ **Network connectivity** - Windows can reach WSL IP/port
7. ✅ **Client disconnects from Auth Server** - Disconnect packet received correctly

## Current Code State

### Server/AuthServer/MasterServerPacket.cpp

**Current implementation (matches Windows except portable macros):**
```cpp
CNtlPacket packet(sizeof(sAU_LOGIN_RES));
sAU_LOGIN_RES * res = (sAU_LOGIN_RES *)packet.GetPacketData();
res->wOpCode = AU_LOGIN_RES;
NTL_SAFE_WCSCPY(res->awchUserId, req->awchUserId);

memcpy(res->abyAuthKey, req->abyAuthKey, sizeof(res->abyAuthKey));
res->dwAllowedFunctionForDeveloper = req->dwAllowedFunctionForDeveloper;
res->accountId = req->accountId;
// ... server info assignment ...
NTL_STRCPY_S(res->aServerInfo[0].szCharacterServerIP, NTL_MAX_LENGTH_OF_IP + 1, srvinfo->achPublicAddress);
res->aServerInfo[0].wCharacterServerPortForClient = srvinfo->wPortForClient;
// ... other fields ...
res->byServerInfoCount = 1;
res->wResultCode = resultcode;
res->lastServerFarmId = req->lastServerFarmId;
res->bIsGM = req->bIsGM;

packet.SetPacketLen(sizeof(sAU_LOGIN_RES));
app->SendTo(session, &packet);
```

**Key differences from Windows:**
- Uses `NTL_STRCPY_S` instead of `strcpy_s` (portable macro, maps to `strcpy_s` on Windows)
- Uses `NTL_SAFE_WCSCPY` instead of direct `wcscpy_s` (portable macro)

**Packet initialization:**
- `CNtlPacket` uses `calloc()` which zeros the buffer (line 211 in NtlPacket.cpp)
- No `ZeroMemory()` call (original Windows code doesn't use it)

## WCHAR Handling Verification Needed

**Critical:** User has emphasized multiple times that WCHAR handling must be verified.

**WCHAR Definition:**
- Windows: `wchar_t` (2 bytes, UTF-16LE)
- Linux: `unsigned short` (2 bytes, UTF-16LE) - defined in `NtlSharedCommon.h`

**WCHAR Copy Macros:**
- `NTL_SAFE_WCSCPY` uses `NTL_WCSNCPY_S`
- Linux `NTL_WCSNCPY_S` implementation (NtlPortable.h lines 53-66) uses manual loop with `(WCHAR)0` null termination
- Windows `NTL_WCSNCPY_S` maps to `wcsncpy_s` (standard CRT)

**Verification Needed:**
1. Verify `NTL_SAFE_WCSCPY` produces identical byte sequence as Windows `wcscpy_s`
2. Verify WCHAR array `awchUserId` is null-terminated correctly
3. Verify packet byte layout matches Windows exactly (use hex dump comparison)

## Packet Structure Verification

**sAU_LOGIN_RES structure:**
```cpp
BEGIN_PROTOCOL(AU_LOGIN_RES)
    WORD wResultCode;
    WCHAR awchUserId[NTL_MAX_SIZE_USERID_UNICODE + 1];  // 17 WCHARs = 34 bytes
    BYTE abyAuthKey[NTL_MAX_SIZE_AUTH_KEY];              // 16 bytes
    ACCOUNTID accountId;                                 // 4 bytes
    SERVERFARMID lastServerFarmId;                       // 1 byte
    DWORD dwAllowedFunctionForDeveloper;                 // 4 bytes
    bool bIsGM;                                          // 1 byte
    BYTE byServerInfoCount;                              // 1 byte
    sSERVER_INFO aServerInfo[DBO_MAX_CHARACTER_SERVER_COUNT]; // Array
END_PROTOCOL()
```

**sSERVER_INFO structure:**
```cpp
struct sSERVER_INFO {
    char szCharacterServerIP[NTL_MAX_LENGTH_OF_IP + 1];  // 65 bytes
    WORD wCharacterServerPortForClient;                  // 2 bytes
    DWORD dwLoad;                                         // 4 bytes
    SERVERFARMID serverfarmID;                           // 1 byte
    BYTE serverchannelID;                                // 1 byte
};
```

**Packet size:** 841 bytes total (includes 2-byte header, so data is 839 bytes)

## Next Steps - Systematic Verification

1. **Hex dump comparison:** Capture raw packet bytes from Windows server and Linux server, compare byte-by-byte
2. **WCHAR array verification:** Verify `awchUserId` array is identical between Windows and Linux
3. **Field offset verification:** Use `offsetof()` to verify all field offsets match
4. **String encoding verification:** Verify IP address string encoding matches
5. **Packet initialization verification:** Verify uninitialized fields are zeroed identically

## Files Modified

- `Server/AuthServer/MasterServerPacket.cpp` - Login response packet construction
- `Server/CharServer/ClientSession.cpp` - Added connection logging
- `Server/NtlNetwork/NtlAcceptor.cpp` - Added connection logging
- `Server/NtlNetwork/NtlConnection.cpp` - Added connection logging

## Related Documentation

- `docs/LINUX_LOGIN_CHAR_SERVER_PORT_LOG.md` - General Linux porting log
- `docs/PACKET_STRUCTURE_VERIFICATION.md` - Previous packet structure verification
- `.cursor/rules/linux-login-char-server.mdc` - Cursor rule for this issue

## Key Reminders

1. **Windows code works 100%** - Any differences are bugs in Linux port
2. **WCHAR handling is critical** - Must verify WCHAR arrays match Windows exactly
3. **Packet structure must be identical** - Client cannot be modified, server must match exactly
4. **No workarounds** - Fix root cause, don't add workarounds
5. **Systematic verification** - Use hex dumps and offset verification, not assumptions
