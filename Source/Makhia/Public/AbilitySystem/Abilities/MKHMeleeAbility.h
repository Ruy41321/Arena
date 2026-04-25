// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/MKHDamageAbility.h"
#include "MKHMeleeAbility.generated.h"

/**
 *  UMKHMeleeAbility is a class that extends the UMKHDamageAbility to provide
 *  melee combat functionalities with combo attack capabilities for the Arena game.
 *  It includes methods to activate and deactivate abilities, handle combo logic,
 *  and calculate damage values.
 */
UCLASS()
class MAKHIA_API UMKHMeleeAbility : public UMKHDamageAbility
{
	GENERATED_BODY()
	
public:
	// ==========================================
	// Overrides
	// ==========================================

	/** 
	 * Activates the melee ability, binding required hit scan events.
	 */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	/** 
	 * Ends the ability natively, resolving any lingering hit scans.
	 */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/**
	 * Natively triggers logic when the melee attack montage begins.
	 */
	virtual void OnMontageStarted_Implementation() override;
	
	/**
	 * Logic evaluated when the input is re-tapped during the ability execution.
	 */
	virtual void OnAbilityActivatedAgain_Implementation(float TimeWaited) override;

	// ==========================================
	// Combo Mechanics
	// ==========================================

	/**
	 * Hook exposed to Blueprint for adding specific logic during a combo attack.
	 * Allows implementation of dynamic damage multipliers or attack redirections.
	 * 
	 * @param HitCounter The sequential index of the upcoming combo strike.
	 */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Arena Melee Ability | Combo Logic")
	void OnComboTriggered(int32 HitCounter);

protected:
	// ==========================================
	// Damage Calculations
	// ==========================================

	/**
	 * Overrides the base damage calculation to inject combo damage multipliers.
	 * @param WeaponDamage The base attack damage from the equipped weapon.
	 * @return The calculated base damage.
	 */
	virtual float GetBaseDamageValue(float WeaponDamage) override;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Arena Melee Ability | Hit Scan")
	void HitScanStart();
	virtual void HitScanStart_Implementation();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Arena Melee Ability | Hit Scan")
	void HitScanEnd();
	virtual void HitScanEnd_Implementation();
		
private:
	// ==========================================
	// Component Configurations
	// ==========================================
	
	/** Indicates whether this melee ability supports combo sequences. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Arena Melee Ability | Core Settings")
	bool bIsComboAbility = false;
	
	/** Added multiplier to base damage dynamically calculated during combos. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true", EditCondition = "bIsComboAbility"), Category = "Arena Melee Ability | Combo Parameters")
	float ComboDamageMultiplier = 0.f;
	
	/** State flag indicating if the player is actively inside a combo timing window. */
	bool bIsWithinComboWindow = false;
	
	/** State flag tracking if the player pressed attack within the window to continue the combo. */
	bool bContinueCombo = false;
	
	/** The current sequential hit index of the active combo. */
	int32 ComboHitCounter = 1;
	
	// ==========================================
	// Internal Logic & Events
	// ==========================================

	/** Binds tasks waiting for start and end tags relative to weapon hit scanning. */
	void BindHitScanEvents();
	
	/** Binds tasks waiting for start and end tags relative to combo progression windows. */
	void BindOnContinueComboEvents();
	
	/** Caught when an active hit scan event payload starts. */
	UFUNCTION()
	virtual void OnHitScanStartReceived(FGameplayEventData Payload);
	
	/** Caught when an active hit scan event payload ends. */
	UFUNCTION()
	virtual void OnHitScanEndReceived(FGameplayEventData Payload);
	
	/** Triggered when the combo window safely opens via animation notifies. */
	UFUNCTION()
	virtual void OnContinueComboStartReceived(FGameplayEventData Payload);
	
	/** Triggered when the combo window effectively closes via animation notifies. */
	UFUNCTION()
	virtual void OnContinueComboEndReceived(FGameplayEventData Payload);
		
};
