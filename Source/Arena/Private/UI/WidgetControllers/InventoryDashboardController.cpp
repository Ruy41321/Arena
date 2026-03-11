// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetControllers/InventoryDashboardController.h"

#include "QuickSlot/QuickSlotManagerComponent.h"
#include "Equipment/EquipmentManagerComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Interfaces/InventoryInterface.h"
#include "Interfaces/EquipmentInterface.h"
#include "Interfaces/QuickSlotInterface.h"
#include "Inventory/InventoryItem/InventoryItem.h"
#include "UI/HUD/Inventory/InventoryDashboardWidget.h"

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
	
	if (IsValid(OwningInventoryDashboardWidget) && IsValid(OwningInventoryComp))
	{
		BindDelegatesToWidget();
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
		OwningInventoryComp->InventoryList.InventoryItemChangedDelegate.AddLambda(
			[this](const FRPGInventoryEntry& DirtyItem)
			{
				UInventoryItem* Item = UInventoryItem::CreateFromInventoryEntry(this, DirtyItem);
				DashboardBagItemChangedDelegate.Broadcast(Item);
			});

		OwningInventoryComp->InventoryList.InventoryItemRemovedDelegate.AddLambda(
			[this](const int64 RemovedItemID)
			{
				DashboardBagItemRemovedDelegate.Broadcast(RemovedItemID);
			});
		
		OwningInventoryComp->InventoryList.QuickSlotItemRelocatedDelegate.AddLambda(
		[this](const FRPGInventoryEntry& InventoryEntry)
			{
				UInventoryItem* Item = UInventoryItem::CreateFromInventoryEntry(this, InventoryEntry);
				QuickSlotItemRelocatedDelegate.Broadcast(Item);
			});
		
		OwningInventoryComp->InventoryList.QuickSlotItemChangeDelegate.AddLambda(
		[this](const FRPGInventoryEntry& InventoryEntry)			{
				UInventoryItem* Item = UInventoryItem::CreateFromInventoryEntry(this, InventoryEntry);
				QuickSlotItemChangedDelegate.Broadcast(Item);
			});
		
		OwningInventoryComp->InventoryList.QuickSlotItemRemovedDelegate.AddLambda(
			[this](const int64 RemovedItemID)
			{
				QuickSlotItemRemovedDelegate.Broadcast(RemovedItemID);
			});
	}

	if (EnsureOwningEquipmentManagerComp())
	{
		OwningEquipmentManagerComp->EquipmentList.EquipmentEntryDelegate.AddLambda(
			[this](const FRPGEquipmentEntry& EquipmentEntry)
			{
				UInventoryItem* Item = UInventoryItem::CreateFromEquipmentEntry(this, EquipmentEntry);
				DashboardEquipmentChangeDelegate.Broadcast(Item);
			});
		OwningEquipmentManagerComp->EquipmentList.UnEquippedEntryDelegate.AddLambda(
			[this](const FRPGEquipmentEntry& UnEquippedEntry)
			{
				DashboardEquipmentRemovedDelegate.Broadcast(UnEquippedEntry.SlotTag);
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
		DashboardBagItemChangedDelegate.Broadcast(Item);
	}

	TArray<FRPGEquipmentEntry> FoundEntries;
	OwningEquipmentManagerComp->EquipmentList.GetEntries(FoundEntries);
	for (const FRPGEquipmentEntry& Entry : FoundEntries)
	{
		UInventoryItem* Item = UInventoryItem::CreateFromEquipmentEntry(this, Entry);
		DashboardEquipmentChangeDelegate.Broadcast(Item);
	}
}

void UInventoryDashboardController::BindDelegatesToWidget_Implementation()
{
	if (!IsValid(OwningInventoryDashboardWidget))
	{
		return;
	}
	
	DashboardBagItemChangedDelegate.AddDynamic(OwningInventoryDashboardWidget, &UInventoryDashboardWidget::OnInventoryItemChanged);
	DashboardBagItemRemovedDelegate.AddDynamic(OwningInventoryDashboardWidget, &UInventoryDashboardWidget::OnInventoryItemRemoved);
	DashboardEquipmentChangeDelegate.AddDynamic(OwningInventoryDashboardWidget, &UInventoryDashboardWidget::OnItemEquipped);
	DashboardEquipmentRemovedDelegate.AddDynamic(OwningInventoryDashboardWidget, &UInventoryDashboardWidget::OnItemUnequipped);
	QuickSlotItemRelocatedDelegate.AddDynamic(OwningInventoryDashboardWidget, &UInventoryDashboardWidget::OnQuickSlotItemRelocated);
	QuickSlotItemChangedDelegate.AddDynamic(OwningInventoryDashboardWidget, &UInventoryDashboardWidget::OnQuickSlotItemChanged);
	QuickSlotItemRemovedDelegate.AddDynamic(OwningInventoryDashboardWidget, &UInventoryDashboardWidget::OnQuickSlotItemRemoved);
}

void UInventoryDashboardController::UnbindAllEventsFromDelegates()
{
	DashboardBagItemChangedDelegate.Clear();
	DashboardBagItemRemovedDelegate.Clear();
	DashboardEquipmentChangeDelegate.Clear();
	DashboardEquipmentRemovedDelegate.Clear();
}
