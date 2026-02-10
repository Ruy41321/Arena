// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemTypesToTables.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Libraries/RPGAbilitySystemLibrary.h"
#include "Net/UnrealNetwork.h"

bool FPackagedInventory::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	SafeNetSerializeTArray_WithNetSerialize<100>(Ar, ItemTags, Map);
	SafeNetSerializeTArray_Default<100>(Ar, ItemQuantities);

	bOutSuccess = true;
	return true;
}

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, CachedInventory);
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

	// Add the item to the inventory map
	if (InventoryTagMap.Contains(ItemTag))
	{
		InventoryTagMap[ItemTag] += NumItems;
		// If is zero delete the entry from the map
		if (InventoryTagMap[ItemTag] <= 0)
			InventoryTagMap.Remove(ItemTag);
	}
	else
	{
		InventoryTagMap.Emplace(ItemTag, NumItems);
	}

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
		FString::Printf(TEXT("Added %d of %s to inventory"), NumItems, *ItemTag.ToString()));

	// Update the cached inventory for replication
	PackageInventory(CachedInventory);
	InventoryPackagedDelegate.Broadcast(CachedInventory);
}

void UInventoryComponent::ServerAddItem_Implementation(const FGameplayTag& ItemTag, int32 NumItems)
{
	AddItem(ItemTag, NumItems);
}

void UInventoryComponent::PackageInventory(FPackagedInventory& OutInventory) const
{
	// Clear
	OutInventory.ItemTags.Empty();
	OutInventory.ItemQuantities.Empty();

	//Fill the struct packageinventory with the data from the map
	for (const auto& Pair : InventoryTagMap)
	{
		OutInventory.ItemTags.Add(Pair.Key);
		OutInventory.ItemQuantities.Add(Pair.Value);
	}
}

void UInventoryComponent::ReconstructInventoryMap(const FPackagedInventory& InInventory)
{
	InventoryTagMap.Empty();
	// Fill the map with the data from the struct packageinventory replicated from the server
	for (int32 i = 0; i < InInventory.ItemTags.Num(); ++i)
	{
		InventoryTagMap.Emplace(InInventory.ItemTags[i], InInventory.ItemQuantities[i]);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue,
			FString::Printf(TEXT("Reconstuct %d of %s to inventory"), InInventory.ItemQuantities[i], *InInventory.ItemTags[i].ToString()));
	}
}

void UInventoryComponent::OnRep_CachedInventory()
{
	ReconstructInventoryMap(CachedInventory);
	InventoryPackagedDelegate.Broadcast(CachedInventory);
}

void UInventoryComponent::UseItem(const FGameplayTag& ItemTag, int32 NumItems)
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
		return;

	// Check if the item is in the inventory and if we have enough quantity to use
	if (!InventoryTagMap.Contains(ItemTag) || InventoryTagMap[ItemTag] < NumItems)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
			FString::Printf(TEXT("Not enough %s in inventory to use"), *ItemTag.ToString()));
		return;
	}
	
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

			AddItem(ItemTag, -1);

			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta,
				FString::Printf(TEXT("Server Used %d of %s from inventory"), NumItems, *Item.ItemName.ToString()));
		}
	}
}

void UInventoryComponent::ServerUseItem_Implementation(const FGameplayTag& ItemTag, int32 NumItems)
{
	UseItem(ItemTag, NumItems);
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

TMap<FGameplayTag, int32> UInventoryComponent::GetInventoryTagMap() const
{
	return InventoryTagMap;
}
