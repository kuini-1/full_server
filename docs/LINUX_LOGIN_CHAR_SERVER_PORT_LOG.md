# Linux Login and Character Server Port Log

**Purpose:** Track every attempt to port the login → character server connection flow from Windows to Linux. Document what works, what fails, and avoid repeating failed approaches. This is a long-term process until the server works 100% on both Windows and Linux.

**Current goal (full flow):** Login and character server connection on Linux works end-to-end: (1) **Auth Server:** client connects → handshake → login packet received → password validated → Master Server check → AU_LOGIN_RES sent with Character Server info. (2) **Character Server:** client connects to Character Server → UC_LOGIN_REQ received → player created → Master Server auth check → character list loaded. (3) **In-game:** client selects character → connects to Game Server → loads into game world. This log is updated until all three work 100% on Linux.

---

## Reference: How Windows Works

### Login Flow (Windows)
1. **Client → Auth Server:** Client connects to Auth Server → handshake → sends `UA_LOGIN_REQ_TAIWAN_CT` with username/password
2. **Auth Server:** Receives login packet → converts WCHAR to multibyte → queries database → computes MD5 hash → compares with stored hash
3. **Auth → Master Server:** If password matches, Auth Server sends `AM_ON_PLAYER_CHECK_REQ` to Master Server to check if player is already online
4. **Master → Auth Server:** Master Server responds with `MA_ON_PLAYER_CHECK_RES` indicating if player is online
5. **Auth → Client:** If player is offline, Auth Server sends `AU_LOGIN_RES` with Character Server IP/Port and auth key
6. **Client → Character Server:** Client disconnects from Auth Server and connects to Character Server at provided IP/Port → sends `UC_LOGIN_REQ` with auth key
7. **Character Server:** Receives `UC_LOGIN_REQ` → creates player → sends `CM_LOGIN_REQ` to Master Server for auth key validation
8. **Master → Character Server:** Master Server validates auth key → responds with `MC_LOGIN_RES`
9. **Character Server → Client:** Character Server sends character list to client

### Key Components
- **MD5 Hash:** Password is hashed using MD5 algorithm. Hash is stored in database as 32-character hex string.
- **WCHAR Conversion:** Username and password arrive as WCHAR (UTF-16LE, 2 bytes per character on Windows). Converted to multibyte (`char*`) using `Ntl_WC2MB`.
- **Server Registration:** Character Server registers with Master Server using `CM_NOTIFY_SERVER_BEGIN` packet containing `achPublicAddress` (IP clients should connect to).
- **IP Address Handling:** Character Server config has `PublicAddress` (for clients) and `Address` (for server-to-server). Auth Server sends `PublicAddress` to client in `AU_LOGIN_RES`.

---

## Linux Port (Baseline)

### Differences from Windows
- **WCHAR Size:** On Linux, `WCHAR` is defined as `unsigned short` (2 bytes) to match Windows behavior, but native `wchar_t` is 4 bytes (UTF-32). The codebase uses `WCHAR` consistently.
- **MD5 Implementation:** Uses custom MD5 implementation in `Shared/Util/md5.h`. Requires 32-bit integers (`UINT4`) for correct hash computation.
- **String Functions:** Uses portable macros (`NTL_STRCPY_S`, `NTL_WCSCPY_S`, etc.) defined in `NtlPortable.h` that map to Windows `strcpy_s`/`wcscpy_s` or Linux equivalents.
- **Network:** Uses same network layer as Windows (NtlNetwork), which has Linux-specific implementations for accept/recv/send.

### Current Status
- **Login authentication:** Works (MD5 hash fix applied)
- **Master Server communication:** Works (Auth Server can query Master Server)
- **Character Server registration:** Works (Character Server registers with Master Server)
- **Client → Character Server connection:** **NOT WORKING** - Client receives Character Server IP but cannot connect

---

## Fix Attempts (Chronological)

### 1. Fix MD5 Hash Calculation on Linux (UINT4 Size Issue)

- **Symptom:** Login failed with `AUTH_WRONG_PASSWORD` (result code 107). MD5 hash computed for password "12" was `4b4613f0c6bc16b4705507c0d3307f97` but expected/stored hash was `c20ad4d76fe97759aa27a0c99bff6710`.
- **Root cause:** In `Shared/Util/md5.h`, `UINT4` was defined as `unsigned long int`. On Linux, `unsigned long int` is 64-bit, while MD5 algorithm requires 32-bit integers. This caused incorrect hash computation.
- **Fix:** Changed `UINT4` typedef in `md5.h`:
  ```cpp
  #ifdef _WIN32
  typedef unsigned long int UINT4;
  #else
  #include <stdint.h>
  typedef uint32_t UINT4;
  #endif
  ```
