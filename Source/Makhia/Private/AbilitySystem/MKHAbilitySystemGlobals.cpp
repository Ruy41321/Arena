// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/MKHAbilitySystemGlobals.h"

#include "AbilitySystem/MKHAbilityTypes.h"

FGameplayEffectContext* UMKHAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FMKHGameplayEffectContext();
}
