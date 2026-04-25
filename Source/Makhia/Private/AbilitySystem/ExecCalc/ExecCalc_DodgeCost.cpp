// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ExecCalc/ExecCalc_DodgeCost.h"

#include "AbilitySystem/Attributes/MKHAttributeSet.h"

struct StaminaStatics
{
	// Source Captures
	DECLARE_ATTRIBUTE_CAPTUREDEF(Stamina);
	DECLARE_ATTRIBUTE_CAPTUREDEF(MaxStamina);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DodgeStaminaCost);

	StaminaStatics()
	{
		// Source Defines
		DEFINE_ATTRIBUTE_CAPTUREDEF(UMKHAttributeSet, Stamina, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UMKHAttributeSet, MaxStamina, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UMKHAttributeSet, DodgeStaminaCost, Source, true);
	}
};

static const StaminaStatics& GetStaminaStatics()
{
	static StaminaStatics SStatics;
	return SStatics;
}

UExecCalc_DodgeCost::UExecCalc_DodgeCost()
{
	RelevantAttributesToCapture.Add(GetStaminaStatics().StaminaDef);
	RelevantAttributesToCapture.Add(GetStaminaStatics().MaxStaminaDef);
	RelevantAttributesToCapture.Add(GetStaminaStatics().DodgeStaminaCostDef);
}

void UExecCalc_DodgeCost::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, 
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& EffectSpec = ExecutionParams.GetOwningSpec();
	
	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = EffectSpec.CapturedSourceTags.GetAggregatedTags();
	
	float CurrentStamina = 0.f;
	float MaxStamina = 1.f;
	float DodgeCost = 0.f;
	
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetStaminaStatics().StaminaDef, EvalParams, CurrentStamina);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetStaminaStatics().MaxStaminaDef, EvalParams, MaxStamina);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetStaminaStatics().DodgeStaminaCostDef, EvalParams, DodgeCost);
	
	CurrentStamina = FMath::Max(CurrentStamina, 0.f);
	MaxStamina = FMath::Max(MaxStamina, 1.f);
	
	// Apply to the stamina the standard dodge cost or the remaining stamina if it's too low to cover the full cost
	
	DodgeCost = FMath::Min(DodgeCost, CurrentStamina);
	
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(GetStaminaStatics().StaminaProperty, EGameplayModOp::Additive, -DodgeCost));
	
}