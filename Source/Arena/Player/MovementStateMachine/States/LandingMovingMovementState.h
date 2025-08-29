// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "../MovementState.h"
#include "LandingMovingMovementState.generated.h"

UCLASS(BlueprintType, Blueprintable)
class ARENA_API ULandingMovingMovementState : public UMovementState
{
	GENERATED_BODY()

public:
	ULandingMovingMovementState();
	virtual EMovementState GetStateType() const override { return EMovementState::LandingMoving; }
	virtual EMovementState GetDesiredTransition_Implementation() const override;
	virtual bool CanTransitionTo_Implementation(EMovementState NewState) const override;
};