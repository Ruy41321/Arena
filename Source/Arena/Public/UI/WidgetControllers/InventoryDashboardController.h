// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UI/WidgetControllers/WidgetController.h"
#include "InventoryDashboardController.generated.h"

class UQuickSlotManagerComponent;
struct FRPGInventoryEntry;
struct FRPGEquipmentEntry;
class UInventoryItem;
class UInventoryDashboardWidget;
class UInventoryComponent;
class UEquipmentManagerComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInventoryEntrySignature, UInventoryItem*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryItemRemoved, const int64, RemovedItemID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEquippedEntrySignature, UInventoryItem*, Item /*Equipped Item*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUnEquippedEntrySignature, const FGameplayTag&, SlotTag /*UnEquipped Slot*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInventoryItemQuickSlottedSignature, UInventoryItem*, Item /*Quick Slotted Item*/);
/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class ARENA_API UInventoryDashboardController : public UWidgetController
{
	GENERATED_BODY()
	
public:

	UPROPERTY(BlueprintAssignable)
	FInventoryEntrySignature InventoryEntryDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnInventoryItemRemoved InventoryItemRemovedDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FEquippedEntrySignature EquippedEntryDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FUnEquippedEntrySignature UnEquippedEntryDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FInventoryItemQuickSlottedSignature InventoryItemQuickSlottedDelegate;
	
	void SetOwningActor(AActor* InOwner);

	void SetOwningInventory();

	void SetOwningEquipmentManagerComp();
	
	void SetOwningQuickSlotManagerComp();
	
	void SetInventoryWidget(UInventoryDashboardWidget* InInventoryWidget);

	void BindCallbacksToDependencies();

	void BroadcastInitialValues();
	
	void UnbindAllEventsFromDelegates();

	UFUNCTION(BlueprintImplementableEvent)
	void BindDelegatesToInventoryWidget();
		
private:
	
	bool EnsureOwningInventory();
	
	bool EnsureOwningEquipmentManagerComp();
	
	bool EnsureOwningQuickSlotManagerComp();

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<AActor> OwningActor;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UInventoryComponent> OwningInventoryComp;
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UEquipmentManagerComponent> OwningEquipmentManagerComp;
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UQuickSlotManagerComponent> OwningQuickSlotManagerComp;
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UInventoryDashboardWidget> OwningInventoryDashboardWidget;	
};
