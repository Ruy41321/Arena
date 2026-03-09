// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/InventoryDashboardWidget.h"

void UInventoryDashboardWidget::UnbindAllEventsFromDelegates()
{
	OnOpenInventoryDashboardDelegate.Clear();
	OnCloseInventoryDashboardDelegate.Clear();
	OnDestructionInventoryDashboardDelegate.Clear();
}
