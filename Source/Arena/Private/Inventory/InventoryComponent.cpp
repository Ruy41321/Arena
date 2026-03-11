// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemTypesToTables.h"
#include "Data/EquipmentStatEffects.h"
#include "AbilitySystemComponent.h"
#include "NativeGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/RPGGameplayTags.h"
#include "Libraries/RPGAbilitySystemLibrary.h"
#include "Libraries/EquipmentRollLibrary.h"
#include "Equipment/EquipmentDefinition.h"
#include "Equipment/Rarity/RarityDefinition.h"
#include "Interfaces/QuickSlotInterface.h"
#include "Net/UnrealNetwork.h"
#include "Inventory/InventoryItem/InventoryItem.h"
#include "QuickSlot/QuickSlotManagerComponent.h"

void FRPGInventoryList::AddItem(const FGameplayTag& ItemTag, int32 NumItems)
{
	// Non-equipment items can be stacked onto an existing entry
	if (!ItemTag.MatchesTag(RPGGameplayTags::Equip::Category_Equipment))
	{
		if (TryStackItem(ItemTag, NumItems))
		{
			return;
		}
	}

	const FMasterItemDefinition ItemDef = OwnerComponent->GetItemDefinitionByTag(ItemTag);

	FRPGInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.ItemTag = ItemTag;
	NewEntry.ItemName = ItemDef.ItemName;
	NewEntry.Quantity = NumItems;
	NewEntry.ItemID = GenerateID();

	if (NewEntry.ItemTag.MatchesTag(RPGGameplayTags::Equip::Category_Equipment)
		&& IsValid(WeakStatsData.Get())
		&& WeakRarityTable.IsValid())
	{
		RollEquipmentEntry(NewEntry, ItemDef);
	}

	BroadcastNewEntry(NewEntry);
}

bool FRPGInventoryList::TryStackItem(const FGameplayTag& ItemTag, int32 NumItems)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FRPGInventoryEntry& Entry = *EntryIt;

		if (Entry.ItemTag.MatchesTagExact(ItemTag))
		{
			Entry.Quantity += NumItems;

			MarkItemDirty(Entry);

			if (OwnerComponent->GetOwner()->HasAuthority())
			{
				BroadcastEntryUpdate(Entry, true);
			}
			return true;
		}
	}

	return false;
}

void FRPGInventoryList::RollEquipmentEntry(FRPGInventoryEntry& NewEntry, const FMasterItemDefinition& ItemDef)
{
	const UEquipmentDefinition* EquipmentCDO = GetDefault<UEquipmentDefinition>(ItemDef.EquipmentItemProps.EquipmentClass);

	const FRarityDefinition* Rarity = UEquipmentRollLibrary::RollRarity(WeakRarityTable.Get());
	if (!Rarity)
	{
		return;
	}

	NewEntry.RarityTag = Rarity->RarityTag;

	NewEntry.EffectPackage.StatEffects = UEquipmentRollLibrary::RollPassiveStats(
		EquipmentCDO, WeakStatsData.Get(), Rarity->NumPassiveStats);

	if (EquipmentCDO->SlotTag.MatchesTag(RPGGameplayTags::Equip::WeaponSlot))
	{
		NewEntry.EffectPackage.Abilities = UEquipmentRollLibrary::RollActiveAbilities(
			EquipmentCDO, WeakStatsData.Get(), Rarity->NumActiveAbilities);
	}
}

void FRPGInventoryList::BroadcastNewEntry(FRPGInventoryEntry& NewEntry)
{
	MarkItemDirty(NewEntry);

	if (OwnerComponent->GetOwner()->HasAuthority())
	{
		BroadcastEntryUpdate(NewEntry, true);
	}
}

