// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/MKHGameplayAbility.h"
#include "AbilitySystem/MKHAbilityTypes.h"
#include "MKHDamageAbility.generated.h"

class AMKHWeaponBase;

/**
 * 
 */
UCLASS()
class MAKHIA_API UMKHDamageAbility : public UMKHGameplayAbility
{
	GENERATED_BODY()
	
public:
	
	/**
	 * Sets default values for this ability's properties.
	 */
	UMKHDamageAbility();
	
	// ==========================================
	// Overrides
	// ==========================================

	/** 
	 * Activates the ability, binding input events and commuting the ability.
	 */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	/**
	 * Pre-activates the ability.
	 */
	virtual void PreActivate(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, FOnGameplayAbilityEnded::FDelegate* OnGameplayAbilityEndedDelegate, const FGameplayEventData* TriggerEventData = nullptr) override;

	/**
	 * Called when the ability is granted, allowing cache configurations.
	 * 
	 * @param ActorInfo 
	 * @param Spec 
	 */
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	
	/**
	 * Applies the cooldown effect to the ability's owner.
	 */
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	
	/**
	 * Retrieves the gameplay tags representing this ability's cooldown.
	 * 
	 * @return A pointer to the cooldown tag container.
	 */
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	
	// ==========================================
	// Config Damage & Effects
	// ==========================================

	/**
	 * Captures information regarding the damage effect onto the target.
	 * @param TargetActor The actor receiving the damage.
	 * @param OutInfo The output struct storing the captured effect details.
	 */
	UFUNCTION(BlueprintCallable, Category = "RPG Damage Ability | Effects")
	void CaptureDamageEffectInfo(AActor* TargetActor, FDamageEffectInfo& OutInfo);

	/**
	 * Sets the damage percentage applied by this ability.
	 * @param InDamagePercent The multiplier for damage.
	 */
	void SetDamagePercent(float InDamagePercent);

	/**
	 * Set the cooldown data relatives to the specific ability.
	 * @param InCooldownTime Time to activate the skill again
	 * @param InCooldownTagContainer Tags to grant when on CD
	 */
	void SetCooldownData(float InCooldownTime, const FGameplayTagContainer& InCooldownTagContainer);

	/**
	 * Set if this ability is a skill
	 * @param InIsSkillAbility True if this ability has to be a skill attack
	 */
	void SetIsSkillAbility(bool InIsSkillAbility);
	
	/**
	 * Triggered when the ability animation montage begins playing.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RPG Damage Ability | Animation")
	void OnMontageStarted();
	virtual void OnMontageStarted_Implementation();
	
protected:
	// ==========================================
	// Weapon & Damage Values
	// ==========================================

	/** The weapon this damage ability is owned by. */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "RPG Damage Ability | Weapon")
	TObjectPtr<AMKHWeaponBase> OwningWeapon;

	/** Initializes and caches the owning weapon from the current avatar. */
	AMKHWeaponBase* InitOwningWeapon();
	
	/**
	 * Computes the base damage value factoring in weapon damage.
	 * @param WeaponDamage The base attack damage from the equipped weapon.
	 * @return The calculated base damage.
	 */
	virtual float GetBaseDamageValue(float WeaponDamage);
	
	/** Set by equipment ability definition when the ability is granted. */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "RPG Damage Ability | Damage")
	float DamagePercent = 1.f;
	
	// ==========================================
	// Animation Events
	// ==========================================

	/**
	 * Event fired natively when the ability's montage finishes playing.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "RPG Damage Ability | Animation")
	void OnMontageFinished();
	virtual void OnMontageFinished_Implementation();
	
private:

	// ==========================================
	// Internal Configurations
	// ==========================================
	
	/** Flag indicating if this is considered a skill ability rather than a basic attack. */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "RPG Damage Ability | Config")
	bool bIsSkillAbility = false;
	
	/** The GameplayEffect class configuration applied upon successfully hitting targets. */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "RPG Damage Ability | Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;
	
	/** The GameplayEffect class used to apply the cooldown. */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "RPG Damage Ability | Cooldown") 
	TSubclassOf<UGameplayEffect> CooldownEffect;
	
	/** Tags representing the cooldown state applied to the character. */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "RPG Damage Ability | Cooldown")
	FGameplayTagContainer CooldownTagContainer = FGameplayTagContainer();
	
	/** The duration of the cooldown in seconds. */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "RPG Damage Ability | Cooldown")
	float CooldownTime = 0.f;
			
	/** The attack montage to orchestrate via this ability. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "RPG Damage Ability | Animation")
	TObjectPtr<UAnimMontage> AttackMontageToPlay;
	
	/**
	 * Executes the internal logic to play the specified AttackMontageToPlay.
	 * @return True if successful.
	 */
	bool PlayAnimation();
	
	/** Adjusts character transform dynamically to face towards an expected target location or look angle. */
	UFUNCTION(BlueprintCallable, Category = "RPG Damage Ability | Movement")
	void FaceCharacterTowardsAttack();
	
	/** Retrieve and assign needed BP Classes from a Data Asset*/
	void AssignBPClasses(const FGameplayAbilityActorInfo* ActorInfo);
};
