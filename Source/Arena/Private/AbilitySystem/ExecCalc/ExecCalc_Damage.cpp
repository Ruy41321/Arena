// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ExecCalc/ExecCalc_Damage.h"

#include "AbilitySystem/RPGGameplayTags.h"
#include "AbilitySystem/Attributes/RPGAttributeSet.h"  
#include "AbilitySystem/RPGAbilityTypes.h"

struct RPGDamageStatics
{
	// Source Captures
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritChance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritDamageMod);

	// Target Captures
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DamageReduction);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Shield);

	RPGDamageStatics()
	{
		// Source Defines
		DEFINE_ATTRIBUTE_CAPTUREDEF(URPGAttributeSet, CritChance, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(URPGAttributeSet, CritDamageMod, Source, true);

		// Target Defines
		DEFINE_ATTRIBUTE_CAPTUREDEF(URPGAttributeSet, IncomingDamage, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(URPGAttributeSet, DamageReduction, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(URPGAttributeSet, Shield, Target, false);
		
	}
};

static const RPGDamageStatics& DamageStatics()
{
	static RPGDamageStatics DStatics;
	return DStatics;
}

UExecCalc_Damage::UExecCalc_Damage()
{
	// Source Captures
	RelevantAttributesToCapture.Add(DamageStatics().CritChanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().CritDamageModDef);

	// Target Captures
	RelevantAttributesToCapture.Add(DamageStatics().IncomingDamageDef);
	RelevantAttributesToCapture.Add(DamageStatics().DamageReductionDef);
	RelevantAttributesToCapture.Add(DamageStatics().ShieldDef);
}

void UExecCalc_Damage::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, 
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& EffectSpec = ExecutionParams.GetOwningSpec();

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = EffectSpec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = EffectSpec.CapturedTargetTags.GetAggregatedTags();

	const FGameplayEffectContextHandle EffectContextHandle = EffectSpec.GetContext();
	FRPGGameplayEffectContext* RPGContext = FRPGGameplayEffectContext::GetEffectContext(EffectContextHandle);

	// Get raw damage from the SetByCaller data passed when applying the GameplayEffect
	float Damage = EffectSpec.GetSetByCallerMagnitude(RPGGameplayTags::Combat::Data_Damage);
	Damage = FMath::Max<float>(Damage, 0.f);

	// Source Captures
	float CritChance = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CritChanceDef, EvalParams, CritChance);
	CritChance = FMath::Max<float>(CritChance, 0.f);

	float CritDamageMod = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CritDamageModDef, EvalParams, CritDamageMod);
	CritDamageMod = FMath::Max<float>(CritDamageMod, 0.f);

	// Target Captures
	float Shield = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ShieldDef, EvalParams, Shield);
	Shield = FMath::Max<float>(Shield, 0.f);

	float DamageReduction = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DamageReductionDef, EvalParams, DamageReduction);
	DamageReduction = FMath::Max<float>(DamageReduction, 0.f);

	// Begin Calculation

	const bool bCriticalHit = FMath::RandRange(0, 100) < CritChance;
	Damage = bCriticalHit ? Damage * (1.f + CritDamageMod) : Damage;
	RPGContext->SetIsCriticalHit(bCriticalHit);

	if (Damage > 0.f && Shield > 0.f)
	{
		Damage *= (100 - DamageReduction) / 100.f;
	}
		
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(DamageStatics().IncomingDamageProperty, EGameplayModOp::Additive, Damage));
}
