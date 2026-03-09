// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetControllers/InventoryDashboardController.h"

#include "QuickSlot/QuickSlotManagerComponent.h"
#include "Equipment/EquipmentManagerComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Interfaces/InventoryInterface.h"
#include "Interfaces/EquipmentInterface.h"
#include "Interfaces/QuickSlotInterface.h"
#include "Inventory/InventoryItem/InventoryItem.h"

void UInventoryDashboardController::SetOwningActor(AActor* InOwner)
{
	OwningActor = InOwner;
}

void UInventoryDashboardController::SetOwningInventory()
{
	OwningInventoryComp = IInventoryInterface::Execute_GetInventoryComponent(OwningActor);
}

void UInventoryDashboardController::SetOwningEquipmentManagerComp()
{
	OwningEquipmentManagerComp = IEquipmentInterface::Execute_GetEquipmentManagerComponent(OwningActor);
}

void UInventoryDashboardController::SetOwningQuickSlotManagerComp()
{
	OwningQuickSlotManagerComp = IQuickSlotInterface::Execute_GetQuickSlotManagerComponent(OwningActor);
}

void UInventoryDashboardController::SetInventoryWidget(UInventoryDashboardWidget* InInventoryWidget)
{
	OwningInventoryDashboardWidget = InInventoryWidget;
	
	if (IsValid(OwningInventoryComp))
	{
		BindDelegatesToInventoryWidget();
	}
}

bool UInventoryDashboardController::EnsureOwningInventory()
{
	if (!IsValid(OwningInventoryComp))
	{
		SetOwningInventory();
	}
	return IsValid(OwningInventoryComp);
}

bool UInventoryDashboardController::EnsureOwningEquipmentManagerComp()
{
	if (!IsValid(OwningEquipmentManagerComp))
	{
		SetOwningEquipmentManagerComp();
	}
	return IsValid(OwningEquipmentManagerComp);	
}

bool UInventoryDashboardController::EnsureOwningQuickSlotManagerComp()
{
	if (!IsValid(OwningQuickSlotManagerComp))
	{
		SetOwningQuickSlotManagerComp();
	}
	return IsValid(OwningQuickSlotManagerComp);
}

void UInventoryDashboardController::BindCallbacksToDependencies()
{
	if (EnsureOwningInventory())
	{
		OwningInventoryComp->InventoryList.DirtyItemDelegate.AddLambda(
			[this](const FRPGInventoryEntry& DirtyItem)
			{
				UInventoryItem* Item = UInventoryItem::CreateFromInventoryEntry(this, DirtyItem);
				InventoryEntryDelegate.Broadcast(Item);
			});

		OwningInventoryComp->InventoryList.InventoryItemRemovedDelegate.AddLambda(
			[this](const int64 RemovedItemID)
			{
				InventoryItemRemovedDelegate.Broadcast(RemovedItemID);
			});
		
		OwningInventoryComp->InventoryList.InventoryItemQuickSlottedDelegate.AddLambda(
		[this](const FRPGInventoryEntry& InventoryEntry)
			{
				UInventoryItem* Item = UInventoryItem::CreateFromInventoryEntry(this, InventoryEntry);
				InventoryItemQuickSlottedDelegate.Broadcast(Item);
			});
	}

	if (EnsureOwningEquipmentManagerComp())
	{
		OwningEquipmentManagerComp->EquipmentList.EquipmentEntryDelegate.AddLambda(
			[this](const FRPGEquipmentEntry& EquipmentEntry)
			{
				UInventoryItem* Item = UInventoryItem::CreateFromEquipmentEntry(this, EquipmentEntry);
				EquippedEntryDelegate.Broadcast(Item);
			});
		OwningEquipmentManagerComp->EquipmentList.UnEquippedEntryDelegate.AddLambda(
			[this](const FRPGEquipmentEntry& UnEquippedEntry)
			{
				UnEquippedEntryDelegate.Broadcast(UnEquippedEntry.SlotTag);
			});
	}
}

void UInventoryDashboardController::BroadcastInitialValues()
{
	if (!EnsureOwningInventory() and !EnsureOwningEquipmentManagerComp())
	{
		return;
	}

	for (const FRPGInventoryEntry& Entry : OwningInventoryComp->GetInventoryEntries())
	{
		UInventoryItem* Item = UInventoryItem::CreateFromInventoryEntry(this, Entry);
		InventoryEntryDelegate.Broadcast(Item);
	}

	TArray<FRPGEquipmentEntry> FoundEntries;
	OwningEquipmentManagerComp->EquipmentList.GetEntries(FoundEntries);
	for (const FRPGEquipmentEntry& Entry : FoundEntries)
	{
		UInventoryItem* Item = UInventoryItem::CreateFromEquipmentEntry(this, Entry);
		EquippedEntryDelegate.Broadcast(Item);
	}
}

void UInventoryDashboardController::UnbindAllEventsFromDelegates()
{
	InventoryEntryDelegate.Clear();
	InventoryItemRemovedDelegate.Clear();
	EquippedEntryDelegate.Clear();
	UnEquippedEntryDelegate.Clear();
}
