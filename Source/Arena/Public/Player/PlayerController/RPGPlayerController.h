// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "Interfaces/InventoryInterface.h"
#include "Interfaces/RPGAbilitySystemInterface.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/EquipmentInterface.h"
#include "Interfaces/QuickSlotInterface.h"
#include "RPGPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UInventoryDashboardWidget;
class URPGInputConfig;
class UInventoryComponent;
class UInventoryDashboardController;
class URPGAbilitySystemComponent;
class UEquipmentManagerComponent;
class UQuickSlotManagerComponent;
/**
 * 
 */
UCLASS()
class ARENA_API ARPGPlayerController : public APlayerController, public IAbilitySystemInterface, public IInventoryInterface, public IEquipmentInterface, public IQuickSlotInterface, public IRPGAbilitySystemInterface
{
	GENERATED_BODY()
	
public:

	ARPGPlayerController();

	virtual void SetupInputComponent() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/* Implement Equipment Interface*/
	virtual UEquipmentManagerComponent* GetEquipmentManagerComponent_Implementation() const override;
	
	/* Implement Inventory Interface*/
	virtual UInventoryComponent* GetInventoryComponent_Implementation() const override;
	
	/* Implement QuickSlot Interface*/
	virtual UQuickSlotManagerComponent* GetQuickSlotManagerComponent_Implementation() const override;

	/* Implement RPG Ability System Interface */ 
	virtual void SetDynamicProjectile_Implementation(const FGameplayTag& ProjectileTag, int32 AbilityLevel) override;

	UInventoryDashboardController* GetInventoryWidgetController();
	
	URPGAbilitySystemComponent* GetRPGAbilitySystemComponent();
	
	UFUNCTION(BlueprintCallable)
	void CreateInventoryWidget();

	UFUNCTION(BlueprintCallable)
	void DestroyInventoryWidget();

protected:

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void ChangeMappingContext(const UInputMappingContext* NewMappingContext) const;
	
	void AbilityInputPressed(const FGameplayTag& InputTag);
	void AbilityInputReleased(const FGameplayTag& InputTag);

private:

	UPROPERTY()
	TObjectPtr<URPGAbilitySystemComponent> RPGAbilitySystemComponent;
	
	// Input actions and mapping context
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Category = "Custom Values | Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Category = "Custom Values | Input")
	TObjectPtr<UInputMappingContext> UIOpenMappingContext;
	
	UPROPERTY(EditDefaultsOnly, category = "Custom Values | Input")
	TObjectPtr<URPGInputConfig> RPGInputConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Category = "Custom Values | Input | Actions")
	TObjectPtr<UInputAction> ToggleInventoryAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Replicated)
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UEquipmentManagerComponent> EquipmentComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UQuickSlotManagerComponent> QuickSlotManagerComponent;
	
	UPROPERTY()
	TObjectPtr<UInventoryDashboardController> InventoryDashboardController;

	UPROPERTY(EditDefaultsOnly, category = "Custom Values | Widgets")
	TSubclassOf<UInventoryDashboardController> InventoryDashboardControllerClass;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UInventoryDashboardWidget> InventoryDashboardWidget;

	UPROPERTY(EditDefaultsOnly, category = "Custom Values | Widgets")
	TSubclassOf<UInventoryDashboardWidget> InventoryDashboardWidgetClass;

	void BindCallbacksToDependencies() const;
	void BindInventoryWidgetDelegates();

	void ToggleInventory();
	void ChangeFocus(FString Focus);
};
