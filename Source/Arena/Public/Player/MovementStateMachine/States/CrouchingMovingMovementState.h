// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Player/MovementStateMachine/MovementState.h"
#include "CrouchingMovingMovementState.generated.h"

UCLASS(BlueprintType, Blueprintable)
class ARENA_API UCrouchingMovingMovementState : public UMovementState
{
	GENERATED_BODY()

public:
	UCrouchingMovingMovementState();
	virtual EMovementState GetStateType() const override { return EMovementState::CrouchingMoving; }
	virtual EMovementState GetDesiredTransition_Implementation() const override;
	virtual bool CanTransitionTo_Implementation(EMovementState NewState) const override;
};