void FRPGInventoryList::BroadcastEntryUpdate(const FRPGInventoryEntry& Entry, bool bChanged)
{
	if (Entry.bIsQuickSlotted)
	{
		if (bChanged)
		{
			QuickSlotItemChangeDelegate.Broadcast(Entry);
		}
		else
		{
			QuickSlotItemRemovedDelegate.Broadcast(Entry.ItemID);
		}
	}
	else
	{
		if (bChanged)
		{
			InventoryItemChangedDelegate.Broadcast(Entry);
		}
		else
		{
			InventoryItemRemovedDelegate.Broadcast(Entry.ItemID);
		}
	}
}


void FRPGInventoryList::RemoveItem(const FRPGInventoryEntry& InventoryEntry, int32 NumItems)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FRPGInventoryEntry& Entry = *EntryIt;

		if (Entry.ItemID == InventoryEntry.ItemID)
		{
			Entry.Quantity = Entry.Quantity - NumItems;

			if (Entry.Quantity <= 0)
			{
				BroadcastEntryUpdate(Entry, false);
				EntryIt.RemoveCurrent();
				MarkArrayDirty();
			}
			else
			{
				MarkItemDirty(Entry);

				if (OwnerComponent->GetOwner()->HasAuthority())
				{
					BroadcastEntryUpdate(Entry, true);
				}
			}
			break;
		}
	}
}

FRPGInventoryEntry* FRPGInventoryList::FindEntryByID(int64 ItemID)
{
	return Entries.FindByPredicate([ItemID](const FRPGInventoryEntry& Entry)
	{
		return Entry.ItemID == ItemID;
	});
}

bool FRPGInventoryList::HasEnough(int64 ItemID, int32 NumItems)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FRPGInventoryEntry& Entry = *EntryIt;

		if (Entry.ItemID == ItemID)
		{
			if (Entry.Quantity >= NumItems)
			{
				return true;
			}
			return false;
		}
	}

	return false;
}

int64 FRPGInventoryList::GenerateID()
{
	int64 NewID = ++LastAssignedID;

	int32 SignatureIndex = 0;
	while (SignatureIndex < 12)
	{
		if (FMath::RandRange(0, 100) < 85)
		{
			NewID |= (int64)1 << FMath::RandRange(0, 62);
		}
		++SignatureIndex;
	}

	return NewID;
}

void FRPGInventoryList::SetStats(UEquipmentStatEffects* InStats)
{
	WeakStatsData = InStats;
}

void FRPGInventoryList::SetRarityTable(UDataTable* InRarityTable)
{
	WeakRarityTable = InRarityTable;
}

void FRPGInventoryList::AddUnEquippedItem(UInventoryItem* Item)
{
	const FMasterItemDefinition ItemDef = OwnerComponent->GetItemDefinitionByTag(Item->GetItemTag());

	// Weapon items are never removed from inventory when equipped, so the entry already exists.
	if (FRPGInventoryEntry* ExistingEntry = FindEntryByID(Item->GetItemID()))
	{
		ExistingEntry->bIsUsed = false;
		MarkItemDirty(*ExistingEntry);
		return;
	}

	FRPGInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.ItemTag = Item->GetItemTag();
	NewEntry.ItemName = Item->GetItemName();
	NewEntry.ItemID = Item->GetItemID();
	NewEntry.bIsQuickSlotted = Item->bIsQuickSlotted;
	NewEntry.QuickSlotTag = Item->QuickSlotTag;

	if (Item->GetItemTag().MatchesTag(RPGGameplayTags::Equip::Category_Equipment))
	{
		// Equipment items: restore rarity, effect package, quantity is always 1
		NewEntry.RarityTag = Item->GetRarityTag();
		NewEntry.EffectPackage = Item->GetEffectPackage();
		NewEntry.Quantity = 1;
	}
	else
	{
		// Fallback for any other item type
		NewEntry.Quantity = Item->GetQuantity() > 0 ? Item->GetQuantity() : 1;
	}

	BroadcastNewEntry(NewEntry);
}

