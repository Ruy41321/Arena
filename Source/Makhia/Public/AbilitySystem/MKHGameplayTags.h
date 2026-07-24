// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

/** Combat and damage-related gameplay tags. */
namespace MKHGameplayTags::Combat
{
	/** Data tags used to pass damage payload values through gameplay effects and execution calculations. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_StaminaDamage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_StatusEffectDuration);
	/** SetByCaller key carrying the dynamic value (%) of the status effect g.e. 15% for empower. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_StatusEffectValue);
	/** SetByCaller key carrying the dynamic cooldown duration of skill abilities. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_AbilityCooldownTime);
	/** Tag to determine if the next ability has to be queued or insta activated.*/
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputBufferWindow);
}

/** Movement state tags synchronized between movement systems, abilities, and animation logic. */
namespace MKHGameplayTags::State::Movement
{
	/** Character is standing still and not transitioning to another locomotion state. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Idle);
	/** Character is moving at non-sprint speed while grounded. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Walking);
	/** Character is moving in sprint mode. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sprinting);
	/** Character is crouched and currently stationary. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CrouchingIdle);
	/** Character is crouched and currently moving. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CrouchingMoving);
	/** Character has started a jump action. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Jumping);
	/** Character is airborne and descending or otherwise not grounded. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Falling);
	/** Character is landing from air while mostly stationary. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LandingInPlace);
	/** Character is landing from air while preserving movement velocity. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(LandingMoving);
	/** Character is in dodging state. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dodging);
	/** Character is in blocking state. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Blocking);
	/** Character is attacking. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attacking);
	/** Character is dead. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dead);
}

/** Generic gameplay state tags not tied to a single subsystem. */
namespace MKHGameplayTags::State
{
	/** Character cannot spend stamina and is considered exhausted. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(OutOfStamina);
	/** Chatacter is Stunned. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stunned);
	/** Character is staggered. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Staggered);
	/** Character is using a quick slot item. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(QuickSlotUse);
}

/** Equipment category and slot tags used by inventory and equipment systems. */
namespace MKHGameplayTags::Equip
{
	/** Generic equipment category root. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Category_Equipment);
	/** Weapon category root. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Category_Weapon);
	/** Consumable category root. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Category_Consumable);
	/** Armor slot tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(ArmorSlot);
	/** Weapon slot tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(WeaponSlot);
	/** First consumable quick-slot tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConsumableQuickSlot1);
	/** Second consumable quick-slot tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConsumableQuickSlot2);
	/** Third consumable quick-slot tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConsumableQuickSlot3);
	/** First weapon quick-slot tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(WeaponQuickSlot1);
	/** Second weapon quick-slot tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(WeaponQuickSlot2);
}

/** Input routing tags used by Enhanced Input and ability activation logic. */
namespace MKHGameplayTags::Input
{
	/** Inventory toggle input tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Inventory);
	/** Base ability input hierarchy tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability);
	/** Base quick-slot input hierarchy tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(QuickSlot);
	/** Category tag used to filter weapon quick-slot entries. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(WeaponQuickSlotCategory);
	/** Category tag used to filter consumable quick-slot entries. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConsumableQuickSlotCategory);
	/** Sheathe weapon input tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SheatheWeapon);
	/** Attack input hierarchy tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attacks);
	/** Dodge input tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dodge);
	/** Block input action tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Block);
	/** Skill input hierarchy tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill);
	/** First skill slot input tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillSlot1);
	/** Second skill slot input tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillSlot2);
	/** Third skill slot input tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillSlot3);
}

/** Gameplay event tags sent through GAS event payloads and anim notifies. */
namespace MKHGameplayTags::Event
{
	/** Event fired when a player dies. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Death);
	/** Event fired when a hitscan window starts. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitScanStart);
	/** Event fired when a hitscan window ends. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitScanEnd);
	/** Event fired when combo input buffering starts. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(ContinueComboStart);
	/** Event fired when combo input buffering ends. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(ContinueComboEnd);
	/** Event fired to apply the DMT in a specific frame e.g. Heavy Attack Charge finished. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(ApplyDMT);
	/** Event fired to spawn projectile-based ability actors. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SpawnProjectile);
	/** Event fired to request quick-slot item usage. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UseQuickSlot);
	/** Event fired to notify that an attack was successfully blocked. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(BlockSuccessful);
	/** Event fired to notify that a blocking defender had their stamina broken. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GuardBreak);
	/** Event fired to notify that an attack was dodged. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AttackDodged);
	/** Event fired to notify that a hit was inflicted. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitInflicted);
	/** Event fired to notify that a hit was received. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReceived);
	/** Event fired to allow the ability to be cancelled early, removing its priority tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(MakeAbilityCancellable);
	/** Event fired to try the activation of a previously queued ability. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(ActivateQueuedAbility);
}

/** Ability lifecycle and grouping tags. */
namespace MKHGameplayTags::Ability
{
	/** Root tag for every ability-specific tag in the project. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(All);
	/** Tag representing abilities that are currently active. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AbilityActive);
	/** Tag representing attack abilities that are currently active. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attacking);
	
	/** First priority for ability activation. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Priority1);
	/** Second priority for ability activation. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Priority2);

	/** Attack ability type tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Type_Attacks);
	/** Basic attack ability type tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Type_Attacks_Basic);
	/** Heavy basic attack ability type tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Type_Attacks_Basic_Heavy);
	/** Light basic attack ability type tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Type_Attacks_Basic_Light);
	/** Skill attack ability type tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Type_Attacks_Skill);
	/** Block ability type tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Type_Block);
}