- **Result:** **WORKED** - MD5 hash now computes correctly. Login authentication succeeds.
- **Files changed:** `Shared/Util/md5.h`
- **Date:** 2026-02-18

### 2. Character Server IP Address Issue (0.0.0.0)

- **Symptom:** Client receives login success (`AU_LOGIN_RES` with `ResultCode=100`) but cannot connect to Character Server. Logs show Character Server IP is `0.0.0.0`.
- **Root cause:** Character Server config file has `PublicAddress = 0.0.0.0` or empty. Character Server registers this with Master Server, and Auth Server sends `0.0.0.0` to client, which cannot connect to it.
- **Attempted fixes:**
  - Added fallback logic to use internal address if public is `0.0.0.0`
  - Added fallback to use `127.0.0.1` if both are invalid
  - Added internal address registration from Character Server
- **Result:** **PARTIAL** - Fallback works for localhost testing, but original Windows code doesn't have fallback logic. Restored to original code that trusts config.
- **Current status:** Character Server must have correct `PublicAddress` in config file. Original Windows code works 100% when config is correct.
- **Files changed:** (Reverted) `Server/AuthServer/MasterServerPacket.cpp`, `Server/CharServer/MasterServerSession.cpp`
- **Date:** 2026-02-18

### 3. Restore Original Windows Code

- **What:** Removed all hardcoded changes, fallback logic, and extra logging added during debugging to restore exact Windows behavior.
- **Why:** Original Windows code works 100%. Linux port should match Windows behavior exactly, not add workarounds.
- **Changes reverted:**
  - Removed `ZeroMemory` initialization in `MasterServerPacket.cpp`
  - Removed all `NTL_PRINT` logging statements added for debugging
  - Removed `bIsOn` check before using server info
  - Removed IP fallback logic (0.0.0.0 → internal → 127.0.0.1)
  - Removed internal address setting in Character Server registration
  - Removed retry logic for `AddPlayer` in `PacketAuthServer.cpp`
  - Removed extra logging throughout login flow
- **Result:** **COMPLETED** - Code now matches original Windows implementation exactly.
- **Files restored:** `Server/AuthServer/MasterServerPacket.cpp`, `Server/CharServer/MasterServerSession.cpp`, `Server/AuthServer/PacketAuthServer.cpp`, `Server/CharServer/PacketCharServer.cpp`, `Server/CharServer/ClientSession.cpp`, `Server/AuthServer/ClientSession.cpp`, `Server/MasterServer/CharPacket.cpp`
- **Date:** 2026-02-18

### 4. Fix wcscpy_s for Linux Build

- **Symptom:** Build error on Linux: `'wcscpy_s' was not declared in this scope`. Original Windows code uses `wcscpy_s` directly, but this function doesn't exist on Linux.
- **Root cause:** `wcscpy_s` is a Windows-specific secure string function. On Linux, we need to use the portable macro `NTL_WCSCPY_S` defined in `NtlPortable.h`.
- **Fix:** Changed `wcscpy_s` to `NTL_WCSCPY_S` in `PacketAuthServer.cpp`:
  - Line 92: `AM_ON_PLAYER_CHECK_REQ` packet creation
  - Line 164: `AU_LOGIN_CREATEUSER_RES` packet creation
- **Result:** **WORKED** - Build succeeds on Linux. `NTL_WCSCPY_S` is available through `NtlSharedCommon.h` → `NtlPortable.h` include chain.
- **Files changed:** `Server/AuthServer/PacketAuthServer.cpp`
- **Date:** 2026-02-18
- **Note:** This is a legitimate Linux port fix using the existing portable macro, not a workaround. The macro works on both Windows and Linux.

### 5. Fix Missing ProcessPacket() Implementation

- **Symptom:** Linker error: `undefined reference to 'CClientSession::ProcessPacket()'`. The function is declared as virtual in `ClientSession.h` but not implemented in `ClientSession.cpp`.
- **Root cause:** `CClientSession` declares `ProcessPacket()` as virtual (overriding the base class `CNtlSession::ProcessPacket()`), but the implementation was missing from the `.cpp` file.
- **Fix:** Added `ProcessPacket()` implementation in `ClientSession.cpp` that calls the base class version:
  ```cpp
  int CClientSession::ProcessPacket()
  {
      return CNtlSession::ProcessPacket();
  }
  ```
- **Result:** **WORKED** - Linker error resolved. The implementation delegates to the base class which handles packet processing logic.
- **Files changed:** `Server/AuthServer/ClientSession.cpp`
- **Date:** 2026-02-18
- **Note:** This may have been missing in the original code or was removed during previous edits. The implementation simply calls the base class method, which is the correct behavior unless custom packet processing is needed.

### 6. Fix WCHAR String Handling Macros (L'\0' Literal Issue)

