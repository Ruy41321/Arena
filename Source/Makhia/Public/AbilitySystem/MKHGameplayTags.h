// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

/** Combat and damage-related gameplay tags. */
namespace MKHGameplayTags::Combat
{
	/** Data tag used to pass damage payload values through gameplay effects and execution calculations. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage);
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
	/** Character is in dodge state. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dodging);
}

/** Generic gameplay state tags not tied to a single subsystem. */
namespace MKHGameplayTags::State
{
	/** Character cannot spend stamina and is considered exhausted. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(OutOfStamina);
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
	/** Category tag used to filter weapon quick-slot entries. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(WeaponQuickSlotCategory);
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
	/** Base quick-slot input hierarchy tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(QuickSlot);
	/** Base ability input hierarchy tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability);
	/** Attack input hierarchy tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attacks);
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
	/** Event fired when a hitscan window starts. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitScanStart);
	/** Event fired when a hitscan window ends. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitScanEnd);
	/** Event fired when combo input buffering starts. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(ContinueComboStart);
	/** Event fired when combo input buffering ends. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(ContinueComboEnd);
	/** Event fired to spawn projectile-based ability actors. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SpawnProjectile);
	/** Event fired to request quick-slot item usage. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UseQuickSlot);
}

/** Ability lifecycle and grouping tags. */
namespace MKHGameplayTags::Ability
{
	/** Root tag for every ability-specific tag in the project. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(All);
	/** Tag representing abilities that are currently active. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AbilityActive);
}
