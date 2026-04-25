// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "ExecCalc_DodgeCost.generated.h"

/**
 * Custom execution calculation for determining the stamina cost of a dodge.
 * Prevents stamina from going negative by capping the cost to available stamina.
 */
UCLASS()
class MAKHIA_API UExecCalc_DodgeCost : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
	
public:
	
	UExecCalc_DodgeCost();

	/**
	 * Executes the stamina cost calculation for doging.
	 *
	 * @param ExecutionParams Parameters containing execution information.
	 * @param OutExecutionOutput Output structure for modifying target attributes.
	 */
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

};
