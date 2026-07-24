// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GenericClassReference.generated.h"

class UGameplayEffect;

/**
 * Data asset mapping well-known names to Blueprint classes that native code needs to
 * reference without hard dependencies (e.g. generic damage and cooldown gameplay effects).
 * The canonical keys live here as named constants: never spell the map keys inline.
 */
UCLASS()
class MAKHIA_API UGenericClassReference : public UDataAsset
{
	GENERATED_BODY()

public:

	/** Map key for the generic damage gameplay effect applied by damage abilities. */
	static const FName DamageEffectKey;

	/** Map key for the generic set-by-caller skill cooldown gameplay effect. */
	static const FName SkillCooldownEffectKey;

	/** Name-to-class map configured in the editor. Keys must match the constants above. */
	UPROPERTY(EditDefaultsOnly)
	TMap<FName, TSubclassOf<UObject>> GenericClassMap;

	/** Resolves a class by its well-known name (see the key constants above). Returns nullptr with a warning when missing. */
	UFUNCTION(BlueprintPure)
	static TSubclassOf<UObject> GetClassByName(const UGenericClassReference* DataAsset, FName ClassName);

	/** Typed wrapper for GetClassByName that also rejects non-UGameplayEffect classes. */
	static TSubclassOf<UGameplayEffect> GetGameplayEffectByName(const UGenericClassReference* DataAsset, FName ClassName);
};
