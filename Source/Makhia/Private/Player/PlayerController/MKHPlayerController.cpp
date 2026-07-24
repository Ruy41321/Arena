// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerController/MKHPlayerController.h"

#include <MKHLogChannels.h>

#include "Input/MKHSystemInputComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Inventory/InventoryComponent.h"
#include "GameFramework/GameMode.h"
#include "GameMode/MKHGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystem/MKHAbilitySystemComponent.h"
#include "AbilitySystem/MKHGameplayTags.h"
#include "Player/PlayerState/MKHPlayerState.h"
#include "Equipment/EquipmentManagerComponent.h"
#include "Inventory/InventoryItem/InventoryItem.h"
#include "Player/MKHPlayerCharacter.h"
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
	QuickSlotManagerComponent->SetIsReplicated(true);
}

UMKHAbilitySystemComponent* AMKHPlayerController::GetMKHAbilitySystemComponent()
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
		
		if (IsValid(ToggleGameMenuAction))
		{
			RPGInputComponent->BindAction(ToggleGameMenuAction, ETriggerEvent::Started, this, &AMKHPlayerController::ToggleGameMenu);
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
	
	ChangeMappingContext(GetGameplayMappingContext());

	BindCallbacksToDependencies();
	
	if (HasAuthority())
	{
		SubscribeToEquipEvents();
	}
	
}

void AMKHPlayerController::AcknowledgePossession(APawn* NewPawn)
{
	Super::AcknowledgePossession(NewPawn);

	// A freshly possessed pawn is alive by definition, so a block left over from a previous life
	// must not survive into it. Matters because this controller outlives its pawn across a seamless
	// travel, and a brand new character never exits the Dead state that would otherwise clear it.
	// No-op in the common case: the flag is already false and SetGameplayInputEnabled early-outs.
	SetGameplayInputEnabled(true);
}

void AMKHPlayerController::ChangeMappingContext(const UInputMappingContext* NewMappingContext) const
{
	if (UEnhancedInputLocalPlayerSubsystem *Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->ClearAllMappings();

		// A null context is a valid request: it means "no mappings at all", used to fully block input.
		if (IsValid(NewMappingContext))
		{
			Subsystem->AddMappingContext(NewMappingContext, 0);
		}

		MappingContextChangedDelegate.Broadcast(NewMappingContext);
	}
}

const UInputMappingContext* AMKHPlayerController::GetGameplayMappingContext() const
{
	return bGameplayInputBlocked ? DeadMappingContext.Get() : DefaultMappingContext.Get();
}

void AMKHPlayerController::SetGameplayInputEnabled(const bool bEnabled)
{
	// Only the machine that owns the local player has an Enhanced Input subsystem to talk to;
	// on every other machine this is a no-op and the flag must not be left out of sync.
	if (!IsLocalController())
	{
		return;
	}

	if (bGameplayInputBlocked == !bEnabled)
	{
		return;
	}

	bGameplayInputBlocked = !bEnabled;
	ChangeMappingContext(GetGameplayMappingContext());
}

void AMKHPlayerController::Server_RequestRematch_Implementation()
{
	AMKHGameMode* GameMode = Cast<AMKHGameMode>(UGameplayStatics::GetGameMode(this));
	if (!IsValid(GameMode))
	{
		UE_LOG(LogMKHAbility, Warning, TEXT("Server_RequestRematch: no authoritative MKH game mode, vote ignored."));
		return;
	}

	GameMode->RequestRematch(this);
}

