// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Player/MovementStateMachine/MovementState.h"
#include "IdleMovementState.generated.h"

/**
 * Movement state for when the player is idle (not moving)
 */
UCLASS(BlueprintType, Blueprintable)
class ARENA_API UIdleMovementState : public UMovementState
{
	GENERATED_BODY()

public:
	UIdleMovementState();

	virtual EMovementState GetStateType() const override { return EMovementState::Idle; }
	virtual EMovementState GetDesiredTransition_Implementation() const override;
	virtual void EnterState(EMovementState PreviousState) override;
};
