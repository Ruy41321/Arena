// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Player/MovementStateMachine/MovementState.h"
#include "WalkingMovementState.generated.h"

/**
 * Movement state for when the player is walking
 */
UCLASS(BlueprintType, Blueprintable)
class ARENA_API UWalkingMovementState : public UMovementState
{
	GENERATED_BODY()

public:
	UWalkingMovementState();

	virtual EMovementState GetStateType() const override { return EMovementState::Walking; }
	virtual EMovementState GetDesiredTransition_Implementation() const override;
};
