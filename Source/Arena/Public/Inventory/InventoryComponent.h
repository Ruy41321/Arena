// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ItemTypes.h"
#include "Delegates/DelegateCombinations.h"
#include "Equipment/EquipmentTypes.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "InventoryComponent.generated.h"

class UInventoryItem;
class UInventoryComponent;
class UItemTypesToTables;
class UEquipmentStatEffects;
struct FStreamableHandle;

DECLARE_MULTICAST_DELEGATE_OneParam(FEquipmentItemUsed, UInventoryItem* /*Inventory Item*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FEquipmentItemUnequipped, int64 /*ItemIDToRemove*/);

USTRUCT(BlueprintType)
struct FRPGInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag ItemTag = FGameplayTag();

	UPROPERTY(BlueprintReadOnly)
	FText ItemName = FText();

	UPROPERTY(BlueprintReadOnly)
	int32 Quantity = 0;

	UPROPERTY(BlueprintReadOnly)
	int64 ItemID = 0;

	UPROPERTY(BlueprintReadOnly)
	FEquipmentEffectPackage EffectPackage = FEquipmentEffectPackage();

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag RarityTag = FGameplayTag();

	UPROPERTY(BlueprintReadOnly, NotReplicated)
	bool bIsQuickSlotted = false;
	
	// Used to track if quickslotted weapons are currently in use 
	UPROPERTY(BlueprintReadOnly)
	bool bIsUsed = false;
	
	UPROPERTY(BlueprintReadOnly, NotReplicated)
	FGameplayTag QuickSlotTag = FGameplayTag();
	
	bool IsValid() const
	{
		return ItemID != 0;
	}
};

DECLARE_MULTICAST_DELEGATE_OneParam(FInventoryItemChangedSignature, const FRPGInventoryEntry& /*DirtyEntry*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FInventoryItemRemovedSignature, const int64 /*RemovedItemID*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FInventoryQuickSlotRelocatedSignature, const FRPGInventoryEntry& /*EntryToRelocate*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FInventoryQuickSlotChangedSignature, const FRPGInventoryEntry& /*QuickSlotEntryToUpdate*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FInventoryQuickSlotRemovedSignature, const int64 /*QuickSlotEntryIDToRemove*/);

USTRUCT()
struct FRPGInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	FRPGInventoryList() : OwnerComponent(nullptr) {}
	FRPGInventoryList(UInventoryComponent* InComponent) : OwnerComponent(InComponent) {}

	void AddItem(const FGameplayTag& ItemTag, int32 NumItems = 1);
	void RemoveItem(const FRPGInventoryEntry& InventoryEntry, int32 NumItems = 1);
	bool HasEnough(int64 ItemID, int32 NumItems);
	int64 GenerateID();
	void SetStats(UEquipmentStatEffects* InStats);
	void SetRarityTable(UDataTable* InRarityTable);
	void AddUnEquippedItem(UInventoryItem* Item);
	
	FRPGInventoryEntry* GetAlreadyQuickSlottedEntry(const FGameplayTag& QuickSlotTag);
	void AddEntryToQuickSlot(int64 ItemID, const FGameplayTag& QuickSlotTag);
	void RemoveEntryFromQuickSlot(const int64 ItemID);

	/** Finds and returns a pointer to the entry with the given ID, or nullptr if not found. */
	FRPGInventoryEntry* FindEntryByID(int64 ItemID);

	// FFastArraySerializer Contract
	void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);
		
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FRPGInventoryEntry, FRPGInventoryList>(Entries, DeltaParms, *this);
	}

	FInventoryItemChangedSignature InventoryItemChangedDelegate;
	FInventoryItemRemovedSignature InventoryItemRemovedDelegate;
	FInventoryQuickSlotRelocatedSignature QuickSlotItemRelocatedDelegate;
	FInventoryQuickSlotChangedSignature QuickSlotItemChangeDelegate;
	FInventoryQuickSlotRemovedSignature QuickSlotItemRemovedDelegate;

