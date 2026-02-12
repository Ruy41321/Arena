// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerController/RPGPlayerController.h"
#include "Input/RPGSystemInputComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Inventory/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "UI/WidgetControllers/InventoryWidgetController.h"
#include "UI/RPGSystemWidget.h"
#include "AbilitySystem/RPGAbilitySystemComponent.h"
#include "Player/PlayerState/RPGPlayerState.h"

ARPGPlayerController::ARPGPlayerController()
{
	bReplicates = true;

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	InventoryComponent->SetIsReplicated(true);
}

void ARPGPlayerController::BeginPlay()
{
	Super::BeginPlay();

}

URPGAbilitySystemComponent* ARPGPlayerController::GetRPGAbilitySystemComponent()
{
	if (!IsValid(RPGAbilitySystemComponent))
	{
		if (ARPGPlayerState* RPGPlayerState = GetPlayerState<ARPGPlayerState>())
		{
			if (URPGAbilitySystemComponent* ASC = RPGPlayerState->GetRPGAbilitySystemComponent())
			{
				RPGAbilitySystemComponent = ASC;
			}
		}
	}

	return RPGAbilitySystemComponent;
}

void ARPGPlayerController::SetupInputComponent()
{
	APlayerController::SetupInputComponent();

	if (URPGSystemInputComponent* RPGInputComponent = Cast<URPGSystemInputComponent>(InputComponent))
	{
		RPGInputComponent->BindAbilityActions(RPGInputConfig, this, &ARPGPlayerController::AbilityInputPressed, &ARPGPlayerController::AbilityInputReleased);
	}
}

void ARPGPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARPGPlayerController, InventoryComponent);
}

UAbilitySystemComponent* ARPGPlayerController::GetAbilitySystemComponent() const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());
}

UInventoryComponent* ARPGPlayerController::GetInventoryComponent_Implementation() const
{
	return InventoryComponent;
}

UInventoryWidgetController* ARPGPlayerController::GetInventoryWidgetController()
{
	if (!IsValid(InventoryWidgetController))
	{
		InventoryWidgetController = NewObject<UInventoryWidgetController>(this, InventoryWidgetControllerClass);
		InventoryWidgetController->SetOwningActor(this);

		InventoryWidgetController->BindCallbacksToDependencies();
	}

	return InventoryWidgetController;
}

void ARPGPlayerController::CreateInventoryWidget()
{
	if (UUserWidget* Widget = CreateWidget<URPGSystemWidget>(this, InventoryWidgetClass))
	{
		InventoryWidget = Cast<URPGSystemWidget>(Widget);
		InventoryWidget->SetWidgetController(GetInventoryWidgetController());
		InventoryWidgetController->BroadcastInitialValues();
		InventoryWidget->AddToViewport();
	}
}

void ARPGPlayerController::AbilityInputPressed(const FGameplayTag& InputTag)
{
	if (IsValid(GetRPGAbilitySystemComponent()))
	{
		RPGAbilitySystemComponent->AbilityInputPressed(InputTag);
	}
}

void ARPGPlayerController::AbilityInputReleased(const FGameplayTag& InputTag)
{
	if (IsValid(GetRPGAbilitySystemComponent()))
	{
		RPGAbilitySystemComponent->AbilityInputReleased(InputTag);
	}
}
