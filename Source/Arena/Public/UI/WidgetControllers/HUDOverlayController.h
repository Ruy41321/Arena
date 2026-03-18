// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetControllers/WidgetController.h"
#include "HUDOverlayController.generated.h"

class UEquipmentManagerComponent;
class UQuickSlotManagerComponent;
struct FGameplayTag;
class UInventoryComponent;
class UHUDOverlayWidget;
class UInventoryItem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHUDItemQuickSlottedSignature, UInventoryItem*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHUDItemChangedSignature, UInventoryItem*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHUDItemRemovedSignature, const int64, RemovedItemID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHUDWeaponEquippedSignature, const int64, EquippedWeaponID);

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class ARENA_API UHUDOverlayController : public UWidgetController
{
	GENERATED_BODY()
	
public:
		
	UPROPERTY(BlueprintAssignable)
	FHUDItemQuickSlottedSignature HUDItemQuickSlottedDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FHUDItemChangedSignature HUDItemChangedDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FHUDItemRemovedSignature HUDItemRemovedDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FHUDWeaponEquippedSignature HUDWeaponEquippedDelegate;
	
	virtual void BindCallbacksToDependencies() override;
	virtual void BroadcastInitialValues() override;
	virtual void BindDelegatesToWidget_Implementation() override;
	virtual void UnbindAllEventsFromDelegates() override;
		
	void SetOwningInventoryComponent();
	void SetOwningEquipmentComponent();
	void SetOwningQuickSlotManagerComp();

	void SetHUDOverlayWidget(UHUDOverlayWidget* InHUDOverlayWidget);
	
private:
	
	bool EnsureOwningInventory();
	bool EnsureOwningEquipment();
	bool EnsureOwningQuickSlotManagerComp();
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UQuickSlotManagerComponent> OwningQuickSlotManagerComp;
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UInventoryComponent> OwningInventoryComp;
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UEquipmentManagerComponent> OwningEquipmentComp;
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UHUDOverlayWidget> OwningHUDOverlayWidget;	
	
};
