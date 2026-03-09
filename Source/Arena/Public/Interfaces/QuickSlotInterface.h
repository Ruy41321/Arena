// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "QuickSlotInterface.generated.h"

class UQuickSlotManagerComponent;
// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UQuickSlotInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ARENA_API IQuickSlotInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	UQuickSlotManagerComponent* GetQuickSlotManagerComponent() const;
};
