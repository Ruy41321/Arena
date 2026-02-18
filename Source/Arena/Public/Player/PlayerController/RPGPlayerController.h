// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "Interfaces/InventoryInterface.h"
#include "Interfaces/RPGAbilitySystemInterface.h"
#include "GameFramework/PlayerController.h"
#include "RPGPlayerController.generated.h"

class URPGInputConfig;
class UInventoryComponent;
class UInventoryWidgetController;
class URPGSystemWidget;
class URPGAbilitySystemComponent;
/**
 * 
 */
UCLASS()
class ARENA_API ARPGPlayerController : public APlayerController, public IAbilitySystemInterface, public IInventoryInterface, public IRPGAbilitySystemInterface
{
	GENERATED_BODY()
	
public:

	ARPGPlayerController();

	virtual void SetupInputComponent() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/* Implement Inventory Interface*/
	virtual UInventoryComponent* GetInventoryComponent_Implementation() const override;

	/* Implement RPG Ability System Interface */ 
	virtual void SetDynamicProjectile_Implementation(const FGameplayTag& ProjectileTag, int32 AbilityLevel) override;

	UInventoryWidgetController* GetInventoryWidgetController();
	
	URPGAbilitySystemComponent* GetRPGAbilitySystemComponent();

	UFUNCTION(BlueprintCallable)
	void CreateInventoryWidget();

protected:

	void AbilityInputPressed(const FGameplayTag& InputTag);
	void AbilityInputReleased(const FGameplayTag& InputTag);

private:

	UPROPERTY()
	TObjectPtr<URPGAbilitySystemComponent> RPGAbilitySystemComponent;

	UPROPERTY(EditDefaultsOnly, category = "Custom Values | Input")
	TObjectPtr<URPGInputConfig> RPGInputConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Replicated)
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY()
	TObjectPtr<UInventoryWidgetController> InventoryWidgetController;

	UPROPERTY(EditDefaultsOnly, category = "Custom Values | Widgets")
	TSubclassOf<UInventoryWidgetController> InventoryWidgetControllerClass;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<URPGSystemWidget> InventoryWidget;

	UPROPERTY(EditDefaultsOnly, category = "Custom Values | Widgets")
	TSubclassOf<URPGSystemWidget> InventoryWidgetClass;
};
