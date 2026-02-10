// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetControllers/InventoryWidgetController.h"
#include "Inventory/InventoryComponent.h"
#include "Interfaces/InventoryInterface.h"

void UInventoryWidgetController::SetOwningActor(AActor* InOwner)
{
	OwningActor = InOwner;
}

void UInventoryWidgetController::UpdateInventory(const FPackagedInventory& PackagedInventory)
{
	if (IsValid(OwningInventory))
	{
		//OwningInventory->ReconstructInventoryMap(PackagedInventory);

		BroadcastInventoryContent();
	}
}

void UInventoryWidgetController::BroadcastInventoryContent()
{
	if (IsValid(OwningInventory))
	{
		TMap<FGameplayTag, int32> LocalInventoryMap = OwningInventory->GetInventoryTagMap();

		for (const auto& Pair : LocalInventoryMap)
		{
			FMasterItemDefinition ItemDefinition = OwningInventory->GetItemDefinitionByTag(Pair.Key);
			ItemDefinition.ItemQuantity = Pair.Value;
			InventoryItemDelegate.Broadcast(ItemDefinition);
		}
	}
}

void UInventoryWidgetController::BindCallbacksToDependencies()
{
	OwningInventory = IInventoryInterface::Execute_GetInventoryComponent(OwningActor);

	if (IsValid(OwningInventory))
	{
		OwningInventory->InventoryPackagedDelegate.AddLambda(
			[this](const FPackagedInventory& PackagedInventory)
			{
				UpdateInventory(PackagedInventory);
			});
	}
}

void UInventoryWidgetController::BroadcastInitialValues()
{
	if (IsValid(OwningInventory))
	{
		BroadcastInventoryContent();
	}
}