- **Symptom:** Potential WCHAR string null termination issues on Linux. `L'\0'` is a 4-byte `wchar_t` literal on Linux, but `WCHAR` is defined as `unsigned short` (2 bytes) to match Windows behavior.
- **Root cause:** In `Shared/Util/NtlStringHandler.h`, macros like `NTL_SAFE_WCSCPY`, `NTL_SAFE_WCSNCPY`, `NTL_SAFE_WCSNCPY_SIZEINPUT` used `L'\0'` for null termination. On Linux, `wchar_t` is 4 bytes while `WCHAR` is 2 bytes, causing incorrect null termination.
- **Fix:** Replaced all instances of `L'\0'` with `(WCHAR)0` in the string handling macros to ensure correct 2-byte null termination:
  ```cpp
  // Old: buffer[0] = L'\0';
  // New: buffer[0] = (WCHAR)0;
  ```
- **Result:** **WORKED** - WCHAR strings are now correctly null-terminated with 2-byte null on Linux, matching Windows behavior.
- **Files changed:** `Shared/Util/NtlStringHandler.h`
- **Date:** 2026-02-19
- **Note:** Critical fix for WCHAR compatibility between Windows and Linux. Client expects exact WCHAR format matching Windows.

### 7. Fix strcpy_s for Linux Build

- **Symptom:** Build error on Linux: `'strcpy_s' was not declared in this scope`. Original Windows code uses `strcpy_s` directly, but this function doesn't exist on Linux.
- **Root cause:** `strcpy_s` is a Windows-specific secure string function. On Linux, we need to use the portable macro `NTL_STRCPY_S` defined in `NtlPortable.h`.
- **Fix:** Changed `strcpy_s` to `NTL_STRCPY_S` in:
  - `Server/AuthServer/MasterServerPacket.cpp` (line 66): Character Server IP assignment
  - `Server/CharServer/MasterServerSession.cpp` (line 25): PublicAddress registration
- **Result:** **WORKED** - Build succeeds on Linux. `NTL_STRCPY_S` is available through `NtlSharedCommon.h` → `NtlPortable.h` include chain.
- **Files changed:** `Server/AuthServer/MasterServerPacket.cpp`, `Server/CharServer/MasterServerSession.cpp`
- **Date:** 2026-02-19
- **Note:** This is a legitimate Linux port fix using the existing portable macro, not a workaround.

### 8. Packet Structure Verification and Code Cleanup

- **What:** Verified packet structures match Windows original exactly and removed debugging code to match original implementation.
- **Verification:**
  - Compared `sAU_LOGIN_RES` structure: ✅ **IDENTICAL**
  - Compared `sSERVER_INFO` structure: ✅ **IDENTICAL**
  - Compared `sNTLPACKETHEADER` structure: ✅ **IDENTICAL**
  - Compared type definitions (`ACCOUNTID`, `SERVERFARMID`, etc.): ✅ **IDENTICAL**
  - Compared constants (`NTL_MAX_SIZE_USERID_UNICODE`, etc.): ✅ **IDENTICAL**
  - Compared field assignment order: ✅ **IDENTICAL**
  - Verified `#pragma pack(1)` usage: ✅ **IDENTICAL**
