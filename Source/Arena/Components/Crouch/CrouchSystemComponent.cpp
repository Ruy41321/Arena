// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "CrouchSystemComponent.h"
#include "../../Player/PlayerCharacter.h"
#include "../../Utils/Utils.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "EnhancedInputComponent.h"

UCrouchSystemComponent::UCrouchSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	// Initialize default values similar to PlayerCharacter constructor
	CrouchTargetHeight = 65.0f;
	StandTargetHeight = 90.0f;
	HeightAdjustmentRate = FMath::Abs(StandTargetHeight - CrouchTargetHeight) * 10.0f;

	StandMeshHeightOffset = -90.0f;
	CrouchMeshHeightOffset = -60.0f;
	MeshOffsetAdjustmentRate = FMath::Abs(StandMeshHeightOffset - CrouchMeshHeightOffset) * 10.0f;

	UncrouchSafetyMargin = 5.0f;
	CrouchSpeed = 100.0f;
}

void UCrouchSystemComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerPlayerCharacter = Cast<APlayerCharacter>(GetOwner());
	
	if (!OwnerPlayerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("CrouchSystemComponent: Owner is not a PlayerCharacter! Owner class: %s"),
			GetOwner() ? *GetOwner()->GetClass()->GetName() : TEXT("NULL"));
	}
}

void UCrouchSystemComponent::SetupInput(UEnhancedInputComponent* EnhancedInputComponent)
{
	if (!EnhancedInputComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("CrouchSystemComponent: EnhancedInputComponent is null"));
		return;
	}

	if (!CrouchPressedAction)
	{
		UE_LOG(LogTemp, Warning, TEXT("CrouchSystemComponent: CrouchPressedAction is not set"));
		return;
	}

	// Bind crouch input
	EnhancedInputComponent->BindActionValueLambda(CrouchPressedAction, ETriggerEvent::Started, 
		[this](const FInputActionValue& Value) {
			CrouchPressed(Value);
		});


}

void UCrouchSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APlayerCharacter* PlayerCharacter = GetValidPlayerCharacter();
	if (!PlayerCharacter)
		return;

	// Handle crouch height adjustment
	if (IsCrouchingInProgress())
	{
		float TargetCapsuleHeight = IsCrouched() ? GetCrouchTargetHeight() : GetStandTargetHeight();
		float TargetMeshHeight = IsCrouched() ? GetCrouchMeshHeightOffset() : GetStandMeshHeightOffset();
		AdjustCapsuleHeight(DeltaTime, TargetCapsuleHeight, TargetMeshHeight);
	}
}

void UCrouchSystemComponent::CrouchPressed(const FInputActionValue& Value)
{
	APlayerCharacter* PlayerCharacter = GetValidPlayerCharacter();
	if (!PlayerCharacter)
		return;

	// Don't allow crouch changes during dodge (not using the state check because its called right after the dodge and the state could be still not updated)
	if (PlayerCharacter->DodgeSystem && PlayerCharacter->DodgeSystem->IsDodging())
		return;

	if (bIsCrouched)
	{
		if (!CanUncrouchSafely())
			return;
		UnCrouch();
	}
	else
		Crouch();
	
	bIsCrouchingInProgress = true;
}

void UCrouchSystemComponent::Crouch(bool bClientSimulation)
{
	APlayerCharacter* PlayerCharacter = GetValidPlayerCharacter();
	if (!PlayerCharacter)
		return;

	if (PlayerCharacter->MovementStateMachine)
	{
		EMovementState CurrentState = PlayerCharacter->MovementStateMachine->GetCurrentState();

		if (CurrentState == EMovementState::Falling || CurrentState == EMovementState::Jumping)
			return;
	}
	bIsCrouched = true;
}

void UCrouchSystemComponent::UnCrouch(bool bClientSimulation)
{
	APlayerCharacter* PlayerCharacter = GetValidPlayerCharacter();
	if (!PlayerCharacter)
		return;

	bIsCrouched = false;
}

bool UCrouchSystemComponent::CanUncrouchSafely() const
{
	APlayerCharacter* PlayerCharacter = GetValidPlayerCharacter();
	if (!PlayerCharacter)
		return false;

	UCapsuleComponent* CapsuleComponent = PlayerCharacter->GetCapsuleComponent();
	UWorld* World = PlayerCharacter->GetWorld();
	
	if (!CapsuleComponent || !World || !bIsCrouched)
	{
		return false;
	}

	// Get current capsule properties
	const float CurrentCapsuleRadius = CapsuleComponent->GetScaledCapsuleRadius();
	const float CurrentCapsuleHalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight();
	
	// Calculate the target standing height
	const float TargetCapsuleHalfHeight = StandTargetHeight;
	const float HeightDifference = TargetCapsuleHalfHeight - CurrentCapsuleHalfHeight;
	
	// If we're already at standing height or taller, no need to check
	if (HeightDifference <= 0.0f)
	{
		return true;
	}

	// Get current location
	const FVector CurrentLocation = PlayerCharacter->GetActorLocation();
	
	// Create a capsule shape for the standing position with safety margin
	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(
		CurrentCapsuleRadius, 
		TargetCapsuleHalfHeight + UncrouchSafetyMargin
	);
	
	// Calculate the position where the capsule would be when standing
	const FVector TargetLocation = CurrentLocation + FVector(0.0f, 0.0f, HeightDifference + UncrouchSafetyMargin);
	
	// Set up collision query parameters
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(PlayerCharacter);
	QueryParams.bTraceComplex = false;
	
	// Perform overlap test at the target standing position
	const bool bHasOverlap = World->OverlapBlockingTestByChannel(
		TargetLocation,
		FQuat::Identity,
		ECollisionChannel::ECC_Pawn,
		CapsuleShape,
		QueryParams
	);
	
	return !bHasOverlap;
}

void UCrouchSystemComponent::AdjustCapsuleHeight(float DeltaTime, float TargetCapsuleHeight, float TargetMeshHeight)
{
	APlayerCharacter* PlayerCharacter = GetValidPlayerCharacter();
	if (!PlayerCharacter)
		return;

	bool IsFinished = true;
	const float Tolerance = 0.01f;

	// Adjusting the capsule height
	UCapsuleComponent* CapsuleComponent = PlayerCharacter->GetCapsuleComponent();
	if (CapsuleComponent)
	{
		float CurrentHeight = CapsuleComponent->GetScaledCapsuleHalfHeight();

		if (!FUtils::HandleGenericInterpolation(CurrentHeight, TargetCapsuleHeight, HeightAdjustmentRate, DeltaTime, Tolerance))
			IsFinished = false;
		CapsuleComponent->SetCapsuleHalfHeight(CurrentHeight);
	}

	// Adjusting the mesh position
	USkeletalMeshComponent* Mesh = PlayerCharacter->GetMesh();
		if (Mesh)
	{
		FVector CurrentLocation = Mesh->GetRelativeLocation();
		float CurrentMeshHeight = CurrentLocation.Z;

		if (!FUtils::HandleGenericInterpolation(CurrentMeshHeight, TargetMeshHeight, MeshOffsetAdjustmentRate, DeltaTime, Tolerance))
			IsFinished = false;
		CurrentLocation.Z = CurrentMeshHeight;
		Mesh->SetRelativeLocation(CurrentLocation);
	}

	bIsCrouchingInProgress = !IsFinished;
}