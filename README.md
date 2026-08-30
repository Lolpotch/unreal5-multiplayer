# PKL Multiplayer — LAN C++ Demo

A minimal Unreal Engine 5.8 project that demonstrates online multiplayer in C++.
It is intentionally tiny: the goal is to *show the networking concepts working*
(server authority, property replication, RPCs), not to be a game.

The same gameplay code is transport-agnostic — moving from LAN to Steam or EOS
later only means adding a session layer (Create/Find/Join), not rewriting any of this.

---

## The mental model

One machine is the **server** (the authority). Every other machine is a **client**.
The server owns the truth; clients ask the server to change things, the server decides,
then the server replicates the result back to everyone so all screens agree.

Three engine features carry the whole demo:

| Feature | Direction | Purpose |
|---------|-----------|---------|
| `Replicated` property (`DOREPLIFETIME`) | server → all clients | Sync a value down |
| Server RPC (`UFUNCTION(Server, ...)`) | client → server | Send a request up |
| `HasAuthority()` | — | Guard so only the server mutates truth |

---

## Files

### `PKLGameMode` — the referee (server only)
- Sets `DefaultPawnClass = APKLCharacter` and `PlayerControllerClass = APKLPlayerController`.
- Exists only on the server. Clients never have a GameMode.
- On `PostLogin` (a player connects) the server spawns that player's pawn and
  offsets each new spawn sideways so players don't stack at the origin.

### `PKLCharacter` — the replicated pawn
Two kinds of synchronization live here:

**Movement (automatic).** `bReplicates = true` plus `SetReplicateMovement(true)`
lets `CharacterMovementComponent` sync position for us — no custom code.

**Colour (manual — the teaching part).**
```cpp
UPROPERTY(ReplicatedUsing = OnRep_BodyColor)
FLinearColor BodyColor;
```
- `Replicated` tells the engine to copy this value from server to clients.
- `ReplicatedUsing = OnRep_BodyColor` runs that function on a client whenever the
  new value arrives, where we repaint the cube.
- The property must also be registered in `GetLifetimeReplicatedProps` via
  `DOREPLIFETIME(APKLCharacter, BodyColor)` — without that line it never replicates.

The colour is applied through a **dynamic material instance** created in `BeginPlay`.
`ApplyBodyColor()` pushes `BodyColor` into the material's `Color` parameter.
(The cube uses `BasicShapeMaterial`, which exposes that parameter — the default
`Cube` material is the checkered WorldGridMaterial and has no colour parameter.)

### `PKLPlayerController` — the connection commands
Open the console with `~` and type:
- `HostLAN` — become a listen server on the current level (`OpenLevel(..., "listen")`).
- `JoinLAN 127.0.0.1` — connect to a host by IP (`ClientTravel`).

`BeginPlay` prints the local role (LISTEN SERVER / CLIENT / STANDALONE) on screen.

---

## The colour round-trip (the heart of the demo)

Pressing **C** produces this flow:

```
client presses C
  -> OnChangeColorPressed()        // runs locally on the client
  -> ServerRandomizeColor()        // Server RPC: request travels UP to the server
       -> BodyColor = random       // server changes the authoritative truth
       -> ApplyBodyColor()         // server repaints its own view immediately
  -> replication                   // value travels DOWN to every client
       -> OnRep_BodyColor()        // each client repaints its cube
```

A client is not allowed to recolour itself directly; it asks the server. The server
changes the replicated `BodyColor`, and the engine pushes that to all clients. That is
why the new colour appears in **every** window — which is the proof that replication works.

---

## Who runs what

| Action | Runs where |
|--------|-----------|
| GameMode / `PostLogin` / pawn spawn | server only |
| Movement sync | automatic, both directions |
| `ServerRandomizeColor` | always executes on the server |
| `OnRep_BodyColor` | clients only |
| `ApplyBodyColor` (inside the RPC) | server, manually |
| Input (WASD / C / Space) | local machine first |

---

## Controls

| Key | Action |
|-----|--------|
| W A S D | Move |
| Mouse | Look |
| Space | Jump |
| C | Randomize colour (replicated to all) |
| `~` then `HostLAN` / `JoinLAN <ip>` | Host / join |

---

## Running it

**Single PC (fastest):** Play dropdown → Number of Players = 2,
Net Mode = *Play As Listen Server* → Play. Two windows appear; move one and watch it
move in the other; press C and watch the colour change on both.

**Two builds / two PCs (real LAN):**
1. Launch two instances (Standalone).
2. In window 1 console: `HostLAN`
3. In window 2 console: `JoinLAN <host-LAN-IP>` (same PC: `JoinLAN 127.0.0.1`).

> The level needs a floor (with collision) and at least one PlayerStart above it,
> plus a light. Without a floor the pawns fall forever.

---

## Building

```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" ^
  PKLMultiplayerEditor Win64 Development ^
  -Project="<path>\PKLMultiplayer.uproject" -WaitMutex
```

Or use Live Coding in the editor (`Ctrl+Alt+F11`) after editing code.

---