void AMKHPlayerController::Server_RestartMatch_Implementation()
{
	AGameMode* GameMode = Cast<AGameMode>(UGameplayStatics::GetGameMode(this));
	if (!IsValid(GameMode))
	{
		UE_LOG(LogMKHAbility, Warning, TEXT("Server_RestartMatch: no authoritative game mode, restart ignored."));
		return;
	}

	GameMode->RestartGame();
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

bool AMKHPlayerController::EnsureWeaponEquipped(const FGameplayTag& InputTag, bool& bForceQueue)
{
	// While a weapon swap is in flight the equipment list may be momentarily stale
	// (PlayerController and PlayerState replicate independently): queue the attack
	// instead of re-triggering the quick-equip flow on stale data.
	UMKHAbilitySystemComponent* MKHAsc = GetMKHAbilitySystemComponent();
	if (IsValid(MKHAsc) && MKHAsc->IsWeaponSwapInFlight())
	{
		bForceQueue = true;
		return true;
	}

	// The ASC already granting this input tag counts as armed too, even if the equipment
	// list (a separate actor) hasn't caught up yet.
	const bool bHasWeapon = EquipmentComponent->GetEquipmentEntryBySlot(MKHGameplayTags::Equip::WeaponSlot) != nullptr
		|| (IsValid(MKHAsc) && MKHAsc->HasGrantedAbilityForInputTag(InputTag));

	// Handle weapon equipping logic if unarmed
	if (!bHasWeapon)
	{       
		const FGameplayTag QuickEquipTag = QuickSlotManagerComponent->GetQuickEquipWeaponSlotTag();
		if (!QuickEquipTag.IsValid())
		{
			// Doesn't have a weapon in quick slots
			return false;
		}

		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
		{
			// States when the ability should be queued
			if (ASC->HasMatchingGameplayTag(MKHGameplayTags::Ability::Priority1))
			{
				// Queue both the ability to equip the weapon and the attack (toggleing bForceQueue)
				bForceQueue = true;
				if (IsValid(GetMKHAbilitySystemComponent()))
				{
					MKHAbilitySystemComponent->AbilityInputPressed(QuickEquipTag, bForceQueue);
				}
			}
			else if (QuickSlotManagerComponent->TryEquipWeapon())
			{
				// On a listen server the equip runs synchronously in authority and the ability may
				// already be granted; only queue the attack if it truly isn't available yet, which
				// happens on remote clients while the equip RPC round trip is still in flight.
				if (!IsValid(MKHAsc) || !MKHAsc->HasGrantedAbilityForInputTag(InputTag))
				{
					bForceQueue = true;
				}
			}
			else
			{
				// Abort input processing if we are not queuing and equip attempt fails
				return false;
			}
		}
	}

	return true;
}

void AMKHPlayerController::AbilityInputPressed(const FGameplayTag& InputTag)
{
    bool bForceQueue = false;

    // Process specific logic for attacks that are not dodges and so need an equipped weapon
    if (InputTag.MatchesTag(MKHGameplayTags::Input::Attacks) && !InputTag.MatchesTag(MKHGameplayTags::Input::Dodge))
    {
        if (IsLocalController() && InputTag.MatchesTag(MKHGameplayTags::Input::Skill))
        {
            SkillActivatedDelegate.ExecuteIfBound(InputTag);
        }
       
    	// Return true if has equipped the weapon or queued it.
        if (!EnsureWeaponEquipped(InputTag, bForceQueue))
        {
            return;
        }
    }
       
    // On clients the ASC lives on the PlayerState and may not have replicated yet
    // (e.g. input pressed right after joining): drop the input instead of crashing.
    if (!IsValid(GetMKHAbilitySystemComponent()))
    {
        UE_LOG(LogMKHAbility, Warning, TEXT("AMKHPlayerController::AbilityInputPressed - ASC not available yet, ignoring input %s"), *InputTag.ToString());
        return;
    }

    MKHAbilitySystemComponent->AbilityInputPressed(InputTag, bForceQueue);
}

void AMKHPlayerController::AbilityInputReleased(const FGameplayTag& InputTag)
{
	if (IsValid(GetMKHAbilitySystemComponent()))
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

void AMKHPlayerController::ChangeFocus(const EFocusOption NewFocus)
{
	switch (NewFocus)
	{
		case EFocusOption::UI:
			SetShowMouseCursor(true);
			SetInputMode(FInputModeGameAndUI());
			ChangeMappingContext(UIOpenMappingContext);
			break;
		case EFocusOption::Game:
		default:
			SetShowMouseCursor(false);
			SetInputMode(FInputModeGameOnly());
			// Returning to game focus must not resurrect the full gameplay bindings while input is blocked
			// (e.g. closing the menu after dying).
			ChangeMappingContext(GetGameplayMappingContext());
	}
}

void AMKHPlayerController::ToggleInventory()
{
	if (AMainHUD* HUD = GetHUD<AMainHUD>())
	{
		HUD->ToggleInventory();
	}
}

void AMKHPlayerController::ToggleGameMenu()
{
	if (AMainHUD* HUD = GetHUD<AMainHUD>())
	{
		HUD->ToggleGameMenu();
	}
}

void AMKHPlayerController::SubscribeToEquipEvents() const
{
	if (!IsValid(EquipmentComponent))
	{
		return;
	}

	// Binding to Equipped
	EquipmentComponent->EquipmentList.EquipmentEntryDelegate.AddLambda(
		[this](const FRPGEquipmentEntry& Entry)
		{
			if (Entry.SlotTag.MatchesTag(MKHGameplayTags::Equip::WeaponSlot))
			{
				AMKHPlayerCharacter* PlayerCharacter = Cast<AMKHPlayerCharacter>(GetPawn());
				if (PlayerCharacter)
				{
					PlayerCharacter->bIsHoldingWeapon = true;
				}
			}
		});

	//Binding to Unequipped
	EquipmentComponent->EquipmentList.UnEquippedEntryDelegate.AddLambda(
		[this](const FRPGEquipmentEntry& Entry)
		{
			if (Entry.SlotTag.MatchesTag(MKHGameplayTags::Equip::WeaponSlot))
			{
				AMKHPlayerCharacter* PlayerCharacter = Cast<AMKHPlayerCharacter>(GetPawn());
				if (PlayerCharacter)
				{
					PlayerCharacter->bIsHoldingWeapon = false;
				}
			}
		});
}
