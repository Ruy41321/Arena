// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Player/MovementStateMachine/MovementState.h"
#include "DodgingMovementState.generated.h"

UCLASS(BlueprintType, Blueprintable)
class ARENA_API UDodgingMovementState : public UMovementState
{
	GENERATED_BODY()

public:
	UDodgingMovementState();
	virtual EMovementState GetStateType() const override { return EMovementState::Dodging; }
	virtual EMovementState GetDesiredTransition_Implementation() const override;
	virtual bool CanTransitionTo_Implementation(EMovementState NewState) const override;
};
