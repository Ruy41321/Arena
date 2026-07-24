// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/MKHAttributeSet.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystem/MKHAbilityTypes.h"
#include "AbilitySystem/MKHGameplayTags.h"
#include "Libraries/MKHAbilitySystemLibrary.h"

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
	DOREPLIFETIME_CONDITION_NOTIFY(UMKHAttributeSet, AdditiveBaseDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMKHAttributeSet, MaxAdditiveBaseDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMKHAttributeSet, Empower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMKHAttributeSet, MaxEmpower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMKHAttributeSet, Weaken, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMKHAttributeSet, MaxWeaken, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMKHAttributeSet, Reinforced, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMKHAttributeSet, MaxReinforced, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMKHAttributeSet, Exposed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMKHAttributeSet, MaxExposed, COND_None, REPNOTIFY_Always);
}

void UMKHAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UMKHAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UMKHAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
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
	else if (Attribute == GetAdditiveBaseDamageAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxAdditiveBaseDamage());
	}
	else if (Attribute == GetEmpowerAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxEmpower());
	}
	else if (Attribute == GetWeakenAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxWeaken());
	}
	else if (Attribute == GetReinforcedAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxReinforced());
	}
	else if (Attribute == GetExposedAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxExposed());
	}
}

void UMKHAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		HandleIncomingDamage(Data);
	}
	else if (Data.EvaluatedData.Attribute == GetIncomingStaminaDamageAttribute())
	{
		HandleIncomingStaminaDamage(Data);
	}

	// Only executions that can move Health are relevant: the damage pipeline and any direct write
	// to Health (drains, heals). PostGameplayEffectExecute runs once per modifier of the spec.
	const bool bAffectsHealth = Data.EvaluatedData.Attribute == GetIncomingDamageAttribute()
		|| Data.EvaluatedData.Attribute == GetHealthAttribute();

	if (bAffectsHealth)
	{
		TrySendDeathEvent(GetOwningAbilitySystemComponent());
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
	// Buff/debuff caps: percentages are not scaled proportionally, only re-clamped to the new cap.
	else if (Attribute == GetMaxAdditiveBaseDamageAttribute())
	{
		ClampAttributeDataToMax(AdditiveBaseDamage, NewValue);
	}
	else if (Attribute == GetMaxEmpowerAttribute())
	{
		ClampAttributeDataToMax(Empower, NewValue);
	}
	else if (Attribute == GetMaxWeakenAttribute())
	{
		ClampAttributeDataToMax(Weaken, NewValue);
	}
	else if (Attribute == GetMaxReinforcedAttribute())
	{
		ClampAttributeDataToMax(Reinforced, NewValue);
	}
	else if (Attribute == GetMaxExposedAttribute())
	{
		ClampAttributeDataToMax(Exposed, NewValue);
	}
}

