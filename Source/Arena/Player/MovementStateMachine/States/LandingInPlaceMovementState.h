// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "../MovementState.h"
#include "LandingInPlaceMovementState.generated.h"

UCLASS(BlueprintType, Blueprintable)
class ARENA_API ULandingInPlaceMovementState : public UMovementState
{
	GENERATED_BODY()

public:
	ULandingInPlaceMovementState();
	virtual EMovementState GetStateType() const override { return EMovementState::LandingInPlace; }
	virtual EMovementState GetDesiredTransition_Implementation() const override;
	virtual bool CanTransitionTo_Implementation(EMovementState NewState) const override;
};