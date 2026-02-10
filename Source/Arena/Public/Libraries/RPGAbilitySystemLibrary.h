// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "RPGAbilitySystemLibrary.generated.h"

class UCharacterClassInfo;

/**
 * 
 */
UCLASS()
class ARENA_API URPGAbilitySystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintPure)
	static UCharacterClassInfo* GetCharacterClassDefaultInfo(const UObject* WorldContextObject);


	template<typename T>
	static T* GetDataTableRowByTag(const UDataTable* DataTable, const FGameplayTag& Tag);

};

template<typename T>
T* URPGAbilitySystemLibrary::GetDataTableRowByTag(const UDataTable* DataTable, const FGameplayTag& Tag)
{
	if (!DataTable)
	{
		return nullptr;
	}
	return DataTable->FindRow<T>(Tag.GetTagName(), FString());
}