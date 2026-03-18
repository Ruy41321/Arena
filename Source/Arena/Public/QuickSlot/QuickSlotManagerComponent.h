// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "QuickSlotManagerComponent.generated.h"

DECLARE_DELEGATE_OneParam(FQuickSlotActivatedSignature, const FGameplayTag& /*QuickSlotTag*/);

class URPGSystemInputComponent;
class URPGInputConfig;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UQuickSlotManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	
	FQuickSlotActivatedSignature QuickSlotActivatedDelegate;
	
	UQuickSlotManagerComponent();

	void SetupInputActions(URPGInputConfig* InputConfig, URPGSystemInputComponent* InputComponent);
	
	void AddQuickSlot(const FGameplayTag& QuickSlotTag, int64 ItemID);
	
	void RemoveQuickSlot(int64 ItemID);
	
	void UseQuickSlot(const FGameplayTag& QuickSlotTag);
	
	int64 GetQuickSlotID(const FGameplayTag& QuickSlotTag) const;

	/**
	 * Used to fast equip weapon when attacking while they are sheathed based on the last used weapon
	 * @return 
	 */
	bool TryEquipWeapon();
	
private:
	
	FGameplayTag QuickEquipWeaponSlotTag;
	
	TMap<FGameplayTag, int64> QuickSlotTagMap;
	
	bool bCanTriggerActivation = true;
};
