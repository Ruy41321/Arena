// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/RPGSystemWidget.h"
#include "InventoryDashboardWidget.generated.h"

struct FGameplayTag;
class UInventoryItem;
DECLARE_MULTICAST_DELEGATE(FOnOpenInventoryDashboardSignature);
DECLARE_MULTICAST_DELEGATE(FOnCloseInventoryDashboardSignature);
DECLARE_MULTICAST_DELEGATE(FOnDestructionInventoryDashboardSignature);

struct FRPGInventoryEntry;
struct FRPGEquipmentEntry;
/**
 * 
 */
UCLASS()
class ARENA_API UInventoryDashboardWidget : public URPGSystemWidget
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsOpen = true;
	
	FOnOpenInventoryDashboardSignature OnOpenInventoryDashboardDelegate;
	FOnCloseInventoryDashboardSignature OnCloseInventoryDashboardDelegate;
	FOnDestructionInventoryDashboardSignature OnDestructionInventoryDashboardDelegate;
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnInventoryItemChanged(UInventoryItem* NewItem);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnInventoryItemRemoved(const int64 RemovedItemID);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnItemEquipped(UInventoryItem* EquipItem);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnItemUnequipped(const FGameplayTag& SlotTag);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnQuickSlotItemRelocated(UInventoryItem* QuickSlottedItem);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnQuickSlotItemChanged(UInventoryItem* QuickSlottedItem);
		
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnQuickSlotItemRemoved(const int64 RemovedItemID);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OpenInventoryDashboard();
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void DestroyInventoryDashboard();
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void CloseInventoryDashboard();
		
	UFUNCTION(BlueprintCallable)
	void CallOnOpenInventoryDashboardDelegate() const
	{
		OnOpenInventoryDashboardDelegate.Broadcast();
	}
	
	UFUNCTION(BlueprintCallable)
	void CallOnDestructionDelegate() const
	{
		OnDestructionInventoryDashboardDelegate.Broadcast();
	}
	
	UFUNCTION(BlueprintCallable)
	void CallOnCloseInventoryDelegate() const
	{
		OnCloseInventoryDashboardDelegate.Broadcast();
	}
	
	void UnbindAllEventsFromDelegates();
};
