// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AbilitySystem/MKHAbilityTypes.h"
#include "Engine/DataAsset.h"
#include "ProjectileInfo.generated.h"

/**
 * 
 */
UCLASS()
class MAKHIA_API UProjectileInfo : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly)
	TMap<FGameplayTag, FProjectileParams> ProjectileInfoMap;
	
};
