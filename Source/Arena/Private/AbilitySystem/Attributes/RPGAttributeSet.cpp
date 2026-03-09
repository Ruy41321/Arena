// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/RPGAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystem/RPGAbilityTypes.h"

void URPGAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(URPGAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URPGAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URPGAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URPGAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URPGAttributeSet, DodgeStaminaCost, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URPGAttributeSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URPGAttributeSet, MaxShield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URPGAttributeSet, CritChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URPGAttributeSet, CritDamageMod, COND_None, REPNOTIFY_Always);
}

void URPGAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
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

void URPGAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		HandleIncomingDamage(Data);
	}
}

void URPGAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
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

void URPGAttributeSet::AdjustAttributeForMaxChange(
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

// ─── Shield Absorption Tuning ────────────────────────────────────────────────
// CalculateShieldAbsorption() uses a hybrid approach:
//   - LINEAR phase for Shield ≤ ReferenceShield: Absorption = 0.5 * Ratio
//   - EXPONENTIAL phase for Shield > ReferenceShield: Absorption → 100% (asymptotic)
//
// ShieldAbsorption_ReferenceShield: Threshold shield value that transitions between phases.
//   Ratio = CurrentShield / ReferenceShield
//   - If Ratio ≤ 1: Absorption = 0.5 * Ratio (linear, 0% at Ratio=0, 50% at Ratio=1)
//   - If Ratio > 1: Absorption = 1 - 0.5^(2*Ratio - 1) (exponential, approaches 100%)
//
// Linear phase (0 to Reference): gradual ramp from 0% to 50%
// Exponential phase (Reference+): rapid approach to 100%, each additional Reference halves remaining gap
// ─────────────────────────────────────────────────────────────────────────────
static constexpr float ShieldAbsorption_ReferenceShield = 100.f;  // Shield value at 50% absorption (half-life)

// ─── Shield Break Tuning ─────────────────────────────────────────────────────
// When incoming damage exceeds (CurrentShield * ShieldBreak_DamageMultiplier),
// a Shield Break occurs: the full shield is destroyed instantly and only the
// surplus damage (Damage - CurrentShield) carries over to Health.
// Raise this multiplier to make shield breaks harder to trigger.
// ─────────────────────────────────────────────────────────────────────────────
static constexpr float ShieldBreak_DamageMultiplier = 2.f;

float URPGAttributeSet::CalculateShieldAbsorption(const float CurrentShield)
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
	//   Shield=∞     -> approaches 100 %     (exponential asymptote)
	const float Ratio = CurrentShield / ShieldAbsorption_ReferenceShield;
	if (Ratio <= 1.f)
	{
		return 0.5f * Ratio;  // Linear approximation for low shield values (0 to 50% absorption)
	}
	return 1.0f - FMath::Pow(0.5f, (Ratio * 2.f - 1.f));  // Exponential curve for higher shield values (approaching 100% absorption)
}

void URPGAttributeSet::HandleIncomingDamage(const FGameplayEffectModCallbackData& Data)
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
		// ── Shield Break check ────────────────────────────────────────────────
		// If damage exceeds (Shield * Multiplier) the shield shatters completely.
		// All damage beyond the shield value flows through to Health unmitigated.
		if (LocalDamage >= CurrentShield * ShieldBreak_DamageMultiplier)
		{
			// Destroy the full shield
			ASC->ApplyModToAttribute(GetShieldAttribute(), EGameplayModOp::Additive, -CurrentShield);

			// Only the surplus damage (beyond the shield value) hits Health
			const float HealthDamage = LocalDamage - CurrentShield;
			if (HealthDamage > 0.f)
			{
				ASC->ApplyModToAttribute(GetHealthAttribute(), EGameplayModOp::Additive, -HealthDamage);
			}
		}
		else
		{
			// ── Normal shield mitigation ──────────────────────────────────────
			// The shield absorbs a percentage of the damage; the rest hits Health.
			// Example: Shield=100 -> 50 % absorption, Damage=50 -> 25 to shield, 25 to HP.
			const float AbsorptionRate  = CalculateShieldAbsorption(CurrentShield);
			const float ShieldDamage    = FMath::RoundToFloat(LocalDamage * AbsorptionRate);
			const float HealthDamage    = LocalDamage - ShieldDamage;

			ASC->ApplyModToAttribute(GetShieldAttribute(), EGameplayModOp::Additive, -ShieldDamage);
			ASC->ApplyModToAttribute(GetHealthAttribute(), EGameplayModOp::Additive, -HealthDamage);
		}
	}
	else
	{
		// No shield: all damage goes directly to Health
		ASC->ApplyModToAttribute(GetHealthAttribute(), EGameplayModOp::Additive, -LocalDamage);
	}
}

void URPGAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URPGAttributeSet, Shield, OldShield);
}

void URPGAttributeSet::OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URPGAttributeSet, MaxShield, OldMaxShield);
}

void URPGAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URPGAttributeSet, Health, OldHealth);
}

void URPGAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URPGAttributeSet, MaxHealth, OldMaxHealth);
}

void URPGAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URPGAttributeSet, Stamina, OldStamina);
}

void URPGAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URPGAttributeSet, MaxStamina, OldMaxStamina);
}

void URPGAttributeSet::OnRep_DodgeStaminaCost(const FGameplayAttributeData& OldDodgeStaminaCost)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URPGAttributeSet, DodgeStaminaCost, OldDodgeStaminaCost);
}

void URPGAttributeSet::OnRep_CritChance(const FGameplayAttributeData& OldCritChance)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URPGAttributeSet, CritChance, OldCritChance);
}

void URPGAttributeSet::OnRep_CritDamageMod(const FGameplayAttributeData& OldCritDamageMod)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URPGAttributeSet, CritDamageMod, OldCritDamageMod);
}
