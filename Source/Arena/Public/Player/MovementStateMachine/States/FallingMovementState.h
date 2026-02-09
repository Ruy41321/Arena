// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Player/MovementStateMachine/MovementState.h"
#include "FallingMovementState.generated.h"

UCLASS(BlueprintType, Blueprintable)
class ARENA_API UFallingMovementState : public UMovementState
{
	GENERATED_BODY()

public:
	UFallingMovementState();
	virtual EMovementStateValue GetStateType() const override { return EMovementStateValue::Falling; }
	virtual EMovementStateValue GetDesiredTransition_Implementation() const override;
};
