// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/MKHAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystem/MKHAbilityTypes.h"

void UMKHAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UMKHAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMKHAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMKHAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMKHAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMKHAttributeSet, DodgeStaminaCost, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMKHAttributeSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMKHAttributeSet, MaxShield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMKHAttributeSet, CritChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMKHAttributeSet, CritDamageMod, COND_None, REPNOTIFY_Always);
}

void UMKHAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	}
	else if (Attribute == GetShieldAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxShield());
	}
}

void UMKHAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		HandleIncomingDamage(Data);
	}
}

void UMKHAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// When max attributes change, scale the current attribute proportionally to maintain the same percentage
	// and clamp BaseValue to not exceed the new max.
	if (Attribute == GetMaxHealthAttribute())
	{
		AdjustAttributeForMaxChange(GetHealthAttribute(), OldValue, NewValue);
		const float ClampedHealth = FMath::Clamp(GetHealth(), 0.f, NewValue);
		Health.SetBaseValue(FMath::Clamp(Health.GetBaseValue(), 0.f, NewValue));
		Health.SetCurrentValue(ClampedHealth);
	}
	else if (Attribute == GetMaxShieldAttribute())
	{
		AdjustAttributeForMaxChange(GetShieldAttribute(), OldValue, NewValue);
		const float ClampedShield = FMath::Clamp(GetShield(), 0.f, NewValue);
		Shield.SetBaseValue(FMath::Clamp(Shield.GetBaseValue(), 0.f, NewValue));
		Shield.SetCurrentValue(ClampedShield);
	}
	else if (Attribute == GetMaxStaminaAttribute())
	{
		AdjustAttributeForMaxChange(GetStaminaAttribute(), OldValue, NewValue);
		const float ClampedStamina = FMath::Clamp(GetStamina(), 0.f, NewValue);
		Stamina.SetBaseValue(FMath::Clamp(Stamina.GetBaseValue(), 0.f, NewValue));
		Stamina.SetCurrentValue(ClampedStamina);
	}
}

void UMKHAttributeSet::AdjustAttributeForMaxChange(
	const FGameplayAttribute& AffectedAttribute,
	const float OldMaxValue,
	const float NewMaxValue) const
{
	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!IsValid(ASC) || FMath::IsNearlyEqual(OldMaxValue, NewMaxValue) || OldMaxValue <= 0.f)
	{
		return;
	}

	// Maintain the same percentage of current / max
	// e.g. Health=50, MaxHealth=100 (50%), MaxHealth changes to 150 -> Health becomes 75 (still 50%)
	const float CurrentValue = ASC->GetNumericAttribute(AffectedAttribute);
	const float Ratio = CurrentValue / OldMaxValue;
	const float NewCurrentValue = FMath::RoundToFloat(Ratio * NewMaxValue);

	ASC->ApplyModToAttribute(AffectedAttribute, EGameplayModOp::Override, NewCurrentValue);
}

// â”€â”€â”€ Shield Absorption Tuning â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// CalculateShieldAbsorption() uses a hybrid approach:
//   - LINEAR phase for Shield â‰¤ ReferenceShield: Absorption = 0.5 * Ratio
//   - EXPONENTIAL phase for Shield > ReferenceShield: Absorption â†’ 100% (asymptotic)
//
// ShieldAbsorption_ReferenceShield: Threshold shield value that transitions between phases.
//   Ratio = CurrentShield / ReferenceShield
//   - If Ratio â‰¤ 1: Absorption = 0.5 * Ratio (linear, 0% at Ratio=0, 50% at Ratio=1)
//   - If Ratio > 1: Absorption = 1 - 0.5^(2*Ratio - 1) (exponential, approaches 100%)
//
// Linear phase (0 to Reference): gradual ramp from 0% to 50%
// Exponential phase (Reference+): rapid approach to 100%, each additional Reference halves remaining gap
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
static constexpr float ShieldAbsorption_ReferenceShield = 100.f;  // Shield value at 50% absorption (half-life)

// â”€â”€â”€ Shield Break Tuning â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// When incoming damage exceeds (CurrentShield * ShieldBreak_DamageMultiplier),
// a Shield Break occurs: the full shield is destroyed instantly and only the
// surplus damage (Damage - CurrentShield) carries over to Health.
// Raise this multiplier to make shield breaks harder to trigger.
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
static constexpr float ShieldBreak_DamageMultiplier = 2.f;

