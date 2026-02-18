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

	UPROPERTY(NotReplicated)
	FEquipmentGrantedHandles GrantedHandles = FEquipmentGrantedHandles();

private:

	friend UEquipmentManagerComponent;
	friend struct FRPGEquipmentList;

	UPROPERTY()
	TSubclassOf<UEquipmentDefinition> EquipmentDefinition = nullptr;

	UPROPERTY()
	TObjectPtr<UEquipmentInstance> Instance = nullptr;

};

DECLARE_MULTICAST_DELEGATE_OneParam(FEquipmentEntrySignature, const FRPGEquipmentEntry& /*Equipment Entry*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FEquipmentUnEquipSignature, const FGameplayTag& /*Equipment Entry*/);

USTRUCT()
struct FRPGEquipmentList : public FFastArraySerializer
{
	GENERATED_BODY()

	FRPGEquipmentList() : OwnerComponent() {}
	FRPGEquipmentList(UActorComponent* InComponent) : OwnerComponent(InComponent) {}

	UEquipmentInstance* AddEntry(const TSubclassOf<UEquipmentDefinition>& EquipmentDefinition);
	void RemoveEntry(UEquipmentInstance* EquipmentInstance);

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
	FEquipmentUnEquipSignature UnEquipDelegate;

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

	void EquipItem(const TSubclassOf<UEquipmentDefinition>& EquipmentDefinition);
	void UnEquipItem(UEquipmentInstance* EquipmentInstance);

private:

	UFUNCTION(Server, Reliable)
	void ServerEquipItem(TSubclassOf<UEquipmentDefinition> EquipmentDefinition);

	UFUNCTION(Server, Reliable)
	void ServerUnEquipItem(UEquipmentInstance* EquipmentInstance);
		
};
