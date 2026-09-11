# PKL Multiplayer — LAN C++ Demo

Tiny Unreal Engine 5.8 project. Show multiplayer work in C++.

Goal: prove networking concepts. Not a game.
- server authority
- property replication
- RPCs

Same gameplay code transport-agnostic. LAN to Steam/EOS later = add session layer (Create/Find/Join). No rewrite.

---

## Mental Model

- one machine = **server** = boss = truth
- other machines = **clients**
- client want change to truth: ask server
- server decide, change truth, send result to everyone
- all screens agree

Three engine features do whole demo:

| Feature | Direction | Job |
|---------|-----------|-----|
| `Replicated` prop (`DOREPLIFETIME`) | server to clients | sync value down |
| Server RPC (`UFUNCTION(Server, ...)`) | client to server | send request up |
| `HasAuthority()` | — | guard: only server change truth |

---

## Files

### `PKLGameMode` — referee (server only)
- set `DefaultPawnClass = APKLCharacter`, `PlayerControllerClass = APKLPlayerController`
- lives on server only. clients no GameMode.
- `PostLogin` (player connect): server spawn pawn, offset sideways so no stack at origin

### `PKLCharacter` — replicated pawn
Two sync types:

**Movement (auto).**
- `bReplicates = true` + `SetReplicateMovement(true)`
- `CharacterMovementComponent` sync position. no custom code.

**Colour (manual — teaching part).**
```cpp
UPROPERTY(ReplicatedUsing = OnRep_BodyColor)
FLinearColor BodyColor;
```
- `Replicated` = engine copy value server to clients
- `ReplicatedUsing = OnRep_BodyColor` = run function on client when new value arrive. repaint cube.
- must register in `GetLifetimeReplicatedProps`: `DOREPLIFETIME(APKLCharacter, BodyColor)`
- no that line = never replicate

Colour via **dynamic material instance**, made in `BeginPlay`.
- `ApplyBodyColor()` push `BodyColor` into material `Color` param
- cube use `BasicShapeMaterial` (has that param)
- default `Cube` material = checkered WorldGridMaterial = no colour param

### `PKLPlayerController` — connection commands
Open console `~`, type:
- `HostLAN` — become listen server on current level (`OpenLevel(..., "listen")`)
- `JoinLAN 127.0.0.1` — connect to host IP (`ClientTravel`)

`BeginPlay` print local role on screen: LISTEN SERVER / CLIENT / STANDALONE.

---

## Colour Round-Trip (heart of demo)

Press **C**:

```
client press C
  -> OnChangeColorPressed()        // local on client
  -> ServerRandomizeColor()        // Server RPC: request go UP to server
       -> BodyColor = random       // server change truth
       -> ApplyBodyColor()         // server repaint own view now
  -> replication                   // value go DOWN to every client
       -> OnRep_BodyColor()        // each client repaint cube
```

- client not allowed recolour self direct. must ask server.
- server change replicated `BodyColor`, engine push to all clients
- new colour appear in **every** window = replication work

---

## Who Runs What

| Action | Where |
|--------|-------|
| GameMode / `PostLogin` / pawn spawn | server only |
| Movement sync | auto, both ways |
| `ServerRandomizeColor` | always on server |
| `OnRep_BodyColor` | clients only |
| `ApplyBodyColor` (inside RPC) | server, manual |
| Input (WASD / C / Space) | local machine first |

---

## Controls

| Key | Action |
|-----|--------|
| W A S D | move |
| Mouse | look |
| Space | jump |
| C | randomize colour (replicated to all) |
| `~` then `HostLAN` / `JoinLAN <ip>` | host / join |

---

## Run It

**Single PC (fastest):**
- Play dropdown → Number of Players = 2
- Net Mode = *Play As Listen Server* → Play
- two windows. move one, watch other move. press C, colour change both.

**Two PCs (real LAN):**
1. launch two instances (Standalone)
2. window 1 console: `HostLAN`
3. window 2 console: `JoinLAN <host-LAN-IP>` (same PC: `JoinLAN 127.0.0.1`)

> Level need floor (with collision) + one PlayerStart above it + light.
> No floor = pawns fall forever.

---

## Build

```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" ^
  PKLMultiplayerEditor Win64 Development ^
  -Project="<path>\PKLMultiplayer.uproject" -WaitMutex
```

Or Live Coding in editor (`Ctrl+Alt+F11`) after edit code.

---

## Package Native Build (run without editor)

Package so it run as standalone `.exe` on any Windows PC.

