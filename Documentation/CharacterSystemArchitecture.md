# Arena Character System Architecture Documentation

## Table of Contents
1. [System Overview](#system-overview)
2. [Architecture Components](#architecture-components)
3. [Movement State Machine](#movement-state-machine)
4. [Component Interactions](#component-interactions)
5. [Observer Pattern Implementation](#observer-pattern-implementation)
6. [Adding New Movement States](#adding-new-movement-states)
7. [Implementing Observer Pattern Subscribers](#implementing-observer-pattern-subscribers)
8. [Input System Integration](#input-system-integration)
9. [Best Practices](#best-practices)

## System Overview

The Arena character system is built using a **component-based architecture** with a **finite state machine** for movement management and an **observer pattern** for state change notifications. This design provides modularity, extensibility, and clean separation of concerns.

### Key Design Principles
- **Component-Based Design**: Each movement ability is encapsulated in its own component
- **State Machine Pattern**: Centralized movement state management with automatic transitions
- **Observer Pattern**: Decoupled notifications for state changes
- **Enhanced Input Integration**: Modern Unreal Engine input system support
- **Blueprint Accessibility**: All systems exposed to Blueprint for designer control

## Architecture Components

### Core Classes

#### APlayerCharacter
The main character class that orchestrates all movement systems.

**Location**: `Source/Arena/Player/PlayerCharacter.h/cpp`

**Key Responsibilities**:
- Manages all movement components
- Provides unified interface for state machine access
- Handles Enhanced Input setup delegation
- Exposes Blueprint-accessible functions

**Key Components**:
```cpp
// Movement Systems
TObjectPtr<UBasicMovementComponent> BasicMovementSystem;
TObjectPtr<UDodgeSystemComponent> DodgeSystem;
TObjectPtr<UCrouchSystemComponent> CrouchSystem;
TObjectPtr<UJumpSystemComponent> JumpSystem;
TObjectPtr<USprintSystemComponent> SprintSystem;

// State Management
TObjectPtr<UMovementStateMachine> MovementStateMachine;

// Camera System
TObjectPtr<USpringArmComponent> CameraBoom;
TObjectPtr<UCameraComponent> Camera;
```

#### Movement Components

All movement components inherit from `UActorComponent` and follow a consistent pattern:

##### UBasicMovementComponent
**Location**: `Source/Arena/Components/BasicMovement/`
- Handles basic movement input (WASD/Gamepad)
- Manages look input (mouse/gamepad)
- Provides movement smoothing
- Updates movement input tracking for other systems

##### UDodgeSystemComponent  
**Location**: `Source/Arena/Components/Dodge/`
- Manages dodge mechanics with direction blending
- Handles dodge cooldown and duration
- Integrates with crouching system
- Supports input-influenced dodge direction

##### UCrouchSystemComponent
**Location**: `Source/Arena/Components/Crouch/`
- Handles crouching/uncrouching with smooth transitions
- Performs collision checking for safe uncrouching
- Manages capsule component height adjustments
- Provides crouch speed settings

##### UJumpSystemComponent
**Location**: `Source/Arena/Components/Jump/`
- Manages jump input and execution
- Tracks landing state for animations
- Handles jump cooldown
- Integrates with Unreal's CharacterMovementComponent

##### USprintSystemComponent
**Location**: `Source/Arena/Components/Sprint/`
- Handles sprint toggling
- Manages sprint interruption conditions
- Integrates with crouching system (auto-uncrouch when sprinting)
- Prevents sprinting while dodging

## Movement State Machine

### Core State Machine Classes

#### UMovementStateMachine
**Location**: `Source/Arena/Player/MovementStateMachine/MovementStateMachine.h/cpp`

**Key Features**:
- Manages state transitions and validation
- Fires state change events via delegate
- Supports both automatic and manual transitions
- Provides Blueprint-accessible interface
- Maintains current and previous state tracking

#### EMovementState Enumeration
**Location**: `Source/Arena/Player/MovementStateMachine/MovementStateTypes.h`

**Available States**:
```cpp
enum class EMovementState : uint8
{
    None = 0,
    Idle,
    Walking,
    Sprinting, 
    CrouchingIdle,
    CrouchingMoving,
    Jumping,
    Falling,
    LandingInPlace,
    LandingMoving,
    Dodging
};
```

#### UMovementState Base Class
**Location**: `Source/Arena/Player/MovementStateMachine/MovementState.h/cpp`

**Abstract base class for all movement states providing**:
- Virtual methods for state lifecycle (Enter, Update, Exit)
- Blueprint implementable events
- Transition validation logic
- Speed management for each state
- Access to PlayerCharacter and StateMachine references

### Individual State Classes

Each movement state inherits from `UMovementState` and implements specific behavior:

#### UIdle MovementState
- Default state when no input is detected
- Transitions to Walking/Sprinting on movement input
- Transitions to Jumping on jump input
- Transitions to Crouching on crouch input

#### UWalkingMovementState  
- Active during normal movement
- Transitions to Idle when movement stops
- Transitions to Sprinting when sprint is activated
- Can transition to Jumping or Dodging

#### USprintingMovementState
- High-speed movement state
- Requires movement input to maintain
- Transitions to Walking when sprint is released
- Auto-transitions to Walking if movement stops

#### UCrouchingIdleMovementState / UCrouchingMovingMovementState
- Crouched variants of Idle and Walking
- Reduced movement speed
- Lower profile for tactical movement
- Smooth transitions with capsule height adjustments

#### UJumpingMovementState / UFallingMovementState
- Airborne states for jump/fall mechanics
- Automatic transitions based on character movement state
- Integration with Unreal's built-in falling detection

#### ULandingInPlaceMovementState / ULandingMovingMovementState
- Brief states after landing for animation timing
- Automatically transition to appropriate ground states
- Provide landing feedback opportunities

#### UDodgingMovementState
- Special evasive movement state
- Fixed duration with automatic reset
- High movement speed with directional control
- Cannot be interrupted except by completion

## Component Interactions

### State Machine Integration

Each movement component interacts with the state machine through the PlayerCharacter:

```cpp
// Example: Dodge component checking current state
EMovementState CurrentState = PlayerCharacter->MovementStateMachine->GetCurrentState();
if (IsInDodgeableState(CurrentState))
{
    // Perform dodge logic
}
```

### Component Cooperation

Components coordinate through shared state and cross-component calls:

**Sprint + Crouch Interaction**:
```cpp
// Sprint component auto-uncrouches when sprint starts
if (CurrentState == EMovementState::CrouchingIdle || CurrentState == EMovementState::CrouchingMoving)
{
    if (!PlayerCharacter->CrouchSystem->CanUncrouchSafely())
    {
        bIsSprinting = false;
        return;
    }
    PlayerCharacter->CrouchSystem->CrouchPressed(Value);
}
```

**Dodge + Crouch Interaction**:
```cpp
// Dodge remembers pre-dodge crouch state
bWasCrouchingPreDodge = (CurrentState == EMovementState::CrouchingIdle || 
                         CurrentState == EMovementState::CrouchingMoving);
```

### Input Flow

1. **PlayerCharacter::SetupPlayerInputComponent** delegates input setup to each component
2. Each component binds its specific input actions
3. Input events trigger component-specific functions
4. Components request state transitions through MovementStateMachine
5. State machine validates transitions and fires events
6. Animation system receives state change notifications

## Observer Pattern Implementation

### State Change Notification System

The system uses Unreal's dynamic multicast delegate system for state change notifications:

#### FOnMovementStateChanged Delegate
```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMovementStateChanged, 
    EMovementState, OldState, 
    EMovementState, NewState);
```

#### MovementStateMachine Publisher
```cpp
// In UMovementStateMachine
UPROPERTY(BlueprintAssignable, Category = "Movement State Machine")
FOnMovementStateChanged OnStateChanged;

// When state changes
void UMovementStateMachine::PerformStateTransition(EMovementState NewState)
{
    EMovementState OldState = CurrentState;
    // ... transition logic ...
    OnStateChanged.Broadcast(OldState, NewState);
}
```

#### PlayerAnimInstance Subscriber

The animation instance demonstrates the observer pattern implementation:

**Subscription Setup**:
```cpp
void UPlayerAnimInstance::SubscribeToMovementStateChanges()
{
    if (!PlayerCharacter || !PlayerCharacter->MovementStateMachine || bIsSubscribedToStateChanges)
        return;
    
    PlayerCharacter->MovementStateMachine->OnStateChanged.AddDynamic(
        this, &UPlayerAnimInstance::OnMovementStateChanged);
    bIsSubscribedToStateChanges = true;
    
    // Initialize current state values
    CurrentMovementState = PlayerCharacter->MovementStateMachine->GetCurrentState();
    PreviousMovementState = PlayerCharacter->MovementStateMachine->GetPreviousState();
}
```

**Event Handler**:
```cpp
UFUNCTION()
void UPlayerAnimInstance::OnMovementStateChanged(EMovementState OldState, EMovementState NewState)
{
    // Update animation properties based on state change
    PreviousMovementState = CurrentMovementState;
    CurrentMovementState = NewState;
    
    // Update animation speed and other properties
    if (PlayerCharacter && PlayerCharacter->GetCharacterMovement())
    {
        Speed = PlayerCharacter->GetCharacterMovement()->MaxWalkSpeed;
        if (NewState == EMovementState::Idle || NewState == EMovementState::CrouchingIdle)
        {
            Speed *= 0.0f;
        }
    }
}
```

**Cleanup**:
```cpp
void UPlayerAnimInstance::UnsubscribeFromMovementStateChanges()
{
    if (!bIsSubscribedToStateChanges || !PlayerCharacter || !PlayerCharacter->MovementStateMachine)
        return;
    
    PlayerCharacter->MovementStateMachine->OnStateChanged.RemoveDynamic(
        this, &UPlayerAnimInstance::OnMovementStateChanged);
    bIsSubscribedToStateChanges = false;
}
```

## Adding New Movement States

### Step 1: Define the New State Enum

Add your new state to the `EMovementState` enum in `MovementStateTypes.h`:

```cpp
enum class EMovementState : uint8
{
    // ... existing states ...
    Dodging             UMETA(DisplayName = "Dodging"),
    YourNewState        UMETA(DisplayName = "Your New State")  // Add this line
};
```

### Step 2: Create State Class Files

Create new header and source files following the naming convention:
- `YourNewStateMovementState.h`
- `YourNewStateMovementState.cpp`

### Step 3: Implement State Class

**Header File Template**:
```cpp
#pragma once

#include "CoreMinimal.h"
#include "../MovementState.h"
#include "YourNewStateMovementState.generated.h"

UCLASS(BlueprintType, Blueprintable)
class ARENA_API UYourNewStateMovementState : public UMovementState
{
    GENERATED_BODY()

public:
    UYourNewStateMovementState();
    
    virtual EMovementState GetStateType() const override { return EMovementState::YourNewState; }
    virtual EMovementState GetDesiredTransition_Implementation() const override;
    virtual bool CanTransitionTo_Implementation(EMovementState NewState) const override;
    
    virtual void EnterState(EMovementState PreviousState) override;
    virtual void UpdateState(float DeltaTime) override;
    virtual void ExitState(EMovementState NextState) override;

protected:
    // State-specific properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Your New State")
    float YourStateProperty = 1.0f;
};
```

**Source File Template**:
```cpp
#include "YourNewStateMovementState.h"
#include "../../PlayerCharacter.h"

UYourNewStateMovementState::UYourNewStateMovementState()
{
    // Initialize state-specific properties
}

EMovementState UYourNewStateMovementState::GetDesiredTransition_Implementation() const
{
    // Implement automatic transition logic
    APlayerCharacter* Player = GetPlayerCharacter();
    if (!Player) return EMovementState::None;
    
    // Example transition logic
    if (SomeCondition())
    {
        return EMovementState::Idle;
    }
    
    return EMovementState::None; // No transition desired
}

bool UYourNewStateMovementState::CanTransitionTo_Implementation(EMovementState NewState) const
{
    // Define which states this state can transition to
    switch (NewState)
    {
    case EMovementState::Idle:
    case EMovementState::Walking:
        return true;
    default:
        return false;
    }
}

void UYourNewStateMovementState::EnterState(EMovementState PreviousState)
{
    Super::EnterState(PreviousState);
    
    // Set appropriate movement speed
    SetStateSpeed();
    
    // Perform enter logic
}

void UYourNewStateMovementState::UpdateState(float DeltaTime)
{
    Super::UpdateState(DeltaTime);
    
    // Perform per-frame logic
}

void UYourNewStateMovementState::ExitState(EMovementState NextState)
{
    Super::ExitState(NextState);
    
    // Perform cleanup logic
}
```

### Step 4: Register State in StateMachine

Add your state class to the default state classes map in `MovementStateMachine.cpp`:

```cpp
void UMovementStateMachine::InitializeDefaultStates()
{
    // ... existing state registrations ...
    DefaultStateClasses.Add(EMovementState::YourNewState, UYourNewStateMovementState::StaticClass());
}
```

### Step 5: Add Utility Functions

Update `MovementStateTypes.cpp` to include your new state in utility functions:

```cpp
FString UMovementStateTypes::MovementStateToString(EMovementState State)
{
    switch (State)
    {
        // ... existing cases ...
        case EMovementState::YourNewState: return TEXT("YourNewState");
        default: return TEXT("Unknown");
    }
}

bool UMovementStateTypes::IsGroundedState(EMovementState State)
{
    switch (State)
    {
        // ... existing grounded states ...
        case EMovementState::YourNewState: return true; // if it's a grounded state
        default: return false;
    }
}
```

## Implementing Observer Pattern Subscribers

### Creating a New Subscriber Class

To create a new class that observes movement state changes:

```cpp
// In your header file
UCLASS()
class ARENA_API UYourSubscriberClass : public UObject
{
    GENERATED_BODY()

public:
    // Subscription management
    void SubscribeToMovementStateChanges(APlayerCharacter* PlayerCharacter);
    void UnsubscribeFromMovementStateChanges();

    // Event handler - must be UFUNCTION() for dynamic delegates
    UFUNCTION()
    void OnMovementStateChanged(EMovementState OldState, EMovementState NewState);

private:
    UPROPERTY()
    TObjectPtr<APlayerCharacter> CachedPlayerCharacter;
    
    bool bIsSubscribed = false;
};
```

```cpp
// In your source file
void UYourSubscriberClass::SubscribeToMovementStateChanges(APlayerCharacter* PlayerCharacter)
{
    if (!PlayerCharacter || !PlayerCharacter->MovementStateMachine || bIsSubscribed)
        return;
    
    CachedPlayerCharacter = PlayerCharacter;
    PlayerCharacter->MovementStateMachine->OnStateChanged.AddDynamic(
        this, &UYourSubscriberClass::OnMovementStateChanged);
    bIsSubscribed = true;
}

void UYourSubscriberClass::UnsubscribeFromMovementStateChanges()
{
    if (!bIsSubscribed || !CachedPlayerCharacter || !CachedPlayerCharacter->MovementStateMachine)
        return;
    
    CachedPlayerCharacter->MovementStateMachine->OnStateChanged.RemoveDynamic(
        this, &UYourSubscriberClass::OnMovementStateChanged);
    bIsSubscribed = false;
    CachedPlayerCharacter = nullptr;
}

void UYourSubscriberClass::OnMovementStateChanged(EMovementState OldState, EMovementState NewState)
{
    // React to state changes
    switch (NewState)
    {
    case EMovementState::Dodging:
        // Handle dodge start
        break;
    case EMovementState::Jumping:
        // Handle jump start  
        break;
    // ... other states
    }
}
```

### Blueprint Subscriber Implementation

You can also subscribe to state changes from Blueprint:

1. **Create a Blueprint Event Handler**:
   - Create a custom event that matches the `FOnMovementStateChanged` signature
   - Name it appropriately (e.g., "OnMovementStateChanged")

2. **Bind in BeginPlay**:
   ```cpp
   // In Blueprint or C++
   PlayerCharacter->MovementStateMachine->OnStateChanged.AddDynamic(
       this, &YourClass::YourEventFunction);
   ```

3. **Handle State Changes**:
   - Implement your reaction logic in the event handler
   - Use the OldState and NewState parameters to determine behavior

## Input System Integration

### Enhanced Input Setup

The system uses Unreal Engine 5's Enhanced Input system:

#### Input Action Assets
Create Input Action assets in the Content Browser for each action:
- `IA_Move` (Vector2D) - Movement input
- `IA_Look` (Vector2D) - Camera look input  
- `IA_Jump` (Boolean) - Jump input
- `IA_Sprint` (Boolean) - Sprint input
- `IA_Crouch` (Boolean) - Crouch input
- `IA_Dodge` (Boolean) - Dodge input

#### Input Mapping Context
Create an Input Mapping Context asset that maps:
- WASD → `IA_Move`
- Mouse Movement → `IA_Look`
- Space → `IA_Jump`
- Left Shift → `IA_Sprint`
- Left Ctrl → `IA_Crouch`
- Alt → `IA_Dodge`

#### Component Input Setup Pattern

Each movement component follows this pattern:

```cpp
void UMovementComponent::SetupInput(UEnhancedInputComponent* EnhancedInputComponent)
{
    if (!EnhancedInputComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("ComponentName: EnhancedInputComponent is null"));
        return;
    }

    if (!InputAction)
    {
        UE_LOG(LogTemp, Warning, TEXT("ComponentName: InputAction is not set"));
        return;
    }

    // Bind input events
    EnhancedInputComponent->BindActionValueLambda(InputAction, ETriggerEvent::Started,
        [this](const FInputActionValue& Value) {
            HandleInputStarted(Value);
        });
    
    EnhancedInputComponent->BindActionValueLambda(InputAction, ETriggerEvent::Completed,
        [this](const FInputActionValue& Value) {
            HandleInputCompleted(Value);
        });
}
```

## Best Practices

### Performance Considerations

1. **Component Tick Management**:
   - Disable ticking for components that don't need per-frame updates
   - Use event-driven logic instead of polling when possible
   - Cache frequently accessed references

2. **State Machine Efficiency**:
   - Minimize state transition validation overhead
   - Use early returns in transition checks
   - Avoid complex calculations in tick functions

3. **Memory Management**:
   - Use `TObjectPtr<>` for UPROPERTY object references
   - Properly unsubscribe from delegates in destructors
   - Clear timer handles when components are destroyed

### Code Organization

1. **Consistent Naming**:
   - Follow Unreal naming conventions (A for Actors, U for UObject-derived, etc.)
   - Use descriptive names for states and transitions
   - Maintain consistent file naming patterns

2. **Documentation**:
   - Document state transition rules
   - Comment complex transition logic
   - Maintain up-to-date state diagrams

3. **Blueprint Exposure**:
   - Mark appropriate functions as `BlueprintCallable` or `BlueprintPure`
   - Use proper categories for organization
   - Provide tooltips for designer-facing functions

### Debugging Support

1. **Logging Levels**:
   - Use `Error` for critical failures only
   - Use `Warning` for important issues
   - Avoid excessive logging in shipping builds

2. **Debug Visualization**:
   - Implement debug draws for movement directions
   - Create debug info displays for current states
   - Provide Blueprint-accessible debug functions

3. **State Validation**:
   - Add validation for impossible state transitions
   - Implement fallback behaviors for edge cases
   - Log transition attempts for debugging

This architecture provides a robust, extensible foundation for character movement systems while maintaining clean separation of concerns and supporting both C++ and Blueprint development workflows.