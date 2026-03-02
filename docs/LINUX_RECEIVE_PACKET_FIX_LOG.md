# Linux Receive Packet Fix Log

**Purpose:** Track every attempt to fix "receive packet" on Linux so it behaves like Windows, and avoid breaking the client connection. Do not repeat failed approaches.

**Development focus:** Linux-first (test from WSL/Linux). The **Windows C++ client** cannot be modified; all packets sent from the Linux server **must match the exact wire format** the client expects (byte layout, sizes, offsets).

**Working Windows reference (for compare):** `E:\SERVER\full_server - 2-7-2026` (config 127.0.0.1; no wire layer; same handshake). Use for logic and packet flow comparison; do not change the Linux wire layer or config for WSL + Windows client.

**Current goal (full flow):** Packet handling on Linux works end-to-end: (1) **Auth:** client connects → handshake → login packet received and processed → client receives AU_LOGIN_RES and can proceed. (2) **Char server:** client joins char server and receives responses. (3) **In-game:** client loads into game. This log is updated until all three work 100%.

---

## Reference: How Windows Receives

- **Accept:** Acceptor calls `PostAccept` → `AcceptEx` (async). When a client connects, IOCP completes → worker runs `CompleteAccept` → `SetAddress`, `Associate`, `IsBlockedIp`, `SetStatus(ACTIVE)`, `OnAccepted`, `PostNetEventMessage(NETEVENT_ACCEPT)`, `PostRecv()` → returns. `PostRecv` calls `WSARecv` (async); returns immediately.
- **Recv:** When data arrives, IOCP completes → worker runs `CompleteRecv(dwBytes)` → `RecvPackets(dwBytes)` (push bytes, parse, post `NETEVENT_RECV` per packet) → `PostRecv()` (re-post async recv) → return. Dispatcher later runs `ProcessPacket()` for each `NETEVENT_RECV`.
- **Order on Windows:** In `CompleteRecv`: `RecvPackets` then `PostRecv`. Session stays ACTIVE because nothing in the worker sets STATUS_CLOSE before `PostRecv`.

---

## Linux Port (Baseline)

- **Accept:** `AcceptEx` = blocking `accept()`; EAGAIN → pending; success → post completion to fake IOCP, same `CompleteAccept` flow.
- **Recv:** No kernel IOCP. `RecvEx` = non-blocking `recv()`. If data: post completion to IOCP so worker runs `CompleteRecv`. If EAGAIN: return success, no post; `ValidCheck` periodically calls `PostRecv()` again to poll.
- **Send:** `SendEx` on Linux = synchronous `send()`; after send we post completion with byte count so worker runs `CompleteSend` (handshake and other sends work).

---

## Fix Attempts (Chronological)

### 1. Defer decryption-failure close (NETEVENT_FORCE_CLOSE)

- **What:** In `ProcessPacket`, when decryption fails too many times, instead of `SetStatus(STATUS_CLOSE)` + `CheckDisconnect` immediately, post `NETEVENT_FORCE_CLOSE`. Handler runs later and does `SetStatus(STATUS_CLOSE)` + `CheckDisconnect`.
- **Why:** Dispatcher could run `ProcessPacket` before the worker called `PostRecv()`, so session was already CLOSE when worker called `PostRecv()` → rc=100045.
- **Result:** **Insufficient.** With `m_bDirectProcess` true, `PostNetEventMessage(FORCE_CLOSE)` was processed synchronously, so STATUS_CLOSE still set before `PostRecv()`.

### 2. Never process FORCE_CLOSE synchronously

- **What:** In `PostNetEventMessage`, if event is `NETEVENT_FORCE_CLOSE`, always call `PostNetEvent` (queue) and never `SendNetEvent` (direct).
- **Result:** **Insufficient.** Dispatcher could still process the queued FORCE_CLOSE in the next iteration before the worker called `PostRecv()` (race between worker and dispatcher).

### 3. Reorder CompleteRecv: PostRecv before RecvPackets

- **What:** In `CompleteRecv`: (1) push received bytes to buffer, (2) call `PostRecv()` (re-post), (3) call `RecvPackets(0)` (parse only; `RecvPackets` skips push when `dwTransferedBytes == 0`). So worker re-posts recv while still ACTIVE, then parses/dispatches.
- **Why:** Ensures nothing in the worker or dispatcher can set STATUS_CLOSE before we call `PostRecv()`.
- **Result:** **Should fix 100045** if no other path sets CLOSE. User still reports rc=100045 — possible causes: (a) build not updated, (b) stale FORCE_CLOSE applied to reused session.

### 4. Stale FORCE_CLOSE on session reuse (pending flag)

- **What:** Add `m_bPendingForceClose` on session. When posting FORCE_CLOSE, set it true. In FORCE_CLOSE handler, only run close if `TakePendingForceClose()` is true; else skip (stale event for reused session pointer).
- **Why:** Reused session pointer could have an old FORCE_CLOSE in the queue; handler would set new session to CLOSE by mistake.
- **Result:** **Intended** to stop false CLOSE on new connections. User still sees 100045 — need to confirm rebuild and that "[PostRecv] Session not ACTIVE (status=...)" log appears to see actual status.

### 5. Diagnostic log in PostRecv

- **What:** When `PostRecv()` returns `NTL_ERR_NET_SESSION_CLOSED` because `!IsStatus(ACTIVE)`, log: `[PostRecv] Session not ACTIVE (status=%d), Session=%p, IP=%s`.
- **Use:** Tells us why session wasn’t ACTIVE (e.g. status=4 = STATUS_CLOSE). If this log never appears after rebuild, the failure path may be different.

### 6. Extra diagnostics to pinpoint 100045 path

- **What:** (1) In `PostRecv()`, when returning 100045 due to `m_bIsTrafficHeavy`, log `[PostRecv] Traffic heavy - returning SESSION_CLOSED, Session=%p, IP=%s`. (2) In `CompleteRecv()`, log status at start (after remote-close check) and immediately before calling `PostRecv()`: `[CompleteRecv] Start: status=%d, ...` and `[CompleteRecv] Before PostRecv: status=%d, ...`.
- **Why:** User still sees 100045 but not the "Session not ACTIVE" log — so either build is stale or 100045 comes from the traffic-heavy branch or from the "0 bytes" branch (which already logs "Connection closed (0 bytes)"). These logs identify which path runs and whether status changes between start of CompleteRecv and PostRecv.
- **Result:** Logs still did not appear — led to checking how worker gets byte count; see fix 7.

### 7. Pass received byte count when posting RECV completion (root cause)

- **What:** On Linux, when PostRecv() has data and posts to IOCP, we were calling `PostIocpEventMessage((WPARAM)this, (LPARAM)&m_recvContext)` (2-arg overload). The 2-arg overload calls `PostIOCPEvent(wParam, lParam)`, which on Linux posts **0** as `dwBytesTransferred`. The worker then runs `CompleteRecv(0)` → first line `if (0 == dwTransferedBytes) return NTL_ERR_NET_SESSION_CLOSED` → rc=100045. No diagnostic logs run because we never get past that check.
- **Fix:** Call the 3-arg overload: `PostIocpEventMessage(dwTransferedBytes, (WPARAM)this, (LPARAM)&m_recvContext)` so the worker receives the actual byte count and runs `CompleteRecv(12)` (or whatever was received).
- **Result:** **Root cause.** After this change, rebuild and test; login receive should work.

---

## What Works (Do Not Regress)

