// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "RPGGameplayAbility.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API URPGGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	

public:
	
	UPROPERTY(EditDefaultsOnly, category = "Custom Values | Input")
	FGameplayTag InputTag;

};
