// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/MovementStateMachine/MovementStateTypes.h"
#include "MovementStateMachine.generated.h"

// Forward declarations
class APlayerCharacter;
class UMovementState;

/**
 * Component that manages movement states for the player character
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ARENA_API UMovementStateMachine : public UActorComponent
{
	GENERATED_BODY()

public:
	UMovementStateMachine();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Transition to a new movement state */
	UFUNCTION(BlueprintCallable, Category = "Movement State Machine")
	bool TransitionToState(EMovementStateValue NewState, bool bForceTransition = false);

	/** Get the current movement state */
	UFUNCTION(BlueprintPure, Category = "Movement State Machine")
	EMovementStateValue GetCurrentState() const { return CurrentState; }

	/** Get the previous movement state */
	UFUNCTION(BlueprintPure, Category = "Movement State Machine")
	EMovementStateValue GetPreviousState() const { return PreviousState; }

	/** Check if we can transition to a specific state */
	UFUNCTION(BlueprintPure, Category = "Movement State Machine")
	bool CanTransitionToState(EMovementStateValue NewState) const;

	/** Register a state class for a specific state type */
	UFUNCTION(BlueprintCallable, Category = "Movement State Machine")
	void RegisterState(EMovementStateValue StateType, TSubclassOf<UMovementState> StateClass);

	/** Get the current state object */
	UFUNCTION(BlueprintPure, Category = "Movement State Machine")
	UMovementState* GetCurrentStateObject() const;

	/** Event called when state changes */
	UPROPERTY(BlueprintAssignable, Category = "Movement State Machine")
	FOnMovementStateChanged OnStateChanged;

	/** Unsubscribe from state change events using the subscriber object */
	UFUNCTION(BlueprintCallable, Category = "Movement State Machine")
	void UnsubscribeFromStateChanges(UObject* Subscriber);

protected:
	/** Current movement state */
	UPROPERTY(BlueprintReadOnly, Category = "Movement State Machine")
	EMovementStateValue CurrentState;

	/** Previous movement state */
	UPROPERTY(BlueprintReadOnly, Category = "Movement State Machine")
	EMovementStateValue PreviousState;

	/** Map of state types to state objects */
	UPROPERTY()
	TMap<EMovementStateValue, TObjectPtr<UMovementState>> StateObjects;

	/** Default state classes that can be overridden in Blueprint */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement State Machine")
	TMap<EMovementStateValue, TSubclassOf<UMovementState>> DefaultStateClasses;

	/** Reference to the player character */
	UPROPERTY()
	TObjectPtr<APlayerCharacter> OwnerPlayerCharacter;

private:
	/** Initialize default states */
	void InitializeDefaultStates();

	/** Create state object for a specific state type */
	UMovementState* CreateStateObject(EMovementStateValue StateType, TSubclassOf<UMovementState> StateClass);

	/** Helper function to get valid player character */
	APlayerCharacter* GetValidPlayerCharacter() const;

	/** Evaluate automatic state transitions */
	void EvaluateStateTransitions();

	/** Perform the actual state transition */
	void PerformStateTransition(EMovementStateValue NewState);
};
