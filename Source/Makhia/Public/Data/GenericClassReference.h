// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GenericClassReference.generated.h"

/**
 * 
 */
UCLASS()
class MAKHIA_API UGenericClassReference : public UDataAsset
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(EditDefaultsOnly)
	TMap<FName, TSubclassOf<UObject>> GenericClassMap;
};
