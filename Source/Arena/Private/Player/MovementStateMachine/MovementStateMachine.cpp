// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/MovementStateMachine/MovementStateMachine.h"
#include "Player/PlayerCharacter.h"
#include "Player/MovementStateMachine/MovementState.h"
#include "Player/MovementStateMachine/States/IdleMovementState.h"
#include "Player/MovementStateMachine/States/WalkingMovementState.h"
#include "Player/MovementStateMachine/States/SprintingMovementState.h"
#include "Player/MovementStateMachine/States/CrouchingIdleMovementState.h"
#include "Player/MovementStateMachine/States/CrouchingMovingMovementState.h"
#include "Player/MovementStateMachine/States/JumpingMovementState.h"
#include "Player/MovementStateMachine/States/FallingMovementState.h"
#include "Player/MovementStateMachine/States/LandingInPlaceMovementState.h"
#include "Player/MovementStateMachine/States/LandingMovingMovementState.h"
#include "Player/MovementStateMachine/States/DodgingMovementState.h"

UMovementStateMachine::UMovementStateMachine()
{
	PrimaryComponentTick.bCanEverTick = true;
	CurrentState = EMovementState::None;
	PreviousState = EMovementState::None;
}

void UMovementStateMachine::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerPlayerCharacter = Cast<APlayerCharacter>(GetOwner());
	
	if (!OwnerPlayerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("MovementStateMachine: Owner is not a PlayerCharacter!"));
		return;
	}

	InitializeDefaultStates();
	
	// Start with Idle state
	TransitionToState(EMovementState::Idle, true);
}

void UMovementStateMachine::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update current state
	if (UMovementState* CurrentStateObject = GetCurrentStateObject())
	{
		CurrentStateObject->UpdateState(DeltaTime);
	}

	// Evaluate automatic transitions
	EvaluateStateTransitions();
}

bool UMovementStateMachine::TransitionToState(EMovementState NewState, bool bForceTransition)
{
	if (NewState == CurrentState)
	{
		return false; // Already in the desired state
	}

	if (!bForceTransition && !CanTransitionToState(NewState))
	{
		return false; // Transition not allowed
	}

	PerformStateTransition(NewState);
	return true;
}

bool UMovementStateMachine::CanTransitionToState(EMovementState NewState) const
{
	if (UMovementState* CurrentStateObject = GetCurrentStateObject())
	{
		return CurrentStateObject->CanTransitionTo(NewState);
	}
	
	return true; // If no current state, allow transition
}

void UMovementStateMachine::RegisterState(EMovementState StateType, TSubclassOf<UMovementState> StateClass)
{
	if (StateClass)
	{
		DefaultStateClasses.Add(StateType, StateClass);
		
		// If we're already initialized, create the state object immediately
		if (OwnerPlayerCharacter)
		{
			if (UMovementState* NewStateObject = CreateStateObject(StateType, StateClass))
			{
				StateObjects.Add(StateType, NewStateObject);
			}
		}
	}
}

UMovementState* UMovementStateMachine::GetCurrentStateObject() const
{
	if (StateObjects.Contains(CurrentState))
	{
		return StateObjects[CurrentState].Get();
	}
	return nullptr;
}

