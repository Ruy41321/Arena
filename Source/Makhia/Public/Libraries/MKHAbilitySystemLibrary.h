// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "MKHAbilitySystemLibrary.generated.h"

struct FRPGInventoryEntry;
class UCharacterClassInfo;
class UProjectileInfo;
struct FDamageEffectInfo;
struct FStatusEffectData;
struct FGameplayEffectContextHandle;
/**
 *
 */
UCLASS()
class MAKHIA_API UMKHAbilitySystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure)
	static UCharacterClassInfo* GetCharacterClassDefaultInfo(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure)
	static UProjectileInfo* GetProjectileInfo(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable)
	static void ApplyDamageEffect(const FDamageEffectInfo& DamageEffectInfo);

	/**
	 * Shared application pipeline for status effects: builds one outgoing spec per entry,
	 * assigns duration and value as set-by-caller magnitudes, and applies it to the target.
	 * Used for target on-hit effects, self on-hit effects and self activation effects,
	 * so every status effect follows the same path from definition to application.
	 *
	 * @param SourceASC      ASC that creates the outgoing specs (the effect instigator).
	 * @param TargetASC      ASC receiving the effects (pass SourceASC for self-applied effects).
	 * @param StatusEffects  Status effect entries to apply.
	 * @param ContextHandle  Effect context forwarded to the specs (crit flag, source object, ...).
	 */
	static void ApplyStatusEffects(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, const TArray<FStatusEffectData>& StatusEffects, const FGameplayEffectContextHandle& ContextHandle);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Loose Tag Count Static"))
	static void K2_SetLooseTagCountStatic(UAbilitySystemComponent* ASC, FGameplayTag Tag, int32 NewCount);

	template<typename T>
	static T* GetDataTableRowByTag(const UDataTable* DataTable, const FGameplayTag& Tag);

	static void AssignDynamicSkillInputTag(FRPGInventoryEntry& NewEntry);
};

template<typename T>
T* UMKHAbilitySystemLibrary::GetDataTableRowByTag(const UDataTable* DataTable, const FGameplayTag& Tag)
{
	if (!DataTable)
	{
		return nullptr;
	}
	return DataTable->FindRow<T>(Tag.GetTagName(), FString());
}
