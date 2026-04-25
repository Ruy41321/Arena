// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "EquipmentTypes.generated.h"

class UGameplayEffect;
class UGameplayAbility;

/** Runtime handles to all GAS grants produced by an equipped item. */
USTRUCT()
struct FEquipmentGrantedHandles
{
	GENERATED_BODY()

	/** Ability spec handles granted while the item is equipped. */
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> GrantedAbilities = TArray<FGameplayAbilitySpecHandle>();

	/** Active gameplay effect handles applied while the item is equipped. */
	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> ActiveEffects = TArray<FActiveGameplayEffectHandle>();

	/** Stores a newly granted ability handle for later removal on unequip. */
	void AddAbilityHandle(const FGameplayAbilitySpecHandle& Handle)
	{
		GrantedAbilities.Add(Handle);
	}

	/** Stores a newly applied effect handle for later removal on unequip. */
	void AddEffectHandle(const FActiveGameplayEffectHandle& Handle)
	{
		ActiveEffects.Add(Handle);
	}
};

/** DataTable row describing one rollable passive stat effect for equipment. */
USTRUCT(BlueprintType)
struct FEquipmentStatEffectDefinition : public FTableRowBase
{
	GENERATED_BODY()

	/** Unique gameplay tag used to identify this stat effect definition. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag StatEffectTag = FGameplayTag();

	/** Friendly attribute label shown in UI. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText AttributeName = FText();

	/** Gameplay effect class applied when this stat roll is selected. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftClassPtr<UGameplayEffect> EffectClass = nullptr;

	/** Minimum roll value for this stat definition. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float MinStatLevel = 1.f;

	/** Maximum roll value for this stat definition. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float MaxStatLevel = 1.f;

	/** Whether this stat allows fractional values instead of integer-style rolls. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bFractionalStat = false;

	/** Relative probability weight used by the roll selection logic. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float ProbabilityToSelect = 0.f;

	/** Runtime rolled value selected for this specific equipment entry. */
	UPROPERTY(BlueprintReadOnly)
	float CurrentValue = 0.f;

};

/** DataTable row describing one rollable active/passive ability for equipment. */
USTRUCT(BlueprintType)
struct  FEquipmentAbilityDefinition : public FTableRowBase
{
	GENERATED_BODY()

	/** Unique gameplay tag used to identify this ability definition. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag AbilityTag = FGameplayTag();

	/** True when this ability is a skill that should appear in skill UI slots. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bIsSkillAbility = false;
	
	/** Optional skill icon used when the ability is marked as a skill ability. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bIsSkillAbility", EditConditionHides))
	TObjectPtr<UTexture2D> Icon = nullptr;
	
	/** Cooldown to apply to skills on use. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bIsSkillAbility", EditConditionHides))
	float CooldownTime = 0.f;
	
	/** Cooldown tag to grants when the ability is on CD. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bIsSkillAbility", EditConditionHides))
	FGameplayTag CooldownTag = FGameplayTag();
	
	/** Input tag assigned dynamically at runtime when the skill is granted. */
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag SkillInputTag = FGameplayTag();
	
	/** Localized ability name displayed in UI. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText AbilityName = FText();

	/** Ability class to grant when this roll is selected. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftClassPtr<UGameplayAbility> AbilityClass = nullptr;

	/** Damage multiplier applied by damage abilities granted from this definition. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float DamagePercent = 1.f;

	/** Context tag used by ability logic to identify attack context/type. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag ContextTag = FGameplayTag();

	/** Relative probability weight used by the roll selection logic. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float ProbabilityToSelect = 0.f;

};

/** Runtime package containing all rolled stat effects and abilities for one equipped item. */
USTRUCT(BlueprintType)
struct FEquipmentEffectPackage
{
	GENERATED_BODY()

	/** Rolled passive stat effects currently associated with the equipped item. */
	UPROPERTY(BlueprintReadOnly)
	TArray<FEquipmentStatEffectDefinition> StatEffects = TArray<FEquipmentStatEffectDefinition>();

	/** Rolled abilities currently associated with the equipped item. */
	UPROPERTY(BlueprintReadOnly)
	TArray<FEquipmentAbilityDefinition> Abilities = TArray<FEquipmentAbilityDefinition>();
	
};