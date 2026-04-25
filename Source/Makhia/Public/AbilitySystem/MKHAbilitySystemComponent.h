// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "MKHAbilitySystemComponent.generated.h"

class UEquipmentManagerComponent;
class UGameplayAbility;
class UGameplayEffect;
struct FEquipmentStatEffectDefinition;
struct FEquipmentAbilityDefinition;
struct FRPGEquipmentEntry;

/** Delegate fired after default attributes are granted to the owning avatar. */
DECLARE_MULTICAST_DELEGATE(FOnAttributesGiven);

/** Gameplay Ability System component with character setup, input routing, and equipment integration. */
UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class MAKHIA_API UMKHAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
	
public:
	// =========================================================================================
	// Delegates & Events
	// =========================================================================================

	/** Delegate fired after default attributes are granted to the owning avatar. */
	FOnAttributesGiven OnAttributesGiven;


	// =========================================================================================
	// Initialization & Core Abilities
	// =========================================================================================

	/**
	 * Grants active character abilities and binds their configured input tags.
	 * @param AbilitiesToGrant Ability classes to grant to this component.
	 */
	UFUNCTION(BlueprintCallable, Category = "GAS|Abilities")
	void AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& AbilitiesToGrant);

	/**
	 * Grants passive abilities and immediately activates them once.
	 * @param PassivesToGrant Passive ability classes to grant and auto-activate.
	 */
	UFUNCTION(BlueprintCallable, Category = "GAS|Abilities")
	void AddCharacterPassiveAbilities(const TArray<TSubclassOf<UGameplayAbility>>& PassivesToGrant);

	/**
	 * Applies the default attribute gameplay effect to self.
	 * @param AttributeEffect Gameplay effect class used to initialize attributes.
	 */
	UFUNCTION(BlueprintCallable, Category = "GAS|Attributes")
	void InitializeDefaultAttributes(const TSubclassOf<UGameplayEffect>& AttributeEffect);


	// =========================================================================================
	// Input Handling
	// =========================================================================================

	/**
	 * Handles pressed input tags by activating or forwarding input to matching abilities.
	 * @param InputTag Gameplay tag produced by the input layer.
	 */
	UFUNCTION(BlueprintCallable, Category = "GAS|Input")
	void AbilityInputPressed(const FGameplayTag& InputTag);

	/**
	 * Handles released input tags for abilities that consume release events.
	 * @param InputTag Gameplay tag produced by the input layer.
	 */
	UFUNCTION(BlueprintCallable, Category = "GAS|Input")
	void AbilityInputReleased(const FGameplayTag& InputTag);


	// =========================================================================================
	// Equipment Integration
	// =========================================================================================

	/**
	 * Grants all gameplay effects declared by an equipment entry.
	 * @param EquipmentEntry Mutable equipment entry owning granted effect handles.
	 */
	void AddEquipmentEffects(FRPGEquipmentEntry* EquipmentEntry);

	/**
	 * Removes every active gameplay effect previously granted by an equipment entry.
	 * @param EquipmentEntry Mutable equipment entry containing granted effect handles.
	 */
	void RemoveEquipmentEffects(FRPGEquipmentEntry* EquipmentEntry);

	/**
	 * Grants all abilities declared by an equipment entry.
	 * @param EquipmentEntry Mutable equipment entry owning granted ability handles.
	 */
	void AddEquipmentAbility(FRPGEquipmentEntry* EquipmentEntry);

	/**
	 * Removes all abilities previously granted by an equipment entry.
	 * @param EquipmentEntry Mutable equipment entry containing granted ability handles.
	 */
	void RemoveEquipmentAbility(FRPGEquipmentEntry* EquipmentEntry);

	// =========================================================================================
	// Utility
	// =========================================================================================

	/**
	 * Gets the remaining cooldown time and total cooldown duration for the Gameplay
	 * Effect that grants the given cooldown tag.
	 * @param CooldownTag The gameplay tag representing the cooldown to query.
	 * @param TimeRemaining Output parameter for the remaining cooldown time in seconds.
	 * @param CooldownDuration Output parameter for the total cooldown duration in seconds.
	 */
	UFUNCTION(BlueprintCallable, Category = "GAS|Abilities")
	void GetCooldownRemainingForTag(FGameplayTag CooldownTag, float& TimeRemaining, float& CooldownDuration) const;
	
private:
	// =========================================================================================
	// Internal Input Logic
	// =========================================================================================

	/** Returns true when the input belongs to the quick-slot input hierarchy. */
	bool IsQuickSlotInput(const FGameplayTag& InputTag) const;

	/** Returns true when an ability spec should react to the provided input tag. */
	bool DoesSpecMatchInput(const FGameplayAbilitySpec& Spec, const FGameplayTag& InputTag, bool bIsQuickSlotInput) const;

	/** Sends the quick-slot gameplay event to the avatar actor. */
	void SendQuickSlotEvent(const FGameplayTag& InputTag) const;

	/** Activates a spec or forwards replicated pressed input to active instances. */
	void HandleAbilityInputPressedForSpec(const FGameplayAbilitySpec& Spec);

	/** Forwards replicated released input to active ability instances. */
	void HandleAbilityInputReleasedForSpec(const FGameplayAbilitySpec& Spec);


	// =========================================================================================
	// Internal Equipment Logic
	// =========================================================================================

	/** Resolves the owning equipment manager through the player controller interface. */
	TWeakObjectPtr<UEquipmentManagerComponent> GetWeakEquipmentManager() const;

	/** Finds a mutable equipment entry by item id and slot tag in the provided manager. */
	FRPGEquipmentEntry* FindEquipmentEntry(UEquipmentManagerComponent* EquipmentManager, int64 ItemId, const FGameplayTag& SlotTag) const;

	/** Applies one equipment stat effect and tracks its active handle on the entry. Used to simplify logic. */
	void ApplyAndTrackStatEffect(FRPGEquipmentEntry& EquipmentEntry, const FEquipmentStatEffectDefinition& StatEffect, const FGameplayEffectContextHandle& ContextHandle);

	/** Applies one equipment stat effect directly or asynchronously and tracks its active handle on the entry. */
	void GrantEquipmentStatEffect(FRPGEquipmentEntry& EquipmentEntry, const FEquipmentStatEffectDefinition& StatEffect, const FGameplayEffectContextHandle& ContextHandle);
	
	/** Extracted helper to just grant an equipment ability and track it, simplifying async callback. */
	void ApplyAndTrackEquipmentAbility(FRPGEquipmentEntry& EquipmentEntry, const FEquipmentAbilityDefinition& AbilityDef);

	/** Grants one equipment ability and tracks its granted handle on the entry. */
	void GrantEquipmentAbilityDefinition(FRPGEquipmentEntry& EquipmentEntry, const FEquipmentAbilityDefinition& AbilityDef);

	/** Builds and grants a single ability spec from an equipment ability definition. */
	FGameplayAbilitySpecHandle GrantEquipmentAbility(const FEquipmentAbilityDefinition& AbilityDef);

};
