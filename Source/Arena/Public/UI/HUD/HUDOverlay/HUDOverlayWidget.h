// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/RPGSystemWidget.h"
#include "HUDOverlayWidget.generated.h"

class UInventoryItem;
/**
 * 
 */
UCLASS()
class ARENA_API UHUDOverlayWidget : public URPGSystemWidget
{
	GENERATED_BODY()
	
public:
		
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnItemQuickSlotted(UInventoryItem* NewItem);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnItemChanged(UInventoryItem* NewItem);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnItemRemoved(const int64 RemovedItemID);
	
};