void UMKHAttributeSet::ClampAttributeDataToMax(FGameplayAttributeData& AttributeData, const float NewMaxValue)
{
	AttributeData.SetBaseValue(FMath::Clamp(AttributeData.GetBaseValue(), 0.f, NewMaxValue));
	AttributeData.SetCurrentValue(FMath::Clamp(AttributeData.GetCurrentValue(), 0.f, NewMaxValue));
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

// Shield damage scaling tuning. Full model, invariants and balance tables:
// see Documentation/ShieldDamageScaling.md

/** How many times faster the Shield drains its own pool than Health: a full shield breaks at (1 - 1/Ratio) health. */
static constexpr float ShieldWear_DrainRatio = 2.f;

/** Extra pool spent per absorbed point once the shield is empty: a battered shield protects less efficiently. */
static constexpr float ShieldWear_DegradationPenalty = 1.f;

/** Cap on the rebalancing boost applied when the shield is proportionally healthier than Health. */
static constexpr float ShieldBalance_MaxBoost = 10.f;

/** Incoming damage above (CurrentShield * this) shatters the whole pool instead of chipping it. */
static constexpr float ShieldBreak_DamageMultiplier = 2.f;

float UMKHAttributeSet::CalculateShieldDrainRatio(const float ShieldFraction, const float HealthFraction)
{
	// Boosted only while the shield is proportionally ahead of health, so it spends itself to rebalance the bars.
	const float BalanceBoost = ShieldFraction / FMath::Max(HealthFraction, UE_KINDA_SMALL_NUMBER);
	return ShieldWear_DrainRatio * FMath::Clamp(BalanceBoost, 1.f, ShieldBalance_MaxBoost);
}

float UMKHAttributeSet::CalculateShieldWearFactor(const float ShieldFraction)
{
	return 1.f + ShieldWear_DegradationPenalty * (1.f - FMath::Clamp(ShieldFraction, 0.f, 1.f));
}

void UMKHAttributeSet::CalculateDamageSplit(const float Damage, float& OutHealthDamage, float& OutShieldDamage) const
{
	const float MaxShieldValue = GetMaxShield();
	const float MaxHealthValue = GetMaxHealth();
	const float CurrentShield = GetShield();

	OutShieldDamage = 0.f;
	OutHealthDamage = FMath::Max(0.f, Damage);

	// No pool to spend (or uninitialised attributes): the hit lands entirely on Health.
	if (CurrentShield <= 0.f || MaxShieldValue <= 0.f || MaxHealthValue <= 0.f)
	{
		return;
	}

	const float ShieldFraction = CurrentShield / MaxShieldValue;
	const float DrainRatio = CalculateShieldDrainRatio(ShieldFraction, GetHealth() / MaxHealthValue);
	const float WearFactor = CalculateShieldWearFactor(ShieldFraction);

	// Split so that, as a fraction of their own pool, the shield drains exactly DrainRatio times faster than Health.
	const float ShieldWeight = DrainRatio * MaxShieldValue;
	const float AbsorbedDamage = OutHealthDamage * ShieldWeight / (WearFactor * MaxHealthValue + ShieldWeight);

	OutShieldDamage = FMath::Clamp(WearFactor * AbsorbedDamage, 0.f, CurrentShield);
	OutHealthDamage = FMath::Max(0.f, OutHealthDamage - AbsorbedDamage);
}

float UMKHAttributeSet::GetShieldDamageReduction() const
{
	float HealthDamage = 0.f;
	float ShieldDamage = 0.f;
	CalculateDamageSplit(1.f, HealthDamage, ShieldDamage);

	return 1.f - HealthDamage;
}

void UMKHAttributeSet::HandleIncomingDamage(const FGameplayEffectModCallbackData& Data)
{
	const float LocalDamage = GetIncomingDamage();
	SetIncomingDamage(0.f);

	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		return;
	}

	const FGameplayEffectContextHandle ContextHandle = Data.EffectSpec.GetContext();
	UAbilitySystemComponent* SourceASC = ContextHandle.GetInstigatorAbilitySystemComponent();
	const FGameplayEventData EventData = BuildCombatEventData(ContextHandle, SourceASC, ASC, LocalDamage);

	// Perfect dodge (flagged by ExecCalc_Damage): notify both actors, apply nothing.
	const FMKHGameplayEffectContext* MKHContext = FMKHGameplayEffectContext::GetEffectContext(ContextHandle);
	if (MKHContext && MKHContext->IsDodged())
	{
		SendCombatEvent(ASC, MKHGameplayTags::Event::AttackDodged, EventData);
		SendCombatEvent(SourceASC, MKHGameplayTags::Event::AttackDodged, EventData);
		return;
	}

	if (LocalDamage <= 0.f)
	{
		return;
	}

	const float CurrentShield = GetShield();

	if (CurrentShield > 0.f)
	{
		// A hit far above the remaining pool shatters it instead of chipping it.
		if (LocalDamage >= CurrentShield * ShieldBreak_DamageMultiplier)
		{
			ApplyShieldBreak(ASC, LocalDamage, CurrentShield);
		}
		else
		{
			ApplyShieldDamageMitigation(ASC, LocalDamage);
		}
	}
	else
	{
		// No shield: all damage goes directly to Health
		ASC->ApplyModToAttribute(GetHealthAttribute(), EGameplayModOp::Additive, -LocalDamage);
	}

	// Post-damage side effects: hit notifications and on-hit status effects.
	SendCombatEvent(ASC, MKHGameplayTags::Event::HitReceived, EventData);
	SendCombatEvent(SourceASC, MKHGameplayTags::Event::HitInflicted, EventData);
	ApplyStatusEffectsFromContext(ContextHandle, SourceASC, ASC);
}

