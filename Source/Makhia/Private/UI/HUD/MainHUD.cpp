// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/MainHUD.h"

#include "AbilitySystem/MKHGameplayTags.h"
#include "Player/PlayerController/MKHPlayerController.h"
#include "UI/HUD/HUDOverlay/HUDOverlayWidget.h"
#include "UI/HUD/Inventory/InventoryDashboardWidget.h"
#include "UI/WidgetControllers/HUDOverlayController.h"
#include "UI/WidgetControllers/InventoryDashboardController.h"

void AMainHUD::BeginPlay()
{
	Super::BeginPlay();
	
	PlayerController = Cast<AMKHPlayerController>(GetOwningPlayerController());
	
	CreateHUDOverlayWidget();
}

UHUDOverlayController* AMainHUD::GetHUDOverlayWidgetController()
{
	if (!IsValid(HUDOverlayController))
	{
		HUDOverlayController = NewObject<UHUDOverlayController>(PlayerController, HUDOverlayControllerClass);
		HUDOverlayController->SetOwningActor(PlayerController);

		HUDOverlayController->BindCallbacksToDependencies();
	}
	return HUDOverlayController;
}

void AMainHUD::CreateHUDOverlayWidget()
{
	if (UUserWidget* Widget = CreateWidget<UHUDOverlayWidget>(PlayerController, HUDOverlayWidgetClass))
	{
		HUDOverlayWidget = Cast<UHUDOverlayWidget>(Widget);
		UHUDOverlayController* Controller = GetHUDOverlayWidgetController();
		Controller->SetHUDOverlayWidget(HUDOverlayWidget);
		Controller->BroadcastInitialValues();
		HUDOverlayWidget->AddToViewport();
	}
}

void AMainHUD::DestroyHUDOverlayWidget()
{
	HUDOverlayWidget->RemoveFromParent();
	HUDOverlayWidget = nullptr;
	GetHUDOverlayWidgetController()->UnbindAllEventsFromDelegates();
	GetHUDOverlayWidgetController()->SetHUDOverlayWidget(nullptr);
}

UInventoryDashboardController* AMainHUD::GetInventoryWidgetController()
{
	if (!IsValid(InventoryDashboardController))
	{
		InventoryDashboardController = NewObject<UInventoryDashboardController>(PlayerController, InventoryDashboardControllerClass);
		InventoryDashboardController->SetOwningActor(PlayerController);

		InventoryDashboardController->BindCallbacksToDependencies();
	}

	return InventoryDashboardController;
}

void AMainHUD::ToggleInventory()
{
	if (!IsValid(InventoryDashboardWidget))
	{
		CreateInventoryWidget();
		HUDOverlayWidget->OnQuickSlotActivated(MKHGameplayTags::Input::Inventory);
		InventoryDashboardWidget->OpenInventoryDashboard();
	}
	else if (InventoryDashboardWidget->bIsOpen)
	{
		InventoryDashboardWidget->CloseInventoryDashboard();
	}
	else
	{
		HUDOverlayWidget->OnQuickSlotActivated(MKHGameplayTags::Input::Inventory);
		InventoryDashboardWidget->OpenInventoryDashboard();
	}
}

void AMainHUD::CreateInventoryWidget()
{
	if (UUserWidget* Widget = CreateWidget<UInventoryDashboardWidget>(PlayerController, InventoryDashboardWidgetClass))
	{
		InventoryDashboardWidget = Cast<UInventoryDashboardWidget>(Widget);
		UInventoryDashboardController* Controller = GetInventoryWidgetController();
		Controller->SetInventoryWidget(InventoryDashboardWidget);
		BindInventoryWidgetDelegates();
		Controller->BroadcastInitialValues();
		InventoryDashboardWidget->AddToViewport();
	}
}

void AMainHUD::DestroyInventoryWidget()
{
	if (IsValid(InventoryDashboardWidget))
	{
		InventoryDashboardWidget->DestroyInventoryDashboard();
	}
}

void AMainHUD::BindInventoryWidgetDelegates()
{
	InventoryDashboardWidget->OnOpenInventoryDashboardDelegate.AddLambda(
		[this]()
		{
			PlayerController->ChangeFocus("Inventory");
		});
	InventoryDashboardWidget->OnCloseInventoryDashboardDelegate.AddLambda(
		[this]()
		{
			PlayerController->ChangeFocus("Game");
		});
	InventoryDashboardWidget->OnDestructionInventoryDashboardDelegate.AddLambda(
		[this]()
		{
			InventoryDashboardWidget->UnbindAllEventsFromDelegates();
			InventoryDashboardWidget = nullptr;
			GetInventoryWidgetController()->UnbindAllEventsFromDelegates();
			GetInventoryWidgetController()->SetInventoryWidget(nullptr);
			PlayerController->ChangeFocus("Game");
		});
}
