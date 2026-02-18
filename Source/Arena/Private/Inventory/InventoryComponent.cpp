// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemTypesToTables.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Libraries/RPGAbilitySystemLibrary.h"
#include "Net/UnrealNetwork.h"

void FRPGInventoryList::AddItem(const FGameplayTag& ItemTag, int32 NumItems)
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
				DirtyItemDelegate.Broadcast(Entry);
			}
			return;
		}
	}

	FRPGInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.ItemTag = ItemTag;
	NewEntry.Quantity = NumItems;

	MarkItemDirty(NewEntry);
	if (OwnerComponent->GetOwner()->HasAuthority())
	{
		DirtyItemDelegate.Broadcast(NewEntry);
	}
}

void FRPGInventoryList::RemoveItem(const FGameplayTag& ItemTag, int32 NumItems)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FRPGInventoryEntry& Entry = *EntryIt;

		if (Entry.ItemTag.MatchesTagExact(ItemTag))
		{
			Entry.Quantity = (Entry.Quantity - NumItems <= 0) ? 0 : (Entry.Quantity - NumItems);

			MarkItemDirty(Entry);

			if (OwnerComponent->GetOwner()->HasAuthority())
			{
				DirtyItemDelegate.Broadcast(Entry);
			}
			return;
		}
	}
}

bool FRPGInventoryList::HasEnough(const FGameplayTag& ItemTag, int32 NumItems)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FRPGInventoryEntry& Entry = *EntryIt;

		if (Entry.ItemTag.MatchesTagExact(ItemTag))
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

void FRPGInventoryList::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
	// Dont know that it is reliably good for
}

void FRPGInventoryList::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	for (const int32 Index : AddedIndices)
	{
		DirtyItemDelegate.Broadcast(Entries[Index]);
	}
}

void FRPGInventoryList::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
	for (const int32 Index : ChangedIndices)
	{
		DirtyItemDelegate.Broadcast(Entries[Index]);
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


void UInventoryComponent::UseItem(const FGameplayTag& ItemTag, int32 NumItems)
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
		return;
		
	if (!InventoryList.HasEnough(ItemTag, NumItems))
		return;

	if (!Owner->HasAuthority())
	{
		ServerUseItem(ItemTag, NumItems);
		return;
	}

	FMasterItemDefinition Item = GetItemDefinitionByTag(ItemTag);

	if (UAbilitySystemComponent* OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
	{
		if (IsValid(Item.ConsumableProps.ItemEffectClass))
		{
			const FGameplayEffectContextHandle EffectContext = OwnerASC->MakeEffectContext();
			const FGameplayEffectSpecHandle SpecHandle = OwnerASC->MakeOutgoingSpec(Item.ConsumableProps.ItemEffectClass, 
				Item.ConsumableProps.ItemEffectLevel, EffectContext);
			OwnerASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

			InventoryList.RemoveItem(ItemTag);

			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta,
				FString::Printf(TEXT("Server Used %d of %s from inventory"), NumItems, *Item.ItemName.ToString()));
		}
		
		if (IsValid(Item.EquipmentItemProps.EquipmentClass))
		{
			EquipmentItemUsedDelegate.Broadcast(Item.EquipmentItemProps.EquipmentClass);
			InventoryList.RemoveItem(ItemTag);
		}
	}
}

void UInventoryComponent::ServerUseItem_Implementation(const FGameplayTag& ItemTag, int32 NumItems)
{
	if (InventoryList.HasEnough(ItemTag, NumItems))
	{
		UseItem(ItemTag, NumItems);
	}
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

TArray<FRPGInventoryEntry> UInventoryComponent::GetInventoryEntries()
{
	return InventoryList.Entries;
}
