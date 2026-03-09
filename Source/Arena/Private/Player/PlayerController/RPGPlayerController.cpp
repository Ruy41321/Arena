// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerController/RPGPlayerController.h"
#include "Input/RPGSystemInputComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Inventory/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "UI/WidgetControllers/InventoryDashboardController.h"
#include "UI/Inventory/InventoryDashboardWidget.h"
#include "AbilitySystem/RPGAbilitySystemComponent.h"
#include "AbilitySystem/RPGGameplayTags.h"
#include "Player/PlayerState/RPGPlayerState.h"
#include "Equipment/EquipmentManagerComponent.h"
#include "Inventory/InventoryItem/InventoryItem.h"
#include "QuickSlot/QuickSlotManagerComponent.h"

ARPGPlayerController::ARPGPlayerController()
{
	bReplicates = true;

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	InventoryComponent->SetIsReplicated(true);

	EquipmentComponent = CreateDefaultSubobject<UEquipmentManagerComponent>(TEXT("EquipmentComponent"));
	EquipmentComponent->SetIsReplicated(true);
	
	QuickSlotManagerComponent = CreateDefaultSubobject<UQuickSlotManagerComponent>(TEXT("QuickSlotManagerComponent"));
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
		RPGInputComponent->BindAbilityActions(RPGInputConfig, RPGGameplayTags::Input::Ability,this, &ARPGPlayerController::AbilityInputPressed, &ARPGPlayerController::AbilityInputReleased);
		QuickSlotManagerComponent->SetupInputActions(RPGInputConfig, RPGInputComponent);
		
		if (IsValid(ToggleInventoryAction))
		{
			RPGInputComponent->BindAction(ToggleInventoryAction, ETriggerEvent::Started, this, &ARPGPlayerController::ToggleInventory);
		}
	}
}

void ARPGPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARPGPlayerController, InventoryComponent);
}

void ARPGPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	ChangeMappingContext(DefaultMappingContext);
	
	BindCallbacksToDependencies();
}

void ARPGPlayerController::ChangeMappingContext(const UInputMappingContext* NewMappingContext) const
{
	if (UEnhancedInputLocalPlayerSubsystem *Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->ClearAllMappings();
		Subsystem->AddMappingContext(NewMappingContext, 0);
	}
}

UAbilitySystemComponent* ARPGPlayerController::GetAbilitySystemComponent() const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());
}

UEquipmentManagerComponent* ARPGPlayerController::GetEquipmentManagerComponent_Implementation() const
{
	return EquipmentComponent;
}

UInventoryComponent* ARPGPlayerController::GetInventoryComponent_Implementation() const
{
	return InventoryComponent;
}

UQuickSlotManagerComponent* ARPGPlayerController::GetQuickSlotManagerComponent_Implementation() const
{
	return QuickSlotManagerComponent;
}

void ARPGPlayerController::SetDynamicProjectile_Implementation(const FGameplayTag& ProjectileTag, int32 AbilityLevel)
{
	if (IsValid(GetRPGAbilitySystemComponent()))
	{
		RPGAbilitySystemComponent->SetDynamicProjectile(ProjectileTag, AbilityLevel);
	}
}

UInventoryDashboardController* ARPGPlayerController::GetInventoryWidgetController()
{
	if (!IsValid(InventoryDashboardController))
	{
		InventoryDashboardController = NewObject<UInventoryDashboardController>(this, InventoryDashboardControllerClass);
		InventoryDashboardController->SetOwningActor(this);

		InventoryDashboardController->BindCallbacksToDependencies();
	}

	return InventoryDashboardController;
}

