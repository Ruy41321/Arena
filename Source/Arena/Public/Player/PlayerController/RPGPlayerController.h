// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Interfaces/InventoryInterface.h"
#include "GameFramework/PlayerController.h"
#include "RPGPlayerController.generated.h"

class UInventoryComponent;
class UInventoryWidgetController;
class URPGSystemWidget;
/**
 * 
 */
UCLASS()
class ARENA_API ARPGPlayerController : public APlayerController, public IAbilitySystemInterface, public IInventoryInterface
{
	GENERATED_BODY()
	
public:

	ARPGPlayerController();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual UInventoryComponent* GetInventoryComponent_Implementation() const override;

	UInventoryWidgetController* GetInventoryWidgetController();
	
	UFUNCTION(BlueprintCallable)
	void CreateInventoryWidget();

private:

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