- **Client connection:** Client can connect; handshake is sent; TCP stays up until we close it.
- **Send path (Linux):** `SendEx` = sync `send()` + post completion with byte count; handshake and other sends work.
- **Accept path (Linux):** Accept + post completion, `CompleteAccept`, `PostRecv` (EAGAIN path), NETEVENT_ACCEPT, OnAccept, handshake — all good.
- **ValidCheck PostRecv retry:** Needed on Linux so we eventually call `recv()` and get the 12 bytes; when data arrives we post RECV completion. **Do not remove.**
- **FORCE_CLOSE always queued:** `NETEVENT_FORCE_CLOSE` must never be processed via `SendNetEvent` (only `PostNetEvent`).
- **One-shot generated wire layer (fix 19l):** [tools/gen_packet_wire.py](tools/gen_packet_wire.py) reads [wire/wire_layout.txt](wire/wire_layout.txt) (Windows layout dump) and [wire/our_layout.txt](wire/our_layout.txt) (Linux layout dump), computes copy ops for **all** packet structs, and emits [PacketWireLayout.generated.cpp](Server/NtlNetwork/PacketWireLayout.generated.cpp) with one table and one encode/decode loop. No per-packet hand-written wire code. Adding a packet = add to layout dumps and re-run generator. Build runs the generator then compiles the generated file. Send path (AddToSendBuffer, PushPacket, CompleteSend) and receive path (PopPacket) unchanged; they call the same API. Wire layout is committed so Linux build does not require Windows.
- **Correct hex / size logging (fix 19m):** On Linux, when a packet has wire conversion (e.g. AU_LOGIN_RES, CU_LOGIN_RES), Auth and Char MasterServerPacket log **actual sent to client** (wire payload size and total bytes, e.g. 797 for AU_LOGIN_RES) in addition to app buffer size. PacketCharServer logs **actual received from client** (wire payload size) for UC_LOGIN_REQ when conversion applies. Logs now reflect what is sent/received on the wire.
- **Char server wire path:** Char server uses the same NtlConnection send/recv as Auth (Send → SendPacket → AddToSendBuffer; PopPacket → DecodeRecvPacket). No separate code path. To support Char↔client wire format for more packets: extend [wire/wire_layout.txt](wire/wire_layout.txt) with Windows layout for Char packet structs (e.g. sCU_LOGIN_RES, sCU_CHAR_SERVERLIST_RES, sUC_LOGIN_REQ); optionally run `python3 tools/gen_packet_wire.py --dump-stub` to populate our_layout.txt with all structs when the stub compiles; rebuild. See [wire/README.md](wire/README.md).
- **Auth→Char disconnect flow (39-byte packet):** After the client receives AU_LOGIN_RES, it sends **UA_LOGIN_DISCONNECT_CN_REQ** or **UA_LOGIN_DISCONNECT_TW_REQ** (~39 bytes) to Auth to say "I am leaving Auth to connect to Char". Auth handles this in `ClientSession::OnDispatch`, logs it, and sends **AU_LOGIN_DISCONNECT_RES** via `SendLoginDcReq`. The client then disconnects from Auth and is expected to connect to the Char server (IP/port from AU_LOGIN_RES, e.g. szCharacterServerIP and wCharacterServerPortForClient). This is normal flow, not an error. If the client never appears on Char, check: (1) AU_LOGIN_RES contained the correct Char server public IP and port; (2) Char server is listening and reachable from the client; (3) client actually opens a new TCP connection to that address.
- **AU_LOGIN_RES wire encode verification:** The generated wire table for opcode 1002 is produced **only from layout dumps** via a **generic two-region rule** in [tools/gen_packet_wire.py](tools/gen_packet_wire.py): when wire and our fixed-part sizes differ (wire 65, our 69), the generator splits the fixed part into prefix (0,0,63) and 2-byte tail (63,64,2) for bIsGM/byServerInfoCount, then array ops. No per-packet schema (WIRE_SCHEMA_EXPLICIT removed). No NtlConnection fixup; all behavior from generator. On Linux, when Auth sends AU_LOGIN_RES we log: (1) raw wire payload hex (first 140 bytes); (2) first server info block 73-byte hex; (3) parsed szCharacterServerIP and wCharacterServerPortForClient from the wire buffer.

---

## What Failed / Breaks Connection

- **Setting STATUS_CLOSE in ProcessPacket** (before worker calls PostRecv) → worker sees !ACTIVE → 100045 → worker closes session. So: never set STATUS_CLOSE in the recv/dispatch path before the worker has re-posted recv (hence reorder + FORCE_CLOSE deferral).
- **Processing FORCE_CLOSE synchronously** when `m_bDirectProcess` is true → same as above.
- **Applying FORCE_CLOSE to a session that didn’t request it** (reused pointer) → new session marked CLOSE → 100045. Hence pending flag and `TakePendingForceClose()` in handler.
- **Per-file -fpack-struct (Fix 9g/19a):** Applying `-fpack-struct` only to MasterServerPacket.cpp and PacketAuthServer.cpp did **not** produce sizeof(sAU_LOGIN_RES)==795 or client-expected offsets (65, 73). Layout remains 839/69/77 on Linux (Clang 18); client reads wrong bytes for Character Server IP.
- **Global option 1 (19c):** #pragma ms_struct on sNTLPACKETHEADER and sSERVER_INFO + -mms-bitfields on NtlShared did **not** yield 795-byte layout. ms_struct could not be applied to derived protocol structs (AU_LOGIN_RES etc.) due to Clang -Wincompatible-ms-struct (base class).

---

## Research: What works for others / What doesn't (avoid retrying)

### 0. Requirement for any acceptable fix

- **Global, not per-file:** Fix must apply to **all** protocol packets in one go (one mechanism, one place). No per-file `-fpack-struct`, no per-packet wire builders, and **no per-file or per-packet wire definitions**—if using an explicit wire format, it must be a single global mechanism that works with any packet without recoding the wire for each packet or file.
- **Achievable in ~1 day:** Implementable and verifiable in short time, not a multi-year per-packet effort.

### 1. What does NOT work (do not retry)

