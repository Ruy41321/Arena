// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Player/MovementStateMachine/MovementStateTypes.h"
#include "MovementState.generated.h"

// Forward declarations
class APlayerCharacter;
class UMovementStateMachine;

/**
 * Base class for all movement states
 */
UCLASS(BlueprintType, Blueprintable, Abstract)
class ARENA_API UMovementState : public UObject
{
	GENERATED_BODY()

public:
	UMovementState();

	/** Initialize the state with the state machine and player character */
	virtual void Initialize(UMovementStateMachine* InStateMachine, APlayerCharacter* InPlayerCharacter);

	/** Called when entering this state */
	UFUNCTION(BlueprintImplementableEvent, Category = "Movement State")
	void OnEnterState(EMovementState PreviousState);

	/** Called every frame while in this state */
	UFUNCTION(BlueprintImplementableEvent, Category = "Movement State")
	void OnUpdateState(float DeltaTime);

	/** Called when exiting this state */
	UFUNCTION(BlueprintImplementableEvent, Category = "Movement State")
	void OnExitState(EMovementState NextState);

	/** Check if this state can transition to another state */
	UFUNCTION(BlueprintNativeEvent, Category = "Movement State")
	bool CanTransitionTo(EMovementState NewState) const;
	virtual bool CanTransitionTo_Implementation(EMovementState NewState) const;

	/** Check if this state should transition to another state automatically */
	UFUNCTION(BlueprintNativeEvent, Category = "Movement State")
	EMovementState GetDesiredTransition() const;
	virtual EMovementState GetDesiredTransition_Implementation() const;

	/** Get the state type this class represents */
	UFUNCTION(BlueprintPure, Category = "Movement State")
	virtual EMovementState GetStateType() const PURE_VIRTUAL(UMovementState::GetStateType, return EMovementState::None;);

	/** C++ versions of the Blueprint events for derived classes */
	virtual void EnterState(EMovementState PreviousState);
	virtual void UpdateState(float DeltaTime);
	virtual void ExitState(EMovementState NextState);

protected:
	/** Reference to the state machine that owns this state */
	UPROPERTY(BlueprintReadOnly, Category = "Movement State")
	TObjectPtr<UMovementStateMachine> StateMachine;

	/** Reference to the player character */
	UPROPERTY(BlueprintReadOnly, Category = "Movement State")
	TObjectPtr<APlayerCharacter> PlayerCharacter;

	/** Helper function to get valid player character */
	UFUNCTION(BlueprintPure, Category = "Movement State")
	APlayerCharacter* GetPlayerCharacter() const { return PlayerCharacter.Get(); }

	/** Helper function to get valid state machine */
	UFUNCTION(BlueprintPure, Category = "Movement State")
	UMovementStateMachine* GetStateMachine() const { return StateMachine.Get(); }

	/** Sets the appropriate movement speed for this state */
	virtual void SetStateSpeed();

	/** Gets the speed value for a specific state type */
	virtual float GetSpeedForState(EMovementState StateType) const;
};
