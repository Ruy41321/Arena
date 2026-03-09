// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EquipmentInterface.generated.h"

class UEquipmentManagerComponent;

UINTERFACE(MinimalAPI)
class UEquipmentInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ARENA_API IEquipmentInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	UEquipmentManagerComponent* GetEquipmentManagerComponent() const;
};