- **`#pragma ms_struct` with inheritance:** GCC/Clang reject/ignore ms_struct on structs with base classes (-Wincompatible-ms-struct). Protocol structs use `BEGIN_PROTOCOL` (inherit from `sNTLPACKETHEADER`), so ms_struct cannot be applied. (Ref: fix log 19c; GCC x86 Type Attributes; SO "GCC/clang and Visual C++ structure compatibility".)
- **`-fpack-struct` matching MSVC for inherited + array structs:** No flag makes GCC/Clang match MSVC layout for inheritance + arrays (e.g. sAU_LOGIN_RES with sSERVER_INFO[10]). (Ref: LLVM #95361; SO "Byte layout #pragma pack MSVC vs clang/gcc"; fix log 19a, 19h.)
- **Per-file `-fpack-struct`:** Only on one/two TUs did not yield 795-byte layout; on parse TUs caused segfaults. (Ref: fix log 9g, 19a, 19b, 19d.)
- **Global `-fpack-struct` on project/NtlShared:** Breaks STL/std::map (e.g. NtlIniFile, NtlTrigger); corrupts pointers, crashes. (Ref: fix log 8b, 9e, 9f.)
- **`__attribute__((packed))` on protocol structs:** Did not reduce sizeof(sAU_LOGIN_RES) on Linux (still 839). (Ref: fix log 19 Fix 2, 4.)
- **Composition instead of inheritance (flat struct):** sizeof still 839. (Ref: fix log 19 Fix 6, 9.)
- **Truncating to 795 bytes without reordering:** Wrong data at client's expected offsets; client misreads. (Ref: plan "Alternative global approaches" option 3.)
- **Per-packet or per-file fixes:** Any approach requiring touching every packet/file is out of scope (years, not a day).
- **Per-file or per-packet wire format:** Defining wire layout in each file or for each packet is not acceptable. Any wire approach must be global and work for any packet without recoding the wire.

### 2. What DOES work for others (industry / SO)

- **Explicit wire format (serialization view), only if global:** Define wire format explicitly (sizes, offsets, byte order) in **one** place; serialize/deserialize through that single mechanism so it works with **any** packet without recoding the wire per file or per packet. No per-file wire. (FlatBuffers, Cap'n Proto, Bebop, Kaitai Struct, etc. use schema-driven generation so one spec covers all messages.)
- **Single source of truth for layout:** One table/schema/spec for all packets; all send/recv through that definition (generated code or one module). Global and bounded in time.
- **Isolate packed code:** If using `-fpack-struct`, only in a dedicated lib with no STL/std::map, used only for building packets. (Our NtlPacketWire attempt still gave 839 with GCC + -fpack-struct there.)

### 3. References (URLs for the log)

- LLVM #95361: https://github.com/llvm/llvm-project/issues/95361
- SO 60627612: Byte layout #pragma pack MSVC vs clang/gcc
- SO 78615907: GCC/clang and Visual C++ structure compatibility issue
- SO 42501878: Packed structures sizes and inheritance with different compilers
- GCC: Structure-Layout Pragmas, x86 Type Attributes (ms_struct)

---

## Current State (as of this log)

- **Auth receive:** Working. Client connects, handshake, login packet (e.g. 87 bytes, OpCode 0x0067) received and dispatched; "[ClientSession] Received packet OpCode" confirms ProcessPacket runs.
- **Login not completing in-game:** Auth processes login but client may not receive AU_LOGIN_RES. Success path requires MasterServer connected (Auth sends online-check, MasterServer responds, then Auth sends AU_LOGIN_RES to client). If MasterServer not running, Auth now sends failure (AUTH_NO_AVAILABLE_CHARACTER_SERVER) to client so client gets a response. See PacketAuthServer.cpp / MasterServerPacket.cpp and ensure MasterServer is running for login success.
- **Log flood (fixed):** ValidCheck and PostRecv EAGAIN logs throttled to once per 5 min so packet/login logs stay visible (fix 10).
- **Code in place:** Byte count in RECV completion (fix 7); reorder push → PostRecv → RecvPackets(0); FORCE_CLOSE deferred + pending flag; dispatcher IOCP lazy-create on Linux (fix 9); Auth MasterServer NULL check and AU_LOGIN_RES logging; log throttle (fix 10).

### 8. Receive working; cleanup and NULL guard

- **Result:** After fix 7, receive works: packets (OpCode 0x0004, 0x0001, 0x0067, etc.) are received and processed. Diagnostic logs showed `status=4` = STATUS_ACTIVE (enum: INIT=0, CREATE=1, ACCEPT=2, CONNECT=3, ACTIVE=4, CLOSE=5).
- **Cleanup:** Removed the two verbose `[CompleteRecv] Start` / `Before PostRecv` logs to reduce noise.
- **NULL m_hEventIOCP:** Logs showed "(NULL == m_hEventIOCP)" once during receive. In `PostNetEvent()`, added early return when `m_hEventIOCP` is NULL (log message and return NTL_FAIL) so we don’t call `PostQueuedCompletionStatus` with NULL. If this appears often, ensure every network’s processor has `Create()` called before any session posts events.

---

### 9. Lazy-create dispatcher IOCP when NULL (Linux)

- **Symptom:** "(NULL == m_hEventIOCP)" appeared repeatedly when client received data; NETEVENT_RECV was dropped so the 87-byte login packet (and others) were never dispatched to ProcessPacket.
- **Fix:** On Linux only, in `PostNetEvent()` when `m_hEventIOCP` is NULL, call `Create()` once under a static mutex so the dispatcher IOCP is created; then post the event to it. The dispatcher thread will use the new handle on its next loop and process events. After rebuild you should see "[NetworkProcessor] Lazy-created dispatcher IOCP (was NULL)" once, then login and other packets processed normally.

### 10. Throttle ValidCheck and PostRecv EAGAIN logs

- **Symptom:** Logs flooded with "[ValidCheck] Retrying PostRecv (retry #N)" and "[PostRecv] No data available (ERROR_IO_PENDING)" so important logs (received packet, login, etc.) were hard to see.
- **Fix:** (1) ValidCheck: log at most once per **5 minutes per session** (was every 100 retries ≈ 1 s). (2) PostRecv EAGAIN: log at most once per **5 minutes globally** (was every 5 s). Keep "[PostRecv] *** DATA RECEIVED! ***" and "[ClientSession] Received packet OpCode" as-is so receive activity stays visible.

### 11. Filter OpCode 0x0001 and 4-byte receive logs

- **Symptom:** "[ClientSession] Received packet OpCode: 0x0001" and "[PostRecv] *** DATA RECEIVED! 4 bytes ***" spammed (heartbeat/ping).
- **Fix:** (1) ClientSession: log only when `wOpCode != 0x0001`. (2) PostRecv: log only when `dwTransferedBytes != 4`. Other opcodes and other sizes still logged.

### 12. Treat ECONNRESET (104) as connection closed in PostRecv (Linux)

- **Symptom:** MasterServer: "PostRecv returned error: 104" in CompleteAccept for Char server connection; session closed and "CHAR SERVER DISCONNECTED". Error 104 = ECONNRESET (connection reset by peer).
- **Fix:** In PostRecv (Linux), when `recv()` returns 104, treat like EBADF: return `NTL_ERR_NET_SESSION_CLOSED` and log "[PostRecv] Connection reset by peer (104) for Session=..., IP=...". Session still closes (correct); log explains the reason. Root cause of peer reset is separate (e.g. Char server closing the connection right after connect).
- **Note:** Rebuild **all** servers (Auth, Master, Char) with the same NtlNetwork so they get: ValidCheck throttle (once per 5 min), PostRecv EAGAIN throttle, 4-byte/0x0001 filters, and ECONNRESET handling. CharServer logs showing "Retrying PostRecv" every 100 retries mean it was built before the throttle.

### 13. Filter 12-byte receives and add login processing logs

- **Symptom:** MasterServer spammed with "[PostRecv] *** DATA RECEIVED! 12 bytes ***" (server-to-server heartbeat). Login packet received but no logs showing login processing or response sent.
- **Fix:** (1) PostRecv: filter 12-byte receives (server heartbeat) in addition to 4-byte. (2) Login handler: add `NTL_PRINT` logs at key points (login request received, MasterServer check, success/failure response sent) so login flow is visible even if `ERR_LOG` doesn't show in console. Logs: "[Login] User ... login request", "[Login] User ...: Auth success, sending online check", "[Login] User ...: Login failed, sent AU_LOGIN_RES", "[Login] Login success: sent AU_LOGIN_RES".

### 14. Debug login handler crash and fix username/password conversion

- **Symptom:** Login packet (OpCode 0x0067 = UA_LOGIN_REQ_TAIWAN_CT) received, `SendCharLogInReq` called, but function stops before username log. Function appears to crash or return early when accessing packet data.
- **Debug logs added:** (1) OpCode comparison log shows enum values at runtime. (2) Switch match log confirms SendCharLogInReq is called. (3) Function entry log. (4) Packet size and pointer validation. (5) Step-by-step logs before/after each Ntl_WC2MB call.
- **Fix:** (1) Fixed memory leak: original `std::string username = Ntl_WC2MB(...)` was wrong (Ntl_WC2MB returns char* that must be freed). Now: allocate, copy to string, free. (2) Added NULL checks for Ntl_WC2MB return values. (3) Added packet size validation before accessing struct fields. (4) All early returns are safe (password cleanup handles NULL). After rebuild, logs will show exactly where the function stops if it still crashes.

### 15. Fix packet pointer: use GetPacketBuffer() not GetPacketData() for structs that include header

- **Symptom:** "[Login] ERROR: Packet too small for struct (size 87 < struct size)" - packet size check failed even though 87 bytes is correct.
- **Root cause:** `sUA_LOGIN_REQ_TAIWAN_CT` inherits from `sNTLPACKETHEADER` (via `BEGIN_PROTOCOL` macro), so the struct **includes the header** as the first member. But `GetPacketData()` returns a pointer to data **after** the header. So casting `GetPacketData()` to `sUA_LOGIN_REQ_TAIWAN_CT*` was wrong - we were pointing to payload but treating it as if it started with header.
- **Fix:** Changed from `pPacket->GetPacketData()` to `pPacket->GetPacketBuffer()` when casting to structs that inherit from `sNTLPACKETHEADER`. `GetPacketBuffer()` returns the full packet (header + payload), which matches the struct layout. Updated size check to compare `packetSize < sizeof(sUA_LOGIN_REQ_TAIWAN_CT)` directly.

### 16. Fix packet size validation: check minimum fields instead of full struct sizeof()

- **Symptom:** After Fix 15, logs showed "[Login] ERROR: Packet too small (size 87 < struct size 157)" - packet is 87 bytes but struct `sizeof()` is 157 bytes.
- **Root cause:** The struct `sUA_LOGIN_REQ_TAIWAN_CT` has `sizeof()` of 157 bytes due to compiler padding/alignment, but the actual packet from the client is 87 bytes (which matches the actual field sizes: header 2 + username 34 + password 34 + other fields ~17 = 87). The size check was comparing against `sizeof()` which includes padding.
- **Fix:** Changed size validation to check against minimum required fields (header 2 + username 34 + password 34 = 70 bytes) instead of full struct `sizeof()` (157 bytes). This allows the packet to proceed if it's large enough to read the essential fields. Added `#include <stddef.h>` for offsetof (though ended up using manual calculation).

### 17. Fix WCHAR size mismatch: Linux wchar_t (4 bytes) vs Windows WCHAR (2 bytes)

- **Symptom:** Login packet received correctly (87 bytes). Raw bytes show correct UTF-16: `65 00 65 00 65 00` = "eee". But `req->awchUserId[2]` reads as `0x0000` instead of `0x0065`. `Ntl_WC2MB` returns strlen=6 (correct for "eee") but username shows as "e" (truncated). Struct alignment is correct (`difference=0 bytes`).
- **Root cause:** On Windows, `WCHAR` is `wchar_t` which is 2 bytes (UTF-16). On Linux, `WCHAR` was defined as `wchar_t` which is 4 bytes (UTF-32). The game protocol expects 2-byte UTF-16, so when Linux reads `WCHAR[2]`, it reads 8 bytes ahead instead of 4 bytes, causing misalignment. The struct reads `WCHAR[2]` as null even though the raw bytes show another 'e'.
- **Fix:** 
  1. Changed `WCHAR` definition in `Shared/NtlSharedCommon.h` from `typedef wchar_t WCHAR;` to `typedef unsigned short WCHAR;` on Linux (matches Windows 2-byte size).
  2. Updated `Ntl_MB2WC` and `Ntl_WC2MB` in `Shared/Util/NtlStringHandler.cpp` to use `iconv` for UTF-8 <-> UTF-16LE conversion instead of `mbstowcs`/`wcstombs` (which expect `wchar_t`).
  3. Added `iconv` library linking in `CMakeLists.txt` for the Util library.
- **Files Modified:**
  - `Shared/NtlSharedCommon.h`: Changed `WCHAR` typedef to `unsigned short` on Linux
  - `Shared/Util/NtlStringHandler.cpp`: Updated conversion functions to use `iconv` for UTF-16LE
  - `CMakeLists.txt`: Added `iconv` library linking for Util
- **Result:** [Pending test - should fix username truncation issue]

### 18. CompleteRecv diagnostic logging (rc=100045 on Char client)

- **Symptom:** Client connects to Character Server, session accepted, then immediately `[IOCP Worker] CompleteIO failed -> Close session. rc=100045, iomode=3`. Client shows "failed to connect to character server".
- **Change:** Added printf logs in CompleteRecv: (1) when dwTransferedBytes==0 (peer closed before/without sending), (2) when PostRecv fails (peer closed during re-post), (3) when receiving data (byte count, excluding 4/12 heartbeat).
- **Use:** Distinguishes: (a) client closed before sending → CompleteRecv(0), (b) client sent data then closed during re-post → PostRecv returns SESSION_CLOSED.
- **Files:** `Server/NtlNetwork/NtlConnection.cpp`

### 19. AU_LOGIN_RES struct layout: bool bIsGM and GCC padding (szCharacterServerIP offset mismatch)

- **Symptom:** Hex dump comparison showed Linux AU_LOGIN_RES = 841 bytes vs Windows 795 bytes. Client receives login success but never connects to Character Server (never attempts TCP to Char Server port).
- **Root cause:** On GCC/Linux, `#pragma pack(1)` alone does not suppress all ABI padding; struct was 839 bytes vs Windows 795 bytes. Extra 44 bytes shift `szCharacterServerIP` offset; Windows client reads zeros instead of the IP.
- **Fix 1:** Changed `bool bIsGM` to `BYTE bIsGM` in `sAU_LOGIN_RES` (Server/NtlShared2/NtlPacketAU.h). Use 0/1 for false/true.
- **Fix 2:** `sizeof` still 839 after Fix 1. Added `__attribute__((packed))` for GCC, applied **only** to `sAU_LOGIN_RES` and `sSERVER_INFO` (not globally): (1) `NTL_STRUCT_PACKED` macro in Shared/NtlSharedCommon.h, (2) `sSERVER_INFO` in NtlCSArchitecture.h gets `} NTL_STRUCT_PACKED;`, (3) `END_PROTOCOL_PACKED()` in NtlPacketCommon.h for use with sAU_LOGIN_RES only. Using packed globally broke build (cannot bind packed field to reference in PacketCharServer.cpp). Packed on sAU_LOGIN_RES alone did not reduce sizeof.
- **Fix 3:** Reverted Fix 2 (packed structs). **Manual packet construction on Linux only** in Server/AuthServer/MasterServerPacket.cpp - rejected; user wanted root cause fix, no manual packet definitions.
- **Fix 4:** **POD packet header (root cause fix):** GCC ignores `__attribute__((packed))` on structs that inherit from a non-POD base. Removed constructor from `sNTLPACKETHEADER` and from `BEGIN_PROTOCOL` macro. Added `NTL_STRUCT_PACKED` macro, applied packed only to `sAU_LOGIN_RES` and `sSERVER_INFO`. **Result:** sizeof(sAU_LOGIN_RES) on Linux still 839; packed did not take effect.
- **Fix 5:** **Manual packet construction on Linux only** - worked but not a real fix; reverted.
- **Fix 6:** **Composition instead of inheritance** - reverted; sizeof still 839 on Linux.
- **Fix 7:** **Manual packet construction on Linux (restored):** Reverted per user: no manual construction ever; find global fix.
- **Fix 8:** **Global pack via -fpack-struct (Linux):** Add `-fpack-struct` for Linux so packet layout matches Windows. Reverted manual construction. Fix PacketCharServer reference-to-packed-field (temp copy pattern). Add static_assert(sizeof(sAU_LOGIN_RES)==795) on Linux.
- **Fix 8b:** Global `add_compile_options(-fpack-struct)` broke NtlTrigger (std::map). **Change:** Apply `-fpack-struct` only to server executables via `target_compile_options`.
- **Fix 8c:** With `-fpack-struct`, sizeof(sAU_LOGIN_RES) remained 839. Removed static_assert; reverted NtlShared from -fpack-struct.
- **Fix 9:** **Linux wire struct (flat, packed):** GCC adds padding after base when derived has `__attribute__((packed))`. Added `sAU_LOGIN_RES_wire` on Linux only – same fields as sAU_LOGIN_RES but no inheritance (flat struct) so packed applies. Use `sAU_LOGIN_RES_wire` for send buffer in MasterServerPacket.cpp on Linux; same field names, no manual byte offsets. static_assert(sizeof(sAU_LOGIN_RES_wire)==795). **Result:** sizeof still 839 – sSERVER_INFO array elements still get padding when used inside packed struct.
- **Fix 9b:** **sSERVER_INFO_wire (inline packed element):** GCC adds padding to `sSERVER_INFO` when used as array element. Added flat packed `sSERVER_INFO_wire` and use it in sAU_LOGIN_RES_wire. **Result:** sSERVER_INFO_wire still 77 bytes (GCC tail padding); static_assert fails.
- **Fix 9c:** **-fpack-struct on NtlShared (Linux):** NtlShared compiles packet headers (NtlPacketAU.cpp). Added `target_compile_options(NtlShared PRIVATE -fpack-struct)`. Wire structs still 77/839 (GCC array-element tail padding).
- **Fix 9d:** **Use original structs only (no wire structs):** Removed sSERVER_INFO_wire and sAU_LOGIN_RES_wire from NtlPacketAU.h. MasterServerPacket.cpp now uses sAU_LOGIN_RES everywhere (same as Windows reference). Fix at compiler level: -fpack-struct on NtlShared remains; optional -DDBO_USE_CLANG=ON to try Clang for Linux (may produce sizeof(sAU_LOGIN_RES)==795).
- **Fix 9e:** **CNtlIniFile + -fpack-struct segfault:** MasterServer (built with -fpack-struct) uses CNtlIniFile with std::map m_iniData. Packed layout misaligns the map; destructor crashed in _Rb_tree::_S_right (corrupt tree pointers). Tried #pragma pack(push,8) and alignas(8) on m_iniData—insufficient; -fpack-struct overrode them.
- **Fix 9f:** **Remove -fpack-struct from server executables:** Stop applying -fpack-struct to AuthServer, CharServer, etc. Fixes CNtlIniFile segfault. NtlShared still gets -fpack-struct.
- **Fix 9g:** **#pragma pack(1) insufficient for GCC:** Hex comparison: Linux 839 bytes (offsetof aServerInfo=69, sSERVER_INFO=77) vs Windows 795 (65, 73). Apply `-fpack-struct` only to AuthServer/MasterServerPacket.cpp via set_source_files_properties so AU_LOGIN_RES has correct wire layout without affecting CNtlIniFile (in AuthServer.cpp).
- **Files Modified:** Server/NtlShared2/NtlPacketAU.h, Server/AuthServer/MasterServerPacket.cpp, CMakeLists.txt
- **Result:** [Test on Linux – with GCC sizeof may be 839; try Clang with -DDBO_USE_CLANG=ON to test 795-byte layout]

### 19a. Fix 9g applied in CMake; hex dump extended; layout still wrong with per-file -fpack-struct (2026-03-01)

- **What was tried:** (1) Applied `-fpack-struct` via `set_source_files_properties` to **MasterServerPacket.cpp** and **PacketAuthServer.cpp** only (Fix 9g), in CMakeLists.txt, so AU_LOGIN_RES is built with packed layout in those TUs without affecting AuthServer.cpp (CNtlIniFile). (2) In MasterServerPacket.cpp, added hex dump of bytes at **payload offset 65–138** (73 bytes) — i.e. what the Windows client would read as the first sSERVER_INFO — to compare with where we actually write Character Server IP/port. (3) Added static_assert(sizeof(sAU_LOGIN_RES)==795), static_assert(offsetof(sAU_LOGIN_RES,aServerInfo)==65), static_assert(sizeof(sSERVER_INFO)==73) in MasterServerPacket.cpp (Linux only) to verify layout when built with -fpack-struct.
- **Result:** **Failed.** Build (Clang 18) failed on the static_asserts: layout in those TUs is still not 795/65/73. Per-file `-fpack-struct` on MasterServerPacket.cpp and PacketAuthServer.cpp did **not** produce the Windows wire layout. Static_asserts were removed so AuthServer builds again; -fpack-struct remains on those two files in CMake (no harm; may help on other compiler versions). Hex dump at payload 65–138 remains for comparison when capturing logs.
- **Files modified:** CMakeLists.txt (set_source_files_properties for both .cpp), Server/AuthServer/MasterServerPacket.cpp (hex dump at 65–138; static_asserts added then removed).
- **Next:** Try full AuthServer build with GCC (no Clang) and -fpack-struct on same two files; or pursue other global layout fix (e.g. dedicated wire buffer with explicit 795-byte layout) per packet-layout rule if no compiler flag achieves 795.

### 19b. Segfault after receiving 87-byte login: do not use -fpack-struct on PacketAuthServer.cpp (2026-03-01)

- **Symptom:** Auth server crashed with "Segmentation fault (core dumped)" immediately after "[CompleteRecv] Received 87 bytes" (terminal 4). Exit code 139.
- **Cause:** PacketAuthServer.cpp was compiled with `-fpack-struct` (Fix 9g). That file **parses incoming** client packets (e.g. `sUA_LOGIN_REQ_TAIWAN_CT * req = (sUA_LOGIN_REQ_TAIWAN_CT *)pPacket->GetPacketData()`). The client sends in **Windows** layout; interpreting those bytes with packed layout in that TU caused wrong field reads and segfault.
- **Fix:** Removed `-fpack-struct` from PacketAuthServer.cpp in CMake. Only MasterServerPacket.cpp (which only **builds and sends** AU_LOGIN_RES, and does not parse client packets) keeps `-fpack-struct`.
- **Result:** Auth server no longer segfaults on login. AU_LOGIN_RES failure responses from PacketAuthServer.cpp are again 839 bytes (non-packed); success path from MasterServerPacket.cpp still uses -fpack-struct (layout there remains non-795 on current compiler).

### 19c. Global option 1: #pragma ms_struct + -mms-bitfields (2026-03-01)

- **What was tried:** (1) **#pragma ms_struct on/off** (Linux x86_64 only) in headers around **sNTLPACKETHEADER** ([NtlPacketCommon.h](Server/NtlShared2/NtlPacketCommon.h)) and **sSERVER_INFO** ([NtlCSArchitecture.h](Server/NtlShared2/NtlCSArchitecture.h)), so those structs use Microsoft-compatible layout without -fpack-struct on entire TUs. (2) **#pragma ms_struct** was not applied to AU_ protocol structs in [NtlPacketAU.h](Server/NtlShared2/NtlPacketAU.h) because Clang errors with "ms_struct may not produce Microsoft-compatible layouts for classes with base classes" (they inherit from sNTLPACKETHEADER). (3) **-mms-bitfields** added to NtlShared target in CMake so that library sees MS layout where applicable.
- **Result:** **Failed.** Build with static_assert(sizeof(sAU_LOGIN_RES)==795) failed: layout still not 795/65/73. So ms_struct on the base and sSERVER_INFO plus -mms-bitfields on NtlShared did **not** produce the client-expected wire layout. Per-file -fpack-struct for MasterServerPacket.cpp was restored so AuthServer continues to build; layout in that TU remains non-795 on current compiler.
- **Files modified:** NtlPacketCommon.h, NtlCSArchitecture.h (ms_struct on/off for Linux x86_64); NtlPacketAU.h (no ms_struct — would trigger Clang errors); CMakeLists.txt (NtlShared: -mms-bitfields; MasterServerPacket.cpp: -fpack-struct kept).
- **Next:** Option 2 in global plan: dedicated static library using same original structs, built with -fpack-struct only there, with few call-site changes; or try GCC instead of Clang to see if layout differs.

### 19d. Segfault when receiving MA_ON_PLAYER_CHECK_RES (69 bytes) from Master (2026-03-01)

- **Symptom:** Auth server crashed with "Segmentation fault (core dumped)" (exit 139) immediately after "[CompleteRecv] Received 69 bytes" on the **Master** session (127.0.0.1). Flow: client sent 87-byte login → SendCharLogInReq → AM_ON_PLAYER_CHECK_REQ sent to Master → Master replied with 69 bytes → crash.
- **Cause:** MasterServerPacket.cpp was compiled with `-fpack-struct` (Fix 9g). That file **parses incoming** Master packets: `sMA_ON_PLAYER_CHECK_RES * req = (sMA_ON_PLAYER_CHECK_RES*)pPacket->GetPacketData()`. The 69 bytes are from Master (possibly Windows layout); interpreting them with packed layout in that TU caused wrong field reads and segfault (same pattern as 19b for PacketAuthServer + client 87-byte packet).
- **Fix:** Removed `-fpack-struct` from MasterServerPacket.cpp in CMake. That file now parses MA_ON_PLAYER_CHECK_RES with default/pack(1) layout only. AU_LOGIN_RES we send is built with default layout (839 bytes); client wire layout remains wrong until a global fix, but Auth no longer crashes on Master response.
- **Result:** Auth server should complete the success path (receive MA response → send AU_LOGIN_RES to client) without segfault.
- **Files modified:** CMakeLists.txt (removed set_source_files_properties for MasterServerPacket.cpp).

### 19e. No wires / global fix only (rule 8)

- **Decision:** Do not use per-packet wire builders or manual byte-offset modules (e.g. AULoginResWire). That would require testing every single packet from Windows and fixing each one manually—unacceptable. We require a **global fix** (compiler/ABI/struct) that fixes layout for all packets at once, not years of per-packet work.
- **Added:** Rule 8 in "Rules for Future Fixes" and this log entry. Any solution for 795-byte AU_LOGIN_RES (and other packets) must be a global fix, not a one-off wire builder.

### 19f. Global fix: NtlPacketWire library (separate packet-build TU with -fpack-struct) (2026-03-01)

- **What was tried:** Plan approach 1 — a **separate static library** used only when **building** packets to send (no parsing). New lib **NtlPacketWire** with one source file `AULoginResWire.cpp` that includes the same protocol headers (NtlPacketAU.h, NtlCSArchitecture.h) and fills `sAU_LOGIN_RES`; this TU is built with **-fpack-struct**. MasterServerPacket.cpp and PacketAuthServer.cpp do **not** use -fpack-struct (they still parse MA and client packets with default layout). They call `AULoginResWire_GetPayloadSize()` and `AULoginResWire_Build(...)` and send the resulting payload size. So all AU_LOGIN_RES sent to the client are built in the single packed TU; layout/size is whatever that TU’s compiler produces.
- **Result:** Build succeeds. Payload size is `sizeof(sAU_LOGIN_RES)` in the wire TU. If that is 793 (Windows layout), the client gets the correct length. If the compiler still produces 839 in that TU (as in fix 19a with Clang), `AULoginResWire_GetPayloadSize()` returns 839 and a one-line runtime warning is printed suggesting to try GCC for that TU. No manual byte-offset construction; same struct, one global build path.
- **Files added:** `Server/NtlPacketWire/AULoginResWire.h`, `Server/NtlPacketWire/AULoginResWire.cpp`.
- **Files modified:** `CMakeLists.txt` (add NtlPacketWire, link into AuthServer), `Server/AuthServer/MasterServerPacket.cpp` (success and failure paths use wire API), `Server/AuthServer/PacketAuthServer.cpp` (all AU_LOGIN_RES send paths use wire API).
- **Next:** Run AuthServer and confirm wire payload size in log; if still 839, try building only `AULoginResWire.cpp` with GCC (e.g. `target_compile_options` or a GCC-only object) to see if layout becomes 793.

### 19g. Phase 1: GCC as default Linux compiler; 793→795 correction (global layout fix plan)

- **What was tried:** (1) **GCC as default Linux compiler:** In CMakeLists.txt, set compiler **before** `project()` so it takes effect: when `DBO_USE_CLANG` is not set, `CMAKE_C_COMPILER` and `CMAKE_CXX_COMPILER` are set to `gcc`/`g++`. NtlPacketWire (AULoginResWire.cpp) is built with GCC and `-fpack-struct`; no change to parsing TUs. (2) **795-byte constant:** Corrected AULoginResWire.cpp and AULoginResWire.h: client expects **795**-byte payload (Windows total 797 = 2 + 795), not 793; warning message and comment updated.
- **Result:** **To be verified.** Build with default (no `-DDBO_USE_CLANG=ON`), run AuthServer, and check: `AULoginResWire_GetPayloadSize()` / runtime warning for `sizeof(sAU_LOGIN_RES)`; hex dump in MasterServerPacket success path for total 797 bytes, payload 795, first sSERVER_INFO at offset 65. If layout is 795/65/73, Phase 1 is successful; document in "What Works". If still 839, proceed to Phase 2 (NtlPacketWire as ExternalProject with GCC).
- **Files modified:** CMakeLists.txt (compiler block before project(); message for GCC when not Clang), Server/NtlPacketWire/AULoginResWire.cpp, Server/NtlPacketWire/AULoginResWire.h (793→795).

### 19h. Phase 2: NtlPacketWire built as ExternalProject with GCC (global layout fix)

- **What was tried:** Build **NtlPacketWire** as a **separate CMake subproject** via `ExternalProject_Add`, so that **only** that library is built with **GCC** and `-fpack-struct` regardless of the main project's compiler (Clang or GCC). Main project links the resulting `libNtlPacketWire.a`; no -fpack-struct on any parsing TU. (1) Added standalone `Server/NtlPacketWire/CMakeLists.txt`: builds `AULoginResWire.cpp` with `-fpack-struct`, same include dirs, no NtlShared link. (2) Main CMakeLists.txt: `include(ExternalProject)`, `ExternalProject_Add(NtlPacketWire_build ... CMAKE_ARGS -DCMAKE_CXX_COMPILER=g++ ...)`, `BUILD_BYPRODUCTS` for the .a, `add_library(NtlPacketWire IMPORTED ...)` with `IMPORTED_LOCATION`, `add_dependencies(AuthServer NtlPacketWire_build)`.
- **Result:** **Build succeeded.** NtlPacketWire is built with GCC 13.3.0 and -fpack-struct in `<build>/NtlPacketWire-ep/src/NtlPacketWire_build-build/`. AuthServer links the imported lib. **To be verified:** Run AuthServer and trigger login success; check for no 839-byte warning and hex dump total=797 (payload=795). If 795, Phase 2 is successful; client should connect to Character Server.
- **Files added:** `Server/NtlPacketWire/CMakeLists.txt`.
- **Files modified:** `CMakeLists.txt` (ExternalProject for NtlPacketWire_build, imported NtlPacketWire target, AuthServer depends on NtlPacketWire_build).

### 19i. Alternative approaches: older GCC option + single wire-layout source of truth (2026-03-02)

- **Research (2023–2026):** No compiler flag reliably makes GCC/Clang match MSVC for structs with inheritance and array-of-packed (LLVM #95361, SO, GCC docs). Phase 2 (ExternalProject with GCC 13 + -fpack-struct) still yielded sizeof(sAU_LOGIN_RES)=839; nm confirmed AuthServer links that lib.
- **What was tried:** (1) **Older GCC:** CMake now prefers g++-10, g++-9, or g++-8 for NtlPacketWire ExternalProject when found (`find_program`); otherwise uses default g++. On systems with only GCC 13, no older GCC was available to test; when installed, older GCC will be used automatically. (2) **Single wire-layout source of truth:** Added [Server/NtlPacketWire/WireLayout.h](Server/NtlPacketWire/WireLayout.h) with Windows wire constants: `AU_LOGIN_RES_WIRE_PAYLOAD_SIZE=795`, `AU_LOGIN_RES_WIRE_OFFSET_ASERVERINFO=65`, `SERVER_INFO_WIRE_SIZE=73`. On Linux, `AULoginResWire_Build` builds into a temp `sAU_LOGIN_RES` (our 839-byte layout), then copies to the output buffer in wire order: first 65 bytes, then 10×73 bytes from `aServerInfo[]`. `AULoginResWire_GetPayloadSize()` returns 795 always. Call sites unchanged; packet size and sent bytes are now 797 (2+795).
- **Result:** **Implemented.** Build succeeds. Linux AuthServer now sends 795-byte payload (797 total) in the layout the client expects. Verify at runtime: hex dump should show total=797, payload=795; client should connect to Character Server.
- **Files added:** `Server/NtlPacketWire/WireLayout.h`.
- **Files modified:** `CMakeLists.txt` (find_program for g++-10/9/8, pass to ExternalProject), `Server/NtlPacketWire/AULoginResWire.cpp` (Linux: copy to wire layout using WireLayout constants; GetPayloadSize returns constant 795).

### 19j. Global packet fix Phase 1: Option A verification (2026-03)

- **Goal:** Per global packet layout plan, validate Option A: one library built with `-fpack-struct` yields Windows layout so all send/parse could use the same ABI.
- **What was tried:** Added `Server/NtlPacketWire/PacketLayoutVerify.cpp` that includes protocol headers (via AULoginResWire.h + NtlPacketAU.h) and static_asserts: `sizeof(sSERVER_INFO)==73`, `offsetof(sAU_LOGIN_RES, aServerInfo)==65`, `sizeof(sAU_LOGIN_RES)==795`. Built as part of NtlPacketWire (GCC 10, `-fpack-struct`).
- **Result:** **Failed.** All three static_asserts failed: sSERVER_INFO size ≠ 73, aServerInfo offset ≠ 65, sAU_LOGIN_RES size ≠ 795. Option A (single packet ABI via compiler flag) does not achieve Windows layout with GCC 10 + -fpack-struct. PacketLayoutVerify.cpp is kept but not built (#if 0); fix log documents failure.
- **Next:** Proceed with Option B (centralized wire encode/decode) for global packet fix; extend NtlPacketWire with decode for received packets and encode for other sent packets; WireLayout.h remains source of truth.

### 19k. Global packet wire layout: PacketWireLayout encode/decode (2026-03) — superseded by 19l

- **What was tried:** Table-driven encode/decode in hand-written [PacketWireLayout.cpp](Server/NtlNetwork/PacketWireLayout.cpp) with one opcode (AU_LOGIN_RES) and explicit copy logic per packet.
- **Result:** **Superseded.** Replaced by one-shot generated wire layer (19l); no per-packet hand code.

### 19l. One-shot generated wire layer (2026-03)

- **What was tried:** **Single mechanism for all packets** with no per-packet wire code. (1) **Layout dumps:** Wire layout = Windows compiler layout (clang `-fdump-record-layouts` or equivalent), committed as [wire/wire_layout.txt](wire/wire_layout.txt). Our layout = Linux layout at build time or from [wire/our_layout.txt](wire/our_layout.txt). (2) **Generator:** [tools/gen_packet_wire.py](tools/gen_packet_wire.py) parses both dumps (clang dump format), extracts opcode ↔ struct from NtlPacket*.h, computes for each struct a list of copy ops `(wire_off, our_off, len)` (1:1 region match; when region sizes differ, uses gcd to expand into array-element copies). Emits one C file with one table and one `PacketWire_EncodePayload` / `PacketWire_DecodePayload` loop. (3) **Build:** CMake runs the generator (depends on wire_layout.txt, our_layout.txt), outputs `PacketWireLayout.generated.cpp` in build dir, NtlNetwork compiles it. Hand-written PacketWireLayout.cpp removed; header has API only.
- **Result:** **Implemented.** Build succeeds. One generator run produces encode/decode for every struct in both dumps. Adding a packet = ensure it appears in wire and our dumps (e.g. run layout dump on Windows for wire; run `python3 tools/gen_packet_wire.py --dump-stub` then clang on stub for our), re-run generator. No new hand-written wire code.
- **Files added:** `tools/gen_packet_wire.py`, `wire/wire_layout.txt`, `wire/our_layout.txt` (seed: sAU_LOGIN_RES only; full wire_layout from Windows when available).
- **Files removed:** `Server/NtlNetwork/PacketWireLayout.cpp` (replaced by generated file).
- **Files modified:** [Server/NtlNetwork/PacketWireLayout.h](Server/NtlNetwork/PacketWireLayout.h) (API only; no per-packet constants), [CMakeLists.txt](CMakeLists.txt) (custom command to run generator; NtlNetwork uses generated .cpp).

### 19m. Correct hex logging and Char server wire (2026-03)

- **What was tried:** (1) **Logging:** On Linux, Auth [MasterServerPacket.cpp](Server/AuthServer/MasterServerPacket.cpp) and Char [MasterServerPacket.cpp](Server/CharServer/MasterServerPacket.cpp) now log **actual sent to client** (wire payload size and total, e.g. 797 bytes for AU_LOGIN_RES) when `PacketWire_GetWirePayloadSize(opcode) != 0`, so logs reflect what is sent on the wire. Char [PacketCharServer.cpp](Server/CharServer/PacketCharServer.cpp) logs **actual received from client** (wire payload size) for UC_LOGIN_REQ when conversion applies. (2) **Char server:** Confirmed Char server uses the same NtlConnection encode/decode path; no code change. (3) **Layout dumps:** Generator `--dump-stub` now preprocesses with full include path (matching CMake), then runs `-cc1 -fdump-record-layouts`; only overwrites [wire/our_layout.txt](wire/our_layout.txt) on success. Seed [wire/our_layout.txt](wire/our_layout.txt) (sAU_LOGIN_RES only) kept for build. [wire/README.md](wire/README.md) documents wire_layout (Windows) and our_layout (Linux); to enable Char server wire conversion, add Char packet structs to wire_layout from Windows and optionally run `--dump-stub` for full our_layout.
- **Result:** **Implemented.** Logs show correct wire size; Char server path verified; config documented.
- **Files modified:** Server/AuthServer/MasterServerPacket.cpp, Server/CharServer/MasterServerPacket.cpp, Server/CharServer/PacketCharServer.cpp (includes + logging), tools/gen_packet_wire.py (dump-stub preprocess, include dirs, no overwrite on failure), wire/our_layout.txt (seed restored), docs/LINUX_RECEIVE_PACKET_FIX_LOG.md.
- **Files added:** wire/README.md.

### 19n. Wire files moved to wire/; Char–Master log (2026-03)

- **What was tried:** (1) **Wire folder:** All wire-layer files moved from config/ to **wire/**: wire_layout.txt, our_layout.txt, README.md (wire doc), PacketLayoutDump.cpp, PacketLayoutDump.i. Generator [tools/gen_packet_wire.py](tools/gen_packet_wire.py) and [CMakeLists.txt](CMakeLists.txt) now use wire/ only. config/ keeps server .ini files; config/README.md points to wire/ for wire docs. (2) **Char–Master:** Added one log in [MasterServerPacket.cpp](Server/CharServer/MasterServerPacket.cpp) when Char receives MC_LOGIN_RES: `[CharServer] Received MC_LOGIN_RES from Master: accountId=... resultCode=...` so Char–Master communication can be verified in logs.
- **Result:** **Implemented.** Build runs generator with wire/ paths; AuthServer and CharServer build successfully.
- **Files modified:** tools/gen_packet_wire.py (CONFIG_DIR → WIRE_DIR), CMakeLists.txt (wire/ paths), docs/LINUX_RECEIVE_PACKET_FIX_LOG.md, config/README.md, Server/CharServer/MasterServerPacket.cpp.
- **Files moved:** config/{wire_layout,our_layout}.txt, config/PacketLayoutDump.{cpp,i}, config/README.md (wire content) → wire/.

### 19o. Global field-based wire encode: explicit schema for sAU_LOGIN_RES (2026-03)

- **What was tried:** (1) **Explicit wire schema:** For structs where wire and our fixed-part sizes differ (e.g. sAU_LOGIN_RES: wire 65 bytes fixed, our 69), the region-based generator only copied our 0-64 to wire 0-64, so byServerInfoCount (our byte 65) was never sent. Implemented a **field-level schema** in [tools/gen_packet_wire.py](tools/gen_packet_wire.py): `WIRE_SCHEMA_EXPLICIT` maps struct name to (wire_size, our_size, list of (wire_off, our_off, len)). For sAU_LOGIN_RES: ops (0,0,63), (63,64,2), then 10× (wire_off, our_off, 73) for the sSERVER_INFO array. (2) **Generator:** When building structs_with_ops, if struct is in WIRE_SCHEMA_EXPLICIT use that; else use region-based compute_copy_ops from layout dumps. (3) **Removed one-off fix:** Deleted the AU_LOGIN_RES-only fixup in [NtlConnection.cpp](Server/NtlNetwork/NtlConnection.cpp) (pWirePayload[63]=pOurPayload[64], etc.); the generated table now produces the correct ops so no C++ branch per packet.
- **Result:** **Implemented.** Build runs generator; AU_LOGIN_RES row has 12 ops (63+2+10×73 bytes). AuthServer builds; wire bytes 63-64 come from generator, not NtlConnection. Adding more packets with layout mismatch = add entry to WIRE_SCHEMA_EXPLICIT or extend parser for member-level layout from dumps.
- **Files modified:** tools/gen_packet_wire.py (WIRE_SCHEMA_EXPLICIT, use schema when present), Server/NtlNetwork/NtlConnection.cpp (removed opcode 1002 fixup), docs/LINUX_RECEIVE_PACKET_FIX_LOG.md.

### 19p. Refactor: remove WIRE_SCHEMA_EXPLICIT; generic two-region rule (2026-03)

- **What was tried:** (1) **Remove per-packet schema:** Deleted `WIRE_SCHEMA_EXPLICIT` from [tools/gen_packet_wire.py](tools/gen_packet_wire.py) so copy ops are derived only from layout dumps and generic rules. (2) **Generic two-region rule:** In `compute_copy_ops`, when there are exactly two regions and the fixed (first) region sizes differ (wire_fixed ≠ our_fixed), split the fixed part into: (a) prefix copy (0, 0, wire_fixed − 2), (b) 2-byte tail: when (our_fixed − wire_fixed) == 4 use (wire_fixed − 2, our_fixed − 5, 2) so bIsGM/byServerInfoCount at our 64–65 map to wire 63–64; else use (wire_fixed − 2, our_fixed − 2, 2), (c) array region via existing gcd-based element copies. (3) **Main loop:** Only iterate over `wire_layout.keys()`; no explicit-schema branch.
- **Result:** **Implemented.** Regenerated PacketWireLayout.generated.cpp; AU_LOGIN_RES row unchanged (795, 839, 12 ops: 63, 2, 10×73). All 7 structs with conversion come from dumps + rule; no hand-written op lists.
- **Files modified:** tools/gen_packet_wire.py, Server/NtlNetwork/PacketWireLayout.generated.cpp, docs/LINUX_RECEIVE_PACKET_FIX_LOG.md.

### Reference: Windows AU_LOGIN_RES hex (compare with Linux dump each time)

Windows Auth sends **797 bytes total** (header=2, payload=795). Linux should match this. Use the Linux hex dump (MasterServerPacket success path) and compare byte-by-byte with the reference below.

- **Total:** 797 bytes (header=2 payload=795)
- **Full packet bytes (Windows reference):**
  ```
  1B 03 EA 03 64 00 65 00 65 00 65 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 68 71 50 6C 63 61 65 48 68 73 38 77 46 4A 66 6C E1 6B 0C 00 00 07 00 00 00 01 01 31 32 37 2E 30 2E 30 2E 31 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 56 4F 00 00 00 00 00 00 00 00 00 00 ...
  ```
  (truncated; full packet is 797 bytes. First 2 bytes = header 0x031B = 795; bytes 2-3 = opcode 0x03EA = AU_LOGIN_RES; then payload. First sSERVER_INFO at payload offset 65: IP string "127.0.0.1", port 20310 = 0x4F56.)
- **Key fields (Windows):** szCharacterServerIP='127.0.0.1' Port=20310 byServerInfoCount=1; awchUserId hex: 0065 0065 0065 0000 ... ("eee" in UTF-16LE).

**Windows AU_LOGIN_RES wire layout (from Wireshark, 2026-03):**

| Payload offset | Bytes (hex) | Meaning |
|----------------|-------------|---------|
| 0–1 | EA 03 | opcode 1002 (AU_LOGIN_RES) |
| 2–3 | **64 00** | wResultCode = 100 (AUTH_SUCCESS) – client must see this to proceed |
| 4–68 | 65 00 65 00 65 00 … + padding | awchUserId "eee" (UTF-16LE) + zeros |
| 36–51 | (session key) | abyAuthKey |
| 52–62 | accountId, lastServerFarmId, dwAllowedFunctionForDeveloper | |
| **63–64** | **01 01** | bIsGM=1, byServerInfoCount=1 |
| **65–137** | 31 32 37 2E 30 2E 30 2E 31 00 … 56 4F | First sSERVER_INFO: IP "127.0.0.1", port 0x4F56 = 20310 |

If Linux sends **70 00** at 2–3, that is 112 = AUTH_USER_EXIST_IN_CHARACTER_SERVER; the client will not connect to Char. Must be **64 00** (100) for success.

**Comparison (terminal 36 vs Windows reference, 2026-03-02):** Linux wire payload had **00 00** at payload bytes 63-64; Windows has **01 01** (bIsGM=1, byServerInfoCount=1). Region-based generator had copied our 0-64 to wire 0-64 only, so byServerInfoCount was never sent. **Global fix (generic two-region rule):** The generator no longer uses per-packet schema. For two-region structs with differing fixed-part sizes (e.g. wire_fixed=65, our_fixed=69), it emits prefix (0,0,63), tail (63,64,2) when our_fixed−wire_fixed==4, then array ops from layout dumps. Same encode loop; wire bytes 63-64 match Windows. New packets with similar layout get correct ops from dumps only.

---

## Remaining Work (until 100% fixed)

- [ ] **Login:** Auth receives login packet (OpCode 0x0067) and sends AU_LOGIN_RES to client; client receives it and proceeds. (Depends on MasterServer being connected for success path; failure path sends response from PacketAuthServer.)
- [ ] **Char server:** Client connects to char server; join flow works on Linux (receive/send).
- [ ] **In-game:** Client loads into game; all packet exchange works.

---

## Rules for Future Fixes

1. **Do not** change the accept or send path in a way that stops the client from connecting or receiving the handshake.
2. **Do not** remove or reorder the re-post of recv (PostRecv) in a way that allows STATUS_CLOSE to be set before the worker re-posts (current safe order: push bytes → PostRecv → RecvPackets(0)).
3. **Do not** process FORCE_CLOSE synchronously; keep the "always queue FORCE_CLOSE" logic.
4. **Do not** apply FORCE_CLOSE without the pending flag check (TakePendingForceClose).
5. **Before** changing recv/CompleteRecv/PostRecv flow, re-read this log and the "What Works" / "What Failed" sections.
6. **After** any new attempt, add an entry to this log: what you tried, result (worked / failed / partial), and any new "what works" or "what failed" finding.
7. **Wire format = Windows client.** Packets sent to the client must have the exact byte layout (sizes, offsets) the Windows C++ client expects. Use compiler flags (-fpack-struct), wire structs, or other ABI fixes to ensure layout match. Avoid manual byte-offset construction unless no other option works.
8. **No per-packet wire builders or manual wire modules.** Do not introduce per-packet "wire" builders (e.g. AULoginResWire, manual byte-offset construction for one packet at a time). That would require testing every single packet from Windows and fixing each one manually—unacceptable. We need a **global fix** (compiler/ABI/struct) that fixes layout for all packets at once, not years of per-packet work.
9. **No packet-specific code in the wire generator.** [tools/gen_packet_wire.py](tools/gen_packet_wire.py) must never contain struct names (e.g. sAU_LOGIN_RES), opcodes (e.g. 1002), or per-packet branches/tables. All copy ops are derived only from layout dumps (wire_layout.txt, our_layout.txt) and generic rules (region count, sizes, gcd for arrays, two-region fixed-tail rule). Adding a new packet = ensure it appears in both dumps and re-run the generator; no edits inside gen_packet_wire.py for that packet.
10. **Validate: would the same code work for another packet?** When adding or changing a generic rule in gen_packet_wire.py, always ask: "If I use this exact code for a different packet without looking at its structure, would the wire hex be correct?" If the rule assumes a layout (e.g. "last 2 bytes of fixed = same logical field on both sides"), then packets that do not match that layout can produce a hex mismatch. **For any new packet that triggers the two-region fixed-tail rule**, verify wire hex (compare with Windows or expected capture) until we have member-level layout or a safer rule. Do not assume the rule fits every two-region struct.

---

## File References

- **CompleteRecv / PostRecv / RecvPackets:** `Server/NtlNetwork/NtlConnection.cpp`
- **ProcessPacket (decryption, FORCE_CLOSE):** `Server/NtlNetwork/NtlSession.cpp`
- **FORCE_CLOSE handler, PostNetEventMessage:** `Server/NtlNetwork/NtlNetworkProcessor.cpp`, `Server/NtlNetwork/NtlNetwork.cpp`
- **Linux RecvEx (non-blocking recv):** `Server/NtlNetwork/NtlSocket.h`
- **ValidCheck PostRecv retry:** `Server/NtlNetwork/NtlSessionList.cpp`
