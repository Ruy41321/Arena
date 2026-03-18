// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/RPGDamageAbility.h"
#include "ArenaMeleeAbility.generated.h"

class AArenaWeaponBase;
/**
 * 
 */
UCLASS()
class ARENA_API UArenaMeleeAbility : public URPGDamageAbility
{
	GENERATED_BODY()
	
public:
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void OnMontageStarted_Implementation() override;
	
	virtual void OnAbilityActivatedAgain_Implementation(float TimeWaited) override;

	/**
	 * 
	 * Through this event, you can add specific logic for your combo ability
	 * like dynamic damage multiplier or the attack redirection
	 * 
	 * @param HitCounter The number of the hit that it's gonna happen
	 */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void OnComboTriggered(int32 HitCounter);
	
private:
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Custom Values | AnimStuff")
	TObjectPtr<AArenaWeaponBase> OwningWeapon;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Custom Values | Combo")
	bool bIsComboAbility = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true", EditCondition = "bIsComboAbility"), Category = "Custom Values | Combo")
	float ComboDamageMultiplier = 0.f;
	
	bool bIsWithinComboWindow = false;
	bool bContinueCombo = false;
	int32 ComboHitCounter = 1;
	
	UFUNCTION(BlueprintCallable)
	AArenaWeaponBase* InitOwningWeapon();
	
	void BindHitScanEvents();
	void BindOnContinueComboEvents();
	
	UFUNCTION()
	virtual void OnHitScanStartReceived(FGameplayEventData Payload);
	
	UFUNCTION()
	virtual void OnHitScanEndReceived(FGameplayEventData Payload);
	
	UFUNCTION()
	virtual void OnContinueComboStartReceived(FGameplayEventData Payload);
	
	UFUNCTION()
	virtual void OnContinueComboEndReceived(FGameplayEventData Payload);
		
};