float UMKHAttributeSet::CalculateShieldAbsorption(const float CurrentShield)
{
	if (CurrentShield <= 0.f)
	{
		return 0.f;
	}

	// Example with Reference = 100:
	//   Shield=0     ->   0.00 % absorption  (no shield, Ratio=0)
	//   Shield=50    ->  25.00 % absorption  (Ratio=0.5, linear phase)
	//   Shield=100   ->  50.00 % absorption  (Ratio=1, transition point)
	//   Shield=150   ->  75.00 % absorption  (Ratio=1.5, exponential phase)
	//   Shield=175   ->  82.32 % absorption  (Ratio=1.75)
	//   Shield=200   ->  87.5 % absorption  (Ratio=2)
	//   Shield=âˆž     -> approaches 100 %     (exponential asymptote)
	const float Ratio = CurrentShield / ShieldAbsorption_ReferenceShield;
	if (Ratio <= 1.f)
	{
		return 0.5f * Ratio;  // Linear approximation for low shield values (0 to 50% absorption)
	}
	return 1.0f - FMath::Pow(0.5f, (Ratio * 2.f - 1.f));  // Exponential curve for higher shield values (approaching 100% absorption)
}

void UMKHAttributeSet::HandleIncomingDamage(const FGameplayEffectModCallbackData& Data)
{
	float LocalDamage = GetIncomingDamage();
	SetIncomingDamage(0.f);

	if (LocalDamage <= 0.f)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		return;
	}

	const float CurrentShield = GetShield();

	if (CurrentShield > 0.f)
	{
		/** Shield Break check
		* If damage exceeds (Shield * Multiplier) the shield shatters completely.
		* All damage beyond the shield value flows through to Health unmitigated.
		*/
		if (LocalDamage >= CurrentShield * ShieldBreak_DamageMultiplier)
		{
			ApplyShieldBreak(ASC, LocalDamage, CurrentShield);
		}
		else
		{
			ApplyShieldDamageMitigation(ASC, LocalDamage, CurrentShield);
		}
	}
	else
	{
		// No shield: all damage goes directly to Health
		ASC->ApplyModToAttribute(GetHealthAttribute(), EGameplayModOp::Additive, -LocalDamage);
	}
}

void UMKHAttributeSet::ApplyShieldBreak(UAbilitySystemComponent* ASC, float Damage, float CurrentShield) const
{
	if (!IsValid(ASC)) return;

	// Destroy the full shield
	ASC->ApplyModToAttribute(GetShieldAttribute(), EGameplayModOp::Additive, -CurrentShield);

	// Only the surplus damage (beyond the shield value) hits Health
	const float HealthDamage = Damage - CurrentShield;
	if (HealthDamage > 0.f)
	{
		ASC->ApplyModToAttribute(GetHealthAttribute(), EGameplayModOp::Additive, -HealthDamage);
	}
}

void UMKHAttributeSet::ApplyShieldDamageMitigation(UAbilitySystemComponent* ASC, float Damage, float CurrentShield) const
{
	if (!IsValid(ASC)) return;

	// The shield absorbs a percentage of the damage; the rest hits Health.
	const float AbsorptionRate  = CalculateShieldAbsorption(CurrentShield);
	const float ShieldDamage    = FMath::RoundToFloat(Damage * AbsorptionRate);
	const float HealthDamage    = Damage - ShieldDamage;

	ASC->ApplyModToAttribute(GetShieldAttribute(), EGameplayModOp::Additive, -ShieldDamage);
	ASC->ApplyModToAttribute(GetHealthAttribute(), EGameplayModOp::Additive, -HealthDamage);
}

void UMKHAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMKHAttributeSet, Shield, OldShield);
}

void UMKHAttributeSet::OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMKHAttributeSet, MaxShield, OldMaxShield);
}

void UMKHAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMKHAttributeSet, Health, OldHealth);
}

void UMKHAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMKHAttributeSet, MaxHealth, OldMaxHealth);
}

void UMKHAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMKHAttributeSet, Stamina, OldStamina);
}

void UMKHAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMKHAttributeSet, MaxStamina, OldMaxStamina);
}

void UMKHAttributeSet::OnRep_DodgeStaminaCost(const FGameplayAttributeData& OldDodgeStaminaCost)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMKHAttributeSet, DodgeStaminaCost, OldDodgeStaminaCost);
}

void UMKHAttributeSet::OnRep_CritChance(const FGameplayAttributeData& OldCritChance)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMKHAttributeSet, CritChance, OldCritChance);
}

void UMKHAttributeSet::OnRep_CritDamageMod(const FGameplayAttributeData& OldCritDamageMod)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMKHAttributeSet, CritDamageMod, OldCritDamageMod);
}
