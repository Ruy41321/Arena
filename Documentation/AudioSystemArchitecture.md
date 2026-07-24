# Audio System Architecture

## Table of Contents

1. [Overview](#overview)
2. [Directory Structure](#directory-structure)
3. [Design Principles](#design-principles)
4. [Signal Flow](#signal-flow)
5. [Content Assets (Editor-Created)](#content-assets-editor-created)
6. [Configuration](#configuration)
7. [C++ Classes](#c-classes)
   - [UMKHAudioSettings](#umkhaudiosettings)
   - [UMKHGameUserSettings (Audio Extension)](#umkhgameusersettings-audio-extension)
   - [UMKHAudioWorldSubsystem](#umkhaudioworldsubsystem)
   - [UMKHMusicSubsystem](#umkhmusicsubsystem)
   - [UMKHAudioSettingsWidget](#umkhaudiosettingswidget)
8. [Runtime Sequences](#runtime-sequences)
9. [How-To Guides](#how-to-guides)
10. [Gotchas & Troubleshooting](#gotchas--troubleshooting)
11. [Roadmap](#roadmap)

---

## Overview

Makhia routes all audio through an **Audio Modulation** mixing graph whose per-category
volumes are driven by user settings and applied automatically for every gameplay world.
Background music is owned by a session-scoped subsystem that survives level travel and
crossfades between tracks.

The system is deliberately **data-driven and decoupled**: gameplay code never hardcodes
volumes or asset paths. A sound's loudness is decided entirely by the submix it is routed
to; the submix volume is decided by a Control Bus; the Control Bus value is decided by the
user's saved settings. No `SetVolumeMultiplier` calls are scattered through gameplay logic.

| Custom Class | Engine Base | Purpose |
|---|---|---|
| `UMKHAudioSettings` | `UDeveloperSettings` | Project-wide audio config: Control Bus references, level→music map, fade defaults |
| `UMKHGameUserSettings` | `UGameUserSettings` | Persisted user volumes (Master/Music/SFX) + push to Control Buses |
| `UMKHAudioWorldSubsystem` | `UWorldSubsystem` | Re-applies volumes and starts level music on world begin play |
| `UMKHMusicSubsystem` | `UGameInstanceSubsystem` | Owns persistent music playback, crossfades, seamless travel |
| `UMKHAudioSettingsWidget` | `UMKHSystemWidget` | Settings panel: three volume sliders, live apply + save |

Logging for this layer uses the dedicated `LogMKHAudio` category (`MKHLogChannels.h`).

The system depends on the **AudioModulation** plugin (enabled in `Makhia.uproject`) and the
`AudioModulation`, `UMG`, `Slate`, `SlateCore`, and `DeveloperSettings` build modules
(`Makhia.Build.cs`).

---

## Directory Structure

```
Source/Makhia/
├── Public/Audio/
│   ├── MKHAudioSettings.h           # UDeveloperSettings: bus refs, level→music map, fade default
│   ├── MKHAudioWorldSubsystem.h     # Per-world bootstrap (volumes + level music)
│   └── MKHMusicSubsystem.h          # Session-scoped music player
├── Public/GameUserSettings/
│   └── MKHGameUserSettings.h        # User volumes (extended in this session)
├── Public/UI/HUD/Settings/
│   └── MKHAudioSettingsWidget.h     # Volume sliders panel
├── Private/Audio/
│   ├── MKHAudioSettings.cpp
│   ├── MKHAudioWorldSubsystem.cpp
│   └── MKHMusicSubsystem.cpp
├── Private/GameUserSettings/
│   └── MKHGameUserSettings.cpp
└── Private/UI/HUD/Settings/
    └── MKHAudioSettingsWidget.cpp

Content/Makhia/Sound/
├── Modulation/
│   ├── MP_Volume       # SoundModulationParameterVolume
│   ├── CB_Master       # Control Bus (Parameter = MP_Volume)
│   ├── CB_Music        # Control Bus (Parameter = MP_Volume)
│   └── CB_SFX          # Control Bus (Parameter = MP_Volume)
├── Submixes/
│   ├── SBM_Main        # Master submix (outputs to hardware) — modulated by CB_Master
│   ├── SBM_Music       # Child of SBM_Main — modulated by CB_Music
│   └── SBM_SFX         # Child of SBM_Main — modulated by CB_SFX
└── Theme/
    └── MainThemeLooped # Looping background track, Submix = SBM_Music
```

---

## Design Principles

1. **A sound never knows how loud it should be.** Loudness is a property of the submix
   the sound is routed to, not of the sound instance. This keeps mixing centralized.

2. **User volume = one persistent Control Bus mix.** Volumes are pushed to Control Buses
   via a global bus mix. This leaves room to *stack* additional, temporary mixes on top
   later (music ducking under dialogue, pause menu low-pass, low-health filter) without
   ever disturbing the user's saved values.

3. **Perceptual sliders for free.** The slider value in `[0, 1]` is passed straight to the
   Control Bus. The `Volume` modulation parameter converts it to decibels internally
   (normalized `1.0` → `0 dB`, `0.0` → `MinVolume` = `-100 dB`), so the fader feels
   logarithmic without any manual `dB` math in game code.

4. **Music is client-local, never replicated.** Music selection is triggered on world
   begin play (which runs on every client), driven by a data map, and owned by the
   GameInstance. The server-only GameMode is intentionally *not* involved.

5. **Settings model has a single writer.** The audio settings widget writes directly to
   `UMKHGameUserSettings` without a WidgetController — the Model→View observer indirection
   used for gameplay data adds nothing for a global, single-writer settings object.

---

## Signal Flow

```
                        ┌── CB_Master ──┐         (user Master slider)
                        │               ▼
MainThemeLooped ─► SBM_Music ─► SBM_Main ─► audio hardware
                        ▲
                        └── CB_Music              (user Music slider)

unrouted sounds ─► SBM_SFX ─► SBM_Main ─► audio hardware
                        ▲
                        └── CB_SFX                (user SFX slider)
```

- `SBM_Main` is declared as the engine **Master Submix**, i.e. the root that outputs to the
  audio device. Its output volume is modulated by `CB_Master`, so the Master slider affects
  everything.
- `SBM_Music` and `SBM_SFX` are children of `SBM_Main`, each modulated by its own bus.
- `SBM_SFX` is the **Base Default Submix**: any sound that does not explicitly set a submix
  is automatically routed here, so future SFX fall under the SFX slider with zero wiring.

---

## Content Assets (Editor-Created)

These are not code — they are assets an author creates once in the editor. The C++/config
layer references them by the exact paths listed in [Directory Structure](#directory-structure).

| Asset | Type | Key settings |
|---|---|---|
| `MP_Volume` | Sound Modulation Parameter | Class `SoundModulationParameterVolume`; defaults (`MinVolume = -100 dB`, normalized default `1.0`) |
| `CB_Master` / `CB_Music` / `CB_SFX` | Sound Control Bus | `Parameter = MP_Volume` |
| `SBM_Main` | Sound Submix | `Parent = None`; Output Volume Modulation → `CB_Master` |
| `SBM_Music` | Sound Submix | `Parent = SBM_Main`; Output Volume Modulation → `CB_Music` |
| `SBM_SFX` | Sound Submix | `Parent = SBM_Main`; Output Volume Modulation → `CB_SFX` |
| `MainThemeLooped` | Sound Wave (looping) | `Submix = SBM_Music` |

---

## Configuration

### `Config/DefaultEngine.ini`

```ini
[/Script/Engine.AudioSettings]
MasterSubmix=/Game/Makhia/Sound/Submixes/SBM_Main.SBM_Main
BaseDefaultSubmix=/Game/Makhia/Sound/Submixes/SBM_SFX.SBM_SFX
```

- `MasterSubmix` — the root submix that outputs to hardware. **Must** be set, otherwise the
  custom submix graph forms a cycle (see [Gotchas](#gotchas--troubleshooting)).
- `BaseDefaultSubmix` — implicit destination for sounds with no submix set.

### `Config/DefaultGame.ini`

```ini
[/Script/Makhia.MKHAudioSettings]
MasterVolumeControlBus=/Game/Makhia/Sound/Modulation/CB_Master.CB_Master
MusicVolumeControlBus=/Game/Makhia/Sound/Modulation/CB_Music.CB_Music
SFXVolumeControlBus=/Game/Makhia/Sound/Modulation/CB_SFX.CB_SFX
LevelMusicMap=(("CastleArena", "/Game/Makhia/Sound/Theme/MainThemeLooped.MainThemeLooped"),("Lvl_ThirdPerson", "/Game/Makhia/Sound/Theme/MainThemeLooped.MainThemeLooped"))
DefaultMusicFadeSeconds=2.0
```

Editable in-editor under **Project Settings → Game → Makhia Audio**.

User volumes persist to `Saved/Config/.../GameUserSettings.ini` (written by `SaveSettings()`).

---

## C++ Classes

### UMKHAudioSettings

`UDeveloperSettings` (config `Game`, section *Makhia Audio*). Holds the design-time audio
configuration so gameplay code reads references instead of hardcoding asset paths.

| Member | Type | Purpose |
|---|---|---|
| `MasterVolumeControlBus` | `TSoftObjectPtr<USoundControlBus>` | Bus driven by the Master slider |
| `MusicVolumeControlBus` | `TSoftObjectPtr<USoundControlBus>` | Bus driven by the Music slider |
| `SFXVolumeControlBus` | `TSoftObjectPtr<USoundControlBus>` | Bus driven by the SFX slider |
| `LevelMusicMap` | `TMap<FString, TSoftObjectPtr<USoundBase>>` | Short level name → music track |
| `DefaultMusicFadeSeconds` | `float` | Default fade for fade-in/out/crossfade |
| `Get()` | `static const UMKHAudioSettings*` | CDO accessor |

### UMKHGameUserSettings (Audio Extension)

Extends the engine user settings with three normalized volumes stored as `UPROPERTY(Config)`.
Setters clamp to `[0, 1]`, store the value, and apply it live to the matching Control Bus.

| Function | Purpose |
|---|---|
| `SetMasterVolume/SetMusicVolume/SetSFXVolume(float)` | Store + live-apply one volume to its bus |
| `GetMasterVolume/GetMusicVolume/GetSFXVolume() const` | Read the normalized value |
| `ApplyAudioSettings()` | Push all three volumes to their buses |
| `ApplyVolumeToControlBus(...)` *(private)* | Loads the bus and calls `SetGlobalBusMixValue` |
| `ApplySettings(bool)` *(override)* | Calls `Super` then `ApplyAudioSettings()` |
| `SetToDefaults()` *(override)* | Resets volumes to `1.0` |

`ApplyVolumeToControlBus` is a safe no-op (verbose log) when there is no active play world,
because Audio Modulation state is per audio device — the world subsystem re-applies later.
It logs a warning if a bus asset is unassigned. Volumes are pushed via
`UAudioModulationStatics::SetGlobalBusMixValue(World, Bus, LinearVolume)`.

### UMKHAudioWorldSubsystem

`UWorldSubsystem` created only for `Game`/`PIE` worlds (`ShouldCreateSubsystem`). On
`OnWorldBeginPlay` it:

1. Returns early on a dedicated server (no audio device).
2. Calls `UMKHGameUserSettings::ApplyAudioSettings()` to re-push saved volumes to the buses
   for this world's audio device.
3. Calls `StartLevelMusic(World)` — resolves the current short level name via
   `UGameplayStatics::GetCurrentLevelName(&World, /*bRemovePrefix*/ true)`, looks it up in
   `LevelMusicMap`, and asks `UMKHMusicSubsystem` to play it (or `StopMusic()` when the
   level has no entry).

This is the single bootstrap point that guarantees the mixing graph is correctly primed for
every world (initial boot, PIE, level travel), so UI code never has to worry about *when*
it is safe to apply audio settings.

### UMKHMusicSubsystem

`UGameInstanceSubsystem` that owns the current music `UAudioComponent`. Because it lives on
the GameInstance and creates the component with `bPersistAcrossLevelTransition = true`,
music continues seamlessly across level travel.

| Function | Behavior |
|---|---|
| `PlayMusicTrack(Track, FadeInSeconds = -1)` | If `Track` is already playing → no-op (seamless). Otherwise fades out the old component and creates a new persistent 2D sound faded in. `-1` fade uses the project default. |
| `StopMusic(FadeOutSeconds = -1)` | Fades out and clears the current track. |
| `GetCurrentTrack() const` | Track currently playing, or `nullptr`. |
| `IsMusicPlaying() const` | `true` if the component is valid and playing. |
| `ResolveFadeSeconds(float)` *(private)* | Returns the argument, or `DefaultMusicFadeSeconds` when negative. |

Playback uses `UGameplayStatics::CreateSound2D(..., bPersistAcrossLevelTransition = true,
bAutoDestroy = true)` followed by `FadeIn`. Crossfades let the outgoing component fade out
and auto-destroy itself.

### UMKHAudioSettingsWidget

`Abstract` subclass of `UMKHSystemWidget`. The Blueprint child only needs three `BindWidget`
sliders named exactly `MasterVolumeSlider`, `MusicVolumeSlider`, `SFXVolumeSlider`; all wiring
is native.

- `NativeConstruct` → `InitializeSliderValues()` (seed from saved settings) + `BindSliderEvents()`.
- `OnValueChanged` per slider → `Set<Category>Volume()` → **live** audible feedback while dragging.
- `OnMouseCaptureEnd` / `OnControllerCaptureEnd` → `HandleVolumeCaptureEnd()` → `SaveSettings()`
  (persist only on release, for both mouse and gamepad).
- `NativeDestruct` → persists any in-flight change (menu closed mid-drag) + `UnbindSliderEvents()`.

No WidgetController is used, by design (see [Design Principles](#design-principles) #5).

---

## Runtime Sequences

### Boot / Level Travel

```
World begins play
  └─ UMKHAudioWorldSubsystem::OnWorldBeginPlay
       ├─ (dedicated server?) → return
       ├─ UMKHGameUserSettings::ApplyAudioSettings()
       │     └─ SetGlobalBusMixValue for CB_Master / CB_Music / CB_SFX
       └─ StartLevelMusic()
             ├─ level name in LevelMusicMap? → UMKHMusicSubsystem::PlayMusicTrack(track)
             │     └─ same track already playing → no-op (seamless travel)
             └─ not found → UMKHMusicSubsystem::StopMusic()
```

### Adjusting a Volume Slider

```
Drag slider
  └─ OnValueChanged → UMKHGameUserSettings::Set<Category>Volume(v)
       └─ clamp + SetGlobalBusMixValue(bus, v)   [audible immediately]
Release slider
  └─ OnMouseCaptureEnd → SaveSettings()          [persist to GameUserSettings.ini]
```

---

## How-To Guides

### Add music for a new level

1. Import the track, set its **Submix = SBM_Music**.
2. **Project Settings → Game → Makhia Audio → Level Music Map**: add an entry keyed by the
   **short** level name (no path, no PIE prefix — e.g. `CastleArena`) → the track.

That's it — `UMKHAudioWorldSubsystem` starts it on begin play. A level with no entry fades
any current music out.

### Route a new sound category under an existing slider

Route the sound's **Submix** to `SBM_Music` (music) or `SBM_SFX` (effects). Sounds with no
submix already fall under `SBM_SFX` via the Base Default Submix, so most SFX need nothing.

### Change music from gameplay (e.g. combat vs. exploration)

```cpp
if (UMKHMusicSubsystem* Music = GetGameInstance()->GetSubsystem<UMKHMusicSubsystem>())
{
    Music->PlayMusicTrack(CombatTrack); // crossfades from the current track
}
```

### Add a new user volume category (e.g. UI, Voice)

1. Create `CB_UI` + `SBM_UI` (child of `SBM_Main`, modulated by `CB_UI`).
2. Add a `TSoftObjectPtr<USoundControlBus> UIVolumeControlBus` to `UMKHAudioSettings` and
   reference `CB_UI` in `DefaultGame.ini`.
3. Add `UIVolume` (`UPROPERTY(Config)`) + `Set/GetUIVolume` to `UMKHGameUserSettings`, and a
   line in `ApplyAudioSettings()`.
4. Add a `UIVolumeSlider` `BindWidget` to `UMKHAudioSettingsWidget` and wire it like the others.

---

## Gotchas & Troubleshooting

### Custom submixes are silent (even in editor preview); only "no submix" plays

**Cause:** the submix graph forms a cycle when `MasterSubmix` is not set. A submix with a
null parent implicitly sends to `BaseDefaultSubmix`; if that is `SBM_SFX` (a child of
`SBM_Main`) while `SBM_Main` itself has a null parent, `SBM_Main → SBM_SFX → SBM_Main` loops.
The engine drops the branch, so nothing routed into the custom graph reaches hardware.

**Fix:** set `MasterSubmix = SBM_Main` in `DefaultEngine.ini` / Project Settings → Audio. As
the declared master, `SBM_Main` outputs to hardware directly and no longer does an implicit
send, breaking the cycle. **Master/Base submix changes require a full editor restart.**

### Music plays but a volume slider does nothing

The sound is not routed through the expected submix. Check the **Submix** property on the
sound asset (music → `SBM_Music`). A sound with no submix goes to `SBM_SFX` (SFX slider).

### Modulation is never the cause of silence

An undriven Control Bus resolves to its parameter's default normalized value of `1.0`
(= `0 dB`, full volume). If audio is silent, the cause is routing, not the buses.

### `ApplyVolumeToControlBus` logs "No active play world"

Expected when called before a world is up (e.g. from a menu on the CDO). `UMKHAudioWorldSubsystem`
re-applies on world begin play; no action needed.

---

## Roadmap

Implemented this session: Control Bus / submix mixing graph, user volumes (Master/Music/SFX)
with persistence and live application, per-world bootstrap, session-scoped music with
crossfades and seamless travel, and the settings slider panel.

Planned:

| Phase | Work |
|---|---|
| SFX | Ability sounds via **GameplayCue** (`GameplayCue.Melee.Sword.Impact` …) — replicated, client-only. Whoosh/footsteps via **AnimNotify** (frame-accurate). Shared `SoundConcurrency` (*stop oldest*) + `SoundAttenuation` per family. All routed to `SBM_SFX`. |
| Mixing | Stacked bus mixes on top of the user mix: music ducking under voice, pause-menu low-pass, low-health filter — via `ActivateBusMix` / `DeactivateBusMix`. |
| Adaptive music | **MetaSound Source** with exposed `Intensity`/`Section` params driven by `SetFloatParameter`; **Quartz** for beat-synced transitions. Swap the track asset for a MetaSound — the subsystem API is unchanged. |
```
