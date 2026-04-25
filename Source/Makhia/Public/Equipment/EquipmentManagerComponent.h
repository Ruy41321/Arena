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
class UMKHAbilitySystemComponent;
class UInventoryItem;

USTRUCT(BlueprintType)
struct FRPGEquipmentEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	/** Unique gameplay tag of the equipped item. */
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag EntryTag = FGameplayTag();

	/** Equipment slot occupied by this entry (for example weapon or armor). */
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag SlotTag = FGameplayTag();
	
	/** Item rarity used by UI and gameplay systems. */
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag RarityTag = FGameplayTag();

	/** Package of effects and abilities granted while the item is equipped. */
	UPROPERTY(BlueprintReadOnly)
	FEquipmentEffectPackage EffectPackage = FEquipmentEffectPackage();

	/** Original ItemID assigned when the item was first created in the inventory. Preserved across equip/unequip cycles. */
	UPROPERTY(BlueprintReadOnly)
	int64 OriginalItemID = 0;

	/** Runtime handles granted by GAS for clean removal during unequip. */
	UPROPERTY(NotReplicated)
	FEquipmentGrantedHandles GrantedHandles = FEquipmentGrantedHandles();

	/** Returns whether this entry contains stat effects to apply. */
	bool HasStats() const { return !EffectPackage.StatEffects.IsEmpty(); }

	/** Returns whether this entry contains abilities to grant. */
	bool HasAbility() const { return !EffectPackage.Abilities.IsEmpty(); }

private:

	friend UEquipmentManagerComponent;
	friend struct FRPGEquipmentList;

	/** Equipment definition class used to resolve CDO data and instance type. */
	UPROPERTY()
	TSubclassOf<UEquipmentDefinition> EquipmentDefinition = nullptr;

	/** Non-replicated runtime instance managing spawned actors and local callbacks. */
	UPROPERTY(NotReplicated)
	TObjectPtr<UEquipmentInstance> Instance = nullptr;

};

DECLARE_MULTICAST_DELEGATE_OneParam(FEquipmentEntrySignature, const FRPGEquipmentEntry& /*Equipment Entry*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUnEquippedEntrySignature, const FRPGEquipmentEntry& /*UnEquipment Entry*/);

USTRUCT()
struct FRPGEquipmentList : public FFastArraySerializer
{
	GENERATED_BODY()

	/** Default constructor required by Unreal serialization. */
	FRPGEquipmentList() : OwnerComponent() {}
	/** Constructor with owner for authority checks and ASC lookup. */
	FRPGEquipmentList(UActorComponent* InComponent) : OwnerComponent(InComponent) {}

	/** Gets the owner's Ability System Component. */
	UMKHAbilitySystemComponent* GetAbilitySystemComponent();
	/** Applies this entry's stat effects to the ASC. */
	void AddEquipmentStats(FRPGEquipmentEntry* Entry);
	/** Removes this entry's stat effects from the ASC. */
	void RemoveEquipmentStats(FRPGEquipmentEntry* Entry);
	/** Grants to the ASC the abilities defined in this entry. */
	void AddEquipmentAbility(FRPGEquipmentEntry* Entry);
	/** Revokes from the ASC the abilities granted by this entry. */
	void RemoveEquipmentAbility(FRPGEquipmentEntry* Entry);

	/** Adds a new equipped entry, handling slot replacement and side effects. */
	UEquipmentInstance* AddEntry(const FRPGEquipmentEntry& InEntry);
	/** Removes the entry currently occupying the provided slot. */
	void RemoveEntryBySlot(const FGameplayTag& SlotTag);

	/** Finds a mutable entry by original ItemID and slot. */
	FRPGEquipmentEntry* FindEntryMutable(int64 OriginalItemID, const FGameplayTag& SlotTag);
	/** Finds a const entry by original ItemID and slot. */
	const FRPGEquipmentEntry* FindEntry(int64 OriginalItemID, const FGameplayTag& SlotTag) const;
	/** Finds a const entry by original ItemID. */
	const FRPGEquipmentEntry* FindEntryByItemID(int64 OriginalItemID) const;
	/** Finds a const entry by slot. */
	const FRPGEquipmentEntry* FindEntryBySlot(const FGameplayTag& SlotTag) const;
	/** Finds a mutable entry by slot. */
	FRPGEquipmentEntry* FindEntryBySlotMutable(const FGameplayTag& SlotTag);

	/** Returns a copy of the current replicated entries. */
	void GetEntries(TArray<FRPGEquipmentEntry>& OutEntries) const { OutEntries = Entries; }

	// FFastArraySerializer Contract
	/** Handles replicated removals on clients. */
	void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);
	/** Handles replicated additions on clients. */
	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
	/** Handles replicated changes on clients. */
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);

	/** FastArray delta serialization entry point. */
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& Parms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FRPGEquipmentEntry, FRPGEquipmentList>(Entries, Parms, *this);
	}

	/** Delegate broadcast when an entry is equipped or updated. */
	FEquipmentEntrySignature EquipmentEntryDelegate;
	/** Delegate broadcast when an entry is unequipped. */
	FOnUnEquippedEntrySignature UnEquippedEntryDelegate;

private:
	/** Applies gameplay side effects (stats and abilities) for a valid entry. */
	void ApplyEntryEffects(FRPGEquipmentEntry& Entry);
	/** Removes gameplay side effects (stats and abilities) for a valid entry. */
	void RemoveEntryEffects(FRPGEquipmentEntry& Entry);

	/** Replicated container of equipped entries. */
	UPROPERTY()
	TArray<FRPGEquipmentEntry> Entries;

	/** Serializer owner used for authority checks and gameplay services. */
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
class MAKHIA_API UEquipmentManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	/** Equipment manager component constructor. */
	UEquipmentManagerComponent();

	/** Replicated equipment state backed by FastArraySerializer. */
	UPROPERTY(replicated)
	FRPGEquipmentList EquipmentList;

	/** Debug utility that prints equipped entries on screen. */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Debug")
	void PrintEquipmentList() const;

	/** Registers the component replicated properties. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Equips an entry, forwarding to server when called by a client. */
	void EquipItem(const FRPGEquipmentEntry& InEntry);

	/** Builds a replication-safe FRPGEquipmentEntry from a UInventoryItem. */
	static FRPGEquipmentEntry BuildEquipmentEntry(const UInventoryItem* InventoryItem);
	
	/** Unequips by original ItemID by resolving its target slot. */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Operations")
	void UnEquipItemByItemID(int64 ItemID);
	
	/** Unequips the entry currently in the given slot. */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Operations")
	void UnEquipItem(const FGameplayTag& SlotTag);
		
	/** Returns the slot associated with an ItemID, or EmptyTag when missing. */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Queries")
	const FGameplayTag& GetSlotTagByItemID(int64 ItemID) const;
	
	/** Returns the equipment instance associated with the slot, if any. */
	UEquipmentInstance* GetEquipmentInstanceBySlot(const FGameplayTag& SlotTag) const;
	
	/** Returns the equipment entry associated with the slot, if any. */
	FRPGEquipmentEntry* GetEquipmentEntryBySlot(const FGameplayTag& SlotTag) const;

private:

	/** Reliable server RPC for equip requests coming from clients. */
	UFUNCTION(Server, Reliable)
	void ServerEquipItem(FRPGEquipmentEntry InEntry);

	/** Reliable server RPC for unequip requests coming from clients. */
	UFUNCTION(Server, Reliable)
	void ServerUnEquipItem(FGameplayTag SlotTag);
		
};
