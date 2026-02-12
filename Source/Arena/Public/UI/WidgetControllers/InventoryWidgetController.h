// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetControllers/WidgetController.h"
#include "InventoryWidgetController.generated.h"

class UInventoryComponent;
struct FPackagedInventory;
struct FMasterItemDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInventoryItemSignature, const FMasterItemDefinition&, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInventoryBroadcastCompleteSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FScrollBoxResetSignature);
/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class ARENA_API UInventoryWidgetController : public UWidgetController
{
	GENERATED_BODY()
	
public:

	UPROPERTY(BlueprintAssignable)
	FInventoryItemSignature InventoryItemDelegate;

	UPROPERTY(BlueprintAssignable)
	FInventoryBroadcastCompleteSignature InventoryBroadcastCompleteDelegate;

	UPROPERTY(BlueprintAssignable)
	FScrollBoxResetSignature ScrollBoxResetDelegate;

	void SetOwningActor(AActor* InOwner);

	void BindCallbacksToDependencies();

	void BroadcastInitialValues();

private:

	void UpdateInventory(const FPackagedInventory& PackagedInventory);

	void BroadcastInventoryContent();

	UPROPERTY()
	TObjectPtr<AActor> OwningActor;

	UPROPERTY()
	TObjectPtr<UInventoryComponent> OwningInventory;
};
