// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetControllers/WidgetController.h"

void UWidgetController::SetOwningActor(AActor* InOwner)
{
	OwningActor = InOwner;
}

void UWidgetController::BindCallbacksToDependencies()
{
	// Base implementation - to be overridden by derived classes if needed
}

void UWidgetController::BroadcastInitialValues()
{
	// Base implementation - to be overridden by derived classes if needed
}

void UWidgetController::UnbindAllEventsFromDelegates()
{
	// Base implementation - to be overridden by derived classes if needed
}

void UWidgetController::BindDelegatesToWidget_Implementation()
{
}

