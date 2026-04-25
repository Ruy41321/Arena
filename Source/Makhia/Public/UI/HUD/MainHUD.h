// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MainHUD.generated.h"

class AMKHPlayerController;
class UInventoryDashboardWidget;
class UInventoryDashboardController;
class UHUDOverlayController;
class UHUDOverlayWidget;
/**
 * 
 */
UCLASS()
class MAKHIA_API AMainHUD : public AHUD
{
	GENERATED_BODY()

public:
	
	/* HUD Widget */
	
	UFUNCTION(BlueprintCallable)
	UHUDOverlayController* GetHUDOverlayWidgetController();
	
	UFUNCTION(BlueprintCallable)
	void CreateHUDOverlayWidget();

	UFUNCTION(BlueprintCallable)
	void DestroyHUDOverlayWidget();
	
	/* Inventory Widget */
	
	void ToggleInventory();
	
	UFUNCTION(BlueprintCallable)
	void CreateInventoryWidget();

	UFUNCTION(BlueprintCallable)
	void DestroyInventoryWidget();
	
	UFUNCTION(BlueprintCallable)
	UInventoryDashboardController* GetInventoryWidgetController();

protected:
	
	virtual void BeginPlay() override;
	
private:
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<AMKHPlayerController> PlayerController;
	
	/* HUD Widget */
	
	UPROPERTY(EditDefaultsOnly, category = "Custom Values | HUDOverlayWidgets")
	TSubclassOf<UHUDOverlayController> HUDOverlayControllerClass;

	UPROPERTY(EditDefaultsOnly, category = "Custom Values | HUDOverlayWidgets")
	TSubclassOf<UHUDOverlayWidget> HUDOverlayWidgetClass;
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UHUDOverlayController> HUDOverlayController;
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UHUDOverlayWidget> HUDOverlayWidget;
	
	/* Inventory Widget */
	
	void BindInventoryWidgetDelegates();
	
	UPROPERTY(EditDefaultsOnly, category = "Custom Values | InventoryWidgets")
	TSubclassOf<UInventoryDashboardController> InventoryDashboardControllerClass;

	UPROPERTY(EditDefaultsOnly, category = "Custom Values | InventoryWidgets")
	TSubclassOf<UInventoryDashboardWidget> InventoryDashboardWidgetClass;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UInventoryDashboardController> InventoryDashboardController;
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UInventoryDashboardWidget> InventoryDashboardWidget;
	
};
