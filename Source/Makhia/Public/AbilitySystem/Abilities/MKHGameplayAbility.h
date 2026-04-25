// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "MKHGameplayAbility.generated.h"

/**
 * 
 */
UCLASS()
class MAKHIA_API UMKHGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	
	UMKHGameplayAbility();
	
	// ==========================================
	// Overrides
	// ==========================================

	/** 
	 * Activates the ability, binding input events and commuting the ability.
	 * @param Handle The ability spec handle.
	 * @param ActorInfo The actor info activating the ability.
	 * @param ActivationInfo The activation info.
	 * @param TriggerEventData Payload data from the event that triggered this ability.
	 */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	/**
	 * Called when the ability is granted, allowing cache configurations.
	 * 
	 * @param ActorInfo 
	 * @param Spec 
	 */
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	
	/**
	 * Called when the ability is being removed from the Ability System Component.
	 * @param ActorInfo The actor info removing the ability.
	 * @param Spec The ability spec being removed.
	 */
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	
	// ==========================================
	// Input & Events
	// ==========================================

	/** 
	 * Input Tag used to bind the action. 
	 * Note: It doesn't matter for skills because they are bound dynamically. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RPG Gameplay Ability | Input")
	FGameplayTag InputTag;

	/**
	 * Called repeatedly when the input is pressed again while the ability is active.
	 * @param TimeWaited The time waited before the button was pressed again.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RPG Gameplay Ability | Input")
	void OnAbilityActivatedAgain(float TimeWaited);
	virtual void OnAbilityActivatedAgain_Implementation(float TimeWaited);
	
	/**
	 * Called when the input bound to this ability is released.
	 * @param TimeWaited The time the button was held before releasing.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RPG Gameplay Ability | Input")
	void OnAbilityReleased(float TimeWaited);
	virtual void OnAbilityReleased_Implementation(float TimeWaited);
	
	/**
	 * Blueprint event triggered when the ability is removed.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "RPG Gameplay Ability | Events")
	void OnAbilityRemoved();

protected:
	
	// ==========================================
	// Helpers
	// ==========================================

	/**
	 * Adjusts the avatar's orientation behavior towards the camera.
	 * @param bFollowCamera If true, the character will use the controller rotation yaw.
	 */
	UFUNCTION(BlueprintCallable, Category = "RPG Gameplay Ability | Movement")
	void SetCharacterOrientation(bool bFollowCamera);
	
private:
	
	// ==========================================
	// Internal Logic
	// ==========================================

	/** Binds the ability to listen for subsequent input press events. */
	void BindInputPressEvent();
	
	/** Binds the ability to listen for input release events. */
	void BindInputReleaseEvent();
	
};
