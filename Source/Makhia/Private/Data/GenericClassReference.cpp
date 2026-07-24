// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/GenericClassReference.h"

#include "MKHLogChannels.h"
#include "GameplayEffect.h"

const FName UGenericClassReference::DamageEffectKey(TEXT("GE_Damage"));
const FName UGenericClassReference::SkillCooldownEffectKey(TEXT("GE_GenericSkillCooldown"));

TSubclassOf<UObject> UGenericClassReference::GetClassByName(const UGenericClassReference* DataAsset, FName ClassName)
{
	if (!DataAsset)
	{
		UE_LOG(LogMKHAbility, Warning, TEXT("GetClassByName: DataAsset is null."));
		return nullptr;
	}

	// Find the class in the map using the key
	if (const TSubclassOf<UObject>* FoundClass = DataAsset->GenericClassMap.Find(ClassName))
	{
		return *FoundClass;
	}

	UE_LOG(LogMKHAbility, Warning, TEXT("GetClassByName: Class with name %s not found in Data Asset."), *ClassName.ToString());
	return nullptr;
}

TSubclassOf<UGameplayEffect> UGenericClassReference::GetGameplayEffectByName(const UGenericClassReference* DataAsset, const FName ClassName)
{
	const TSubclassOf<UObject> FoundClass = GetClassByName(DataAsset, ClassName);
	if (FoundClass && !FoundClass->IsChildOf(UGameplayEffect::StaticClass()))
	{
		UE_LOG(LogMKHAbility, Warning, TEXT("GetGameplayEffectByName: Class %s is not a UGameplayEffect."), *ClassName.ToString());
		return nullptr;
	}

	return TSubclassOf<UGameplayEffect>(*FoundClass);
}