// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/MKHGameplayAbility.h"
#include "AbilitySystem/MKHAbilityTypes.h"
#include "MKHDamageAbility.generated.h"

struct FAttackData;
class AMKHWeaponBase;
class UMKHAbilityGrantPayload;

/**
 * Base class for damage-dealing abilities (basic attacks and skills).
 *
 * Provides:
 * - Per-attack damage data (AttacksData) and damage capture into FDamageEffectInfo.
 * - Combo support driven by animation notifies (ContinueComboStart/End events).
 * - Skill support with a dynamic set-by-caller cooldown (CooldownTime + CooldownTags).
 * - Owning weapon resolution from the actors attached to the avatar.
 *
 * Per-grant data (attack data, skill flag, cooldown) is injected at grant time through
 * UMKHAbilityGrantPayload — never configured by mutating the class CDO.
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
	
	/**
	 * Natively triggers logic when the melee attack montage begins.
	 */
	virtual void OnMontageStarted_Implementation() override;
	
	// ==========================================
	// Config Damage & Effects
	// ==========================================

	/**
	 * Captures information regarding the damage effect onto the target.
	 * @param TargetActor The actor receiving the damage.
	 * @param OutInfo The output struct storing the captured effect details.
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Ability | Effects")
	void CaptureDamageEffectInfo(AActor* TargetActor, FDamageEffectInfo& OutInfo);

	/**
	 * Sets the AttacksData for this ability.
	 * @param InAttacksData The AttackData to set for this ability.
	 */
	void SetAttacksData(const TArray<FAttackData>& InAttacksData);
	
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
	 * Logic evaluated when the input is re-tapped during the ability execution.
	 */
	virtual void OnAbilityActivatedAgain_Implementation(float TimeWaited) override;

	// ==========================================
	// Combo Mechanics
	// ==========================================

	/**
	 * Hook exposed to Blueprint for adding specific logic during a combo attack.
	 * Allows implementation of dynamic damage multipliers.
	 * The base c++ implementation face the char where the player is aiming.
	 * 
	 * @param HitCounter The sequential index of the upcoming combo strike.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Melee Ability | Combo Logic")
	void OnComboTriggered(int32 HitCounter);
	virtual void OnComboTriggered_Implementation(int32 HitCounter);

protected:

	/**
	 * Returns the spec's grant payload, or nullptr when granted without one (remote clients,
	 * non-equipment grants). Instanced-per-actor abilities already start from CDO defaults,
	 * so a null payload just leaves those defaults in place.
	 */
	static const UMKHAbilityGrantPayload* GetGrantPayload(const FGameplayAbilitySpec& Spec);

	/** Adjusts character transform dynamically to face towards an expected target location or look angle. */
	UFUNCTION(BlueprintCallable, Category = "Damage Ability | Movement")
	void FaceCharacterTowardsAttack();

	/**
	 * Returns the current sequential hit index of the active combo.
	 *
	 * @return The zero-based combo strike index.
	 */
	int32 GetComboHitCounter() const { return ComboHitCounter; }

	/**
	 * Aligns the combo counter with an authoritative combo index received from the
	 * locally-controlled machine (melee target data). The counter only moves forward and is
	 * clamped to the configured attack data, so a malicious client cannot price a strike
	 * beyond the strongest configured combo step.
	 *
	 * @param InComboIndex  Combo strike index reported by the sending machine.
	 */
	void SyncAuthoritativeComboCounter(int32 InComboIndex);
	
	// ==========================================
	// Weapon & Damage Values
	// ==========================================

	/** The weapon this damage ability is owned by. */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Damage Ability | Weapon")
	TObjectPtr<AMKHWeaponBase> OwningWeapon;

	/**
	 * Initializes and caches the owning weapon by scanning the avatar's attached actors.
	 * Works on every machine: the weapon actor replicates already attached to the character
	 * mesh, unlike its equipment instance which is server-only.
	 *
	 * @return The resolved weapon, or nullptr when unavailable on this machine.
	 */
	AMKHWeaponBase* InitOwningWeapon();
	
	/**
	 * Returns the damage multiplier of the current attack (e.g. 1.5 for a heavy attack).
	 * The weapon's base damage is intentionally NOT factored in here: UExecCalc_Damage
	 * resolves it at execution time and combines it with the attacker's damage buffs.
	 *
	 * @return The attack's damage multiplier, 1.0 when no per-attack data is available.
	 */
	virtual float GetAttackDamageMultiplier() const;

	/**
	 * Applies the current combo strike's SelfActivationStatusEffects to the owner.
	 * Runs on the authority only and at most once per combo strike; called when the attack
	 * starts (montage start, combo continuation) and from SyncAuthoritativeComboCounter as
	 * a latency catch-up, so buffs like Empower are always active before the strike's
	 * damage spec is created and thus captured by its damage calculation.
	 */
	void TryApplySelfActivationStatusEffects();

	/** Struct containing the Damage and Status effects applied by each attack of this ability. */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Damage Ability | Damage")
	TArray<FAttackData> AttacksData;

	/** Last combo strike index whose SelfActivationStatusEffects were applied (authority-side dedup). */
	int32 LastSelfActivationComboIndex = INDEX_NONE;
	
private:

	// ==========================================
	// Internal Configurations
	// ==========================================
	
	/** Flag indicating if this is considered a skill ability rather than a basic attack. */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Damage Ability | Config")
	bool bIsSkillAbility = false;
	
	/** The GameplayEffect class configuration applied upon successfully hitting targets. */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Damage Ability | Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;
	
	/** The GameplayEffect class used to apply the cooldown. */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Damage Ability | Cooldown") 
	TSubclassOf<UGameplayEffect> CooldownEffect;
	
	/** Tags representing the cooldown state applied to the character. */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Damage Ability | Cooldown")
	FGameplayTagContainer CooldownTagContainer = FGameplayTagContainer();
	
	/** The duration of the cooldown in seconds. */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Damage Ability | Cooldown")
	float CooldownTime = 0.f;
				
	/** Retrieve and assign needed BP Classes from a Data Asset*/
	void AssignBPClasses(const FGameplayAbilityActorInfo* ActorInfo);
	
	/** Indicates whether this melee ability supports combo sequences. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Melee Ability | Core Settings")
	bool bIsComboAbility = false;
	
	/** The current sequential hit index of the active combo. */
	int32 ComboHitCounter = 0;
	
	/** State flag indicating if the player is actively inside a combo timing window. */
	bool bIsWithinComboWindow = false;
	
	/** State flag tracking if the player pressed attack within the window to continue the combo. */
	bool bContinueCombo = false;
	
	/** Binds tasks waiting for start and end tags relative to combo progression windows. */
	void BindOnContinueComboEvents();
	
	/** Triggered when the combo window safely opens via animation notifies. */
	UFUNCTION()
	virtual void OnContinueComboStartReceived(FGameplayEventData Payload);
	
	/** Triggered when the combo window effectively closes via animation notifies. */
	UFUNCTION()
	virtual void OnContinueComboEndReceived(FGameplayEventData Payload);
		
	
};