FRPGInventoryEntry* FRPGInventoryList::GetAlreadyQuickSlottedEntry(const FGameplayTag& QuickSlotTag)
{
	const UQuickSlotManagerComponent* QuickSlotManagerComponent = IQuickSlotInterface::Execute_GetQuickSlotManagerComponent(OwnerComponent->GetOwner());
	const int64 AlreadySlottedItemID = QuickSlotManagerComponent->GetQuickSlotID(QuickSlotTag);
	
	if (AlreadySlottedItemID == 0)
	{
		return nullptr;
	}
	return FindEntryByID(AlreadySlottedItemID);
}

void FRPGInventoryList::AddEntryToQuickSlot(int64 ItemID, const FGameplayTag& QuickSlotTag)
{
	if (ItemID == 0 || !QuickSlotTag.IsValid())
	{
		return;
	}

	FRPGInventoryEntry* Entry = FindEntryByID(ItemID);
	if (Entry)
	{
		FRPGInventoryEntry* SwappedEntry = GetAlreadyQuickSlottedEntry(QuickSlotTag);
		if (SwappedEntry && SwappedEntry != Entry)
		{
			SwappedEntry->bIsQuickSlotted = Entry->bIsQuickSlotted;
			SwappedEntry->QuickSlotTag = Entry->QuickSlotTag;
			if (!SwappedEntry->bIsQuickSlotted && SwappedEntry->bIsUsed)
			{
				OwnerComponent->EquipmentItemUnequippedDelegate.Broadcast(SwappedEntry->ItemID);
			}
			QuickSlotItemRelocatedDelegate.Broadcast(*SwappedEntry);
		}
		Entry->bIsQuickSlotted = true;
		Entry->QuickSlotTag = QuickSlotTag;
		QuickSlotItemRelocatedDelegate.Broadcast(*Entry);
	}
}

void FRPGInventoryList::RemoveEntryFromQuickSlot(const int64 ItemID)
{
	if (FRPGInventoryEntry* Entry = FindEntryByID(ItemID))
	{
		if (Entry->bIsQuickSlotted)
		{
			if (Entry->bIsUsed)
			{
				OwnerComponent->EquipmentItemUnequippedDelegate.Broadcast(ItemID);
			}
			Entry->bIsQuickSlotted = false;
			Entry->QuickSlotTag = FGameplayTag();
			QuickSlotItemRelocatedDelegate.Broadcast(*Entry);
		}
	}
}

void FRPGInventoryList::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
	for (const int32 Index : RemovedIndices)
	{
		BroadcastEntryUpdate(Entries[Index], false);
	}
}

void FRPGInventoryList::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	for (const int32 Index : AddedIndices)
	{
		BroadcastEntryUpdate(Entries[Index], true);
	}
}

void FRPGInventoryList::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
	for (const int32 Index : ChangedIndices)
	{
		BroadcastEntryUpdate(Entries[Index], true);
	}
}

UInventoryComponent::UInventoryComponent() : InventoryList(this)
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, InventoryList);
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority())
	{
		InventoryList.SetStats(StatEffectsData);
		InventoryList.SetRarityTable(RarityTable);
	}

}

void UInventoryComponent::AddItem(const FGameplayTag& ItemTag, int32 NumItems)
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
		return;

	if (!Owner->HasAuthority())
	{
		ServerAddItem(ItemTag, NumItems);
		return;
	}

	InventoryList.AddItem(ItemTag, NumItems);
}

void UInventoryComponent::ServerAddItem_Implementation(const FGameplayTag& ItemTag, int32 NumItems)
{
	AddItem(ItemTag, NumItems);
}