FGameplayEventData UMKHAttributeSet::BuildCombatEventData(const FGameplayEffectContextHandle& ContextHandle,
	UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, const float Magnitude)
{
	FGameplayEventData EventData;
	EventData.ContextHandle = ContextHandle;
	EventData.Instigator = IsValid(SourceASC) ? SourceASC->GetAvatarActor() : nullptr;
	EventData.Target = IsValid(TargetASC) ? TargetASC->GetAvatarActor() : nullptr;
	EventData.EventMagnitude = Magnitude;
	return EventData;
}

void UMKHAttributeSet::SendCombatEvent(UAbilitySystemComponent* RecipientASC, const FGameplayTag& EventTag,
	const FGameplayEventData& EventData)
{
	if (IsValid(RecipientASC))
	{
		RecipientASC->HandleGameplayEvent(EventTag, &EventData);
	}
}

void UMKHAttributeSet::ApplyStatusEffectsFromContext(const FGameplayEffectContextHandle& ContextHandle,
	UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC) const
{
	const FMKHGameplayEffectContext* MKHContext = FMKHGameplayEffectContext::GetEffectContext(ContextHandle);
	if (!MKHContext)
	{
		return;
	}

	// Target-facing effects land on the hit target; self on-hit effects land back on the
	// attacker. Both run after the damage pipeline, so a self on-hit buff (e.g. Empower)
	// never affects the damage of the hit that triggered it.
	UMKHAbilitySystemLibrary::ApplyStatusEffects(SourceASC, TargetASC, MKHContext->GetTargetStatusEffects(), ContextHandle);
	UMKHAbilitySystemLibrary::ApplyStatusEffects(SourceASC, SourceASC, MKHContext->GetSelfOnHitStatusEffects(), ContextHandle);
}

void UMKHAttributeSet::TrySendDeathEvent(UAbilitySystemComponent* ASC)
{
	// Damage effects only execute on the server already, but check authority explicitly
	// in case a predicted path is ever added on top of this.
	AActor* Avatar = IsValid(ASC) ? ASC->GetAvatarActor() : nullptr;
	if (!IsValid(Avatar) || !Avatar->HasAuthority())
	{
		return;
	}

	if (GetHealth() > 0.f)
	{
		// Alive: arm the notification for the next time health is depleted.
		bHasBeenAlive = true;
		bOutOfHealth = false;
		return;
	}

	// Zero health before the class defaults have run means the character was never alive yet.
	if (!bHasBeenAlive || bOutOfHealth || ASC->HasMatchingGameplayTag(MKHGameplayTags::State::Movement::Dead))
	{
		return;
	}

	bOutOfHealth = true;

	FGameplayEventData Payload;
	Payload.EventTag = MKHGameplayTags::Event::Death;
	Payload.Instigator = Avatar;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Avatar, MKHGameplayTags::Event::Death, Payload);
}

void UMKHAttributeSet::HandleIncomingStaminaDamage(const FGameplayEffectModCallbackData& Data)
{
	const float LocalDamage = GetIncomingStaminaDamage();
	SetIncomingStaminaDamage(0.f);

	if (LocalDamage <= 0.f)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = GetOwningAbilitySystemComponent();
	UAbilitySystemComponent* SourceASC = Data.EffectSpec.GetContext().GetInstigatorAbilitySystemComponent();
	if (!IsValid(TargetASC))
	{
		return;
	}

	const float CurrentStamina = GetStamina();
	const FGameplayEventData EventData = BuildCombatEventData(Data.EffectSpec.GetContext(), SourceASC, TargetASC, LocalDamage);

	// Always notify a successful block first
	SendCombatEvent(TargetASC, MKHGameplayTags::Event::BlockSuccessful, EventData);
	SendCombatEvent(SourceASC, MKHGameplayTags::Event::BlockSuccessful, EventData);

	if (LocalDamage >= CurrentStamina)
	{
		// Guard break scenario: Stamina depleted
		TargetASC->ApplyModToAttribute(GetStaminaAttribute(), EGameplayModOp::Additive, -CurrentStamina);

		SendCombatEvent(TargetASC, MKHGameplayTags::Event::GuardBreak, EventData);
		SendCombatEvent(SourceASC, MKHGameplayTags::Event::GuardBreak, EventData);
	}
	else
	{
		// Normal block: subtract stamina
		TargetASC->ApplyModToAttribute(GetStaminaAttribute(), EGameplayModOp::Additive, -LocalDamage);
	}
}

