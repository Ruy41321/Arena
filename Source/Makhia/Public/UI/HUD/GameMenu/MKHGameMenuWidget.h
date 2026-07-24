// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/MKHSystemWidget.h"
#include "MKHGameMenuWidget.generated.h"

DECLARE_MULTICAST_DELEGATE(FGameMenuDelegateSignature);

/**
 * 
 */
UCLASS()
class MAKHIA_API UMKHGameMenuWidget : public UMKHSystemWidget
{
	GENERATED_BODY()
	
public:
	
	FGameMenuDelegateSignature FOnOpenGameMenuDelegate;
	FGameMenuDelegateSignature FOnCloseGameMenuDelegate;
	FGameMenuDelegateSignature FOnDestroyGameMenuDelegate;
	
	bool IsOpen() const {return bIsOpen;}
	
	UFUNCTION(BlueprintCallable, Category = "GameMenu | Visibility")
	void OpenGameMenu();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "GameMenu | Visibility")
	void OnOpenGameMenu();
	
	UFUNCTION(BlueprintCallable, Category = "GameMenu | Visibility")
	void CloseGameMenu();
	
	void DestroyGameMenu();
	
	void UnbindAllEventsFromDelegates();
	
protected:

	/**
	 * Restarts the match for every player.
	 * Delegated to AMKHPlayerController::Server_RestartMatch because a UFUNCTION(Server) declared
	 * on a UUserWidget never leaves the client (UUserWidget has no RPC callspace resolution).
	 */
	UFUNCTION(BlueprintCallable, Category = "GameMenu | Actions")
	void RestartGame();

	UFUNCTION(BlueprintCallable, Category = "GameMenu | Actions")
	void ReturnToLobby();
	
	UFUNCTION(BlueprintCallable, Category = "GameMenu | Actions")
	void QuitGame();
	
private:

	bool bIsOpen = false;

};
