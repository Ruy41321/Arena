# Match Flow Architecture

How a 1v1 arena duel is orchestrated, replicated and presented: `AMKHGameMode` (authoritative rules),
`AMKHGameState` (replicated state + event hub) and `UMatchOverWidget` (end-of-match panel and rematch ballot).

## Table of Contents

1. [Overview](#overview)
2. [Match Phases](#match-phases)
3. [AMKHGameMode — The Authoritative Orchestrator](#amkhgamemode--the-authoritative-orchestrator)
4. [AMKHGameState — The Replicated Hub](#amkhgamestate--the-replicated-hub)
5. [UMatchOverWidget — The End-of-Match Panel](#umatchoverwidget--the-end-of-match-panel)
6. [Full Match Walkthrough](#full-match-walkthrough)
7. [Extending the System](#extending-the-system)
8. [Known Limits & Future Work](#known-limits--future-work)

---

## Overview

The system splits along a single line: **the game mode decides, the game state publishes, the widgets react.**

| Concern | Lives in | Exists on |
|---|---|---|
| Match rules, timers, scoring, drains, rematch arbitration | `AMKHGameMode` | Server only |
| Phase + countdown, scoreboard, rematch ballot, Blueprint events | `AMKHGameState` | Every peer (replicated) |
| Result presentation, ballot feedback, focus handling | `UMatchOverWidget` / `AMainHUD` | Owning client only |

The reason the game mode never talks to UI directly is that `AGameMode` **does not exist on clients**. Any
state a widget needs must travel through the game state, which is why every setter on `AMKHGameState`
(`SetMatchPhase`, `SetScore`, `SetRematchVotes`) is authority-guarded and paired with a replicated property.

The complementary invariant: **the game state never decides anything.** It stores what the server pushed
and fans it out as events. All branching (who scored, whether the match is over, whether a rematch starts)
happens in the game mode.

---

## Match Phases

`EMKHMatchPhase` (declared in `MKHGameState.h`) drives which timer runs and which rules are active.

| Phase | Countdown | Meaning |
|---|---|---|
| `None` | — | Match not started; players still joining |
| `InitialPreparation` | `InitialPreparationDuration` (60s) | Equip phase before the first duel |
| `Duel` | `DuelDuration` (180s) | Active fight |
| `SuddenDeath` | none | Duel timer expired; a drain effect ticks both players down |
| `IntermissionPreparation` | `IntermissionPreparationDuration` (10s) | Short re-prep between rounds |
| `MatchOver` | none | A player reached `ScoreToWin` (3), or both did (draw) |

```
InitialPreparation ──> Duel ──(timer)──> SuddenDeath
                        │                      │
                        └──(death)──┬──────────┘
                                    v
                          [DeathResolveDelay]
                                    v
                              ResolveRound
                              /          \
                    score < ScoreToWin   score reached
                            v                  v
              IntermissionPreparation      MatchOver
                            │                  │
                            └──> Duel          └──(ballot passes)──> InitialPreparation
```

---

## AMKHGameMode — The Authoritative Orchestrator

### Participant registration

Registration hooks `RestartPlayer` rather than `PostLogin`: after `Super::RestartPlayer` the pawn is
spawned *and* possessed, which is the first moment `Controller->GetPawn()` is guaranteed valid.

`RegisterParticipant` is idempotent (it early-outs on an already-registered character) and fills the first
free slot of the fixed two-element `Players` array. Each slot caches:

| Field | Role |
|---|---|
| `PlayerCharacter` | The participant |
| `Score` | Rounds won |
| `SpawnTransform` | Where the round reset teleports them back to |
| `SuddenDeathDrainHandle` | Active drain, so it can be removed surgically |
| `bDiedThisRound` | Guards double-counting a death within one round |

Registration also mirrors the slot onto the game state (`GS->RegisterParticipant`) and binds the death
event via `ASC->GenericGameplayEventCallbacks.FindOrAdd(Event::Death)`, passing the character as a bound
payload so the handler resolves the slot without re-casting the event instigator.

`TryStartMatch` fires the moment both slots are filled and the phase is still `None`.

### Phase scheduling

Every phase follows the same three-line shape: `SetPhase(...)`, `ClearPhaseTimer()`, then schedule the next
transition on the single shared `PhaseTimerHandle`. One handle for every phase *and* for the death-resolve
delay is deliberate — at most one match-flow timer is ever pending, so cancelling is unambiguous.

`SetPhase` writes the local authoritative `MatchPhase` and mirrors it (with the countdown length) onto the
game state in one call, keeping the two from drifting.

### Death handling and the double-KO case

`HandlePlayerDeath` only reacts during `Duel` or `SuddenDeath`. The subtle part is that it deliberately
**does not change phase on the first death**, so a second death arriving in the same frame still passes the
phase guard and still awards its point — a genuine double KO scores for both players.

The first death instead:

1. sets `bRoundResolving` (re-entrancy guard),
2. clears the phase timer,
3. schedules drain removal **on the next tick**, not immediately,
4. schedules `ResolveRound` after `DeathResolveDelay` (3s) so the death animation can play.

Step 3 is the non-obvious one. During sudden death both players are draining on identical periodic
executions; removing the drains synchronously on the first death would cancel the pending execution on the
second player, so two players on equal health would resolve as a clean win for whoever's periodic tick
happened to land first. Deferring by one tick lets both drains finish the frame, turning that race into the
draw it actually is.

### Round resolution and match end

`ResolveRound` ends the match once either score reaches `ScoreToWin`; **equal scores at that point are a
draw**, which is how a double KO on match point is reported. Otherwise it resets both participants
(`ResetParticipantsForNewRound`: teleport to `SpawnTransform`, apply `RoundResetEffect`, forced transition
back to `Idle`) and enters the intermission.

The forced `Multicast_TransitionToMovementState(Idle, true)` is what revives the loser: `UDeadState`
refuses every ordinary transition (`CanTransitionTo` returns `false`), so only a forced transition can leave
it. Its `ExitState` restores capsule collision and hands gameplay input back to the owning client.

### The rematch ballot

`RequestRematch` is a **per-player ballot**, not a counter: votes land in a `TSet<APlayerController*>`, so
one player hammering the button cannot restart the duel alone. The threshold
(`CountRematchVotersNeeded`) is recomputed on every vote from `GameState->PlayerArray.Num()` — counted
from player states rather than from live pawns, because at match over one participant is dead and whether
its pawn is still possessed must not shift the target. It is clamped to a minimum of 1 so an empty ballot
can never auto-pass.

When the ballot passes, `RestartMatchInPlace` runs instead of `AGameMode::RestartGame`:

> No server travel means no reconnection, no reloaded level and no rebuilt HUD — the rematch is just a
> phase change back to `InitialPreparation`, which every peer already knows how to react to.

It closes the ballot, zeroes both scoreboards (local *and* replicated), clears `bRoundResolving`, the phase
timer and any lingering drain, revives both participants, then re-enters the initial preparation phase.

> The pause menu's "Restart" is the other path and is deliberately different: `Server_RestartMatch` calls
> `AGameMode::RestartGame`, a full unilateral level restart with no ballot.

---

## AMKHGameState — The Replicated Hub

### Atomic snapshots

The three replicated properties are structs, not loose fields:

| Property | Contents | Why grouped |
|---|---|---|
| `DuelState` | `Phase`, `PhaseEndServerTime`, `PhaseDuration` | One `OnRep` delivers phase and timing together, so a client can never render a new phase paired with the previous phase's countdown |
| `PlayerDatas` | Per-slot character + score | Single array replication for the whole scoreboard |
| `RematchVotes` | `CurrentVotes`, `RequiredVotes` | Prevents rendering "2/0" while the second field is still stale |

Each setter calls `ForceNetUpdate()` so UI-critical transitions reach clients on the next net tick instead
of waiting for the actor's normal cadence.

### The authority broadcast rule

`OnRep_*` never runs on the server, so every authority-side setter broadcasts its event **manually** after
mutating state (`SetMatchPhase` → `HandlePhaseChanged()`, `SetRematchVotes` → `OnRematchVotesChanged`).
This is why the listen-server host sees the same events as a remote client. Forgetting this call is the
classic way to make a feature that works on a client and silently does nothing on the host.

### Client-side countdown

The countdown is **not** replicated per frame. The server publishes a single deadline in server time
(`PhaseEndServerTime = GetServerWorldTimeSeconds() + Duration`), and each client computes
`GetPhaseTimeRemaining()` locally every frame against its own `GetServerWorldTimeSeconds()` (which
`AGameStateBase` already keeps synchronized). The result is a smooth timer with zero per-frame bandwidth.
`GetPhaseElapsedFraction()` derives the normalized progress for bars from the same two numbers.

### Diffing the scoreboard

A single `OnRep_PlayerDatas` covers any change to the array, so the callback diffs against
`PreviousPlayerDatas` to tell a *registration* (character pointer changed) from a *score update* (score
changed) and fires only the matching event. The snapshot is refreshed at the end of the callback.

### Late binders

`BroadcastExistingPlayerDatas()` re-fires `OnParticipantRegistered` and `OnScoreChanged` for every filled
slot. Any widget created after registration should call it right after binding, otherwise it renders an
empty scoreboard until the next change.

### Event surface

| Event | Fired when | Typical listener |
|---|---|---|
| `OnMatchPhaseChanged` | Any phase change | Generic phase-driven UI |
| `OnInitialPreparationStarted` | Match start **and every rematch** | `AMainHUD::OnMatchRestarted` (tears the panel down) |
| `OnDuelStarted` | Each duel begins | Round banners |
| `OnDuelTimeOver` | Sudden death begins | Warning VFX/SFX |
| `OnIntermissionPreparationStarted` | Between rounds | Prep banners |
| `OnParticipantRegistered` / `OnScoreChanged` | Scoreboard changes | `W_MatchInfo` |
| `OnPlayerDied` | Multicast on each death | Kill feed |
| `OnMatchOver` | Match decided | `AMainHUD::OnMatchOver` |
| `OnRematchVotesChanged` | Ballot changes | `UMatchOverWidget` |

`OnPlayerDied` and `OnMatchOver` are `NetMulticast` RPCs rather than `OnRep`s because they are *moments*,
not state: a client joining later should not replay them.

---

## UMatchOverWidget — The End-of-Match Panel

### Lifecycle

The panel is owned by `AMainHUD`, which binds both ends of its life at `BeginPlay`:

```
GameState->OnMatchOver              ──> AMainHUD::OnMatchOver     ──> CreateMatchOverWidget + ShowMatchResult
GameState->OnInitialPreparationStarted ──> AMainHUD::OnMatchRestarted ──> DestroyMatchOverWidget
```

Binding to `OnInitialPreparationStarted` for teardown is what makes the in-place rematch work: with no
level reload, nothing else would ever remove the panel from the viewport.

On clients the game state has usually not replicated when the HUD spawns, so `BindMatchOverEvent` falls
back to `World->GameStateSetEvent` and binds as soon as the world receives it.

Win/lose is decided by comparing **player states, not pawns** (`AMainHUD::OnMatchOver`): the losing pawn is
often already destroyed by the time the panel is built.

### C++ / Blueprint split

C++ owns the flow — visibility, ballot subscription, button actions, teardown. The Blueprint owns the look
via `SetMatchResult(bLocalPlayerWon, bIsDraw)`, a `BlueprintImplementableEvent` that fills in the headline
and accent colours. The only widget C++ reaches into is `Txt_Rematch`, bound with `BindWidgetOptional` so a
designer renaming or removing the label degrades the caption instead of breaking the panel.

### Why the RPC goes through the player controller

`UUserWidget` has **no RPC callspace resolution**, so a `UFUNCTION(Server)` declared on a widget is simply
never sent. Both widget actions therefore delegate to the controller:

| Action | Route |
|---|---|
| Rematch | `UMatchOverWidget::RequestRematch` → `AMKHPlayerController::Server_RequestRematch` → `AMKHGameMode::RequestRematch` |
| Pause-menu restart | `UMKHGameMenuWidget::RestartGame` → `AMKHPlayerController::Server_RestartMatch` → `AGameMode::RestartGame` |

### Ballot feedback

`BindRematchBallot` subscribes with `AddUniqueDynamic` (so a second `ShowMatchResult` cannot double-subscribe)
and then **immediately replays the current tally**, because the opponent may have voted before this panel
existed. `NativeDestruct` and `DestroyMatchOverPanel` both unsubscribe.

The caption reflects three states, driven by `bLocalRematchVoteCast`:

| Condition | Caption |
|---|---|
| No votes yet | `REMATCH` |
| Someone voted, not us | `REMATCH n/m` |
| We voted | `WAITING n/m` |

Return-to-lobby always calls `BackToLobby(true)` on both this panel and the pause menu: leaving a 1v1
match is always a solo return, so the player re-enters the lobby as a listen server rather than trying to
rejoin the dead session.

---

## Full Match Walkthrough

```
SERVER (AMKHGameMode)                          EVERY PEER (AMKHGameState)         OWNING CLIENT (UI)
---------------------                          --------------------------         ------------------
RestartPlayer x2
  RegisterParticipant                    ──>   RegisterParticipant                 OnParticipantRegistered
  BindDeathEvent                                                                   (W_MatchInfo)
TryStartMatch
StartInitialPreparationPhase             ──>   SetMatchPhase(InitialPreparation)   OnInitialPreparationStarted
                                               PhaseEndServerTime = T+60           local countdown ticks
StartDuelPhase                           ──>   SetMatchPhase(Duel)                 OnDuelStarted
  [timer expires]
HandleDuelTimeOver                       ──>   SetMatchPhase(SuddenDeath)          OnDuelTimeOver
  ApplySuddenDeathDrain x2
  [ASC broadcasts Event.Death]
HandlePlayerDeath
  bDiedThisRound = true
  ScoringData.Score++                    ──>   SetScore(slot, n)                   OnScoreChanged
  next tick: RemoveSuddenDeathDrain      ──>   Multicast_PlayerDied                OnPlayerDied
  +3s: ResolveRound
ResolveRound (score reached)             ──>   SetMatchPhase(MatchOver)
                                               Multicast_MatchOver                 AMainHUD::OnMatchOver
                                                                                     CreateMatchOverWidget
                                                                                     ShowMatchResult
Server_RequestRematch (both players)
RequestRematch                           ──>   SetRematchVotes(n, m)               OnRematchVotesChanged
                                                                                     caption REMATCH/WAITING
RestartMatchInPlace
  scores zeroed                          ──>   SetScore(x, 0)                      OnScoreChanged
  participants revived
StartInitialPreparationPhase             ──>   SetMatchPhase(InitialPreparation)   OnMatchRestarted
                                                                                     DestroyMatchOverWidget
                                                                                     focus back to gameplay
```

---

## Extending the System

### Adding a new match phase

1. Add the value to `EMKHMatchPhase` in `MKHGameState.h`.
2. Add a `StartXPhase()` on the game mode following the `SetPhase` → `ClearPhaseTimer` → `SetTimer` shape,
   plus its `EditDefaultsOnly` duration property under `Duel|Rules`.
3. Add the granular `FMKHDuelPhaseSignature` event and its `case` in `AMKHGameState::HandlePhaseChanged`.
4. Re-check the phase guard in `HandlePlayerDeath` — a death during the new phase is either meaningful
   (add it to the guard) or must be handled explicitly.

### Adding replicated match state

Add it to an existing snapshot struct rather than as a loose `UPROPERTY` whenever it is read together with
fields already in one — that is the whole reason the structs exist. Then: `DOREPLIFETIME` it, guard the
setter with `HasAuthority()`, call `ForceNetUpdate()`, and **broadcast the event manually in the setter**
because `OnRep` will not run on the host.

---

## Known Limits & Future Work

- **Deaths outside `Duel`/`SuddenDeath` are swallowed.** `HandlePlayerDeath` returns early in any other
  phase. If a player can die during `InitialPreparation` or `IntermissionPreparation` (fall damage, kill-Z,
  residual DoT), the character enters `Dead` — collision off, input blocked — but no round resolves and no
  reset revives them, so the next duel starts with an unplayable corpse. Either make death impossible
  during prep phases or revive immediately when one occurs there.
- **`UDeadState::ExitState` hardcodes the `Pawn` collision profile.** `EnterState` only changes channel
  *responses*, so the pair is asymmetric: a character Blueprint using a custom capsule profile loses it
  after the first revive. Saving and restoring the responses (or reading the profile from the CDO) would
  make the round trip lossless.
- **`bLocalRematchVoteCast` is optimistic.** It is set before the RPC leaves the client, so a vote the
  server rejects (wrong phase) leaves the local button permanently captioned `WAITING`. Deriving it from
  replicated state instead would be self-correcting.
- **`Logout` ballot cleanup is currently unreachable.** A disconnect in a 1v1 sends the survivor back to the
  lobby, so the vote-removal path in `AMKHGameMode::Logout` can never affect a live match. Harmless, but
  the comment justifying it describes a scenario that no longer exists.
- **Slots are never freed.** `Players` entries are only cleared by GC when the pawn dies. Fine while a
  disconnect always ends the match; it would need explicit handling if reconnection is ever supported.
