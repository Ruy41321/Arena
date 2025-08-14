// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/InputComponent.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "../PlayerAnimation/PlayerAnimInstance.h"

// Sets default values
APlayerCharacter::APlayerCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	UCharacterMovementComponent* const mov = GetCharacterMovement();
	if (mov)
		mov->bOrientRotationToMovement = true; // Character will rotate to movement direction
	bUseControllerRotationYaw = false; // Disable controller rotation yaw to allow character movement direction to control rotation

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Boom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Player Camera"));
	Camera->SetupAttachment(CameraBoom);
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveForwardAction, ETriggerEvent::Triggered, this, &APlayerCharacter::MoveForward);
		EnhancedInputComponent->BindAction(MoveRightAction, ETriggerEvent::Triggered, this, &APlayerCharacter::MoveRight);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &APlayerCharacter::Sprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &APlayerCharacter::Sprint);
		EnhancedInputComponent->BindAction(JumpPressedAction, ETriggerEvent::Started, this, &APlayerCharacter::JumpPressed);
		EnhancedInputComponent->BindAction(JumpPressedAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerInputComponent is not of type UEnhancedInputComponent"));
	}
}

void APlayerCharacter::MoveForward(const FInputActionValue& Value)
{
	const float Direction = Value.Get<float>();
	const FRotator Rotation = Controller ? Controller->GetControlRotation() : FRotator::ZeroRotator;
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
	const FVector Forward = FRotationMatrix(Rotation).GetUnitAxis(EAxis::X);

	if (Direction != 0.0f)
	{
		AddMovementInput(Forward, Direction);
	}
}

void APlayerCharacter::MoveRight(const FInputActionValue& Value)
{
	const float Direction = Value.Get<float>();
	const FRotator Rotation = Controller ? Controller->GetControlRotation() : FRotator::ZeroRotator;
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	if (Direction != 0.0f)
	{
		AddMovementInput(Right, Direction);
	}
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxis = Value.Get<FVector2D>();
	if (Controller)
	{
		AddControllerYawInput(LookAxis.X);
		AddControllerPitchInput(LookAxis.Y);
	}
}

void APlayerCharacter::Sprint(const FInputActionValue& Value)
{
	const bool SprintValue = Value.Get<bool>();
	if (SprintValue)
	{
		IsSprinting = true;
		GetCharacterMovement()->MaxWalkSpeed *= 2.0f; // Increase speed when sprinting
	}
	else
	{
		IsSprinting = false;
		GetCharacterMovement()->MaxWalkSpeed /= 2.0f; // Reset speed when not sprinting
	}
}

void APlayerCharacter::JumpPressed(const FInputActionValue& Value)
{
	if (!IsLanding)
	{
		Jump();
	}
}

void APlayerCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	IsLanding = true;
	// set a timer of x seconds to reset IsLanding
	GetWorld()->GetTimerManager().SetTimer(LandingTimerHandle, this, &APlayerCharacter::ResetLanding, 0.1f, false);
}

void APlayerCharacter::ResetLanding()
{
	IsLanding = false;
}