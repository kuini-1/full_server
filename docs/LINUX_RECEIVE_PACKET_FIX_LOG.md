# Linux Receive Packet Fix Log

**Purpose:** Track every attempt to fix "receive packet" on Linux so it behaves like Windows, and avoid breaking the client connection. Do not repeat failed approaches.

**Current goal:** Client connects → handshake sent → client sends 12-byte login packet → server receives it, parses it, and processes login (no disconnect with rc=100045).

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

- **Symptom (resolved):** After client connects and sends 12 bytes, server was closing with rc=100045 because the RECV completion was posted with **0** as the byte count.
- **Root cause (fix 7):** PostRecv (Linux) used the 2-arg `PostIocpEventMessage(this, &m_recvContext)`, so the IOCP worker got `CompleteRecv(0)` and returned 100045 at the remote-close check.
- **Code in place:** PostRecv now calls `PostIocpEventMessage(dwTransferedBytes, this, &m_recvContext)` so the worker gets the real byte count; reorder (push → PostRecv → RecvPackets(0)); FORCE_CLOSE deferred and pending-flag check; optional diagnostics in CompleteRecv/PostRecv.
- **Next:** Rebuild AuthServer, test login; if it works, optionally reduce or remove the verbose [CompleteRecv] diagnostic prints.

### 8. Receive working; cleanup and NULL guard

- **Result:** After fix 7, receive works: packets (OpCode 0x0004, 0x0001, 0x0067, etc.) are received and processed. Diagnostic logs showed `status=4` = STATUS_ACTIVE (enum: INIT=0, CREATE=1, ACCEPT=2, CONNECT=3, ACTIVE=4, CLOSE=5).
- **Cleanup:** Removed the two verbose `[CompleteRecv] Start` / `Before PostRecv` logs to reduce noise.
- **NULL m_hEventIOCP:** Logs showed "(NULL == m_hEventIOCP)" once during receive. In `PostNetEvent()`, added early return when `m_hEventIOCP` is NULL (log message and return NTL_FAIL) so we don’t call `PostQueuedCompletionStatus` with NULL. If this appears often, ensure every network’s processor has `Create()` called before any session posts events.

---

### 9. Lazy-create dispatcher IOCP when NULL (Linux)

- **Symptom:** "(NULL == m_hEventIOCP)" appeared repeatedly when client received data; NETEVENT_RECV was dropped so the 87-byte login packet (and others) were never dispatched to ProcessPacket.
- **Fix:** On Linux only, in `PostNetEvent()` when `m_hEventIOCP` is NULL, call `Create()` once under a static mutex so the dispatcher IOCP is created; then post the event to it. The dispatcher thread will use the new handle on its next loop and process events. After rebuild you should see "[NetworkProcessor] Lazy-created dispatcher IOCP (was NULL)" once, then login and other packets processed normally.

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
