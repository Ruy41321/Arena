// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "RPGAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 
 */
UCLASS()
class ARENA_API URPGAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;


	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(URPGAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(URPGAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Shield)
	FGameplayAttributeData Shield;
	ATTRIBUTE_ACCESSORS(URPGAttributeSet, Shield)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxShield)
	FGameplayAttributeData MaxShield;
	ATTRIBUTE_ACCESSORS(URPGAttributeSet, MaxShield)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Stamina)
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(URPGAttributeSet, Stamina)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStamina)
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(URPGAttributeSet, MaxStamina)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DodgeStaminaCost)
	FGameplayAttributeData DodgeStaminaCost;
	ATTRIBUTE_ACCESSORS(URPGAttributeSet, DodgeStaminaCost)
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CritChance)
	FGameplayAttributeData CritChance;
	ATTRIBUTE_ACCESSORS(URPGAttributeSet, CritChance);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CritDamageMod)
	FGameplayAttributeData CritDamageMod;
	ATTRIBUTE_ACCESSORS(URPGAttributeSet, CritDamageMod);

	UPROPERTY()
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(URPGAttributeSet, IncomingDamage);

private:

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
	 * Shield=∞     -> approaches 100 %     (exponential asymptote)
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