void UMKHAttributeSet::ApplyShieldBreak(UAbilitySystemComponent* ASC, const float Damage, const float CurrentShield) const
{
	if (!IsValid(ASC)) return;

	// The armor still mitigates the blow that lands on it, then the whole pool shatters.
	float HealthDamage = 0.f;
	float ShieldDamage = 0.f;
	CalculateDamageSplit(Damage, HealthDamage, ShieldDamage);

	ASC->ApplyModToAttribute(GetShieldAttribute(), EGameplayModOp::Additive, -CurrentShield);

	if (HealthDamage > 0.f)
	{
		ASC->ApplyModToAttribute(GetHealthAttribute(), EGameplayModOp::Additive, -FMath::RoundToFloat(HealthDamage));
	}
}

void UMKHAttributeSet::ApplyShieldDamageMitigation(UAbilitySystemComponent* ASC, const float Damage) const
{
	if (!IsValid(ASC)) return;

	float HealthDamage = 0.f;
	float ShieldDamage = 0.f;
	CalculateDamageSplit(Damage, HealthDamage, ShieldDamage);

	ASC->ApplyModToAttribute(GetShieldAttribute(), EGameplayModOp::Additive, -FMath::RoundToFloat(ShieldDamage));
	ASC->ApplyModToAttribute(GetHealthAttribute(), EGameplayModOp::Additive, -FMath::RoundToFloat(HealthDamage));
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

void UMKHAttributeSet::OnRep_AdditiveBaseDamage(const FGameplayAttributeData& OldAdditiveBaseDamage)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMKHAttributeSet, AdditiveBaseDamage, OldAdditiveBaseDamage);
}

void UMKHAttributeSet::OnRep_MaxAdditiveBaseDamage(const FGameplayAttributeData& OldMaxAdditiveBaseDamage)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMKHAttributeSet, MaxAdditiveBaseDamage, OldMaxAdditiveBaseDamage);
}

void UMKHAttributeSet::OnRep_Empower(const FGameplayAttributeData& OldEmpower)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMKHAttributeSet, Empower, OldEmpower);
}

void UMKHAttributeSet::OnRep_MaxEmpower(const FGameplayAttributeData& OldMaxEmpower)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMKHAttributeSet, MaxEmpower, OldMaxEmpower);
}

void UMKHAttributeSet::OnRep_Weaken(const FGameplayAttributeData& OldWeaken)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMKHAttributeSet, Weaken, OldWeaken);
}

void UMKHAttributeSet::OnRep_MaxWeaken(const FGameplayAttributeData& OldMaxWeaken)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMKHAttributeSet, MaxWeaken, OldMaxWeaken);
}

void UMKHAttributeSet::OnRep_Reinforced(const FGameplayAttributeData& OldReinforced)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMKHAttributeSet, Reinforced, OldReinforced);
}

void UMKHAttributeSet::OnRep_MaxReinforced(const FGameplayAttributeData& OldMaxReinforced)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMKHAttributeSet, MaxReinforced, OldMaxReinforced);
}

void UMKHAttributeSet::OnRep_Exposed(const FGameplayAttributeData& OldExposed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMKHAttributeSet, Exposed, OldExposed);
}

void UMKHAttributeSet::OnRep_MaxExposed(const FGameplayAttributeData& OldMaxExposed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMKHAttributeSet, MaxExposed, OldMaxExposed);
}
