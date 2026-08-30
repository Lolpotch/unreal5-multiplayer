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