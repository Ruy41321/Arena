// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/MKHSystemWidget.h"
#include "GameplayTagContainer.h"
#include "HUDOverlayWidget.generated.h"

class UInventoryItem;
/**
 * 
 */
UCLASS()
class MAKHIA_API UHUDOverlayWidget : public UMKHSystemWidget
{
	GENERATED_BODY()
	
public:
		
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnItemQuickSlotted(UInventoryItem* NewItem);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnItemChanged(UInventoryItem* NewItem);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnItemRemoved(const int64 RemovedItemID);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnQuickSlotActivated(const FGameplayTag& QuickSlotTag);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnWeaponEquipped(const int64 EquippedWeaponID);
	
};