## Packaging a native build (run without the editor)

Package the project so it runs as a standalone `.exe` on any Windows PC.

### From the editor (simplest)

1. Open the project in UE 5.8.
2. **Platforms → Windows → Build Configuration → Development.**
   Use **Development**, not Shipping: Development keeps the `~` console alive, which the
   `HostLAN` / `JoinLAN` commands need. Shipping disables the console (see the note below
   for how to host/join without it).
3. **Platforms → Windows → Package Project** and pick an output folder.

### From the command line (equivalent)

Run in PowerShell. Note the leading `&` call operator, and keep it on a single line —
PowerShell does **not** accept cmd's `^` line-continuation.

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="C:\Users\Chandra\Documents\Unreal Projects\PKLMultiplayer\PKLMultiplayer.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -allmaps -stage -pak -archive -archivedirectory="C:\Users\Chandra\Documents\Unreal Projects\PKLMultiplayer\Packaged"
```

### Output layout

```
<Build>\Windows\PKLMultiplayer.exe                                  <- launcher
<Build>\Windows\PKLMultiplayer\Binaries\Win64\PKLMultiplayer.exe    <- real binary (binds the network)
```

Copy the whole `Windows` folder to the other PC; it is self-contained.

---

## Running the native build on a LAN

### Same PC (quick check)

Launch `PKLMultiplayer.exe` twice. In one, open the console (`~`) and type `HostLAN`;
in the other, `JoinLAN 127.0.0.1`. This proves the server logic works.

### Two PCs (real LAN)

1. Both PCs on the **same subnet** (e.g. `192.168.1.11` and `192.168.1.12`), no guest
   Wi-Fi / AP isolation between them.
2. Host: run the `.exe`, open console (`~`), type `HostLAN`. Confirm the on-screen role
   reads **LISTEN SERVER**.
3. Client: run the `.exe`, console, `JoinLAN <hostIP>` (e.g. `JoinLAN 192.168.1.11`).

**Without the console** (also works in Shipping) — launch with command-line args instead:

```
PKLMultiplayer.exe /Game/Level1?listen     (host)
PKLMultiplayer.exe 192.168.1.11            (client — a bare IP arg auto-connects)
```

The listen server binds **UDP 7777** by default.

---

## Troubleshooting LAN connection

If the client cannot join, work through it in this order. This is exactly the path that
fixed it here — a passing `ping` does **not** prove the game port is open (ping is ICMP;
the game is UDP 7777).

### 1. Diagnose

```powershell
ipconfig                                          # IPv4 of both PCs — must share 192.168.1.x
ping 192.168.1.11                                 # client -> host reachability
netstat -an -p UDP | Select-String 7777           # on host: expect "UDP 0.0.0.0:7777" (listening)
Get-Process PKLMultiplayer* | Select-Object -ExpandProperty Path -Unique   # find the real exe
Get-NetFirewallRule -DisplayName "UE LAN 7777"    # confirm the port rule exists
```

### 2. Firewall (run as Administrator on the HOST)

`New-NetFirewallRule` silently does nothing without an elevated (Administrator) PowerShell.

```powershell
# allow ping (ICMP)
New-NetFirewallRule -DisplayName "Allow ICMP Ping" -Protocol ICMPv4 -IcmpType 8 -Direction Inbound -Action Allow

# allow the game port (UDP 7777)
New-NetFirewallRule -DisplayName "UE LAN 7777" -Direction Inbound -Protocol UDP -LocalPort 7777 -Action Allow

# allow the two exes per-application (adjust paths to your build)
New-NetFirewallRule -DisplayName "UE PKL App1" -Direction Inbound -Program "C:\...\Windows\PKLMultiplayer\Binaries\Win64\PKLMultiplayer.exe" -Action Allow -Profile Any
New-NetFirewallRule -DisplayName "UE PKL App2" -Direction Inbound -Program "C:\...\Windows\PKLMultiplayer.exe" -Action Allow -Profile Any
```

### 3. The gotcha: a BLOCK rule beats an ALLOW rule

If you ever dismissed the Windows Firewall pop-up when the game first launched, Windows
created a per-application **block** rule for the exe. A block rule overrides any port or
program allow rule, so the connection fails even though everything above looks correct.
Find and remove it:

```powershell
# list any block rules targeting the exe
Get-NetFirewallRule -Action Block -Enabled True | Get-NetFirewallApplicationFilter | Where-Object Program -match "PKLMultiplayer" | Select-Object Program

# remove them
Get-NetFirewallRule -Action Block -Enabled True | Where-Object { ($_ | Get-NetFirewallApplicationFilter).Program -match "PKLMultiplayer" } | Remove-NetFirewallRule
```

### 4. Confirm the firewall is the culprit

Temporarily disable the firewall on the **host only** and test the join. If it connects,
the problem is a firewall rule — go back to steps 2–3.

```powershell
Set-NetFirewallProfile -Profile Private -Enabled False   # test with firewall off
Set-NetFirewallProfile -Profile Private -Enabled True    # MUST turn it back on afterwards
```