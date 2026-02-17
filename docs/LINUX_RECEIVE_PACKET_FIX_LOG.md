# Linux Receive Packet Fix Log

**Purpose:** Track every attempt to fix "receive packet" on Linux so it behaves like Windows, and avoid breaking the client connection. Do not repeat failed approaches.

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

---

## What Failed / Breaks Connection

- **Setting STATUS_CLOSE in ProcessPacket** (before worker calls PostRecv) → worker sees !ACTIVE → 100045 → worker closes session. So: never set STATUS_CLOSE in the recv/dispatch path before the worker has re-posted recv (hence reorder + FORCE_CLOSE deferral).
- **Processing FORCE_CLOSE synchronously** when `m_bDirectProcess` is true → same as above.
- **Applying FORCE_CLOSE to a session that didn’t request it** (reused pointer) → new session marked CLOSE → 100045. Hence pending flag and `TakePendingForceClose()` in handler.

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

---

## File References

- **CompleteRecv / PostRecv / RecvPackets:** `Server/NtlNetwork/NtlConnection.cpp`
- **ProcessPacket (decryption, FORCE_CLOSE):** `Server/NtlNetwork/NtlSession.cpp`
- **FORCE_CLOSE handler, PostNetEventMessage:** `Server/NtlNetwork/NtlNetworkProcessor.cpp`, `Server/NtlNetwork/NtlNetwork.cpp`
- **Linux RecvEx (non-blocking recv):** `Server/NtlNetwork/NtlSocket.h`
- **ValidCheck PostRecv retry:** `Server/NtlNetwork/NtlSessionList.cpp`
