// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetControllers/HUDOverlayController.h"

#include "Interfaces/InventoryInterface.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItem/InventoryItem.h"
#include "UI/HUD/HUDOverlay/HUDOverlayWidget.h"

void UHUDOverlayController::BindCallbacksToDependencies()
{
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
}

void UHUDOverlayController::UnbindAllEventsFromDelegates()
{
	HUDItemQuickSlottedDelegate.Clear();
	HUDItemChangedDelegate.Clear();
	HUDItemRemovedDelegate.Clear();
}

void UHUDOverlayController::SetOwningInventoryComponent()
{
	OwningInventoryComp = IInventoryInterface::Execute_GetInventoryComponent(OwningActor);
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