private:

	friend class UInventoryComponent;

	UPROPERTY()
	TArray<FRPGInventoryEntry> Entries;

	UPROPERTY(NotReplicated)
	TObjectPtr<UInventoryComponent> OwnerComponent;

	UPROPERTY(NotReplicated)
	int64 LastAssignedID = 0;

	UPROPERTY(NotReplicated)
	TWeakObjectPtr<UEquipmentStatEffects> WeakStatsData;

	UPROPERTY(NotReplicated)
	TWeakObjectPtr<UDataTable> WeakRarityTable;

	/** Tries to find an existing stackable entry and increment its quantity. Returns true if stacked. */
	bool TryStackItem(const FGameplayTag& ItemTag, int32 NumItems);

	/** Fills the equipment-specific fields of NewEntry (rarity, stats, abilities). */
	void RollEquipmentEntry(FRPGInventoryEntry& NewEntry, const FMasterItemDefinition& ItemDef);

	/** Marks an entry dirty and broadcasts it if running with authority. */
	void BroadcastNewEntry(FRPGInventoryEntry& NewEntry);

	/**
	 * Dispatches the appropriate delegate based on the entry's QuickSlot state and the operation type.
	 * @param Entry    The inventory entry involved in the change or removal.
	 * @param bChanged If true, broadcasts a "changed/updated" delegate; if false, broadcasts a "removed" delegate.
	 */
	void BroadcastEntryUpdate(const FRPGInventoryEntry& Entry, bool bChanged);

};

template<>
struct TStructOpsTypeTraits<FRPGInventoryList> : public TStructOpsTypeTraitsBase2<FRPGInventoryList>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ARENA_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	FEquipmentItemUsed EquipmentItemUsedDelegate;
	FEquipmentItemUnequipped EquipmentItemUnequippedDelegate;

	UInventoryComponent();

	UPROPERTY(Replicated)
	FRPGInventoryList InventoryList;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	void AddItem(const FGameplayTag& ItemTag, int32 NumItems = 1);

	UFUNCTION(BlueprintCallable)
	void UseItem(int64 ItemID, int32 NumItems = 1);

	UFUNCTION(BlueprintPure)
	FMasterItemDefinition GetItemDefinitionByTag(const FGameplayTag& ItemTag) const;

	/** Returns a copy of the entry with the given ID. Check IsValid() on the result. */
	UFUNCTION(BlueprintPure)
	FRPGInventoryEntry FindInventoryEntryByID(int64 ItemID);

	TArray<FRPGInventoryEntry> GetInventoryEntries();

	void AddUnEquippedItemEntry(UInventoryItem* Item);
	
	UFUNCTION(BlueprintCallable)
	void AddEntryToQuickSlot(int64 ItemID, const FGameplayTag& QuickSlotTag);
	
	UFUNCTION(BlueprintCallable)
	void RemoveEntryFromQuickSlot(int64 ItemID);

	/**
	 * This Function Preload the asset of the weapon that is being quickslotted,
	 * so if the players attacks while the weapon is sheathed the attack starts directly
	 * @param Entry 
	 */
	void PreloadItem(FRPGInventoryEntry* Entry);

	void RemovePreloadedItemRef(int64 ItemID);
	
protected:

	virtual void BeginPlay() override;

private:

	UPROPERTY(EditDefaultsOnly, category = "Custom Values | Stat Effects")
	TObjectPtr<UEquipmentStatEffects> StatEffectsData;

	UPROPERTY(EditDefaultsOnly, category = "Custom Values | Rarity")
	TObjectPtr<UDataTable> RarityTable;

	UPROPERTY(EditDefaultsOnly, category = "Custom Values | Item Definitions")
	TObjectPtr<UItemTypesToTables> InventoryDefinitions;

	UFUNCTION(Server, Reliable)
	void ServerAddItem(const FGameplayTag& ItemTag, int32 NumItems);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerUseItem(int64 ItemID, int32 NumItems);

	bool ServerUseItem_Validate(int64 ItemID, int32 NumItems);

	TMap<int64, TArray<TSharedPtr<FStreamableHandle>>> PreloadedItemHandles;

};
