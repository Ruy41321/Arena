// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/InventoryItem/InventoryItem.h"

#include "Equipment/EquipmentManagerComponent.h"
#include "Interfaces/InventoryInterface.h"
#include "Inventory/InventoryComponent.h"

UInventoryItem* UInventoryItem::CreateFromInventoryEntry(UObject* WorldContextObject, const FRPGInventoryEntry& InEntry)
{
	if (!IsValid(WorldContextObject))
	{
		return nullptr;
	}

	UInventoryItem* NewItem = NewObject<UInventoryItem>(WorldContextObject);
	if (IsValid(NewItem))
	{
		NewItem->InitFromInventoryEntry(InEntry);
	}
	return NewItem;
}

UInventoryItem* UInventoryItem::CreateFromEquipmentEntry(UObject* WorldContextObject, const FRPGEquipmentEntry& InEntry)
{
	if (!IsValid(WorldContextObject))
	{
		return nullptr;
	}

	UInventoryItem* NewItem = NewObject<UInventoryItem>(WorldContextObject);
	if (IsValid(NewItem))
	{
		NewItem->InitFromEquipmentEntry(InEntry);
	}
	return NewItem;
}

void UInventoryItem::InitFromInventoryEntry(const FRPGInventoryEntry& InEntry)
{
	ItemTag = InEntry.ItemTag;
	RarityTag = InEntry.RarityTag;
	Quantity = InEntry.Quantity;
	ItemName = InEntry.ItemName;
	EffectPackage = InEntry.EffectPackage;
	ItemID = InEntry.ItemID;
	bIsEquipped = false;
	bIsQuickSlotted = InEntry.bIsQuickSlotted;
	QuickSlotTag = InEntry.QuickSlotTag;
	ResolveItemDefinition();
}

void UInventoryItem::InitFromEquipmentEntry(const FRPGEquipmentEntry& InEquipEntry)
{
	ItemTag = InEquipEntry.EntryTag;
	SlotTag = InEquipEntry.SlotTag;
	RarityTag = InEquipEntry.RarityTag;
	Quantity = 1;
	EffectPackage = InEquipEntry.EffectPackage;
	ItemID = InEquipEntry.OriginalItemID;
	bIsEquipped = true;
	ResolveItemDefinition();
	ItemName = ItemDefinition.ItemName;
}

void UInventoryItem::CopyFrom(const UInventoryItem* Other)
{
	if (!IsValid(Other))
	{
		return;
	}

	ItemTag = Other->ItemTag;
	SlotTag = Other->SlotTag;
	RarityTag = Other->RarityTag;
	Quantity = Other->Quantity;
	ItemName = Other->ItemName;
	EffectPackage = Other->EffectPackage;
	ItemID = Other->ItemID;
	bIsEquipped = Other->bIsEquipped;
	bIsQuickSlotted = Other->bIsQuickSlotted;
	QuickSlotTag = Other->QuickSlotTag;
	ItemDefinition = Other->ItemDefinition;
	Icon = Other->Icon;
	Description = Other->Description;

	OnItemUpdatedDelegate.Broadcast();
}

void UInventoryItem::ResolveItemDefinition()
{
	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!IsValid(PC) || !PC->Implements<UInventoryInterface>())
	{
		return;
	}

	const UInventoryComponent* InventoryComp = IInventoryInterface::Execute_GetInventoryComponent(PC);
	if (!IsValid(InventoryComp))
	{
		return;
	}

	ItemDefinition = InventoryComp->GetItemDefinitionByTag(ItemTag);
	Icon = ItemDefinition.Icon;
	Description = ItemDefinition.Description;
}
