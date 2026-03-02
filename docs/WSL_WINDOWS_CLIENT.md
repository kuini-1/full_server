# Server in WSL, Client on Windows

When the **server runs in WSL2** and the **game client runs on Windows**, the client must be able to reach the Char server (and Auth) at the IP and port sent in AU_LOGIN_RES.

**Working Windows reference (for compare):** `E:\SERVER\full_server - 2-7-2026` (config 127.0.0.1; no wire layer; same handshake).

## Ports and what exposes Auth / Char (WSL and Windows)

From the current config:

| Service   | Config (Address / Port) | Client connects to        | How client reaches it |
|-----------|--------------------------|----------------------------|------------------------|
| **Auth**  | AuthServer.ini: Address=172.28.112.18, Port=20210 | 172.28.112.18:20210 | Direct: Windows can connect to the WSL IP. No port forwarding needed if the client uses 172.28.112.18. |
| **Char**  | CharServer.ini: Address, PublicAddress=172.28.112.18, Port=20310 | 172.28.112.18:20310 | Same: direct to WSL. Test-NetConnection to 172.28.112.18:20310 should succeed from Windows. |

**What you may have done to allow the client to connect to Auth:**

- Set **Auth Server** in `config/AuthServer.ini` to **Address = 172.28.112.18** (your WSL IP) and **Port = 20210**. The client then connects to that IP:port. Windows can reach WSL at 172.28.112.18 without any extra setup (WSL2 virtual network).
- Alternatively, you might have used **Windows port forwarding** (e.g. `netsh interface portproxy add v4tov4 listenaddress=127.0.0.1 listenport=20210 connectaddress=172.28.112.18 connectport=20210`) so the client connects to 127.0.0.1:20210 and Windows forwards to WSL. In that case Auth would need to advertise 127.0.0.1 (or the client would be configured to use 127.0.0.1). If you are not using port forwarding, the client must use 172.28.112.18 for Auth (and Char).

**Check listening ports in WSL:**

```bash
ss -tlnp | grep -E '20210|20310'
```

You should see Auth on 20210 and Char on 20310 (on 0.0.0.0 or 172.28.112.18 depending on config). No extra firewall rule is usually needed for WSL; the Windows host can connect to the WSL IP.

## Current setup (from config)

- **CharServer.ini:** `Address = 0.0.0.0`, `Port = 20310`, `PublicAddress` (see below).
- The server **listens** in WSL on all interfaces (`0.0.0.0:20310`). The **client is told** to connect to `PublicAddress:Port`.

**PublicAddress:**
- **127.0.0.1** – Use when the client connects via **wslrelay** (or another port relay). wslrelay listens on `127.0.0.1:20310` on Windows and forwards to WSL; the server must advertise `127.0.0.1` so the client connects to the relay. Ensure wslrelay is configured to forward port 20310 to WSL’s Char server (e.g. to `172.28.112.18:20310` or current WSL IP).
- **172.28.112.18** (or current WSL IP) – Use for **direct** client→WSL; no relay. Client connects straight to WSL. Requires `Test-NetConnection -ComputerName 172.28.112.18 -Port 20310` to succeed from Windows.

WSL2 gives your distro an IP on a virtual network (e.g. `172.28.112.18`). The Windows host can usually reach that IP.

## Check from WSL (server side)

From a terminal in WSL (e.g. in Cursor):

```bash
# WSL IP (should match PublicAddress in CharServer.ini)
hostname -I | awk '{print $1}'

# Is Char server listening?
ss -tlnp | grep 20310
```

If the first line matches `PublicAddress` and the second shows `CharServer` listening on `0.0.0.0:20310`, the server side is correct.

## Check from Windows (client reachability)

Run this in **PowerShell on Windows** (not in WSL). Replace the IP/port if you changed config.

```powershell
# Test TCP connection to Char server (WSL)
Test-NetConnection -ComputerName 172.28.112.18 -Port 20310
```

- **TcpTestSucceeded : True** → Windows can reach the Char server; the client should be able to connect.
- **TcpTestSucceeded : False** → Windows cannot reach that IP:port. Common causes:
  - WSL2 IP changed (restart WSL or run `hostname -I` in WSL and update `PublicAddress` in `config/CharServer.ini` to match).
  - Firewall on Windows or WSL blocking the port.
  - Using a different network (VPN, another machine); then the client must use an IP reachable from the client (e.g. your LAN IP if client is on another PC).

