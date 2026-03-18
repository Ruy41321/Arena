// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/RPGGameplayAbility.h"
#include "AbilitySystem/RPGAbilityTypes.h"
#include "RPGDamageAbility.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API URPGDamageAbility : public URPGGameplayAbility
{
	GENERATED_BODY()
	
public:
	
	UFUNCTION(BlueprintCallable)
	void CaptureDamageEffectInfo(AActor* TargetActor, FDamageEffectInfo& OutInfo, const float DamageMultiplier = 0.f);
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void PreActivate(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, FOnGameplayAbilityEnded::FDelegate* OnGameplayAbilityEndedDelegate, const FGameplayEventData* TriggerEventData = nullptr) override;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnMontageStarted();
	virtual void OnMontageStarted_Implementation();
	
private:

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Damage Effect")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Damage Effect")
	FScalableFloat BaseDamage;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Custom Values | AnimStuff")
	TObjectPtr<UAnimMontage> AttackMontageToPlay;
	
	bool PlayAnimation();
	
	UFUNCTION(BlueprintCallable)
	void FaceCharacterTowardsAttack();
	
	UFUNCTION()
	void OnMontageFinished();
	
};
