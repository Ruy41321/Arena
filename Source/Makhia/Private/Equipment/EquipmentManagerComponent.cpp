// Fill out your copyright notice in the Description page of Project Settings.


#include "Equipment/EquipmentManagerComponent.h"

#include "Equipment/EquipmentDefinition.h"
#include "Equipment/EquipmentInstance.h"
#include "Inventory/InventoryItem/InventoryItem.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystem/MKHAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

UMKHAbilitySystemComponent* FRPGEquipmentList::GetAbilitySystemComponent()
{
	check(OwnerComponent);
	check(OwnerComponent->GetOwner());

	return Cast<UMKHAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerComponent->GetOwner()));
}

void FRPGEquipmentList::ApplyEntryEffects(FRPGEquipmentEntry& Entry)
{
	if (Entry.HasStats())
	{
		AddEquipmentStats(&Entry);
	}

	if (Entry.HasAbility())
	{
		AddEquipmentAbility(&Entry);
	}
}

void FRPGEquipmentList::RemoveEntryEffects(FRPGEquipmentEntry& Entry)
{
	RemoveEquipmentStats(&Entry);
	RemoveEquipmentAbility(&Entry);
}

void FRPGEquipmentList::AddEquipmentStats(FRPGEquipmentEntry* Entry)
{
	if (UMKHAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->AddEquipmentEffects(Entry);
	}
}

void FRPGEquipmentList::RemoveEquipmentStats(FRPGEquipmentEntry* Entry)
{
	if (UMKHAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->RemoveEquipmentEffects(Entry);
	}
}

void FRPGEquipmentList::AddEquipmentAbility(FRPGEquipmentEntry* Entry)
{
	if (UMKHAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->AddEquipmentAbility(Entry);
	}
}

void FRPGEquipmentList::RemoveEquipmentAbility(FRPGEquipmentEntry* Entry)
{
	if (UMKHAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->RemoveEquipmentAbility(Entry);
	}
}

UEquipmentInstance* FRPGEquipmentList::AddEntry(const FRPGEquipmentEntry& InEntry)
{
	check(InEntry.EquipmentDefinition);
	check(OwnerComponent);
	check(OwnerComponent->GetOwner()->HasAuthority());

	const UEquipmentDefinition* EquipmentCDO = GetDefault<UEquipmentDefinition>(InEntry.EquipmentDefinition);
	TSubclassOf<UEquipmentInstance> InstanceType = EquipmentCDO->InstanceType;

	if (!IsValid(InstanceType))
	{
		InstanceType = UEquipmentInstance::StaticClass();
	}

	if (FindEntryBySlot(EquipmentCDO->SlotTag) != nullptr)
	{
		// If the slot is already occupied, remove the existing entry before adding the new one.
		RemoveEntryBySlot(EquipmentCDO->SlotTag);
	}

	FRPGEquipmentEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.EntryTag = InEntry.EntryTag;
	NewEntry.RarityTag = InEntry.RarityTag;
	NewEntry.SlotTag = EquipmentCDO->SlotTag;
	NewEntry.EquipmentDefinition = InEntry.EquipmentDefinition;
	NewEntry.EffectPackage = InEntry.EffectPackage;
	NewEntry.OriginalItemID = InEntry.OriginalItemID;
	NewEntry.Instance = NewObject<UEquipmentInstance>(OwnerComponent->GetOwner(), InstanceType);

	ApplyEntryEffects(NewEntry);

	if (IsValid(NewEntry.Instance))
	{
		NewEntry.Instance->SpawnEquipmentActors(EquipmentCDO->ActorsToSpawn, EquipmentCDO->BaseDamage);
	}

	MarkItemDirty(NewEntry);
	EquipmentEntryDelegate.Broadcast(NewEntry);

	return NewEntry.Instance;
}

void FRPGEquipmentList::RemoveEntryBySlot(const FGameplayTag& SlotTag)
{
	check(OwnerComponent);

	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FRPGEquipmentEntry& Entry = *EntryIt;
		if (Entry.SlotTag.MatchesTagExact(SlotTag))
		{
			RemoveEntryEffects(Entry);

			if (IsValid(Entry.Instance))
			{
				Entry.Instance->DestroySpawnedActors();
			}

			// Broadcast the unequip event before removal so listeners can return the item to inventory.
			UnEquippedEntryDelegate.Broadcast(Entry);
			EntryIt.RemoveCurrent();
			MarkArrayDirty();
			break;
		}
	}
}

FRPGEquipmentEntry* FRPGEquipmentList::FindEntryMutable(int64 OriginalItemID, const FGameplayTag& SlotTag)
{
	return Entries.FindByPredicate([OriginalItemID, &SlotTag](FRPGEquipmentEntry& Entry)
	{
		return Entry.OriginalItemID == OriginalItemID && Entry.SlotTag.MatchesTagExact(SlotTag);
	});
}

const FRPGEquipmentEntry* FRPGEquipmentList::FindEntry(int64 OriginalItemID, const FGameplayTag& SlotTag) const
{
	return Entries.FindByPredicate([OriginalItemID, &SlotTag](const FRPGEquipmentEntry& Entry)
	{
		return Entry.OriginalItemID == OriginalItemID && Entry.SlotTag.MatchesTagExact(SlotTag);
	});
}

const FRPGEquipmentEntry* FRPGEquipmentList::FindEntryByItemID(int64 OriginalItemID) const
{
	return Entries.FindByPredicate([OriginalItemID](const FRPGEquipmentEntry& Entry)
	{
		return Entry.OriginalItemID == OriginalItemID;
	});
}

