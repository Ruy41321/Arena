// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Player/MovementStateMachine/MovementState.h"
#include "SprintingMovementState.generated.h"

/**
 * Movement state for when the player is sprinting
 */
UCLASS(BlueprintType, Blueprintable)
class ARENA_API USprintingMovementState : public UMovementState
{
	GENERATED_BODY()

public:
	USprintingMovementState();

	virtual EMovementState GetStateType() const override { return EMovementState::Sprinting; }
	virtual EMovementState GetDesiredTransition_Implementation() const override;
};
