// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemGlobals.h"
#include "MKHAbilitySystemGlobals.generated.h"

/**
 * Ability system globals implementation responsible for allocating Arena's custom
 * gameplay effect context type during spec creation.
 */
UCLASS()
class MAKHIA_API UMKHAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()

	/** Allocates the custom FMKHGameplayEffectContext used by this project. */
	virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;

};
