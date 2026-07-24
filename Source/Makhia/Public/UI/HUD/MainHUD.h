// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MainHUD.generated.h"

class UMKHGameMenuWidget;
class UMKHGameMenuController;
class AMKHPlayerController;
class UInventoryDashboardWidget;
class UInventoryDashboardController;
class UHUDOverlayController;
class UHUDOverlayWidget;
class UMatchOverWidget;
class AMKHPlayerCharacter;
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
	
	/* GameMenu Widget */
	
	void ToggleGameMenu();
	
	UFUNCTION(BlueprintCallable)
	void CreateGameMenuWidget();

	UFUNCTION(BlueprintCallable)
	void DestroyGameMenuWidget();
	
	UFUNCTION(BlueprintCallable)
	UMKHGameMenuController* GetGameMenuWidgetController();

	/* MatchOver Widget */

	/** Creates the end-of-match panel and adds it to the viewport. Does nothing if it already exists. */
	UFUNCTION(BlueprintCallable)
	void CreateMatchOverWidget();

	/** Removes the end-of-match panel from the viewport. */
	UFUNCTION(BlueprintCallable)
	void DestroyMatchOverWidget();

protected:

	virtual void BeginPlay() override;
	
private:
	
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = true))
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
	
	/* Game Menu Widget */
	
	void BindGameMenuWidgetDelegates();
	
	UPROPERTY(EditDefaultsOnly, category = "Custom Values | GameMenuWidgets")
	TSubclassOf<UMKHGameMenuController> GameMenuControllerClass;

	UPROPERTY(EditDefaultsOnly, category = "Custom Values | GameMenuWidgets")
	TSubclassOf<UMKHGameMenuWidget> GameMenuWidgetClass;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UMKHGameMenuController> GameMenuController;
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UMKHGameMenuWidget> GameMenuWidget;

	/* MatchOver Widget */

	/**
	 * Subscribes to the game state events that drive the end-of-match panel: OnMatchOver to show it
	 * and OnInitialPreparationStarted to take it down again.
	 */
	void BindMatchOverEvent();

	void BindMatchOverWidgetDelegates();

	/**
	 * Match-over callback: shows the result panel and closes any menu still open.
	 *
	 * @param WinningPlayer  The winner, or nullptr on a draw. Only compared against the local
	 *                       player to decide between the victory and defeat headline.
	 * @param bIsDraw        True when the match ended without a winner.
	 */
	UFUNCTION()
	void OnMatchOver(AMKHPlayerCharacter* WinningPlayer, bool bIsDraw);

	/**
	 * Start-of-match callback: takes the result panel down and hands the focus back to gameplay.
	 * A rematch restarts the match in place, so nothing else would remove the panel from the viewport.
	 */
	UFUNCTION()
	void OnMatchRestarted();

	UPROPERTY(EditDefaultsOnly, category = "Custom Values | MatchOverWidgets")
	TSubclassOf<UMatchOverWidget> MatchOverWidgetClass;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UMatchOverWidget> MatchOverWidget;

};
