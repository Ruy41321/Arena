// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerController/MKHPlayerController.h"
#include "Input/MKHSystemInputComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Inventory/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystem/MKHAbilitySystemComponent.h"
#include "AbilitySystem/MKHGameplayTags.h"
#include "Player/PlayerState/MKHPlayerState.h"
#include "Equipment/EquipmentManagerComponent.h"
#include "Inventory/InventoryItem/InventoryItem.h"
#include "QuickSlot/QuickSlotManagerComponent.h"
#include "UI/HUD/MainHUD.h"

AMKHPlayerController::AMKHPlayerController()
{
	bReplicates = true;

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	InventoryComponent->SetIsReplicated(true);

	EquipmentComponent = CreateDefaultSubobject<UEquipmentManagerComponent>(TEXT("EquipmentComponent"));
	EquipmentComponent->SetIsReplicated(true);
	
	QuickSlotManagerComponent = CreateDefaultSubobject<UQuickSlotManagerComponent>(TEXT("QuickSlotManagerComponent"));
}

UMKHAbilitySystemComponent* AMKHPlayerController::GetRPGAbilitySystemComponent()
{
	if (!IsValid(MKHAbilitySystemComponent))
	{
		if (AMKHPlayerState* MKHPlayerState = GetPlayerState<AMKHPlayerState>())
		{
			if (UMKHAbilitySystemComponent* ASC = MKHPlayerState->GetRPGAbilitySystemComponent())
			{
				MKHAbilitySystemComponent = ASC;
			}
		}
	}

	return MKHAbilitySystemComponent;
}

void AMKHPlayerController::SetupInputComponent()
{
	APlayerController::SetupInputComponent();
	if (UMKHSystemInputComponent* RPGInputComponent = Cast<UMKHSystemInputComponent>(InputComponent))
	{
		RPGInputComponent->BindAbilityActions(MKHInputConfig, MKHGameplayTags::Input::Ability,this, &AMKHPlayerController::AbilityInputPressed, &AMKHPlayerController::AbilityInputReleased);
		
		if (IsValid(ToggleInventoryAction))
		{
			RPGInputComponent->BindAction(ToggleInventoryAction, ETriggerEvent::Started, this, &AMKHPlayerController::ToggleInventory);
		}
	}
}

void AMKHPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMKHPlayerController, InventoryComponent);
}

void AMKHPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	ChangeMappingContext(DefaultMappingContext);
	
	BindCallbacksToDependencies();
	
}

void AMKHPlayerController::ChangeMappingContext(const UInputMappingContext* NewMappingContext) const
{
	if (UEnhancedInputLocalPlayerSubsystem *Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->ClearAllMappings();
		Subsystem->AddMappingContext(NewMappingContext, 0);
		MappingContextChangedDelegate.Broadcast(NewMappingContext);
	}
}

UAbilitySystemComponent* AMKHPlayerController::GetAbilitySystemComponent() const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());
}

UEquipmentManagerComponent* AMKHPlayerController::GetEquipmentManagerComponent_Implementation() const
{
	return EquipmentComponent;
}

UInventoryComponent* AMKHPlayerController::GetInventoryComponent_Implementation() const
{
	return InventoryComponent;
}

UQuickSlotManagerComponent* AMKHPlayerController::GetQuickSlotManagerComponent_Implementation() const
{
	return QuickSlotManagerComponent;
}

bool AMKHPlayerController::EnsureWeaponEquipped() const
{
	if (EquipmentComponent->GetEquipmentEntryBySlot(MKHGameplayTags::Equip::WeaponSlot))
	{
		return true;
	}
	// If not already equipped, Equip a weapon from quickslot 
	return QuickSlotManagerComponent->TryEquipWeapon();
}

void AMKHPlayerController::AbilityInputPressed(const FGameplayTag& InputTag)
{
	if (InputTag.MatchesTag(MKHGameplayTags::Input::Attacks))
	{
		const bool bHasWeapon = EnsureWeaponEquipped();
		
		if (InputTag.MatchesTag(MKHGameplayTags::Input::Skill))
			SkillActivatedDelegate.Execute(InputTag);
		
		if (!bHasWeapon)
			return ;
	}
		
	if (IsValid(GetRPGAbilitySystemComponent()))
	{
		MKHAbilitySystemComponent->AbilityInputPressed(InputTag);
	}
}

void AMKHPlayerController::AbilityInputReleased(const FGameplayTag& InputTag)
{
	if (IsValid(GetRPGAbilitySystemComponent()))
	{
		MKHAbilitySystemComponent->AbilityInputReleased(InputTag);
	}
}

/**
 * Callbacks between components (stays in the model)
 */
void AMKHPlayerController::BindCallbacksToDependencies() const
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
		InventoryComponent->InventoryList.QuickSlotItemRelocatedDelegate.AddLambda(
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

void AMKHPlayerController::ChangeFocus(FString Focus)
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

void AMKHPlayerController::ToggleInventory()
{
	if (AMainHUD* HUD = GetHUD<AMainHUD>())
	{
		HUD->ToggleInventory();
	}
}
