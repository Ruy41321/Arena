// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Player/MovementStateMachine/MovementState.h"
#include "JumpingMovementState.generated.h"

UCLASS(BlueprintType, Blueprintable)
class ARENA_API UJumpingMovementState : public UMovementState
{
	GENERATED_BODY()

public:
	UJumpingMovementState();
	virtual EMovementState GetStateType() const override { return EMovementState::Jumping; }
	virtual EMovementState GetDesiredTransition_Implementation() const override;
};
