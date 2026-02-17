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
- **Result:** Pending user rebuild and repro; then interpret logs to fix root cause.

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

- **Symptom:** After client connects and sends 12 bytes, server logs: `[IOCP Worker] CompleteIO failed -> Close session. rc=100045, iomode=3`. Client disconnects; login packet not processed.
- **rc=100045:** `NTL_ERR_NET_SESSION_CLOSED` — returned by `PostRecv()` when `IsStatus(STATUS_ACTIVE)` is false.
- **Code in place:** CompleteRecv reorder (push bytes → PostRecv → RecvPackets(0)); FORCE_CLOSE deferred and always queued; FORCE_CLOSE handler only closes if `TakePendingForceClose()`; diagnostic log in PostRecv when !ACTIVE; extra logs: PostRecv "Traffic heavy" when that branch returns 100045, and CompleteRecv "Start" / "Before PostRecv" with status.
- **Next checks:** (1) Rebuild AuthServer and run again; (2) Reproduce login and check which of these appears: "[PostRecv] Session not ACTIVE", "[PostRecv] Traffic heavy", "[PostRecv] Connection closed (0 bytes)", and the two "[CompleteRecv] ... status=..." lines — use them to see which path returns 100045 and whether status is ACTIVE at start/before PostRecv; (3) Fix root cause based on that.

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
