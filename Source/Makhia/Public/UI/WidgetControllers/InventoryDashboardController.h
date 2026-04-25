// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UI/WidgetControllers/WidgetController.h"
#include "InventoryDashboardController.generated.h"

class UInventoryItem;
class UInventoryDashboardWidget;
class UInventoryComponent;
class UEquipmentManagerComponent;
class UQuickSlotManagerComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDashboardBagItemChangedSignature, UInventoryItem*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDashboardBagItemRemovedSignature, const int64, RemovedItemID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDashboardEquipmentChangeSignature, UInventoryItem*, Item /*Equipped Item*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDashboardEquipmentRemovedSignature, const FGameplayTag&, SlotTag /*UnEquipped Slot*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDashboardQuickSlotRelocatedSignature, UInventoryItem*, Item /*Quick Slotted Item*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDashboardQuickSlotChangedSignature, UInventoryItem*, Item /*Quick Slotted Item*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDashboardQuickSlotRemovedSignature, const int64, RemovedItemID);

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class MAKHIA_API UInventoryDashboardController : public UWidgetController
{
	GENERATED_BODY()
	
public:

	UPROPERTY(BlueprintAssignable)
	FDashboardBagItemChangedSignature DashboardBagItemChangedDelegate;

	UPROPERTY(BlueprintAssignable)
	FDashboardBagItemRemovedSignature DashboardBagItemRemovedDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FDashboardEquipmentChangeSignature DashboardEquipmentChangeDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FDashboardEquipmentRemovedSignature DashboardEquipmentRemovedDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FDashboardQuickSlotRelocatedSignature QuickSlotItemRelocatedDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FDashboardQuickSlotChangedSignature QuickSlotItemChangedDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FDashboardQuickSlotRemovedSignature QuickSlotItemRemovedDelegate;
	
	void SetOwningInventory();

	void SetOwningEquipmentManagerComp();
	
	void SetOwningQuickSlotManagerComp();
	
	void SetInventoryWidget(UInventoryDashboardWidget* InInventoryWidget);

	virtual void BindCallbacksToDependencies() override;

	virtual void BroadcastInitialValues() override;
	
	virtual void BindDelegatesToWidget_Implementation() override;
	
	virtual void UnbindAllEventsFromDelegates() override;
		
private:
	
	bool EnsureOwningInventory();
	
	bool EnsureOwningEquipmentManagerComp();
	
	bool EnsureOwningQuickSlotManagerComp();

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UInventoryComponent> OwningInventoryComp;
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UEquipmentManagerComponent> OwningEquipmentManagerComp;
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UQuickSlotManagerComponent> OwningQuickSlotManagerComp;
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UInventoryDashboardWidget> OwningInventoryDashboardWidget;	
};
