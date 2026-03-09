// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Equipment/EquipmentTypes.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "EquipmentManagerComponent.generated.h"

class UEquipmentManagerComponent;
class UEquipmentDefinition;
class UEquipmentInstance;
class URPGAbilitySystemComponent;
class UInventoryItem;

USTRUCT(BlueprintType)
struct FRPGEquipmentEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag EntryTag = FGameplayTag();

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag SlotTag = FGameplayTag();
	
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag RarityTag = FGameplayTag();

	UPROPERTY(BlueprintReadOnly)
	FEquipmentEffectPackage EffectPackage = FEquipmentEffectPackage();

	/** Original ItemID assigned when the item was first created in the inventory. Preserved across equip/unequip cycles. */
	UPROPERTY(BlueprintReadOnly)
	int64 OriginalItemID = 0;

	UPROPERTY(NotReplicated)
	FEquipmentGrantedHandles GrantedHandles = FEquipmentGrantedHandles();

	bool HasStats() const { return !EffectPackage.StatEffects.IsEmpty(); }

	bool HasAbility() const { return !EffectPackage.Abilities.IsEmpty(); }

private:

	friend UEquipmentManagerComponent;
	friend struct FRPGEquipmentList;

	UPROPERTY()
	TSubclassOf<UEquipmentDefinition> EquipmentDefinition = nullptr;

	UPROPERTY(NotReplicated)
	TObjectPtr<UEquipmentInstance> Instance = nullptr;

};

DECLARE_MULTICAST_DELEGATE_OneParam(FEquipmentEntrySignature, const FRPGEquipmentEntry& /*Equipment Entry*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUnEquippedEntrySignature, const FRPGEquipmentEntry& /*UnEquipment Entry*/);

USTRUCT()
struct FRPGEquipmentList : public FFastArraySerializer
{
	GENERATED_BODY()

	FRPGEquipmentList() : OwnerComponent() {}
	FRPGEquipmentList(UActorComponent* InComponent) : OwnerComponent(InComponent) {}

	URPGAbilitySystemComponent* GetAbilitySystemComponent();
	void AddEquipmentStats(FRPGEquipmentEntry* Entry);
	void RemoveEquipmentStats(FRPGEquipmentEntry* Entry);
	void AddEquipmentAbility(FRPGEquipmentEntry* Entry);
	void RemoveEquipmentAbility(FRPGEquipmentEntry* Entry);
	UEquipmentInstance* AddEntry(const FRPGEquipmentEntry& InEntry);
	void RemoveEntryBySlot(const FGameplayTag& SlotTag);

	void GetEntries(TArray<FRPGEquipmentEntry>& OutEntries) const { OutEntries = Entries; }

	// FFastArraySerializer Contract
	void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& Parms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FRPGEquipmentEntry, FRPGEquipmentList>(Entries, Parms, *this);
	}

	FEquipmentEntrySignature EquipmentEntryDelegate;
	FOnUnEquippedEntrySignature UnEquippedEntryDelegate;

private:

	UPROPERTY()
	TArray<FRPGEquipmentEntry> Entries;

	UPROPERTY()
	TObjectPtr<UActorComponent> OwnerComponent;

};

template<>
struct TStructOpsTypeTraits<FRPGEquipmentList> : public TStructOpsTypeTraitsBase2<FRPGEquipmentList>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ARENA_API UEquipmentManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	
	UEquipmentManagerComponent();

	UPROPERTY(replicated)
	FRPGEquipmentList EquipmentList;

	UFUNCTION(BlueprintCallable)
	void PrintEquipmentList() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void EquipItem(const FRPGEquipmentEntry& InEntry);

	/** Builds a replication-safe FRPGEquipmentEntry from a UInventoryItem. */
	static FRPGEquipmentEntry BuildEquipmentEntry(const UInventoryItem* InventoryItem);
	
	UFUNCTION(BlueprintCallable)
	void UnEquipItemByItemID(int64 ItemID);
	
	UFUNCTION(BlueprintCallable)
	void UnEquipItem(const FGameplayTag& SlotTag);
		
	UFUNCTION(BlueprintCallable)
	const FGameplayTag& GetSlotTagByItemID(int64 ItemID) const;

private:

	UFUNCTION(Server, Reliable)
	void ServerEquipItem(FRPGEquipmentEntry InEntry);

	UFUNCTION(Server, Reliable)
	void ServerUnEquipItem(FGameplayTag SlotTag);
		
};
