// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayAbilitySpecHandle.h"
#include "EquipmentTypes.generated.h"

USTRUCT()
struct FEquipmentGrantedHandles
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> GrantedAbilities = TArray<FGameplayAbilitySpecHandle>();

	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> ActiveEffects = TArray<FActiveGameplayEffectHandle>();

	void AddAbilityHandle(const FGameplayAbilitySpecHandle& Handle)
	{
		GrantedAbilities.Add(Handle);
	}

	void AddEffectHandle(const FActiveGameplayEffectHandle& Handle)
	{
		ActiveEffects.Add(Handle);
	}
};
