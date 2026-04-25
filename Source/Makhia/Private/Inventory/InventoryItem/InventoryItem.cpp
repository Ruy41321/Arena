// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/InventoryItem/InventoryItem.h"

#include "Equipment/EquipmentDefinition.h"
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
	SlotTag = FGameplayTag();
	RarityTag = InEntry.RarityTag;
	Quantity = InEntry.Quantity;
	ItemName = InEntry.ItemName;
	EffectPackage = InEntry.EffectPackage;
	ItemID = InEntry.ItemID;
	bIsEquipped = InEntry.bIsUsed;
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
	bIsQuickSlotted = false;
	QuickSlotTag = FGameplayTag();
	ResolveItemDefinition();
	ItemName = ItemDefinition.ItemName;
}

void UInventoryItem::CopyFrom(const UInventoryItem* Other)
{
	if (!IsValid(Other))
	{
		return;
	}

	CopyCoreDataFrom(*Other);
	OnItemUpdatedDelegate.Broadcast();
}

void UInventoryItem::CopyCoreDataFrom(const UInventoryItem& Other)
{
	ItemTag = Other.ItemTag;
	SlotTag = Other.SlotTag;
	RarityTag = Other.RarityTag;
	Quantity = Other.Quantity;
	ItemName = Other.ItemName;
	EffectPackage = Other.EffectPackage;
	ItemID = Other.ItemID;
	bIsEquipped = Other.bIsEquipped;
	bIsQuickSlotted = Other.bIsQuickSlotted;
	QuickSlotTag = Other.QuickSlotTag;
	ItemDefinition = Other.ItemDefinition;
	Icon = Other.Icon;
	BaseWeaponDamage = Other.BaseWeaponDamage;
	Description = Other.Description;
}

bool UInventoryItem::TryResolveItemDefinition(FMasterItemDefinition& OutItemDefinition) const
{
	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!IsValid(PC) || !PC->Implements<UInventoryInterface>())
	{
		return false;
	}

	const UInventoryComponent* InventoryComp = IInventoryInterface::Execute_GetInventoryComponent(PC);
	if (!IsValid(InventoryComp))
	{
		return false;
	}

	OutItemDefinition = InventoryComp->GetItemDefinitionByTag(ItemTag);
	return true;
}

void UInventoryItem::CacheDerivedDataFromItemDefinition()
{
	Icon = ItemDefinition.Icon;
	BaseWeaponDamage = 0.f;

	if (IsValid(ItemDefinition.EquipmentItemProps.EquipmentClass))
	{
		if (const UEquipmentDefinition* EquipmentDefinition = GetDefault<UEquipmentDefinition>(ItemDefinition.EquipmentItemProps.EquipmentClass))
		{
			BaseWeaponDamage = EquipmentDefinition->BaseDamage;
		}
	}

	Description = ItemDefinition.ItemDescription;
}

void UInventoryItem::ResetDerivedData()
{
	ItemDefinition = FMasterItemDefinition();
	Icon = nullptr;
	BaseWeaponDamage = 0.f;
	Description = FText();
}

void UInventoryItem::ResolveItemDefinition()
{
	FMasterItemDefinition ResolvedDefinition;
	if (!TryResolveItemDefinition(ResolvedDefinition))
	{
		ResetDerivedData();
		return;
	}

	ItemDefinition = ResolvedDefinition;
	CacheDerivedDataFromItemDefinition();
}

