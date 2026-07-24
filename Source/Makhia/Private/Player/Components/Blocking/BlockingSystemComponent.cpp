// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/Components/Blocking/BlockingSystemComponent.h"
#include "Player/MKHPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/MKHGameplayTags.h"

// ============================================================
// Lifecycle (Constructor, BeginPlay, Tick, EndPlay, etc.)
// ============================================================

UBlockingSystemComponent::UBlockingSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UBlockingSystemComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerPlayerCharacter = Cast<AMKHPlayerCharacter>(GetOwner());
}

void UBlockingSystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UBlockingSystemComponent, bWantsToBlock);
}

// ============================================================
// Public Interface
// ============================================================

bool UBlockingSystemComponent::StartBlocking()
{
	AMKHPlayerCharacter* Character = GetValidPlayerCharacter();
	if (!IsValid(Character))
	{
		return false;
	}

	bWantsToBlock = true;
	
	EnterBlockingState();
	return true;
}

void UBlockingSystemComponent::StopBlocking()
{
	bWantsToBlock = false;

	AMKHPlayerCharacter* Character = GetValidPlayerCharacter();
	if (IsValid(Character) && Character->GetCurrentMovementState() == EMovementStateValue::Blocking)
	{
		// Exit blocking state, let state machine decide the next state (usually Idle/Walking)
		Character->TransitionToMovementState(EMovementStateValue::Idle);
	}
}

void UBlockingSystemComponent::ApplyBlockingStateModifiers_Implementation()
{
	AMKHPlayerCharacter* Character = GetValidPlayerCharacter();
	if (!IsValid(Character)) return;
	
	UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
	if (IsValid(MovementComp))
	{
		MovementComp->bOrientRotationToMovement = false;
		MovementComp->bUseControllerDesiredRotation = true;
	}
	
	UCrouchSystemComponent* CrouchSystem = Character->FindComponentByClass<UCrouchSystemComponent>();
	if (IsValid(CrouchSystem) && CrouchSystem->IsCrouched())
	{
		CrouchSystem->UnCrouch();
	}

	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Character))
	{
		UGameplayAbility* InstanceToIgnore = nullptr;
		if (IsValid(BlockAbilityClass))
		{
			if (FGameplayAbilitySpec* BlockSpec = ASC->FindAbilitySpecFromClass(BlockAbilityClass))
			{
				InstanceToIgnore = BlockSpec->GetPrimaryInstance();
			}
		}

		ASC->CancelAllAbilities(InstanceToIgnore);
	}
}

void UBlockingSystemComponent::RemoveBlockingStateModifiers_Implementation()
{
	AMKHPlayerCharacter* Character = GetValidPlayerCharacter();
	if (!IsValid(Character)) return;
	
	UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
	if (IsValid(MovementComp))
	{
		MovementComp->bOrientRotationToMovement = true;
		MovementComp->bUseControllerDesiredRotation = false;
	}
}

// ============================================================
// Protected / Internal Logic
// ============================================================

bool UBlockingSystemComponent::EnterBlockingState()
{
	AMKHPlayerCharacter* Character = GetValidPlayerCharacter();
	if (IsValid(Character) && IsInBlockableState(Character->GetCurrentMovementState()))
	{
		return Character->TransitionToMovementState(EMovementStateValue::Blocking);
	}
	return false;
}

void UBlockingSystemComponent::OnRep_WantsToBlock(bool bOldValue)
{
	if (bWantsToBlock)
		EnterBlockingState();
}

bool UBlockingSystemComponent::IsInBlockableState(EMovementStateValue CurrentState) const
{
	// Define states where blocking is not allowed
	switch (CurrentState)
	{
	case EMovementStateValue::CrouchingIdle:
	case EMovementStateValue::CrouchingMoving:
		return GetValidPlayerCharacter()->CrouchSystem->CanUncrouchSafely();
	case EMovementStateValue::Blocking:
	case EMovementStateValue::Dodging:
	case EMovementStateValue::LandingInPlace:
	case EMovementStateValue::LandingMoving:
	case EMovementStateValue::Jumping:
	case EMovementStateValue::Falling:
		return false; // Non Transitionable states
	default:
		return true;
	}
}

AMKHPlayerCharacter* UBlockingSystemComponent::GetValidPlayerCharacter() const
{
	if (IsValid(OwnerPlayerCharacter))
	{
		return OwnerPlayerCharacter;
	}
	
	return Cast<AMKHPlayerCharacter>(GetOwner());
}
