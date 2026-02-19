// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemTypesToTables.h"
#include "Data/EquipmentStatEffects.h"
#include "AbilitySystemComponent.h"
#include "NativeGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Libraries/RPGAbilitySystemLibrary.h"
#include "Net/UnrealNetwork.h"

namespace RPGGameplayTags::Static
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Category_Equipment, "Item.Equipment");
}

void FRPGInventoryList::AddItem(const FGameplayTag& ItemTag, int32 NumItems)
{
	if (!(ItemTag.MatchesTag(RPGGameplayTags::Static::Category_Equipment)))
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
	}

	const FMasterItemDefinition ItemDef = OwnerComponent->GetItemDefinitionByTag(ItemTag);

	FRPGInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.ItemTag = ItemTag;
	NewEntry.ItemName = ItemDef.ItemName;
	NewEntry.Quantity = NumItems;
	NewEntry.ItemID = GenerateID();

	if (NewEntry.ItemTag.MatchesTag(RPGGameplayTags::Static::Category_Equipment) && IsValid(WeakStatsData.Get()))
	{
		RollForStats(ItemDef.EquipmentItemProps.EquipmentClass, &NewEntry);
	}

	MarkItemDirty(NewEntry);
	if (OwnerComponent->GetOwner()->HasAuthority())
	{
		DirtyItemDelegate.Broadcast(NewEntry);
	}
}

void FRPGInventoryList::RollForStats(const TSubclassOf<UEquipmentDefinition>& EquipmentDef, FRPGInventoryEntry* Entry)
{
	UEquipmentStatEffects* StatEffectsData = WeakStatsData.Get();
	const UEquipmentDefinition* EquipmentCDO = GetDefault<UEquipmentDefinition>(EquipmentDef);

	const int32 NumStatsToRoll = FMath::RandRange(EquipmentCDO->MinPossibleStats, EquipmentCDO->MaxPossibleStats);
	int32 StatRollIndex = -1;
	while (++StatRollIndex < NumStatsToRoll)
	{
		const int32 RandomIndex = FMath::RandRange(0, EquipmentCDO->PossibleStatRolls.Num() - 1);
		const FGameplayTag& RandomTag = EquipmentCDO->PossibleStatRolls.GetByIndex(RandomIndex);

		for (const auto& Pair : StatEffectsData->MasterStatMap)
		{
			if (RandomTag.MatchesTag(Pair.Key))
			{
				if (const FEquipmentStatEffectGroup* PossibleStat = URPGAbilitySystemLibrary
					::GetDataTableRowByTag<FEquipmentStatEffectGroup>(Pair.Value, RandomTag))
				{
					if (FMath::FRandRange(0.f, 1.f) < PossibleStat->ProbabilityToSelect)
					{
						FEquipmentStatEffectGroup NewStat = *PossibleStat;

						NewStat.CurrentValue = PossibleStat->bFractionalStat ? FMath::FRandRange(PossibleStat->MinStatLevel, PossibleStat->MaxStatLevel) :
							FMath::TruncToInt(FMath::RandRange(PossibleStat->MinStatLevel, PossibleStat->MaxStatLevel));

						Entry->StatEffects.Add(NewStat);
					}
				}
			}
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

			/*if (Entry.Quantity <= 0)
			{
				EntryIt.RemoveCurrent();
			}*/

			MarkItemDirty(Entry);

			if (OwnerComponent->GetOwner()->HasAuthority())
			{
				DirtyItemDelegate.Broadcast(Entry);
			}
			break;
		}
	}
}

bool FRPGInventoryList::HasEnough(uint64 ItemID, int32 NumItems)
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

uint64 FRPGInventoryList::GenerateID()
{
	uint64 NewID = ++LastAssignedID;

	int32 SignatureIndex = 0;
	while (SignatureIndex < 12)
	{
		if (FMath::RandRange(0, 100) < 85)
		{
			NewID |= (uint64)1 << FMath::RandRange(0, 63);
		}
		++SignatureIndex;
	}

	return NewID;
}

void FRPGInventoryList::SetStats(UEquipmentStatEffects* InStats)
{
	WeakStatsData = InStats;
}

void FRPGInventoryList::AddUnEquippedItem(const FGameplayTag& ItemTag, const TArray<FEquipmentStatEffectGroup>& InStatEffects)
{
	const FMasterItemDefinition ItemDef = OwnerComponent->GetItemDefinitionByTag(ItemTag);

	FRPGInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.ItemTag = ItemTag;
	NewEntry.ItemName = ItemDef.ItemName;
	NewEntry.Quantity = 1;
	NewEntry.ItemID = GenerateID();
	NewEntry.StatEffects = InStatEffects;

	DirtyItemDelegate.Broadcast(NewEntry);
	MarkItemDirty(NewEntry);
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

	if (GetOwner()->HasAuthority())
	{
		InventoryList.SetStats(StatEffectsData);
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


void UInventoryComponent::UseItem(const FRPGInventoryEntry& Entry, int32 NumItems)
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
		return;
		
	if (!InventoryList.HasEnough(Entry.ItemID, NumItems))
		return;

	if (!Owner->HasAuthority())
	{
		ServerUseItem(Entry, NumItems);
		return;
	}

	FMasterItemDefinition Item = GetItemDefinitionByTag(Entry.ItemTag);

	if (UAbilitySystemComponent* OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
	{
		if (IsValid(Item.ConsumableProps.ItemEffectClass))
		{
			const FGameplayEffectContextHandle EffectContext = OwnerASC->MakeEffectContext();
			const FGameplayEffectSpecHandle SpecHandle = OwnerASC->MakeOutgoingSpec(Item.ConsumableProps.ItemEffectClass, 
				Item.ConsumableProps.ItemEffectLevel, EffectContext);
			OwnerASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

			InventoryList.RemoveItem(Entry);

			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta,
				FString::Printf(TEXT("Server Used %d of %s from inventory"), NumItems, *Item.ItemName.ToString()));
		}
		
		if (IsValid(Item.EquipmentItemProps.EquipmentClass))
		{
			EquipmentItemUsedDelegate.Broadcast(Item.EquipmentItemProps.EquipmentClass, Entry.StatEffects);
			InventoryList.RemoveItem(Entry);
		}
	}
}

void UInventoryComponent::ServerUseItem_Implementation(const FRPGInventoryEntry& Entry, int32 NumItems)
{
	UseItem(Entry, NumItems);
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

void UInventoryComponent::AddUnEquippedItemEntry(const FGameplayTag& ItemTag, const TArray<FEquipmentStatEffectGroup>& InStatEffects)
{
	InventoryList.AddUnEquippedItem(ItemTag, InStatEffects);
}
