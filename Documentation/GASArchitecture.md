# Gameplay Ability System (GAS) Architecture

## Table of Contents

1. [Overview](#overview)
2. [Directory Structure](#directory-structure)
3. [Initialization Flow](#initialization-flow)
4. [Ability System Component](#ability-system-component)
5. [Attribute Set](#attribute-set)
6. [Gameplay Abilities](#gameplay-abilities)
7. [Execution Calculations](#execution-calculations)
8. [Custom Effect Context](#custom-effect-context)
9. [Gameplay Tags](#gameplay-tags)
10. [Damage Pipeline](#damage-pipeline)
11. [Equipment Integration](#equipment-integration)
12. [Input Binding](#input-binding)
13. [Data Assets](#data-assets)

---

## Overview

Makhia uses Unreal Engine's **Gameplay Ability System (GAS)** to manage character attributes (Health, Stamina, Shield), combat logic, equipment effects, and ability activation. The GAS integration is built on several custom classes that extend the default engine types:

| Custom Class | Engine Base | Purpose |
|---|---|---|
| `UMKHAbilitySystemComponent` | `UAbilitySystemComponent` | Manages abilities, effects, and equipment integration |
| `UMKHAttributeSet` | `UAttributeSet` | Defines character attributes with replication |
| `UMKHGameplayAbility` | `UGameplayAbility` | Base ability with input tag binding |
| `UMKHDamageAbility` | `UMKHGameplayAbility` | Abilities that deal damage |
| `UProjectileAbility` | `UMKHDamageAbility` | Abilities that spawn projectiles |
| `FMKHGameplayEffectContext` | `FGameplayEffectContext` | Extended context with critical hit tracking |
| `UMKHAbilitySystemGlobals` | `UAbilitySystemGlobals` | Overrides default effect context allocation |

---

## Directory Structure

```
Source/Makhia/
├── Public/AbilitySystem/
│   ├── MKHAbilitySystemComponent.h
│   ├── MKHAbilitySystemGlobals.h
│   ├── MKHAbilityTypes.h            # FMKHGameplayEffectContext, FProjectileParams, FDamageEffectInfo
│   ├── MKHGameplayTags.h            # Native gameplay tag declarations
│   ├── Abilities/
│   │   ├── MKHGameplayAbility.h     # Base ability with InputTag
│   │   ├── MKHDamageAbility.h       # Damage-dealing ability base
│   │   └── ProjectileAbility.h      # Projectile-spawning ability
│   ├── Attributes/
│   │   └── MKHAttributeSet.h        # Character attributes
│   └── ExecCalc/
│       ├── ExecCalc_Damage.h        # Damage calculation with critical hits
│       └── ExecCalc_DodgeCost.h     # Stamina cost for dodging
├── Private/AbilitySystem/
│   └── (mirrors Public/ with .cpp implementations)
├── Public/Interfaces/
│   └── MKHAbilitySystemInterface.h  # Interface for GAS-aware actors
├── Public/Libraries/
│   └── MKHAbilitySystemLibrary.h    # Static helper functions
└── Public/Data/
    ├── CharacterClassInfo.h         # Per-class ability/attribute defaults
    └── ProjectileInfo.h             # Projectile configuration data
```

---

## Initialization Flow

GAS components are initialised at different points depending on whether the character is a player or an AI enemy.

### Player Characters

For player characters the ASC and attribute set live on the **Player State** (`AMKHPlayerState`), not on the character itself. This follows the recommended GAS pattern for multiplayer games where the Player State persists across respawns.

```
AMKHPlayerState (constructor)
  ├─ Creates UMKHAbilitySystemComponent (Replicated, Mixed mode)
  └─ Creates UMKHAttributeSet

AMKHPlayerCharacter::PossessedBy()          [Server]
  └─ InitAbilityActorInfo()
       ├─ Gets ASC and AttributeSet from PlayerState
       ├─ Calls ASC->InitAbilityActorInfo(PlayerState, this)
       ├─ BindCallbacksToDependencies()   (Health, Shield, Stamina delegates)
       ├─ MovementStateMachine->SyncCurrentStateTagToASC()
       └─ InitClassDefaults()             [Authority only]
            ├─ Loads CharacterClassInfo by CharacterTag
            ├─ ASC->AddCharacterAbilities(StartingAbilities)
            ├─ ASC->AddCharacterPassiveAbilities(StartingPassives)
            └─ ASC->InitializeDefaultAttributes(DefaultAttributes)

AMKHPlayerCharacter::OnRep_PlayerState()    [Client]
  └─ InitAbilityActorInfo()              (same flow, minus InitClassDefaults)
```

### Enemy Characters

Enemies own their ASC and attribute set directly:

```
AEnemyBase (constructor)
  ├─ Creates UMKHAbilitySystemComponent (Replicated, Minimal mode)
  └─ Creates UMKHAttributeSet

AEnemyBase::BeginPlay()
  ├─ BindCallbacksToDependencies()
  └─ InitAbilityActorInfo()
       ├─ ASC->InitAbilityActorInfo(this, this)
       └─ InitClassDefaults()            [Authority only]
```

**Replication modes**:
- **Player**: `Mixed` — the owner client predicts abilities locally while the server replicates Gameplay Effects to all clients.
- **Enemy**: `Minimal` — the server handles all ability logic; only effect results are replicated.

---

## Ability System Component

**Class**: `UMKHAbilitySystemComponent` (inherits `UAbilitySystemComponent`)  
**Location**: `Source/Makhia/Public/AbilitySystem/MKHAbilitySystemComponent.h`

### Public API

| Method | Description |
|---|---|
| `AddCharacterAbilities(Abilities)` | Grants active abilities. Each ability's `InputTag` is added to the spec's dynamic source tags so it can be activated by input. |
| `AddCharacterPassiveAbilities(Passives)` | Grants and immediately activates passive abilities via `GiveAbilityAndActivateOnce`. |
| `InitializeDefaultAttributes(AttributeEffect)` | Applies a `UGameplayEffect` that sets the initial attribute values, then broadcasts `OnAttributesGiven`. |
| `AbilityInputPressed(InputTag)` | Iterates activatable abilities, finds the one matching the tag, and calls `TryActivateAbility` or sends `InputPressed` replicated event. |
| `AbilityInputReleased(InputTag)` | Sends `InputReleased` replicated event to matching active abilities. |
| `SetDynamicProjectile(ProjectileTag, Level)` | Replaces the current projectile ability at runtime. Handles authority with a Server RPC. |
| `AddEquipmentEffects(Entry)` | Applies stat effects from an equipment entry. Supports async loading via `FStreamableManager`. |
| `RemoveEquipmentEffects(Entry)` | Removes all active effects granted by an equipment entry. |
| `AddEquipmentAbility(Entry)` | Grants abilities from equipment. Also supports async loading. |
| `RemoveEquipmentAbility(Entry)` | Clears all abilities granted by an equipment entry. |

### Delegate

```cpp
DECLARE_MULTICAST_DELEGATE(FOnAttributesGiven)
```

Broadcast after `InitializeDefaultAttributes` completes. Used by `AEnemyBase` to set a replicated flag so clients know attributes are ready.

---

## Attribute Set

**Class**: `UMKHAttributeSet` (inherits `UAttributeSet`)  
**Location**: `Source/Makhia/Public/AbilitySystem/Attributes/MKHAttributeSet.h`

### Attributes

| Attribute | Replicated | Purpose |
|---|---|---|
| `Health` | Yes | Current health points, clamped to `[0, MaxHealth]` |
| `MaxHealth` | Yes | Upper bound for Health |
| `Shield` | Yes | Protective shield value, clamped to `[0, MaxShield]` |
| `MaxShield` | Yes | Upper bound for Shield |
| `Stamina` | Yes | Resource for dodge/sprint, clamped to `[0, MaxStamina]` |
| `MaxStamina` | Yes | Upper bound for Stamina |
| `DodgeStaminaCost` | Yes | How much Stamina a dodge consumes |
| `CritChance` | Yes | Probability of a critical hit (0–100) |
| `CritDamageMod` | Yes | Bonus damage multiplier on critical hits |
| `IncomingDamage` | No | Transient meta-attribute written by `ExecCalc_Damage` and consumed in `PostGameplayEffectExecute` |

Each replicated attribute has an `OnRep_*` function that calls `GAMEPLAYATTRIBUTE_REPNOTIFY`.

### Key Overrides

- **`PreAttributeChange`**: Clamps Health, Stamina, and Shield to their respective max values before application.
- **`PostAttributeChange`**: When a max attribute changes, scales the current value proportionally via `AdjustAttributeForMaxChange` so the character keeps the same percentage.
- **`PostGameplayEffectExecute`**: When `IncomingDamage` is written, delegates to `HandleIncomingDamage` which applies the shield absorption formula and then distributes remaining damage to Health.

### Shield Absorption Formula

The absorption rate uses a hybrid linear/exponential model:

```
if Shield ≤ ReferenceShield (100):
    Absorption = 0.5 × (Shield / ReferenceShield)     // 0–50%

if Shield > ReferenceShield (100):
    Ratio = Shield / ReferenceShield
    Absorption = 1.0 − 0.5^(Ratio × 2 − 1)           // 50–100% (asymptotic)
```

A **Shield Break** occurs if the incoming damage exceeds `CurrentShield × 2.0`, destroying the shield entirely and letting only the surplus damage pass to Health.

---

## Gameplay Abilities

### Ability Hierarchy

```
UGameplayAbility
  └─ UMKHGameplayAbility          (adds InputTag)
       └─ UMKHDamageAbility       (adds BaseDamage + DamageEffect + CaptureDamageEffectInfo)
            └─ UProjectileAbility (adds projectile spawning)
```

### UMKHGameplayAbility

**Location**: `Source/Makhia/Public/AbilitySystem/Abilities/MKHGameplayAbility.h`

The base class for all custom abilities. Its only addition is:

```cpp
UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Input")
FGameplayTag InputTag;
```

This tag is added to the ability spec's dynamic source tags on grant. When the player presses an input bound to this tag, `AbilityInputPressed` finds the matching spec and activates it.

### UMKHDamageAbility

**Location**: `Source/Makhia/Public/AbilitySystem/Abilities/MKHDamageAbility.h`

Adds damage configuration:

- `DamageEffect` — the `UGameplayEffect` subclass that carries the damage execution calculation.
- `BaseDamage` — an `FScalableFloat` that scales with ability level.
- `CaptureDamageEffectInfo(TargetActor, OutInfo)` — fills an `FDamageEffectInfo` struct with all data needed to apply damage (source ASC, target ASC, avatar actor, ability level, base damage, effect class).

### UProjectileAbility

**Location**: `Source/Makhia/Public/AbilitySystem/Abilities/ProjectileAbility.h`

Spawns projectile actors. Key details:

- **Instancing**: `InstancedPerActor` — each character gets its own ability instance.
- **`OnGiveAbility`**: Loads projectile parameters from `UProjectileInfo` using the `ProjectileToSpawnTag`.
- **`SpawnProjectile`** (BlueprintCallable): Spawns a `AMKHMKHProjectileBase` at the character's dynamic spawn point (obtained via `IMKHAbilitySystemInterface`), applies projectile parameters and damage info, then calls `FinishSpawning`.

---

## Execution Calculations

### ExecCalc_Damage

**Location**: `Source/Makhia/Public/AbilitySystem/ExecCalc/ExecCalc_Damage.h`

Handles damage calculation including critical hits:

1. **Get Base Damage**: Read from `SetByCaller` using `MKHGameplayTags::Combat::Data_Damage`.
2. **Capture Source Attributes**: `CritChance`, `CritDamageMod`.
3. **Capture Target Attributes**: `Shield` (for context).
4. **Critical Hit Roll**: Random 0–100 compared against `CritChance`. On crit: `Damage *= (1 + CritDamageMod)`.
5. **Store Result**: Sets `bCriticalHit` on the `FMKHGameplayEffectContext` and outputs the final value to `IncomingDamage`.

The actual health/shield split happens later in `UMKHAttributeSet::PostGameplayEffectExecute`.

### ExecCalc_DodgeCost

**Location**: `Source/Makhia/Public/AbilitySystem/ExecCalc/ExecCalc_DodgeCost.h`

Deducts stamina for dodge actions:

1. **Capture Source Attributes**: `Stamina`, `MaxStamina`, `DodgeStaminaCost`.
2. **Clamp**: Ensures non-negative values.
3. **Calculate Actual Cost**: `ActualCost = Min(DodgeStaminaCost, CurrentStamina)` — if stamina is too low, consume whatever is left.
4. **Output**: Subtracts `ActualCost` from `Stamina`.

---

## Custom Effect Context

**Struct**: `FMKHGameplayEffectContext` (inherits `FGameplayEffectContext`)  
**Location**: `Source/Makhia/Public/AbilitySystem/MKHAbilityTypes.h`

Extends the default effect context with:

- `bCriticalHit` — set by `ExecCalc_Damage` and readable by UI or other systems.
- **Network serialization**: Uses the 8th bit (bit 7) of the existing `RepBits` flags for the critical hit boolean.
- **Duplication**: Properly copies the HitResult when duplicating contexts.

### Global Allocation

`UMKHAbilitySystemGlobals` overrides `AllocGameplayEffectContext` to return `FMKHGameplayEffectContext` instances instead of the default type. This ensures every effect in the game uses the extended context.

---

## Gameplay Tags

**Location**: `Source/Makhia/Public/AbilitySystem/MKHGameplayTags.h`

Tags are declared using `UE_DECLARE_GAMEPLAY_TAG_EXTERN` and defined in the `.cpp` file. They are organized into namespaces:

### Combat

| Tag | String | Purpose |
|---|---|---|
| `Data_Damage` | `Combat.Data.Damage` | `SetByCaller` key for passing damage values |

### State::Movement

Each movement state has a corresponding tag added/removed by `UMovementState::EnterState`/`ExitState`:

| Tag | String |
|---|---|
| `Idle` | `State.Movement.Idle` |
| `Walking` | `State.Movement.Walking` |
| `Sprinting` | `State.Movement.Sprinting` |
| `CrouchingIdle` | `State.Movement.CrouchingIdle` |
| `CrouchingMoving` | `State.Movement.CrouchingMoving` |
| `Jumping` | `State.Movement.Jumping` |
| `Falling` | `State.Movement.Falling` |
| `LandingInPlace` | `State.Movement.LandingInPlace` |
| `LandingMoving` | `State.Movement.LandingMoving` |
| `Dodging` | `State.Movement.Dodging` |

### State (General)

| Tag | String | Purpose |
|---|---|---|
| `OutOfStamina` | `State.General.Stamina.Out` | Added when Stamina reaches 0, removed when fully regenerated |

### Equipment

| Tag | String | Purpose |
|---|---|---|
| `Category_Equipment` | `Item.Equipment` | General equipment category |
| `Category_Weapon` | `Item.Equipment.Weapon` | Weapon category (child of Equipment) |
| `Category_Consumable` | `Item.Consumable` | Consumable item category |
| `ArmorSlot` | `Equipment.Slot.Armor` | Armor equipment slot |
| `WeaponSlot` | `Equipment.Slot.Weapon` | Weapon equipment slot |
| `ConsumableQuickSlot1–3` | `Equipment.Slot.QuickSlot.{First,Second,Third}` | Consumable quick slots |
| `WeaponQuickSlot1–2` | `Equipment.Slot.QuickSlot.Weapon.{Primary,Secondary}` | Weapon quick slots |

### Input

| Tag | String | Purpose |
|---|---|---|
| `Ability` | `Input.Ability` | Filter for ability input actions |
| `QuickSlot` | `Equipment.Slot.QuickSlot` | Filter for quick slot input actions |

---

## Damage Pipeline

The complete damage flow from ability activation to health reduction:

```
1. Ability activates (e.g., UProjectileAbility::SpawnProjectile)
       │
2. CaptureDamageEffectInfo() fills FDamageEffectInfo
       │
3. UMKHAbilitySystemLibrary::ApplyDamageEffect()
       ├─ Creates effect context with source actor
       ├─ Creates outgoing spec from DamageEffect class
       ├─ Sets BaseDamage via SetByCaller (Combat.Data.Damage tag)
       └─ Applies spec to target ASC
              │
4. ExecCalc_Damage::Execute_Implementation()
       ├─ Reads BaseDamage from SetByCaller
       ├─ Captures CritChance and CritDamageMod from source
       ├─ Rolls for critical hit
       ├─ Applies crit multiplier: Damage *= (1 + CritDamageMod)
       ├─ Stores bCriticalHit in FMKHGameplayEffectContext
       └─ Outputs result to IncomingDamage attribute
              │
5. UMKHAttributeSet::PostGameplayEffectExecute()
       └─ HandleIncomingDamage()
              ├─ Calculates shield absorption rate
              ├─ Checks for shield break condition
              ├─ Applies absorbed portion to Shield
              └─ Applies remaining damage to Health
```

---

## Equipment Integration

Equipment items can grant both passive stat effects and active abilities through the GAS.

### Equipping Flow

```
UEquipmentManagerComponent::EquipItem(InventoryItem)
  └─ BuildEquipmentEntry()
       ├─ Rolls rarity via EquipmentRollLibrary::RollRarity()
       ├─ Rolls stat effects via EquipmentRollLibrary::RollPassiveStats()
       └─ Rolls abilities via EquipmentRollLibrary::RollActiveAbilities()
  └─ FMKHEquipmentList::AddEntry()
       ├─ Creates UEquipmentInstance
       ├─ ASC->AddEquipmentEffects()     (applies GE stat modifiers)
       ├─ ASC->AddEquipmentAbility()     (grants abilities)
       └─ Instance->SpawnEquipmentActors()
```

### Effect Handles

`FEquipmentGrantedHandles` tracks both granted ability spec handles and active effect handles, enabling clean removal when the equipment is unequipped:

```cpp
struct FEquipmentGrantedHandles
{
    TArray<FGameplayAbilitySpecHandle> GrantedAbilities;
    TArray<FActiveGameplayEffectHandle> ActiveEffects;
};
```

### Async Loading

Both `AddEquipmentEffects` and `AddEquipmentAbility` support soft class pointers (`TSoftClassPtr`) with asynchronous loading via `FStreamableManager::RequestAsyncLoad`. This prevents hitches when loading gameplay effects or ability classes that have not been loaded yet.

---

## Input Binding

Abilities are activated through a tag-based input system using Enhanced Input.

### Configuration

`UMKHInputConfig` is a data asset that maps `UInputAction` assets to `FGameplayTag` values:

```cpp
USTRUCT()
struct FMKHInputAction
{
    TObjectPtr<UInputAction> InputAction;
    FGameplayTag InputTag;
};
```

### Binding

`UMKHSystemInputComponent` (inherits `UEnhancedInputComponent`) provides a template method `BindAbilityActions` that:

1. Iterates all actions in an `UMKHInputConfig`.
2. Filters by a parent tag (e.g., `Input.Ability`).
3. Binds `ETriggerEvent::Started` → `AbilityInputPressed(Tag)`.
4. Binds `ETriggerEvent::Completed` → `AbilityInputReleased(Tag)`.

### Activation

When a bound input fires, `UMKHAbilitySystemComponent::AbilityInputPressed` scans all activatable abilities for one whose dynamic source tags include the pressed `InputTag`, then calls `TryActivateAbility`.

---

## Data Assets

### UCharacterClassInfo

**Location**: `Source/Makhia/Public/Data/CharacterClassInfo.h`

A `UDataAsset` that maps `FGameplayTag` character class identifiers to their default configuration:

```cpp
USTRUCT()
struct FCharacterClassDefaultInfo
{
    TSubclassOf<UGameplayEffect> DefaultAttributes;     // Initial attribute values
    TArray<TSubclassOf<UGameplayAbility>> StartingAbilities;  // Active abilities
    TArray<TSubclassOf<UGameplayAbility>> StartingPassives;   // Passive abilities
};

UCLASS()
class UCharacterClassInfo : public UDataAsset
{
    TMap<FGameplayTag, FCharacterClassDefaultInfo> ClassDefaultInfoMap;
};
```

Stored on `AMKHGameMode` and accessed via `UMKHAbilitySystemLibrary::GetCharacterClassDefaultInfo`.

### UProjectileInfo

**Location**: `Source/Makhia/Public/Data/ProjectileInfo.h`

Maps `FGameplayTag` to `FProjectileParams`:

```cpp
UCLASS()
class UProjectileInfo : public UDataAsset
{
    TMap<FGameplayTag, FProjectileParams> ProjectileInfoMap;
};
```

`FProjectileParams` defines:
- `ProjectileClass` — the actor class to spawn.
- `ProjectileMesh` — the static mesh for the projectile.
- `InitialSpeed`, `GravityScale` — physics configuration.
- `bShouldBounce`, `Bounciness` — bounce behaviour.


