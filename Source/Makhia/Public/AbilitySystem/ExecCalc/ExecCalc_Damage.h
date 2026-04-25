// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "ExecCalc_Damage.generated.h"

/**
 * Custom execution calculation for determining the final damage value applied to a target.
 * Modifies base damage by critical hit chance and critical hit modifier.
 */
UCLASS()
class MAKHIA_API UExecCalc_Damage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
	
public:

	UExecCalc_Damage();

	/**
	 * Executes the custom damage calculation.
	 * Extensively modifies damage to account for crits and other contextual values.
	 *
	 * @param ExecutionParams Parameters containing execution information.
	 * @param OutExecutionOutput Output structure for modifying target attributes.
	 */
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

};
