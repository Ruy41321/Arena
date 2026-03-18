// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "RPGGameplayAbility.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API URPGGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	

public:
	/**
	 * Input Tag used to bind the action, It doesn't matter for skills because they are bind dynamically
	 */
	UPROPERTY(EditDefaultsOnly, category = "Custom Values | Input")
	FGameplayTag InputTag;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnAbilityActivatedAgain(float TimeWaited);
	virtual void OnAbilityActivatedAgain_Implementation(float TimeWaited);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnAbilityReleased(float TimeWaited);
	virtual void OnAbilityReleased_Implementation(float TimeWaited);
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	
	void BindInputPressEvent();
	void BindInputReleaseEvent();
	
};
