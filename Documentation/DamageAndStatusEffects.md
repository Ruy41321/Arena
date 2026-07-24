# Damage Formula & Status Effects

## Table of Contents

1. [Overview](#overview)
2. [The Damage Formula](#the-damage-formula)
3. [Buff / Debuff Attributes](#buff--debuff-attributes)
4. [Where Damage Is Defined](#where-damage-is-defined)
5. [Where Damage Is Computed](#where-damage-is-computed)
6. [Where Damage Is Applied](#where-damage-is-applied)
7. [Status Effects](#status-effects)
8. [Special Outcomes: Block, Dodge, Death](#special-outcomes-block-dodge-death)
9. [Extending the System](#extending-the-system)

---

## Overview

Damage in Makhia flows through three distinct stages, each with a single owner:

| Stage | Owner | Responsibility |
|---|---|---|
| **Definition** | Equipment data + `FAttackData` | Weapon base damage, per-attack multiplier, status effect lists |
| **Computation** | `UExecCalc_Damage` | The full damage formula (buffs, crit, mitigation modifiers) — pure computation, no side effects |
| **Application** | `UMKHAttributeSet::PostGameplayEffectExecute` | Shield/Health distribution, combat events, status effect application, death |

All buff/debuff values are **decimal fractions**: `0.3` means 30%.

---

## The Damage Formula

Computed entirely in `UExecCalc_Damage::HandleIncomingDamage`, in this exact order:

```
1. BaseDamage  = WeaponBaseDamage × (AttackMultiplier + AdditiveBaseDamage)
2. Damage      = BaseDamage × (1 + (Empower − Weaken) + CritBonus)
3. FinalDamage = Damage × (1 + (Exposed − Reinforced))
```

Where:

- **WeaponBaseDamage** — base damage of the attacker's equipped weapon (e.g. `500`).
- **AttackMultiplier** — the specific attack's damage multiplier (`FAttackData::DamagePercent`, e.g. `1.5` for a heavy attack). Travels as the set-by-caller magnitude `Combat.Data.Damage`.
- **AdditiveBaseDamage** — attacker buff, added to the attack multiplier so the weapon damage is scaled exactly once (`1.5 + 0.3 = 1.8`).
- **Empower / Weaken** — attacker buff/debuff pair; only the net value matters.
- **CritBonus** — `CritDamageMod` when the crit roll succeeds (`RandRange(1,100) <= CritChance × 100`), `0` otherwise. It stacks *additively* with net Empower in the same multiplication — this intentionally lets a crit push the offensive multiplier beyond the Empower cap.
- **Exposed / Reinforced** — defender debuff/buff pair; only the net value matters, applied to the final (post-crit) damage.

Every intermediate result is clamped to be non-negative.

### Worked Example

Weapon `500`, heavy attack `1.5`, attacker has AdditiveBaseDamage `0.3` and Empower `0.2`, crit lands with CritDamageMod `0.5`, defender has Reinforced `0.1`:

```
1. 500 × (1.5 + 0.3)        = 900
2. 900 × (1 + 0.2 + 0.5)    = 1530
3. 1530 × (1 + (0 − 0.1))   = 1377
```

---

## Buff / Debuff Attributes

Declared in `UMKHAttributeSet` (`Source/Makhia/Public/AbilitySystem/Attributes/MKHAttributeSet.h`, section *Buff / Debuff Attributes*). Each attribute has its own cap attribute (default `0.5` = 50%, tunable per character via the `DefaultAttributes` gameplay effect).

| Attribute | Read From | Role | Formula Step |
|---|---|---|---|
| `AdditiveBaseDamage` | Source (attacker) | Adds to the attack multiplier | 1 |
| `Empower` | Source (attacker) | Increases outgoing damage | 2 |
| `Weaken` | Source (attacker) | Decreases outgoing damage | 2 |
| `Reinforced` | Target (defender) | Decreases incoming damage | 3 |
| `Exposed` | Target (defender) | Increases incoming damage | 3 |

Clamping to `[0, Max]` happens in `UMKHAttributeSet::ClampAttribute`, wired into both `PreAttributeChange` (CurrentValue — duration/infinite effects) and `PreAttributeBaseChange` (BaseValue — instant effects). When a cap changes at runtime, `PostAttributeChange` re-clamps the current value via `ClampAttributeDataToMax` (no proportional scaling — these are percentages).

**Capture semantics** (defined in `RPGDamageStatics`, `ExecCalc_Damage.cpp`):

- Source attributes (`AdditiveBaseDamage`, `Empower`, `Weaken`, `CritChance`, `CritDamageMod`) are captured with **snapshot = true**: values are read when the damage spec is created, i.e. at hit time in `ApplyDamageEffect`. This is why self-activation buffs (applied at attack start) are included, and self on-hit buffs (applied after the damage pipeline) are not.
- Target attributes (`Reinforced`, `Exposed`) are captured with **snapshot = false**: the defender's state at the moment the effect executes.

---

## Where Damage Is Defined

| Data | Location | Notes |
|---|---|---|
| Weapon base damage | `FEquipmentDefinition::BaseDamage` (`Equipment/EquipmentDefinition.h`) | Authored per equipment definition |
| → propagated to | `AMKHWeaponBase::WeaponDamage` via `UEquipmentInstance::ApplyWeaponDamageIfWeapon` | Set on the spawned weapon actor |
| Attack multiplier | `FAttackData::DamagePercent` (`Equipment/EquipmentTypes.h`) | One entry per combo strike, rolled into `FEquipmentAbilityDefinition::AttacksData` |
| Stamina damage (on block) | `FAttackData::StaminaDamageValue` | Consumed only when the target blocks |
| Status effect lists | `FAttackData::TargetStatusEffects`, `SelfActivationStatusEffects`, `SelfOnHitStatusEffects` | See [Status Effects](#status-effects) |
| Status effect entry | `FStatusEffectData` (`Equipment/EquipmentTypes.h`) | `EffectClass` + `EffectDuration` + `EffectValue` (decimal fraction) |

The per-attack data reaches the ability through the equipment grant payload (`UMKHAbilityGrantPayload` → `UMKHDamageAbility::OnGiveAbility`).

### From definition to the damage effect

1. `UMKHDamageAbility::CaptureDamageEffectInfo` fills `FDamageEffectInfo` (`AbilitySystem/MKHAbilityTypes.h`): `DamageMultiplier` (from `GetAttackDamageMultiplier()` — the current strike's `DamagePercent` only, **not** pre-multiplied by weapon damage), `StaminaDamage`, `TargetStatusEffects`, `SelfOnHitStatusEffects`.
2. `UMKHAbilitySystemLibrary::ApplyDamageEffect` (`Libraries/MKHAbilitySystemLibrary.cpp`) builds the damage spec: stores both status effect lists on the `FMKHGameplayEffectContext`, assigns `Combat.Data.Damage` and `Combat.Data.StaminaDamage` as set-by-caller magnitudes, and applies the spec to the target ASC. **This is the snapshot moment for all Source attribute captures.**

---

## Where Damage Is Computed

`UExecCalc_Damage` (`Source/Makhia/Public|Private/AbilitySystem/ExecCalc/ExecCalc_Damage.h|.cpp`) — pure computation, per project convention no side effects happen here.

| Step | Function | What it does |
|---|---|---|
| Entry | `Execute_Implementation` | Early-outs on dead target; routes to block (stamina damage) or health damage |
| Weapon resolution | `GetEquippedWeapon` → `GetWeaponBaseDamage` | Resolves the attacker's equipped weapon through the equipment manager on its instigator controller (authority-only data; executions run on the server). No weapon → damage 0 + warning |
| Formula step 1 | `ComputeBaseDamage` | `WeaponBaseDamage × (AttackMultiplier + AdditiveBaseDamage)` |
| Dodge check | `IsAttackDodged` | Perfect dodge: flags the context, forwards the would-be damage as event magnitude only |
| Formula step 2 | `ApplyOffensiveModifiers` + `RollCriticalHitBonus` | `× (1 + (Empower − Weaken) + CritBonus)`; the crit roll also flags `bCriticalHit` on the context |
| Formula step 3 | `ApplyDefensiveModifiers` | `× (1 + (Exposed − Reinforced))` |
| Output | `AddOutputModifier` on `IncomingDamage` | Writes the result into the `IncomingDamage` **meta-attribute** (never displayed, consumed immediately by the attribute set) |

The blocked path (`HandleStaminaDamage` + `RefineStaminaDamage`) bypasses the damage formula entirely and outputs `IncomingStaminaDamage`, reduced by the defender weapon's block stability.

---

## Where Damage Is Applied

`UMKHAttributeSet::PostGameplayEffectExecute` (`Source/Makhia/Private/AbilitySystem/Attributes/MKHAttributeSet.cpp`) reacts to the meta-attributes written by the exec calc:

- `HandleIncomingDamage`:
  1. Consumes and resets `IncomingDamage`.
  2. Dodged? → sends `Event.AttackDodged` to both actors, applies nothing, stops.
  3. Shield present? → `ApplyShieldBreak` (damage ≥ shield × 2) or `ApplyShieldDamageMitigation`, both splitting the hit via `CalculateDamageSplit` — see [ShieldDamageScaling.md](ShieldDamageScaling.md); otherwise all damage goes to Health.
  4. Side effects: `Event.HitReceived` / `Event.HitInflicted`, status effect application (see below), `TrySendDeathEvent`.
- `HandleIncomingStaminaDamage`: blocked attacks — stamina reduction, `Event.BlockSuccessful`, `Event.GuardBreak` on depletion.

Health/Stamina/Shield clamping on application is guaranteed by the same `ClampAttribute` hooks described above.

---

## Status Effects

### The three lists (per attack, in `FAttackData`)

| List | Applied To | When | Affects the triggering hit's damage? |
|---|---|---|---|
| `TargetStatusEffects` | Hit target | On successful hit | — (they land on the defender after damage) |
| `SelfActivationStatusEffects` | Attacker | When the attack **starts**, hit or miss | **Yes** — active before the damage spec snapshot |
| `SelfOnHitStatusEffects` | Attacker | Only after a successful hit | **No** — applied after the damage pipeline; they only affect later hits |

### Shared application pipeline

Every list goes through the same function — `UMKHAbilitySystemLibrary::ApplyStatusEffects(SourceASC, TargetASC, Effects, Context)`:

```
for each FStatusEffectData:
    MakeOutgoingSpec(EffectClass)
    set-by-caller  Combat.Data.StatusEffectDuration = EffectDuration
    set-by-caller  Combat.Data.StatusEffectValue    = EffectValue
    ApplyGameplayEffectSpecToSelf on the target ASC
```

Self-applied effects simply pass the same ASC as source and target. Status effect gameplay effects are expected to read duration/value via those set-by-caller tags (e.g. an Empower effect adds `EffectValue` to the `Empower` attribute for `EffectDuration` seconds; the attribute cap clamps stacking).

### Application points

| List | Transport | Applied In | Trigger |
|---|---|---|---|
| `TargetStatusEffects` | `FMKHGameplayEffectContext::TargetStatusEffects` (replicated, `NetSerialize` bit 8) | `UMKHAttributeSet::ApplyStatusEffectsFromContext` → target ASC | End of `HandleIncomingDamage`, after damage |
| `SelfOnHitStatusEffects` | `FMKHGameplayEffectContext::SelfOnHitStatusEffects` (replicated, `NetSerialize` bit 9) | `UMKHAttributeSet::ApplyStatusEffectsFromContext` → **source** ASC | Same point — after damage, so never part of the triggering hit |
| `SelfActivationStatusEffects` | Not transported — read directly from `AttacksData` | `UMKHDamageAbility::TryApplySelfActivationStatusEffects` → own ASC | Attack start (see below) |

`TryApplySelfActivationStatusEffects` runs on the authority only, at most once per combo strike (`LastSelfActivationComboIndex` dedup), from three call sites:

1. `OnMontageStarted_Implementation` — strike 0, including non-combo abilities.
2. `OnContinueComboEndReceived` — right after `ComboHitCounter` advances, before `OnComboTriggered`.
3. `SyncAuthoritativeComboCounter` — latency catch-up: if the server learns about a combo strike only when its hit arrives, the effects are applied immediately before `CaptureDamageEffectInfo`/`ApplyDamageEffect` creates the damage spec, so buffs like Empower are still captured by that hit's damage calculation.

---

## Special Outcomes: Block, Dodge, Death

| Outcome | Detected In | Effect on damage | Effect on status effects |
|---|---|---|---|
| **Block** (defender blocking + facing attacker) | `UExecCalc_Damage::IsAttackBlocked` | No health damage; stamina damage path instead | Target & self on-hit effects **not** applied |
| **Perfect dodge** (defender in Dodging state) | `UExecCalc_Damage::IsAttackDodged` | No damage applied; `Event.AttackDodged` to both actors | Target & self on-hit effects **not** applied |
| **Death** (Health reaches 0) | `UMKHAttributeSet::TrySendDeathEvent` | `Event.Death` sent once (guarded by `State.Movement.Dead`) | — |
| Any of the above | — | — | Self **activation** effects were already applied at attack start regardless |

---

## Extending the System

- **New offensive/defensive modifier attribute**: add the attribute (+ its Max) to `UMKHAttributeSet` with clamping in `ClampAttribute`, add a capture definition in `RPGDamageStatics` + constructor registration, then multiply/add it in the appropriate step of `HandleIncomingDamage`. Nothing in the attribute set application code needs to change.
- **New status effect trigger** (e.g. "on kill"): add a `TArray<FStatusEffectData>` to `FAttackData`, transport it (context array + `NetSerialize` bit if it must travel with the damage spec), and call `UMKHAbilitySystemLibrary::ApplyStatusEffects` from the new trigger point — the definition and application pipeline is already shared.
- **Renamed data properties**: `FAttackData::StatusEffects` → `TargetStatusEffects` and `FDamageEffectInfo::AdditionalStatusEffects` → `TargetStatusEffects` are covered by `PropertyRedirects` in `Config/DefaultEngine.ini`; resave the affected assets, then the redirects can be removed.
