// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetControllers/WidgetController.h"
#include "MKHGameMenuController.generated.h"

class UMKHGameMenuWidget;
/**
 * 
 */
UCLASS()
class MAKHIA_API UMKHGameMenuController : public UWidgetController
{
	GENERATED_BODY()
	
public:
	
	void SetGameMenuWidget(UMKHGameMenuWidget* InGameMenuWidget);
	
private:
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UMKHGameMenuWidget> GameMenuWidget;
	
};