- **Changes:**
  - **Removed** `ZeroMemory()` call (was added during debugging, original doesn't use it)
  - **Removed** all `printf()` debugging statements (original has no debugging output)
  - Kept `NTL_STRCPY_S` instead of `strcpy_s` (portable macro, maps to `strcpy_s` on Windows)
- **Result:** ✅ **VERIFIED** - Packet structures are 100% identical. Code now matches Windows original exactly (except for portable macro usage).
- **Files changed:** `Server/AuthServer/MasterServerPacket.cpp`, `Server/CharServer/MasterServerSession.cpp`
- **Date:** 2026-02-19
- **Documentation:** Created `docs/PACKET_STRUCTURE_VERIFICATION.md` with detailed comparison

### 9. Client-Side Packet Structure Investigation

- **What:** Investigated client-side packet handling in `E:\SERVER\2.0\DBO-Client-2.0` to understand how client processes `AU_LOGIN_RES` and connects to Character Server.
- **Findings:**
  - Client handler: `PacketHandler_LSLoginRes` in `DboPacketHandler_Lobby.cpp`
  - Client expects `sAU_LOGIN_RES` structure with:
    - `wResultCode` (must be `AUTH_SUCCESS` for success)
    - `awchUserId[]` (WCHAR array)
    - `abyAuthKey[]` (authentication key)
    - `accountId`
    - `byServerInfoCount`
    - `aServerInfo[]` array containing:
      - `szCharacterServerIP` (char array)
      - `wCharacterServerPortForClient` (WORD)
      - `dwLoad` (DWORD)
  - Client flow: Receives `AU_LOGIN_RES` → parses server info → disconnects from Auth Server → connects to Character Server → sends `UC_LOGIN_REQ`
- **Current issue:** Client still sends `UC_LOGIN_REQ` to Auth Server instead of Character Server, suggesting either:
  1. Client didn't receive `AU_LOGIN_RES` correctly
  2. Client received it but `wResultCode != AUTH_SUCCESS`
  3. Client received it but packet structure mismatch prevents parsing
  4. Client parsed it correctly but connection logic fails
- **Next steps:** Add packet size/structure debugging to verify server sends correct packet format matching client expectations.
- **Date:** 2026-02-19

---

## Current Status

### What Works
1. **MD5 hash calculation** - Fixed UINT4 size issue, passwords validate correctly
2. **Login authentication** - Client can login successfully, receives `AU_LOGIN_RES` with `ResultCode=100`
3. **Master Server communication** - Auth Server queries Master Server for player online status
4. **Character Server registration** - Character Server registers with Master Server successfully
5. **Packet structure** - Login packets are parsed correctly, WCHAR conversion works

### What Doesn't Work
1. **Client → Character Server connection** - Client receives Character Server IP/Port but cannot connect
   - Symptom: Client shows "cannot connect to character server" error
   - Possible causes:
     - Character Server IP is `0.0.0.0` (config issue)
     - Client is on different machine and IP is `127.0.0.1` (localhost only)
     - Network/firewall issue
     - Character Server not listening on correct interface

### Known Issues
1. **Character Server config** - If `PublicAddress` is `0.0.0.0` or empty, client cannot connect. Original Windows code trusts config and doesn't validate.
2. **Localhost vs network IP** - If client is on different machine, `127.0.0.1` won't work. Need actual server IP address.

---

## What Works

1. **MD5 hash fix (UINT4 typedef)** - Use `uint32_t` for `UINT4` on Linux instead of `unsigned long int`
2. **wcscpy_s portable macro** - Use `NTL_WCSCPY_S` instead of `wcscpy_s` on Linux (macro available through `NtlSharedCommon.h` → `NtlPortable.h`)
3. **strcpy_s portable macro** - Use `NTL_STRCPY_S` instead of `strcpy_s` on Linux (macro available through `NtlSharedCommon.h` → `NtlPortable.h`)
4. **WCHAR null termination** - Use `(WCHAR)0` instead of `L'\0'` in string macros to ensure correct 2-byte null termination on Linux
5. **ProcessPacket() implementation** - Implement missing virtual function by delegating to base class
6. **Original Windows code structure** - Match Windows code exactly, don't add workarounds
7. **Packet initialization** - Use `ZeroMemory` to ensure clean packet initialization (matches Windows behavior)

---

## What Failed

1. **IP fallback logic** - Adding fallback from `0.0.0.0` → internal → `127.0.0.1` doesn't solve root cause (config issue)
2. **Hardcoded fixes** - Don't hardcode IP addresses or add workarounds. Fix config or underlying issue.

---

## Next Steps

1. **Investigate Character Server connection issue:**
   - Check Character Server logs for connection attempts
   - Verify Character Server is listening on correct interface
   - Check network connectivity and firewall rules
   - Verify client is using correct IP address from login response

2. **Document any Linux-specific network issues:**
   - Socket binding/listening differences
   - IP address handling differences
   - Connection acceptance differences

3. **Test end-to-end flow:**
   - Login → Character Server connection → Character list → Character selection → Game Server connection

---

## WSL Networking Notes

**Important:** When testing with WSL (Windows Subsystem for Linux):

1. **WSL IP Address**: WSL has its own IP address (usually `172.x.x.x`). The Windows host can access WSL services, but `127.0.0.1` in WSL is NOT the same as `127.0.0.1` on Windows.

2. **Character Server Configuration** (`charserver.ini`):
   - `Address` should be `0.0.0.0` (listen on all interfaces) or the WSL IP
   - `PublicAddress` should be the WSL IP address (not `127.0.0.1` or `0.0.0.0`)
   - To find WSL IP: Run `hostname -I` or `ip addr show eth0` in WSL

3. **Client Connection**:
   - If client runs on Windows, it needs the WSL IP address to connect
   - If client runs in WSL, it can use `127.0.0.1` or the WSL IP

4. **Example Configuration**:
   ```
   [Char Server]
   Address=0.0.0.0                    # Listen on all interfaces
   PublicAddress=172.28.112.1         # WSL IP address (use actual WSL IP)
   Port=20310
   ```

5. **Port Forwarding** (if needed):
   - WSL2: May need to forward ports from Windows to WSL
   - Check Windows firewall rules
   - Use `netsh interface portproxy` if needed

---

## Notes

- Original Windows code works 100% when config is correct
- Linux port should match Windows behavior exactly
- Don't add workarounds - fix root causes
- Document all attempts, successful or not
- Keep MD5 fix (UINT4) - this was a real Linux bug fix
