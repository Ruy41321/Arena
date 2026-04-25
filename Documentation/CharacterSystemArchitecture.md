# Character System Architecture

## Table of Contents

1. [Overview](#overview)
2. [Class Hierarchy](#class-hierarchy)
3. [ACharacterBase](#acharacterbase)
4. [AMKHPlayerCharacter](#AMKHPlayerCharacter)
5. [AEnemyBase](#aenemybase)
6. [Movement Components](#movement-components)
7. [Movement State Machine](#movement-state-machine)
8. [Animation System](#animation-system)
9. [Camera Setup](#camera-setup)
10. [Input System](#input-system)
11. [Adding a New Movement State](#adding-a-new-movement-state)
12. [Adding a New Observer](#adding-a-new-observer)

---

## Overview

The Makhia character system uses a **component-based architecture** combined with a **finite state machine** for movement management and an **observer pattern** for state-change notifications. This design separates each movement ability into its own component, delegates state management to a dedicated state machine, and notifies dependent systems (such as animation) through delegates.

Key principles:
- **Single Responsibility**: Each component handles exactly one movement mechanic.
- **Open/Closed**: New movement states can be added without modifying existing state classes.
- **Decoupled Communication**: Systems react to state changes through delegates, not direct coupling.
- **Blueprint Accessibility**: All major properties and functions are exposed for designer iteration.

---

## Class Hierarchy

```
ACharacter (Engine)
  └─ ACharacterBase                  [IAbilitySystemInterface]
       ├─ AMKHPlayerCharacter           [IMKHAbilitySystemInterface]
       └─ AEnemyBase
```

---

## ACharacterBase

**Location**: `Source/Makhia/Public/Character/CharacterBase.h`

The abstract base class shared by players and enemies. It provides the GAS integration point and common attribute-change delegates.

### Responsibilities

- Implements `IAbilitySystemInterface::GetAbilitySystemComponent()`.
- Declares virtual hooks for GAS initialisation: `InitAbilityActorInfo()`, `BindCallbacksToDependencies()`, `InitClassDefaults()`, `BroadcastInitialValues()`.
- Manages the `OutOfStamina` gameplay tag in `HandleStaminaChanged()`.

### Key Properties

```cpp
UPROPERTY(EditAnywhere, Category = "Custom Values | Character Info")
FGameplayTag CharacterTag;                              // Identifies the character class for data lookup

TObjectPtr<UMKHAbilitySystemComponent> MKHAbilitySystemComponent;
TObjectPtr<UMKHAttributeSet> MKHAttributeSet;
```

### Delegates

| Delegate | Signature | Fired When |
|---|---|---|
| `OnHealthChanged` | `(float OldHealth, float CurrentHealth, float MaxHealth)` | Health attribute changes |
| `OnStaminaChanged` | `(float OldStamina, float CurrentStamina, float MaxStamina)` | Stamina attribute changes |
| `OnShieldChanged` | `(float OldShield, float CurrentShield, float MaxShield)` | Shield attribute changes |

---

## AMKHPlayerCharacter

**Location**: `Source/Makhia/Public/Player/MKHPlayerCharacter.h`

The player-controlled character. It owns the movement components, state machine, and camera, and delegates GAS ownership to `AMKHPlayerState`.

### Components Created in Constructor

```cpp
// Camera
TObjectPtr<USpringArmComponent> CameraBoom;
TObjectPtr<UCameraComponent> Camera;

// Movement Systems
TObjectPtr<UDodgeSystemComponent> DodgeSystem;
TObjectPtr<UCrouchSystemComponent> CrouchSystem;
TObjectPtr<UBasicMovementComponent> BasicMovementSystem;
TObjectPtr<UJumpSystemComponent> JumpSystem;
TObjectPtr<USprintSystemComponent> SprintSystem;

// State Management
TObjectPtr<UMovementStateMachine> MovementStateMachine;

// Projectile spawn point for GAS abilities
TObjectPtr<USceneComponent> DynamicProjectileSpawnPoint;
```

### GAS Initialisation

The ASC and attribute set are obtained from `AMKHPlayerState` in `InitAbilityActorInfo()`:

```
PossessedBy() [Server]
  └─ InitAbilityActorInfo()
       ├─ ASC = PlayerState->GetMKHAbilitySystemComponent()
       ├─ AttributeSet = PlayerState->GetMKHAttributeSet()
       ├─ ASC->InitAbilityActorInfo(PlayerState, this)
       ├─ BindCallbacksToDependencies()
       ├─ MovementStateMachine->SyncCurrentStateTagToASC()
       └─ InitClassDefaults()  [Authority only]

OnRep_PlayerState() [Client]
  └─ InitAbilityActorInfo()   (same flow, minus InitClassDefaults)
```

### Input Setup

`SetupPlayerInputComponent` casts to `UEnhancedInputComponent` and delegates binding to each movement component's `SetupInput` method.

### Public Helpers

| Method | Description |
|---|---|
| `SetMaxWalkSpeed(float)` | Sets `CharacterMovement->MaxWalkSpeed` |
| `GetCurrentMovementState()` | Returns `MovementStateMachine->GetCurrentState()` |
| `GetPreviousMovementState()` | Returns `MovementStateMachine->GetPreviousState()` |
| `TransitionToMovementState(NewState, bForce)` | Calls `MovementStateMachine->TransitionToState()` |
| `GetDynamicSpawnPoint_Implementation()` | Returns the `DynamicProjectileSpawnPoint` scene component |

---

## AEnemyBase

**Location**: `Source/Makhia/Public/Character/EnemyBase.h`

AI-controlled characters. Unlike the player, enemies own their ASC directly.

### Key Differences from Player

| Aspect | Player | Enemy |
|---|---|---|
| ASC Owner | `AMKHPlayerState` | `AEnemyBase` itself |
| ASC Actor Info | `InitAbilityActorInfo(PlayerState, this)` | `InitAbilityActorInfo(this, this)` |
| Replication Mode | `Mixed` | `Minimal` |
| Init Trigger | `PossessedBy` / `OnRep_PlayerState` | `BeginPlay` |

Enemies replicate a `bInitAttributes` flag so clients can broadcast initial values after attribute setup completes server-side.

---

## Movement Components

All movement components inherit from `UActorComponent` and follow a consistent pattern: each component receives input, manages its own state, and coordinates with other components through the shared `AMKHPlayerCharacter` reference.

### UBasicMovementComponent

**Location**: `Source/Makhia/Public/Player/Components/BasicMovement/BasicMovementComponent.h`

- Processes WASD/gamepad movement input and mouse/gamepad look input.
- Tracks whether the character has active movement input (used by other systems).
- Manages movement smoothing and sensitivity settings.

### UDodgeSystemComponent

**Location**: `Source/Makhia/Public/Player/Components/Dodge/DodgeSystemComponent.h`

- Handles dodge initiation, direction blending, and cooldown.
- Checks the current movement state against a dodgeable-state whitelist (`Idle`, `CrouchingIdle`, `CrouchingMoving`, `Walking`, `Sprinting`).
- Remembers pre-dodge crouch state and restores it after the dodge ends.
- Replicates `bIsDodging` for multiplayer; has a Server RPC for `StartDodge`.
- Broadcasts `OnDodgeFinishedDelegate` when the dodge ends.

### UCrouchSystemComponent

**Location**: `Source/Makhia/Public/Player/Components/Crouch/CrouchSystemComponent.h`

- Toggles crouching with collision-safe uncrouch checks.
- Smoothly adjusts capsule height transitions.
- Provides `CanUncrouchSafely()` used by Sprint and Dodge components.
- Manages crouch speed settings.

### UJumpSystemComponent

**Location**: `Source/Makhia/Public/Player/Components/Jump/JumpSystemComponent.h`

- Handles jump input and delegates to `ACharacter::Jump()`.
- Tracks landing state and timing for animation.
- Manages jump cooldown.
- Receives `Landed()` callbacks from `AMKHPlayerCharacter`.

### USprintSystemComponent

**Location**: `Source/Makhia/Public/Player/Components/Sprint/SprintSystemComponent.h`

- Toggles sprinting on/off.
- Automatically uncrouches when sprint starts (with collision check).
- Tracks sprint interruption conditions.
- Prevents sprinting while dodging.

### Component Cooperation

Components coordinate through the shared `AMKHPlayerCharacter` reference:

**Sprint ↔ Crouch**: Sprint auto-uncrouches if `CanUncrouchSafely()` returns true; otherwise sprint is cancelled.

**Dodge ↔ Crouch**: Dodge records whether the character was crouching before the dodge (`bWasCrouchingPreDodge`) and restores the crouch state on dodge end.

**Dodge ↔ State Machine**: Before dodging, the component checks `MovementStateMachine->GetCurrentState()` against the dodgeable-state whitelist.

---

## Movement State Machine

### UMovementStateMachine

**Location**: `Source/Makhia/Public/Player/MovementStateMachine/MovementStateMachine.h`

An `UActorComponent` that manages a map of `EMovementStateValue` → `UMovementState*` objects.

**Tick behaviour**:
1. Calls `UpdateState(DeltaTime)` on the current state.
2. Calls `EvaluateStateTransitions()` which asks the current state for a `GetDesiredTransition()`. If the returned value is not `None`, it attempts a transition.

**Transition logic**:
- `TransitionToState(NewState, bForceTransition)` returns `false` if already in the target state or if the current state's `CanTransitionTo()` returns `false` (unless forced).
- `PerformStateTransition(NewState)` calls `ExitState` on the old state, updates `PreviousState`/`CurrentState`, calls `EnterState` on the new state, and broadcasts `OnStateChanged`.

### EMovementStateValue

**Location**: `Source/Makhia/Public/Player/MovementStateMachine/MovementStateTypes.h`

```cpp
UENUM(BlueprintType)
enum class EMovementStateValue : uint8
{
    None              UMETA(DisplayName = "None"),
    Idle              UMETA(DisplayName = "Idle"),
    Walking           UMETA(DisplayName = "Walking"),
    Sprinting         UMETA(DisplayName = "Sprinting"),
    CrouchingIdle     UMETA(DisplayName = "Crouching Idle"),
    CrouchingMoving   UMETA(DisplayName = "Crouching Moving"),
    Jumping           UMETA(DisplayName = "Jumping"),
    Falling           UMETA(DisplayName = "Falling"),
    LandingInPlace    UMETA(DisplayName = "Landing In Place"),
    LandingMoving     UMETA(DisplayName = "Landing Moving"),
    Dodging           UMETA(DisplayName = "Dodging")
};
```

### UMovementStateTypes Utilities

Static helper class with:
- `MovementStateToString(EMovementStateValue)` — enum to display string.
- `IsGroundedState()` — returns `true` for Idle, Walking, Sprinting, CrouchingIdle, CrouchingMoving, LandingInPlace, LandingMoving, Dodging.
- `IsAirborneState()` — returns `true` for Jumping, Falling.
- `CanReceiveMovementInput()` — returns `true` for Idle, Walking, Sprinting, CrouchingIdle, CrouchingMoving, Jumping, Falling.

### UMovementState (Base)

**Location**: `Source/Makhia/Public/Player/MovementStateMachine/MovementState.h`

Abstract base for all movement states.

**Lifecycle**:
- `EnterState(PreviousState)` — Sets movement speed via `SetStateSpeed()`, adds the state's gameplay tag to the ASC, calls the Blueprint event `OnEnterState`.
- `UpdateState(DeltaTime)` — Calls the Blueprint event `OnUpdateState`.
- `ExitState(NextState)` — Removes the state's gameplay tag from the ASC, calls the Blueprint event `OnExitState`.

**Transition hooks** (BlueprintNativeEvent):
- `CanTransitionTo(NewState)` — Default returns `true`; subclasses override to restrict transitions.
- `GetDesiredTransition()` — Default returns `None`; subclasses override for automatic transitions.

**Speed management** (`GetSpeedForState`):
| State | Default Speed |
|---|---|
| Dodging | `DodgeSystem->DodgeSpeed` (typically 600) |
| CrouchingIdle / CrouchingMoving | `CrouchSystem->CrouchSpeed` (typically 150) |
| Sprinting | `SprintSystem->SprintSpeed` (typically 600) |
| All others | `BasicMovementSystem->BaseMovementSpeed` (typically 300) |

### Concrete State Classes

Located in `Source/Makhia/Public/Player/MovementStateMachine/States/`:

| State Class | Auto-Transitions To |
|---|---|
| `UIdleMovementState` | Walking, Sprinting, Jumping, CrouchingIdle |
| `UWalkingMovementState` | Idle, Sprinting, Jumping, Dodging, CrouchingMoving |
| `USprintingMovementState` | Walking, Idle, Jumping |
| `UCrouchingIdleMovementState` | CrouchingMoving, Idle, Dodging |
| `UCrouchingMovingMovementState` | CrouchingIdle, Walking, Dodging |
| `UJumpingMovementState` | Falling |
| `UFallingMovementState` | LandingInPlace, LandingMoving |
| `ULandingInPlaceMovementState` | Idle |
| `ULandingMovingMovementState` | Walking |
| `UDodgingMovementState` | Idle, Walking (when dodge ends) |

### State Transition Diagram

```
                    ┌──────────────┐
         ┌─────────┤    Idle      ├──────────┐
         │         └──────┬───────┘          │
         │                │                  │
         ▼                ▼                  ▼
  ┌─────────────┐  ┌─────────────┐   ┌──────────────┐
  │ CrouchIdle  │  │   Walking   │   │   Jumping    │
  └──────┬──────┘  └──────┬──────┘   └──────┬───────┘
         │                │                  │
         ▼                ▼                  ▼
  ┌─────────────┐  ┌─────────────┐   ┌──────────────┐
  │ CrouchMove  │  │  Sprinting  │   │   Falling    │
  └──────┬──────┘  └─────────────┘   └──────┬───────┘
         │                                   │
         │         ┌─────────────┐           │
         └────────►│   Dodging   │◄──────────┘
                   └──────┬──────┘     ┌─────────────┐
                          │            │ LandInPlace  │
                          └───────────►└─────────────┘
                                       ┌─────────────┐
                                       │ LandMoving  │
                                       └─────────────┘
```

### Observer Pattern — OnStateChanged

The state machine exposes a dynamic multicast delegate:

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMovementStateChanged,
    EMovementStateValue, OldState,
    EMovementStateValue, NewState);

UPROPERTY(BlueprintAssignable, Category = "Movement State Machine")
FOnMovementStateChanged OnStateChanged;
```

Any system can subscribe. The animation instance (`UMKHMKHPlayerAnimInstance`) is the primary subscriber:

```cpp
void UMKHMKHPlayerAnimInstance::SubscribeToMovementStateChanges()
{
    PlayerCharacter->MovementStateMachine->OnStateChanged.AddDynamic(
        this, &UMKHMKHPlayerAnimInstance::OnMovementStateChanged);
}
```

`UnsubscribeFromStateChanges(UObject*)` removes all bindings for a given subscriber.

---

## Animation System

**Class**: `UMKHMKHPlayerAnimInstance` (inherits `UAnimInstance`)  
**Location**: `Source/Makhia/Public/Player/PlayerAnimation/MKHMKHPlayerAnimInstance.h`

### Key Properties

| Property | Type | Purpose |
|---|---|---|
| `Speed` | `float` | Character speed for blend spaces |
| `CurrentMovementState` | `EMovementStateValue` | Current state for animation state machine |
| `PreviousMovementState` | `EMovementStateValue` | Previous state for transition logic |
| `CrouchingTransitionTime` | `float` | 0–100 blend value for crouching pose |
| `CrouchingTransitionTarget` | `float` | Target value: 100 when crouched, 0 when standing |

### Lifecycle

1. **`NativeBeginPlay`**: Gets `AMKHPlayerCharacter` reference, initialises Speed/State to defaults, subscribes to state changes.
2. **`OnMovementStateChanged`**: Updates state properties, snaps crouch transition value, reads `MaxWalkSpeed` for blend space.
3. **`NativeUninitializeAnimation`**: Unsubscribes from state changes.

---

## Camera Setup

`AMKHPlayerCharacter` creates a standard third-person camera rig in its constructor:

```cpp
CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Boom"));
CameraBoom->SetupAttachment(RootComponent);
CameraBoom->bUsePawnControlRotation = true;

Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Player Camera"));
Camera->SetupAttachment(CameraBoom);
```

The character uses `bUseControllerRotationYaw = false` with `CharacterMovement->bOrientRotationToMovement = true` so the character rotates to face the direction of movement while the camera follows the controller rotation.

---

## Input System

Makhia uses the Enhanced Input system. Each movement component exposes a `SetupInput(UEnhancedInputComponent*)` method called from `AMKHPlayerCharacter::SetupPlayerInputComponent`.

Components bind their specific `UInputAction` properties (set via Blueprint/editor) to trigger events:
- `ETriggerEvent::Started` / `ETriggerEvent::Triggered` for input start.
- `ETriggerEvent::Completed` for input release.

For ability inputs, `UMKHSystemInputComponent::BindAbilityActions` iterates an `UMKHInputConfig` data asset, filters by a parent gameplay tag, and binds each action to `AbilityInputPressed`/`AbilityInputReleased` on the ASC.

---

## Adding a New Movement State

### Step 1 — Add the Enum Value

In `MovementStateTypes.h`, add the new value to `EMovementStateValue`:

```cpp
enum class EMovementStateValue : uint8
{
    // ... existing values ...
    Dodging           UMETA(DisplayName = "Dodging"),
    YourNewState      UMETA(DisplayName = "Your New State")
};
```

### Step 2 — Create the State Class

Create `YourNewStateMovementState.h` and `.cpp` in `Source/Makhia/Public/Player/MovementStateMachine/States/`:

```cpp
UCLASS(BlueprintType, Blueprintable)
class Makhia_API UYourNewStateMovementState : public UMovementState
{
    GENERATED_BODY()

public:
    virtual EMovementStateValue GetStateType() const override
    {
        return EMovementStateValue::YourNewState;
    }

    virtual EMovementStateValue GetDesiredTransition_Implementation() const override;
    virtual bool CanTransitionTo_Implementation(EMovementStateValue NewState) const override;

    virtual void EnterState(EMovementStateValue PreviousState) override;
    virtual void UpdateState(float DeltaTime) override;
    virtual void ExitState(EMovementStateValue NextState) override;
};
```

### Step 3 — Register the State

In `MovementStateMachine.cpp`, add to `InitializeDefaultStates()`:

```cpp
DefaultStateClasses.Add(EMovementStateValue::YourNewState, UYourNewStateMovementState::StaticClass());
```

### Step 4 — Update Utilities

In `MovementStateTypes.cpp`, add the new state to `MovementStateToString`, `IsGroundedState`, `IsAirborneState`, and `CanReceiveMovementInput` as appropriate.

### Step 5 — Add a Gameplay Tag

In `MKHGameplayTags.h/.cpp`, declare and define a tag for the new state:

```cpp
UE_DECLARE_GAMEPLAY_TAG_EXTERN(YourNewState);
UE_DEFINE_GAMEPLAY_TAG_COMMENT(YourNewState, "State.Movement.YourNewState", "Description");
```

Then add the mapping in `UMovementState::GetTagForState`.

---

## Adding a New Observer

To subscribe a new system to movement state changes:

```cpp
UCLASS()
class UMySystem : public UObject
{
    GENERATED_BODY()

public:
    void Subscribe(AMKHPlayerCharacter* PC)
    {
        PC->MovementStateMachine->OnStateChanged.AddDynamic(
            this, &UMySystem::OnMovementStateChanged);
    }

    UFUNCTION()
    void OnMovementStateChanged(EMovementStateValue OldState, EMovementStateValue NewState)
    {
        // React to state change
    }
};
```

For cleanup, call `RemoveDynamic` or use `MovementStateMachine->UnsubscribeFromStateChanges(this)`.

Blueprint subscribers can bind to `OnStateChanged` directly in the Event Graph.


