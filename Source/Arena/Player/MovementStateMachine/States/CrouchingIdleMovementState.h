// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "../MovementState.h"
#include "CrouchingIdleMovementState.generated.h"

UCLASS(BlueprintType, Blueprintable)
class ARENA_API UCrouchingIdleMovementState : public UMovementState
{
	GENERATED_BODY()

public:
	UCrouchingIdleMovementState();
	virtual EMovementState GetStateType() const override { return EMovementState::CrouchingIdle; }
	virtual EMovementState GetDesiredTransition_Implementation() const override;
	virtual bool CanTransitionTo_Implementation(EMovementState NewState) const override;
};