void UMovementStateMachine::InitializeDefaultStates()
{
	// Set default state classes if not overridden
	if (!DefaultStateClasses.Contains(EMovementState::Idle))
		DefaultStateClasses.Add(EMovementState::Idle, UIdleMovementState::StaticClass());
	if (!DefaultStateClasses.Contains(EMovementState::Walking))
		DefaultStateClasses.Add(EMovementState::Walking, UWalkingMovementState::StaticClass());
	if (!DefaultStateClasses.Contains(EMovementState::Sprinting))
		DefaultStateClasses.Add(EMovementState::Sprinting, USprintingMovementState::StaticClass());
	if (!DefaultStateClasses.Contains(EMovementState::CrouchingIdle))
		DefaultStateClasses.Add(EMovementState::CrouchingIdle, UCrouchingIdleMovementState::StaticClass());
	if (!DefaultStateClasses.Contains(EMovementState::CrouchingMoving))
		DefaultStateClasses.Add(EMovementState::CrouchingMoving, UCrouchingMovingMovementState::StaticClass());
	if (!DefaultStateClasses.Contains(EMovementState::Jumping))
		DefaultStateClasses.Add(EMovementState::Jumping, UJumpingMovementState::StaticClass());
	if (!DefaultStateClasses.Contains(EMovementState::Falling))
		DefaultStateClasses.Add(EMovementState::Falling, UFallingMovementState::StaticClass());
	if (!DefaultStateClasses.Contains(EMovementState::LandingInPlace))
		DefaultStateClasses.Add(EMovementState::LandingInPlace, ULandingInPlaceMovementState::StaticClass());
	if (!DefaultStateClasses.Contains(EMovementState::LandingMoving))
		DefaultStateClasses.Add(EMovementState::LandingMoving, ULandingMovingMovementState::StaticClass());
	if (!DefaultStateClasses.Contains(EMovementState::Dodging))
		DefaultStateClasses.Add(EMovementState::Dodging, UDodgingMovementState::StaticClass());

	// Create state objects
	for (const auto& StateClassPair : DefaultStateClasses)
	{
		if (UMovementState* NewStateObject = CreateStateObject(StateClassPair.Key, StateClassPair.Value))
		{
			StateObjects.Add(StateClassPair.Key, NewStateObject);
		}
	}
}

UMovementState* UMovementStateMachine::CreateStateObject(EMovementState StateType, TSubclassOf<UMovementState> StateClass)
{
	if (!StateClass || !OwnerPlayerCharacter)
	{
		return nullptr;
	}

	UMovementState* NewStateObject = NewObject<UMovementState>(this, StateClass);
	if (NewStateObject)
	{
		NewStateObject->Initialize(this, OwnerPlayerCharacter);
	}

	return NewStateObject;
}

APlayerCharacter* UMovementStateMachine::GetValidPlayerCharacter() const
{
	return OwnerPlayerCharacter.Get();
}

void UMovementStateMachine::EvaluateStateTransitions()
{
	if (UMovementState* CurrentStateObject = GetCurrentStateObject())
	{
		EMovementState DesiredState = CurrentStateObject->GetDesiredTransition();
		if (DesiredState != EMovementState::None && DesiredState != CurrentState)
		{
			TransitionToState(DesiredState);
		}
	}
}

void UMovementStateMachine::PerformStateTransition(EMovementState NewState)
{
	EMovementState OldState = CurrentState;

	// Exit current state
	if (UMovementState* CurrentStateObject = GetCurrentStateObject())
	{
		CurrentStateObject->ExitState(NewState);
	}

	// Update states
	PreviousState = CurrentState;
	CurrentState = NewState;

	// Enter new state
	if (UMovementState* NewStateObject = GetCurrentStateObject())
	{
		NewStateObject->EnterState(PreviousState);
	}

	// Broadcast state change
	OnStateChanged.Broadcast(OldState, NewState);
	// stampa server: jumpcomponent->bIsLanding o client: playercharacter->bIsLanding
	APlayerCharacter* PlayerCharacter = GetValidPlayerCharacter();
	if (!PlayerCharacter)
		return;

	UE_LOG(LogTemp, Log, TEXT("MovementStateMachine: Transitioned from %s to %s"), 
		*UMovementStateTypes::MovementStateToString(OldState),
		*UMovementStateTypes::MovementStateToString(NewState));
}

void UMovementStateMachine::UnsubscribeFromStateChanges(UObject* Subscriber)
{
	if (!Subscriber)
	{
		UE_LOG(LogTemp, Warning, TEXT("MovementStateMachine: Cannot unsubscribe null object from state changes"));
		return;
	}

	// Remove all dynamic delegates for this subscriber
	OnStateChanged.RemoveAll(Subscriber);

	UE_LOG(LogTemp, Log, TEXT("MovementStateMachine: Unsubscribed '%s' from state changes"), 
		*Subscriber->GetClass()->GetName());
}