void ARPGPlayerController::CreateInventoryWidget()
{
	if (UUserWidget* Widget = CreateWidget<UInventoryDashboardWidget>(this, InventoryDashboardWidgetClass))
	{
		InventoryDashboardWidget = Cast<UInventoryDashboardWidget>(Widget);
		UInventoryDashboardController* Controller = GetInventoryWidgetController();
		Controller->SetInventoryWidget(InventoryDashboardWidget);
		BindInventoryWidgetDelegates();
		Controller->BroadcastInitialValues();
		InventoryDashboardWidget->AddToViewport();
	}
}

void ARPGPlayerController::BindInventoryWidgetDelegates()
{
	InventoryDashboardWidget->OnOpenInventoryDashboardDelegate.AddLambda(
		[this]()
		{
			ChangeFocus("Inventory");
		});
	InventoryDashboardWidget->OnCloseInventoryDashboardDelegate.AddLambda(
		[this]()
		{
			ChangeFocus("Game");
		});
	InventoryDashboardWidget->OnDestructionInventoryDashboardDelegate.AddLambda(
		[this]()
		{
			InventoryDashboardWidget->UnbindAllEventsFromDelegates();
			InventoryDashboardWidget = nullptr;
			GetInventoryWidgetController()->UnbindAllEventsFromDelegates();
			GetInventoryWidgetController()->SetInventoryWidget(nullptr);
			ChangeFocus("Game");
		});
}

void ARPGPlayerController::DestroyInventoryWidget()
{
	if (IsValid(InventoryDashboardWidget))
	{
		InventoryDashboardWidget->DestroyInventoryDashboard();
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

void ARPGPlayerController::BindCallbacksToDependencies() const
{
	if (IsValid(InventoryComponent) && IsValid(EquipmentComponent))
	{
		InventoryComponent->EquipmentItemUnequippedDelegate.AddLambda(
			[this](int64 ItemID)
			{
				EquipmentComponent->UnEquipItemByItemID(ItemID);
			});
		InventoryComponent->EquipmentItemUsedDelegate.AddLambda(
			[this](UInventoryItem* InventoryItem)
			{
				// Equipping Item from Inventory on Use
				EquipmentComponent->EquipItem(UEquipmentManagerComponent::BuildEquipmentEntry(InventoryItem));
			});
		EquipmentComponent->EquipmentList.UnEquippedEntryDelegate.AddLambda(
			[this](const FRPGEquipmentEntry& UnEquippedEntry)
			{
				// Returning Item to Inventory on UnEquip
				InventoryComponent->AddUnEquippedItemEntry(UInventoryItem::CreateFromEquipmentEntry(GetWorld(), UnEquippedEntry));
			});
		InventoryComponent->InventoryList.InventoryItemQuickSlottedDelegate.AddLambda(
			[this](const FRPGInventoryEntry& QuickSlottedEntry)
			{
				if (QuickSlottedEntry.bIsQuickSlotted)
				{
					QuickSlotManagerComponent->AddQuickSlot(QuickSlottedEntry.QuickSlotTag, QuickSlottedEntry.ItemID);
				}
				else
				{
					QuickSlotManagerComponent->RemoveQuickSlot(QuickSlottedEntry.ItemID);
				}
			});
	}
}

void ARPGPlayerController::ToggleInventory()
{
	if (!IsValid(InventoryDashboardWidget))
	{
		CreateInventoryWidget();
		InventoryDashboardWidget->OpenInventoryDashboard();
	}
	else if (InventoryDashboardWidget->bIsOpen)
	{
		InventoryDashboardWidget->CloseInventoryDashboard();
	}
	else
	{
		InventoryDashboardWidget->OpenInventoryDashboard();
	}
}

void ARPGPlayerController::ChangeFocus(FString Focus)
{
	if (Focus == "Inventory")
	{
		SetShowMouseCursor(true);
		SetInputMode(FInputModeGameAndUI());
		ChangeMappingContext(UIOpenMappingContext);
	}
	else if (Focus == "Game")
	{
		SetShowMouseCursor(false);
		SetInputMode(FInputModeGameOnly());
		ChangeMappingContext(DefaultMappingContext);
	}
}