const FRPGEquipmentEntry* FRPGEquipmentList::FindEntryBySlot(const FGameplayTag& SlotTag) const
{
	return Entries.FindByPredicate([&SlotTag](const FRPGEquipmentEntry& Entry)
	{
		return Entry.SlotTag.MatchesTagExact(SlotTag);
	});
}

FRPGEquipmentEntry* FRPGEquipmentList::FindEntryBySlotMutable(const FGameplayTag& SlotTag)
{
	return Entries.FindByPredicate([&SlotTag](FRPGEquipmentEntry& Entry)
	{
		return Entry.SlotTag.MatchesTagExact(SlotTag);
	});
}

void FRPGEquipmentList::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
	for (const int32 Index : RemovedIndices)
	{
		UnEquippedEntryDelegate.Broadcast(Entries[Index]);

		// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
		// 	FString::Printf(TEXT("UnEquipped Item: %s"), *Entries[Index].EntryTag.ToString()));
	}
}

void FRPGEquipmentList::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	for (const int32 Index : AddedIndices)
	{
		EquipmentEntryDelegate.Broadcast(Entries[Index]);
	}
}

void FRPGEquipmentList::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
	for (const int32 Index : ChangedIndices)
	{
		EquipmentEntryDelegate.Broadcast(Entries[Index]);
	}
}

UEquipmentManagerComponent::UEquipmentManagerComponent() : EquipmentList(this)
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

}

void UEquipmentManagerComponent::PrintEquipmentList() const
{
	TArray<FRPGEquipmentEntry> Entries;
	EquipmentList.GetEntries(Entries);
	for (const FRPGEquipmentEntry& Entry : Entries)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
			FString::Printf(TEXT("Slot: %s, Item: %s"), *Entry.SlotTag.ToString(), *Entry.EntryTag.ToString()));
	}
}

void UEquipmentManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UEquipmentManagerComponent, EquipmentList);
}

FRPGEquipmentEntry UEquipmentManagerComponent::BuildEquipmentEntry(const UInventoryItem* InventoryItem)
{
	check(IsValid(InventoryItem));

	const TSubclassOf<UEquipmentDefinition> EquipDefClass = InventoryItem->ItemDefinition.EquipmentItemProps.EquipmentClass;
	check(EquipDefClass);

	// const UEquipmentDefinition* EquipCDO = GetDefault<UEquipmentDefinition>(EquipDefClass);

	FRPGEquipmentEntry Entry;
	Entry.EquipmentDefinition  = EquipDefClass;
	Entry.EntryTag             = InventoryItem->ItemTag;
	Entry.SlotTag              = InventoryItem->SlotTag;
	Entry.RarityTag            = InventoryItem->GetRarityTag();
	Entry.EffectPackage        = InventoryItem->GetEffectPackage();
	Entry.OriginalItemID       = InventoryItem->GetItemID();
	return Entry;
}

void UEquipmentManagerComponent::EquipItem(const FRPGEquipmentEntry& InEntry)
{
	if (!GetOwner()->HasAuthority())
	{
		ServerEquipItem(InEntry);
		return;
	}

	if (UEquipmentInstance* Result = EquipmentList.AddEntry(InEntry))
	{
		Result->OnEquipped();
	}
}

void UEquipmentManagerComponent::UnEquipItemByItemID(int64 ItemID)
{
	if (const FGameplayTag& Tag = GetSlotTagByItemID(ItemID); Tag.IsValid())
		UnEquipItem(Tag);
}

void UEquipmentManagerComponent::UnEquipItem(const FGameplayTag& SlotTag)
{
	if (!GetOwner()->HasAuthority())
	{
		ServerUnEquipItem(SlotTag);
		return;
	}

	// Call OnUnEquipped before removing the entry to ensure the event is broadcast with valid runtime data.
	const FRPGEquipmentEntry* FoundEntry = EquipmentList.FindEntryBySlot(SlotTag);

	if (!FoundEntry)
	{
		// Weapon sheathe with no equipped weapon is a valid no-op.
		// UE_LOG(LogTemp, Warning, TEXT("UEquipmentManagerComponent::UnEquipItem - No entry found for SlotTag: %s"), *SlotTag.ToString());
		return;
	}

	if (IsValid(FoundEntry->Instance))
	{
		FoundEntry->Instance->OnUnEquipped();
	}

	EquipmentList.RemoveEntryBySlot(SlotTag);
}

const FGameplayTag& UEquipmentManagerComponent::GetSlotTagByItemID(int64 ItemID) const
{
	if (const FRPGEquipmentEntry* FoundEntry = EquipmentList.FindEntryByItemID(ItemID))
	{
		return FoundEntry->SlotTag;
	}

	return FGameplayTag::EmptyTag;
}

UEquipmentInstance* UEquipmentManagerComponent::GetEquipmentInstanceBySlot(const FGameplayTag& SlotTag) const
{	
	const FRPGEquipmentEntry* FoundEntry = EquipmentList.FindEntryBySlot(SlotTag);
	return (FoundEntry) ? FoundEntry->Instance : nullptr;
}

FRPGEquipmentEntry* UEquipmentManagerComponent::GetEquipmentEntryBySlot(const FGameplayTag& SlotTag) const
{
	return const_cast<FRPGEquipmentEntry*>(EquipmentList.FindEntryBySlot(SlotTag));
}

void UEquipmentManagerComponent::ServerEquipItem_Implementation(FRPGEquipmentEntry InEntry)
{
	EquipItem(InEntry);
}

void UEquipmentManagerComponent::ServerUnEquipItem_Implementation(FGameplayTag SlotTag)
{
	UnEquipItem(SlotTag);
}