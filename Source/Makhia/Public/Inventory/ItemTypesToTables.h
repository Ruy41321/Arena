// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ItemTypesToTables.generated.h"

/**
 * 
 */
UCLASS()
class MAKHIA_API UItemTypesToTables : public UDataAsset
{
	GENERATED_BODY()

public:

	/* Map used in the engine to create a DataAsset to store Item Tables */
	UPROPERTY(EditDefaultsOnly)
	TMap<FGameplayTag, TObjectPtr<UDataTable>> TagsToTables;
	
};
