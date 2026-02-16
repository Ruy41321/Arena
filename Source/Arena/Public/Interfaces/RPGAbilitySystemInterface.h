// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RPGAbilitySystemInterface.generated.h"

UINTERFACE(MinimalAPI)
class URPGAbilitySystemInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ARENA_API IRPGAbilitySystemInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	USceneComponent* GetDynamicSpawnPoint();

};
