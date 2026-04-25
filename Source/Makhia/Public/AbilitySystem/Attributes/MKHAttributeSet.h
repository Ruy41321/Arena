// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "MKHAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * Core attribute set containing character stats (Health, Stamina, Shield)
 * and combat related attributes.
 */
UCLASS()
class MAKHIA_API UMKHAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:

	/** Registers properties for network replication */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	/** Clamps attributes before they are modified */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
	/** Adjusts base/current values when max values change */
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	
	/** Handles execution of gameplay effects (used for damage processing) */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	// -------------------------------------------------------------------
	// Health Attributes
	// -------------------------------------------------------------------
	
	/** Current Health of the character */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health", ReplicatedUsing=OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UMKHAttributeSet, Health)

	/** Maximum Health of the character */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UMKHAttributeSet, MaxHealth)

	// -------------------------------------------------------------------
	// Shield Attributes
	// -------------------------------------------------------------------

	/** Current Shield of the character */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Shield", ReplicatedUsing = OnRep_Shield)
	FGameplayAttributeData Shield;
	ATTRIBUTE_ACCESSORS(UMKHAttributeSet, Shield)

	/** Maximum Shield of the character */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Shield", ReplicatedUsing = OnRep_MaxShield)
	FGameplayAttributeData MaxShield;
	ATTRIBUTE_ACCESSORS(UMKHAttributeSet, MaxShield)

	// -------------------------------------------------------------------
	// Stamina Attributes
	// -------------------------------------------------------------------

	/** Current Stamina of the character */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Stamina", ReplicatedUsing=OnRep_Stamina)
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UMKHAttributeSet, Stamina)

	/** Maximum Stamina of the character */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Stamina", ReplicatedUsing = OnRep_MaxStamina)
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UMKHAttributeSet, MaxStamina)

	/** Cost of a dodge action */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Stamina", ReplicatedUsing = OnRep_DodgeStaminaCost)
	FGameplayAttributeData DodgeStaminaCost;
	ATTRIBUTE_ACCESSORS(UMKHAttributeSet, DodgeStaminaCost)
	
	// -------------------------------------------------------------------
	// Combat Attributes
	// -------------------------------------------------------------------

	/** Chance to score a critical hit (0.0 - 100.0) */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat", ReplicatedUsing = OnRep_CritChance)
	FGameplayAttributeData CritChance;
	ATTRIBUTE_ACCESSORS(UMKHAttributeSet, CritChance);

	/** Damage multiplier when scoring a critical hit */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat", ReplicatedUsing = OnRep_CritDamageMod)
	FGameplayAttributeData CritDamageMod;
	ATTRIBUTE_ACCESSORS(UMKHAttributeSet, CritDamageMod);

	// -------------------------------------------------------------------
	// Meta Attributes
	// -------------------------------------------------------------------

	/** Meta attribute for incoming damage computation. Not replicated. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Meta")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UMKHAttributeSet, IncomingDamage);

private:

	/** 
	 * Processes the incoming damage attribute, applying it to shields and health. 
	 * @param Data Gameplay Effect callback data containing the damage modification. 
	 */
	void HandleIncomingDamage(const FGameplayEffectModCallbackData& Data);

	/**
	 * Calculates the fraction of incoming damage that is absorbed by the Shield (0.0 - ~1.0).
	 *
	 * Example with Reference = 100:
	 * Shield=0     ->   0.00 % absorption  (no shield, Ratio=0)
	 * Shield=50    ->  25.00 % absorption  (Ratio=0.5, linear phase)
	 * Shield=100   ->  50.00 % absorption  (Ratio=1, transition point)
	 * Shield=150   ->  75.00 % absorption  (Ratio=1.5, exponential phase)
	 * Shield=175   ->  82.32 % absorption  (Ratio=1.75)
	 * Shield=200   ->  87.5 % absorption  (Ratio=2)
	 * Shield=âˆž     -> approaches 100 %     (exponential asymptote)
	 *
	 * Tweak ShieldAbsorption_ReferenceShield in the .cpp to shift the curve left/right.
	 *
	 * @param CurrentShield  Current shield value of the target.
	 * @return               Absorption fraction in [0, ~1.0).
	 */
	static float CalculateShieldAbsorption(float CurrentShield);

	/** Scales a current attribute proportionally when its associated max attribute changes. */
	void AdjustAttributeForMaxChange(
		const FGameplayAttribute& AffectedAttribute,
		float OldMaxValue,
		float NewMaxValue) const;

	/** Helper to apply damage mitigating through the shield using CalculateShieldAbsorption()*/
	void ApplyShieldDamageMitigation(UAbilitySystemComponent* ASC, float Damage, float CurrentShield) const;

	/** Helper to handle full shield breaks.
	 * 
	 * All damage beyond the shield value flows through to Health unmitigated.
	 */
	void ApplyShieldBreak(UAbilitySystemComponent* ASC, float Damage, float CurrentShield) const;

	UFUNCTION()
	void OnRep_Shield(const FGameplayAttributeData& OldShield);

	UFUNCTION()
	void OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield);

	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldStamina);

	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina);

	UFUNCTION()
	void OnRep_DodgeStaminaCost(const FGameplayAttributeData& OldDodgeStaminaCost);
	
	UFUNCTION()
	void OnRep_CritChance(const FGameplayAttributeData& OldCritChance);

	UFUNCTION()
	void OnRep_CritDamageMod(const FGameplayAttributeData& OldCritDamageMod);
};