void UInventoryComponent::UseItem(int64 ItemID, int32 NumItems)
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
		return;
		
	if (!InventoryList.HasEnough(ItemID, NumItems))
		return;

	if (!Owner->HasAuthority())
	{
		ServerUseItem(ItemID, NumItems);
		return;
	}

	FRPGInventoryEntry* Entry = InventoryList.FindEntryByID(ItemID);
	if (!Entry)
		return;
	
	if (Entry->bIsUsed)
	{
		EquipmentItemUnequippedDelegate.Broadcast(ItemID);
		return;
	}
	
	FMasterItemDefinition Item = GetItemDefinitionByTag(Entry->ItemTag);

	if (UAbilitySystemComponent* OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
	{
		if (IsValid(Item.ConsumableProps.ItemEffectClass))
		{
			const FGameplayEffectContextHandle EffectContext = OwnerASC->MakeEffectContext();
			const FGameplayEffectSpecHandle SpecHandle = OwnerASC->MakeOutgoingSpec(Item.ConsumableProps.ItemEffectClass, 
				Item.ConsumableProps.ItemEffectLevel, EffectContext);
			OwnerASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

			InventoryList.RemoveItem(*Entry);

			// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta,
			// 	FString::Printf(TEXT("Server Used %d of %s from inventory"), NumItems, *Item.ItemName.ToString()));
		}
		
		if (IsValid(Item.EquipmentItemProps.EquipmentClass))
		{
			UInventoryItem* InventoryItem = UInventoryItem::CreateFromInventoryEntry(this, *Entry);
			EquipmentItemUsedDelegate.Broadcast(InventoryItem);

			// Do not Remove from inventory list the Weapons items but marks them as Used to allow the unequip
			if (InventoryItem->ItemTag.MatchesTag(RPGGameplayTags::Equip::Category_Weapon))
			{
				Entry->bIsUsed = true;
				InventoryList.MarkItemDirty(*Entry);
				return;
			}
			InventoryList.RemoveItem(*Entry);
		}
	}
}

void UInventoryComponent::ServerUseItem_Implementation(int64 ItemID, int32 NumItems)
{
	UseItem(ItemID, NumItems);
}

bool UInventoryComponent::ServerUseItem_Validate(int64 ItemID, int32 NumItems)
{
	return InventoryList.HasEnough(ItemID, NumItems);
}

FMasterItemDefinition UInventoryComponent::GetItemDefinitionByTag(const FGameplayTag& ItemTag) const
{
	checkf(InventoryDefinitions, TEXT("InventoryDefinitions is not set on %s"), *GetNameSafe(this));

	for (const auto& Pair : InventoryDefinitions->TagsToTables)
	{
		if (ItemTag.MatchesTag(Pair.Key))
		{
			const UDataTable* DataTable = Pair.Value;
			if (DataTable)
			{
				FMasterItemDefinition* ItemDef = URPGAbilitySystemLibrary::GetDataTableRowByTag<FMasterItemDefinition>(DataTable, ItemTag);
				if (ItemDef)
				{
					return *ItemDef;
				}
			}
		}
	}

	return FMasterItemDefinition();
}

FRPGInventoryEntry UInventoryComponent::FindInventoryEntryByID(int64 ItemID)
{
	if (const FRPGInventoryEntry* Found = InventoryList.FindEntryByID(ItemID))
	{
		return *Found;
	}
	return FRPGInventoryEntry();
}

TArray<FRPGInventoryEntry> UInventoryComponent::GetInventoryEntries()
{
	return InventoryList.Entries;
}

void UInventoryComponent::AddUnEquippedItemEntry(UInventoryItem* Item)
{
	// Only the server adds the item back to the inventory.
	// The client receives the updated inventory via replication — no RPC needed.
	AActor* Owner = GetOwner();
	if (!IsValid(Owner) || !Owner->HasAuthority())
	{
		return;
	}

	InventoryList.AddUnEquippedItem(Item);
}

void UInventoryComponent::AddEntryToQuickSlot(int64 ItemID, const FGameplayTag& QuickSlotTag)
{
	InventoryList.AddEntryToQuickSlot(ItemID, QuickSlotTag);
}

void UInventoryComponent::RemoveEntryFromQuickSlot(int64 ItemID)
{
	InventoryList.RemoveEntryFromQuickSlot(ItemID);
}
