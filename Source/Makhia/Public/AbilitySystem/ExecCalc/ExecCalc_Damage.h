// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "ExecCalc_Damage.generated.h"

struct FMKHGameplayEffectContext;
class AMKHWeaponBase;

/**
 * Custom execution calculation for determining the final damage value applied to a target.
 *
 * The full damage formula lives here, in explicit order (buff/debuff attributes are
 * decimal fractions: 0.3 = 30%):
 *   1. Base damage: weapon base damage * (attack multiplier from set-by-caller + AdditiveBaseDamage)
 *   2. Offensive multiplier: 1 + (Empower - Weaken) + critical bonus (CritDamageMod on a successful roll)
 *   3. Defensive multiplier: 1 + (Exposed - Reinforced)
 *
 * The result is written into the IncomingDamage meta-attribute; distribution across
 * Shield/Health and all side effects happen in UMKHAttributeSet::PostGameplayEffectExecute.
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

// ============================================================
// Protected / Internal Logic
// ============================================================
private:

	/** Extracts common properties and context required for damage evaluation. */
	void RetrieveProperties(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FGameplayEffectSpec*& OutEffectSpec, FAggregatorEvaluateParameters& OutEvalParams, FMKHGameplayEffectContext*& OutRPGContext, AActor*& OutAttacker, AActor*& OutDefender) const;

	/** Checks if the attack was blocked based on defender's state and facing direction. */
	bool IsAttackBlocked(const FAggregatorEvaluateParameters& EvalParams, const AActor* Attacker, const AActor* Defender) const;

	/** Modifies stamima damage based on the block stability percent of the defender's weapon. */
	void RefineStaminaDamage(const AActor* Defender, float& InOutStaminaDamage) const;

	/**
	 * Resolves the weapon currently equipped in the actor's weapon slot via the
	 * equipment manager on its instigator controller (server-only data, valid here
	 * because executions run on the authority).
	 *
	 * @param OwnerActor  Avatar actor whose equipped weapon should be resolved.
	 * @return            The equipped weapon, or nullptr if none could be found.
	 */
	static const AMKHWeaponBase* GetEquippedWeapon(const AActor* OwnerActor);

	/**
	 * Returns the base damage of the attacker's equipped weapon.
	 * Logs a warning and returns 0 when no weapon is found (resulting in zero damage).
	 */
	static float GetWeaponBaseDamage(const AActor* Attacker);

	/**
	 * Computes the pre-modifier damage of the hit:
	 * weapon base damage * (attack damage multiplier + AdditiveBaseDamage buff).
	 * The two multipliers stack additively so the weapon damage is scaled exactly once.
	 *
	 * @return  The base damage before offensive/defensive multipliers, never below zero.
	 */
	float ComputeBaseDamage(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FAggregatorEvaluateParameters& EvalParams, const FGameplayEffectSpec& EffectSpec, const AActor* Attacker) const;
	
	/**
	 * Evaluates a captured attribute and returns its magnitude, clamped to be non-negative.
	 *
	 * @param ExecutionParams  Execution parameters holding the captured attributes.
	 * @param CaptureDef       Capture definition of the attribute to evaluate.
	 * @param EvalParams       Aggregator evaluation parameters (source/target tags).
	 * @return                 The evaluated magnitude, never below zero.
	 */
	static float GetCapturedMagnitude(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FGameplayEffectAttributeCaptureDefinition& CaptureDef, const FAggregatorEvaluateParameters& EvalParams);

	/**
	 * Applies the attacker's offensive multiplier to the damage:
	 * 1 + (Empower - Weaken) + critical bonus. The crit bonus stacks additively with
	 * the net Empower, intentionally allowing the total to exceed the Empower cap.
	 *
	 * @param InOutDamage  Damage value modified in place, never below zero.
	 */
	void ApplyOffensiveModifiers(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FAggregatorEvaluateParameters& EvalParams, FMKHGameplayEffectContext* RPGContext, float& InOutDamage) const;

	/**
	 * Applies the defender's defensive multiplier to the final damage:
	 * 1 + (Exposed - Reinforced).
	 *
	 * @param InOutDamage  Damage value modified in place, never below zero.
	 */
	void ApplyDefensiveModifiers(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FAggregatorEvaluateParameters& EvalParams, float& InOutDamage) const;

	/**
	 * Rolls the critical hit and flags the result on the effect context.
	 *
	 * @return  CritDamageMod when the roll succeeds, 0 otherwise. The caller folds this
	 *          bonus into the offensive multiplier instead of applying it separately.
	 */
	float RollCriticalHitBonus(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	                           const FAggregatorEvaluateParameters& EvalParams, FMKHGameplayEffectContext* RPGContext) const;

	/** Processes and applies damage to stamina when an attack is blocked. */
	void HandleStaminaDamage(const FGameplayEffectSpec& EffectSpec, const AActor* Defender, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const;

	/** Runs the full health-damage pipeline (base damage, offensive and defensive multipliers) and outputs IncomingDamage. */
	void HandleIncomingDamage(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FAggregatorEvaluateParameters& EvalParams, const FGameplayEffectSpec& EffectSpec, FMKHGameplayEffectContext* RPGContext, const AActor* Attacker, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const;

	/** Checks if the attack was dodged based on defender's movement state (if in dodging). */
	bool IsAttackDodged(const FAggregatorEvaluateParameters& EvalParams) const;
};
