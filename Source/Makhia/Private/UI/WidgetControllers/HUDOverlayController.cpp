// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetControllers/HUDOverlayController.h"

#include "AbilitySystem/MKHGameplayTags.h"
#include "Interfaces/InventoryInterface.h"
#include "Interfaces/EquipmentInterface.h"
#include "Interfaces/QuickSlotInterface.h"
#include "Inventory/InventoryComponent.h"
#include "Equipment/EquipmentManagerComponent.h"
#include "Inventory/InventoryItem/InventoryItem.h"
#include "Player/PlayerController/MKHPlayerController.h"
#include "QuickSlot/QuickSlotManagerComponent.h"
#include "UI/HUD/HUDOverlay/HUDOverlayWidget.h"

void UHUDOverlayController::BindCallbacksToDependencies()
{
	if (AMKHPlayerController* PC = Cast<AMKHPlayerController>(OwningActor))
	{
		PC->SkillActivatedDelegate.BindLambda(
			[this](const FGameplayTag& SkillTag)
			{
				if (SkillTag.MatchesTag(MKHGameplayTags::Input::Skill))
				OwningHUDOverlayWidget->OnQuickSlotActivated(SkillTag);
			});
	}
	
	if (EnsureOwningQuickSlotManagerComp())
	{
		OwningQuickSlotManagerComp->QuickSlotActivatedDelegate.BindLambda(
			[this](const FGameplayTag& QuickSlotTag)
			{
				OwningHUDOverlayWidget->OnQuickSlotActivated(QuickSlotTag);
			});
	}
	
	if (EnsureOwningEquipment())
	{
		OwningEquipmentComp->EquipmentList.EquipmentEntryDelegate.AddLambda(
			[this](const FRPGEquipmentEntry& EquipmentEntry)
			{
				if (EquipmentEntry.SlotTag.MatchesTagExact(MKHGameplayTags::Equip::WeaponSlot))
				{
					HUDWeaponEquippedDelegate.Broadcast(EquipmentEntry.OriginalItemID);
				}
			});
	}
	
	if (EnsureOwningInventory())
	{
		OwningInventoryComp->InventoryList.QuickSlotItemRelocatedDelegate.AddLambda(
		[this](const FRPGInventoryEntry& InventoryEntry)
			{
				UInventoryItem* Item = UInventoryItem::CreateFromInventoryEntry(this, InventoryEntry);
				HUDItemQuickSlottedDelegate.Broadcast(Item);
			});
		
		OwningInventoryComp->InventoryList.QuickSlotItemChangeDelegate.AddLambda(
		[this](const FRPGInventoryEntry& InventoryEntry)			{
				UInventoryItem* Item = UInventoryItem::CreateFromInventoryEntry(this, InventoryEntry);
				HUDItemChangedDelegate.Broadcast(Item);
			});
		
		OwningInventoryComp->InventoryList.QuickSlotItemRemovedDelegate.AddLambda(
			[this](const int64 RemovedItemID)
			{
				HUDItemRemovedDelegate.Broadcast(RemovedItemID);
			});
	}
}

void UHUDOverlayController::BroadcastInitialValues()
{
	if (!EnsureOwningInventory())
	{
		return;
	}

	for (const FRPGInventoryEntry& Entry : OwningInventoryComp->GetInventoryEntries())
	{
		if (Entry.IsValid() && Entry.bIsQuickSlotted)
		{
			UInventoryItem* Item = UInventoryItem::CreateFromInventoryEntry(this, Entry);
			HUDItemQuickSlottedDelegate.Broadcast(Item);
		}
	}
}

void UHUDOverlayController::BindDelegatesToWidget_Implementation()
{
	if (!IsValid(OwningHUDOverlayWidget))
	{
		return;
	}
	
	HUDItemQuickSlottedDelegate.AddDynamic(OwningHUDOverlayWidget, &UHUDOverlayWidget::OnItemQuickSlotted);
	HUDItemChangedDelegate.AddDynamic(OwningHUDOverlayWidget, &UHUDOverlayWidget::OnItemChanged);
	HUDItemRemovedDelegate.AddDynamic(OwningHUDOverlayWidget, &UHUDOverlayWidget::OnItemRemoved);
	HUDWeaponEquippedDelegate.AddDynamic(OwningHUDOverlayWidget, &UHUDOverlayWidget::OnWeaponEquipped);
}

void UHUDOverlayController::UnbindAllEventsFromDelegates()
{
	HUDItemQuickSlottedDelegate.Clear();
	HUDItemChangedDelegate.Clear();
	HUDItemRemovedDelegate.Clear();
	HUDWeaponEquippedDelegate.Clear();
}

void UHUDOverlayController::SetOwningInventoryComponent()
{
	OwningInventoryComp = IInventoryInterface::Execute_GetInventoryComponent(OwningActor);
}

void UHUDOverlayController::SetOwningEquipmentComponent()
{
	OwningEquipmentComp = IEquipmentInterface::Execute_GetEquipmentManagerComponent(OwningActor);
}

void UHUDOverlayController::SetOwningQuickSlotManagerComp()
{
	OwningQuickSlotManagerComp = IQuickSlotInterface::Execute_GetQuickSlotManagerComponent(OwningActor);
}

void UHUDOverlayController::SetHUDOverlayWidget(UHUDOverlayWidget* InHUDOverlayWidget)
{
	OwningHUDOverlayWidget = InHUDOverlayWidget;
	
	if (IsValid(OwningHUDOverlayWidget) && IsValid(OwningInventoryComp))
	{
		BindDelegatesToWidget();
	}
}

bool UHUDOverlayController::EnsureOwningInventory()
{
	if (!IsValid(OwningInventoryComp))
	{
		SetOwningInventoryComponent();
	}
	return IsValid(OwningInventoryComp);
}

bool UHUDOverlayController::EnsureOwningEquipment()
{
	if (!IsValid(OwningEquipmentComp))
	{
		SetOwningEquipmentComponent();
	}
	return IsValid(OwningEquipmentComp);
}

bool UHUDOverlayController::EnsureOwningQuickSlotManagerComp()
{
	if (!IsValid(OwningQuickSlotManagerComp))
	{
		SetOwningQuickSlotManagerComp();
	}
	return IsValid(OwningQuickSlotManagerComp);
}
