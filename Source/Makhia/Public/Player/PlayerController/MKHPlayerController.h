// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "Interfaces/InventoryInterface.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/EquipmentInterface.h"
#include "Interfaces/QuickSlotInterface.h"
#include "MKHPlayerController.generated.h"

class UGenericClassReference;
class UInputAction;
class UInputMappingContext;
class UInventoryDashboardWidget;
class UMKHInputConfig;
class UInventoryComponent;
class UInventoryDashboardController;
class UMKHAbilitySystemComponent;
class UEquipmentManagerComponent;
class UQuickSlotManagerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMappingContextChangedSignature, const UInputMappingContext*, NewMappingContext);
DECLARE_DELEGATE_OneParam(FSkillActivatedSignature, FGameplayTag)

/**
 * 
 */
UCLASS()
class MAKHIA_API AMKHPlayerController : public APlayerController, public IAbilitySystemInterface, public IInventoryInterface, public IEquipmentInterface, public IQuickSlotInterface
{
	GENERATED_BODY()
	
	friend class AMainHUD;
	
public:

	AMKHPlayerController();

	virtual void SetupInputComponent() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/* Implement Equipment Interface*/
	virtual UEquipmentManagerComponent* GetEquipmentManagerComponent_Implementation() const override;
	
	/* Implement Inventory Interface*/
	virtual UInventoryComponent* GetInventoryComponent_Implementation() const override;
	
	/* Implement QuickSlot Interface*/
	virtual UQuickSlotManagerComponent* GetQuickSlotManagerComponent_Implementation() const override;

	UMKHAbilitySystemComponent* GetRPGAbilitySystemComponent();
	
	UPROPERTY(BlueprintAssignable)
	FMappingContextChangedSignature MappingContextChangedDelegate;
	
	FSkillActivatedSignature SkillActivatedDelegate;
	
	/** Data Asset to allow c++ code to take references of Blueprint Classes */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Category = "Custom Values | Utility")
	TObjectPtr<UGenericClassReference> GenericClassReference;
	
protected:

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void ChangeMappingContext(const UInputMappingContext* NewMappingContext) const;

	bool EnsureWeaponEquipped() const;
	
	void AbilityInputPressed(const FGameplayTag& InputTag);
	void AbilityInputReleased(const FGameplayTag& InputTag);

private:

	UPROPERTY()
	TObjectPtr<UMKHAbilitySystemComponent> MKHAbilitySystemComponent;
	
	// Input actions and mapping context
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Category = "Custom Values | Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Category = "Custom Values | Input")
	TObjectPtr<UInputMappingContext> UIOpenMappingContext;
	
	UPROPERTY(EditDefaultsOnly, category = "Custom Values | Input")
	TObjectPtr<UMKHInputConfig> MKHInputConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Category = "Custom Values | Input | Actions")
	TObjectPtr<UInputAction> ToggleInventoryAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Replicated)
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UEquipmentManagerComponent> EquipmentComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UQuickSlotManagerComponent> QuickSlotManagerComponent;
	
	void BindCallbacksToDependencies() const;

	void ChangeFocus(FString Focus);
	void ToggleInventory();
};
