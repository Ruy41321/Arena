// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ItemTypes.h"
#include "Delegates/DelegateCombinations.h"
#include "Equipment/EquipmentTypes.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "InventoryComponent.generated.h"

class UInventoryComponent;
class UItemTypesToTables;
class UEquipmentStatEffects;

DECLARE_MULTICAST_DELEGATE_TwoParams(FEquipmentItemUsed, const TSubclassOf<UEquipmentDefinition>& /*Equipment Definition*/, const TArray<FEquipmentStatEffectGroup>& /* Status Effects*/);

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
	TArray<FEquipmentStatEffectGroup> StatEffects = TArray<FEquipmentStatEffectGroup>();

	bool IsValid() const
	{
		return ItemID != 0;
	}

};

DECLARE_MULTICAST_DELEGATE_OneParam(FDirtyInventoryItemSignature, const FRPGInventoryEntry& /*DirtyEntry*/);

USTRUCT()
struct FRPGInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	FRPGInventoryList() : OwnerComponent(nullptr) {}
	FRPGInventoryList(UInventoryComponent* InComponent) : OwnerComponent(InComponent) {}

	void AddItem(const FGameplayTag& ItemTag, int32 NumItems = 1);
	void RollForStats(const TSubclassOf<UEquipmentDefinition>& EquipmentDef, FRPGInventoryEntry* Entry);
	void RemoveItem(const FRPGInventoryEntry& InventoryEntry, int32 NumItems = 1);
	bool HasEnough(uint64 ItemID, int32 NumItems);
	uint64 GenerateID();
	void SetStats(UEquipmentStatEffects* InStats);
	void AddUnEquippedItem(const FGameplayTag& ItemTag, const TArray<FEquipmentStatEffectGroup>& InStatEffects);

	// FFastArraySerializer Contract
	void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);
		
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FRPGInventoryEntry, FRPGInventoryList>(Entries, DeltaParms, *this);
	}

	FDirtyInventoryItemSignature DirtyItemDelegate;

private:

	friend class UInventoryComponent;

	UPROPERTY()
	TArray<FRPGInventoryEntry> Entries;

	UPROPERTY(NotReplicated)
	TObjectPtr<UInventoryComponent> OwnerComponent;

	UPROPERTY(NotReplicated)
	uint64 LastAssignedID = 0;

	UPROPERTY(NotReplicated)
	TWeakObjectPtr<UEquipmentStatEffects> WeakStatsData;

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

	UInventoryComponent();

	UPROPERTY(Replicated)
	FRPGInventoryList InventoryList;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	void AddItem(const FGameplayTag& ItemTag, int32 NumItems = 1);

	UFUNCTION(BlueprintCallable)
	void UseItem(const FRPGInventoryEntry& Entry, int32 NumItems = 1);

	UFUNCTION(BlueprintPure)
	FMasterItemDefinition GetItemDefinitionByTag(const FGameplayTag& ItemTag) const;

	TArray<FRPGInventoryEntry> GetInventoryEntries();

	void AddUnEquippedItemEntry(const FGameplayTag& ItemTag, const TArray<FEquipmentStatEffectGroup>& InStatEffects);
	
protected:

	virtual void BeginPlay() override;

private:

	UPROPERTY(EditDefaultsOnly, category = "Custom Values | Stat Effects")
	TObjectPtr<UEquipmentStatEffects> StatEffectsData;

	UPROPERTY(EditDefaultsOnly, category = "Custom Values | Item Definitions")
	TObjectPtr<UItemTypesToTables> InventoryDefinitions;

	UFUNCTION(Server, Reliable)
	void ServerAddItem(const FGameplayTag& ItemTag, int32 NumItems);

	UFUNCTION(Server, Reliable)
	void ServerUseItem(const FRPGInventoryEntry& Entry, int32 NumItems);

};