### Using wslrelay (127.0.0.1)

If **wslrelay.exe** is listening on `127.0.0.1:20310` on Windows, it acts as a relay: the client connects to `127.0.0.1:20310`, and wslrelay should forward that to the Char server in WSL. For this to work:

1. **CharServer.ini:** set `PublicAddress = 127.0.0.1` so the server tells the client to connect to 127.0.0.1.
2. **wslrelay** must be configured to forward port **20310** to WSL (e.g. to `172.28.112.18:20310` or your current WSL IP). Check wslrelay’s config or documentation (e.g. which ports it relays, and to which WSL address).
3. Restart Char server after changing `PublicAddress` so it re-registers with Master and Auth sends the new address in AU_LOGIN_RES.

After login, run `netstat -ano | findstr 20310` on Windows: you should see an **ESTABLISHED** connection to `127.0.0.1:20310` when the client connects to Char, and the Char server terminal in WSL should show `[CharServer] Client connection accepted`.

## Summary

| Where        | What to check |
|-------------|----------------|
| **WSL**     | `hostname -I` matches `PublicAddress`; `ss -tlnp \| grep 20310` shows CharServer listening. |
| **Windows** | `Test-NetConnection -ComputerName 172.28.112.18 -Port 20310` → TcpTestSucceeded : True. |

If both pass and the client still doesn’t reach Char, the problem is likely in the client or in the packet flow (e.g. wrong IP/port in AU_LOGIN_RES payload).

## Troubleshooting: "Nothing on Char server" after login

If Test-NetConnection succeeds but the Char server terminal never shows a client (no `[CharServer] Client connection accepted` or `[Acceptor] OnAccepted`):

**1. Confirm the client actually connects to 172.28.112.18:20310**

On Windows, while the client is running and you try to connect to Char after login, run:

```powershell
netstat -ano | findstr 20310
```

- If you see a line with `172.28.112.18:20310` and `ESTABLISHED`, the client is connecting (you may have multiple Char servers; check you're watching the right one).
- If you see nothing with `20310`, the client is not opening a connection to that port. The client may be using a different server list (config/launcher/cached IP) and ignoring the IP/port from AU_LOGIN_RES, or never attempting to connect to Char.

**2. Confirm what Auth sent**

Auth log should show: `szCharacterServerIP='172.28.112.18' Port=20310`. That is what the client is supposed to use. If the client uses its own config instead, it may connect elsewhere.

**3. Only one Char server**

Ensure only one CharServer process is running. If two are running, the client might connect to the other terminal.

**4. Client must use the address from the login packet**

Some clients read the character server from a config file and ignore AU_LOGIN_RES. Then either set the client config to `172.28.112.18` and port `20310`, or set the server's `PublicAddress` to whatever the client is configured to use.

## Test Char connection from Windows (PowerShell)

To confirm that Char server accepts a TCP connection (without using the game client):

**Option A – Use the launcher (no execution policy change):**  
Double-click **`docs/run_test_char_connection.cmd`** (or run it from cmd). It runs the PowerShell script with `-ExecutionPolicy Bypass` and then pauses.

**Option B – From PowerShell:**  
If you get "running scripts is disabled", either run once:  
`Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned`  
or run the script with bypass (use the real path to your repo’s `docs` folder):

```powershell
powershell -ExecutionPolicy Bypass -File "C:\path\to\server\docs\test_char_connection.ps1"
```

Or from the `docs` folder after `cd` there:

```powershell
.\test_char_connection.ps1
```

Or paste the one-liner (replace path if needed):

```powershell
$c = New-Object System.Net.Sockets.TcpClient("172.28.112.18", 20310); $s = $c.GetStream(); $b = New-Object byte[] 32; $r = $s.Read($b, 0, 32); $c.Close(); Write-Host "Received $r bytes:" (($b[0..($r-1)] | % { "{0:X2}" -f $_ }) -join " ")
```

If it works, you get "Received 6 bytes: 03 00 AC 86 F5 74" (the handshake) and the **Char server terminal in WSL** will show `[CharServer] Client connection accepted`. That proves the path from Windows to Char is open; if the game client still does not show up, the client is not connecting to 172.28.112.18:20310.
