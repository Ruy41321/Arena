// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/GameMenu/MKHGameMenuWidget.h"

#include "GameInstance/MKHGameInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MKHLogChannels.h"
#include "Player/PlayerController/MKHPlayerController.h"

void UMKHGameMenuWidget::OpenGameMenu()
{
	SetVisibility(ESlateVisibility::Visible);
	bIsOpen = true;
	FOnOpenGameMenuDelegate.Broadcast();
	OnOpenGameMenu();
}

void UMKHGameMenuWidget::CloseGameMenu()
{
	SetVisibility(ESlateVisibility::Collapsed);
	bIsOpen = false;
	FOnCloseGameMenuDelegate.Broadcast();
}

void UMKHGameMenuWidget::DestroyGameMenu()
{
	bIsOpen = false;
	FOnDestroyGameMenuDelegate.Broadcast();
	RemoveFromParent();
}

void UMKHGameMenuWidget::UnbindAllEventsFromDelegates()
{
	FOnOpenGameMenuDelegate.Clear();
	FOnCloseGameMenuDelegate.Clear();
	FOnDestroyGameMenuDelegate.Clear();
}

void UMKHGameMenuWidget::RestartGame()
{
	AMKHPlayerController* PC = Cast<AMKHPlayerController>(GetOwningPlayer());
	if (!IsValid(PC))
	{
		UE_LOG(LogMKHAbility, Warning, TEXT("UMKHGameMenuWidget::RestartGame: no owning MKH player controller."));
		return;
	}

	PC->Server_RestartMatch();
}

void UMKHGameMenuWidget::ReturnToLobby()
{
	const APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}
	
	UMKHGameInstance* GameInstance = UMKHGameInstance::GetMKHGameInstance(PC);
	if (!GameInstance)
	{
		return;
	}
	
	GameInstance->BackToLobby(true);
}

void UMKHGameMenuWidget::QuitGame()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}
	
	UKismetSystemLibrary::QuitGame(
		GetWorld(), 
		PC, 
		EQuitPreference::Quit, 
		false
	);
}