### From editor (simplest)
1. open project in UE 5.8
2. **Platforms → Windows → Build Configuration → Development**
   - use **Development**, not Shipping
   - Development keep `~` console alive. `HostLAN` / `JoinLAN` need it.
   - Shipping kill console (see note below to host/join without it)
3. **Platforms → Windows → Package Project** → pick output folder

### From command line (same thing)
Run in PowerShell. Note leading `&` call operator. Keep on one line — PowerShell no accept cmd `^` continuation.

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="C:\Users\Chandra\Documents\Unreal Projects\PKLMultiplayer\PKLMultiplayer.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -allmaps -stage -pak -archive -archivedirectory="C:\Users\Chandra\Documents\Unreal Projects\PKLMultiplayer\Packaged"
```

### Output layout
```
<Build>\Windows\PKLMultiplayer.exe                                  <- launcher
<Build>\Windows\PKLMultiplayer\Binaries\Win64\PKLMultiplayer.exe    <- real binary (bind network)
```

Copy whole `Windows` folder to other PC. Self-contained.

---

## Run Native Build on LAN

### Same PC (quick check)
- launch `PKLMultiplayer.exe` twice
- one: console `~`, type `HostLAN`
- other: `JoinLAN 127.0.0.1`
- proves server logic work

### Two PCs (real LAN)
1. both PCs on **same subnet** (e.g. `192.168.1.11`, `192.168.1.12`). no guest Wi-Fi / AP isolation.
2. host: run `.exe`, console `~`, `HostLAN`. on-screen role must read **LISTEN SERVER**.
3. client: run `.exe`, console, `JoinLAN <hostIP>` (e.g. `JoinLAN 192.168.1.11`)

**No console** (also work in Shipping) — launch with command-line args:
```
PKLMultiplayer.exe /Game/Level1?listen     (host)
PKLMultiplayer.exe 192.168.1.11            (client — bare IP arg auto-connect)
```

Listen server bind **UDP 7777** by default.

---

## Troubleshoot LAN Connection

Client cannot join? Work in this order.
Passing `ping` does **not** prove game port open (ping = ICMP; game = UDP 7777).

### 1. Diagnose
```powershell
ipconfig                                          # IPv4 of both PCs — must share 192.168.1.x
ping 192.168.1.11                                 # client -> host reach
netstat -an -p UDP | Select-String 7777           # on host: expect "UDP 0.0.0.0:7777" (listening)
Get-Process PKLMultiplayer* | Select-Object -ExpandProperty Path -Unique   # find real exe
Get-NetFirewallRule -DisplayName "UE LAN 7777"    # confirm port rule exist
```

### 2. Firewall (run as Administrator on HOST)
`New-NetFirewallRule` silently do nothing without elevated (Administrator) PowerShell.

```powershell
# allow ping (ICMP)
New-NetFirewallRule -DisplayName "Allow ICMP Ping" -Protocol ICMPv4 -IcmpType 8 -Direction Inbound -Action Allow

# allow game port (UDP 7777)
New-NetFirewallRule -DisplayName "UE LAN 7777" -Direction Inbound -Protocol UDP -LocalPort 7777 -Action Allow

# allow two exes per-application (adjust paths to your build)
New-NetFirewallRule -DisplayName "UE PKL App1" -Direction Inbound -Program "C:\...\Windows\PKLMultiplayer\Binaries\Win64\PKLMultiplayer.exe" -Action Allow -Profile Any
New-NetFirewallRule -DisplayName "UE PKL App2" -Direction Inbound -Program "C:\...\Windows\PKLMultiplayer.exe" -Action Allow -Profile Any
```

### 3. The Gotcha: BLOCK rule beats ALLOW rule
- ever dismissed Windows Firewall pop-up on first launch? Windows made per-app **block** rule for the exe.
- block rule override any port/program allow rule. connection fail even though everything above look correct.
- find and remove:

```powershell
# list block rules targeting exe
Get-NetFirewallRule -Action Block -Enabled True | Get-NetFirewallApplicationFilter | Where-Object Program -match "PKLMultiplayer" | Select-Object Program

# remove them
Get-NetFirewallRule -Action Block -Enabled True | Where-Object { ($_ | Get-NetFirewallApplicationFilter).Program -match "PKLMultiplayer" } | Remove-NetFirewallRule
```

### 4. Confirm Firewall Is Culprit
Disable firewall on **host only**, test join. Connect = firewall rule problem, back to steps 2–3.

```powershell
Set-NetFirewallProfile -Profile Private -Enabled False   # test with firewall off
Set-NetFirewallProfile -Profile Private -Enabled True    # MUST turn back on after
```