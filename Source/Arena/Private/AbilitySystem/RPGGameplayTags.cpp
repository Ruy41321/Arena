// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/RPGGameplayTags.h"

namespace RPGGameplayTags::Combat
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage, "Combat.Data.Damage", "Set By Caller Data Tag For Combat");
}

namespace RPGGameplayTags::State::Movement
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Idle,            "State.Movement.Idle",            "Character is idle");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Walking,         "State.Movement.Walking",         "Character is walking");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Sprinting,       "State.Movement.Sprinting",       "Character is sprinting");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CrouchingIdle,   "State.Movement.CrouchingIdle",   "Character is crouching idle");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CrouchingMoving, "State.Movement.CrouchingMoving", "Character is crouching and moving");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Jumping,         "State.Movement.Jumping",         "Character is jumping");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Falling,         "State.Movement.Falling",         "Character is falling");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LandingInPlace,  "State.Movement.LandingInPlace",  "Character is landing in place");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(LandingMoving,   "State.Movement.LandingMoving",   "Character is landing while moving");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dodging,         "State.Movement.Dodging",         "Character is dodging");
}

namespace RPGGameplayTags::State
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(OutOfStamina, "State.General.Stamina.Out", "Character is out of stamina");
}

namespace RPGGameplayTags::Equip
{
	UE_DEFINE_GAMEPLAY_TAG(Category_Equipment, "Item.Equipment");
	UE_DEFINE_GAMEPLAY_TAG(Category_Weapon, "Item.Equipment.Weapon");
	UE_DEFINE_GAMEPLAY_TAG(Category_Consumable, "Item.Consumable");
	UE_DEFINE_GAMEPLAY_TAG(ArmorSlot, "Equipment.Slot.Armor");
	UE_DEFINE_GAMEPLAY_TAG(WeaponSlot, "Equipment.Slot.Weapon");
	UE_DEFINE_GAMEPLAY_TAG(ConsumableQuickSlot1, "Equipment.Slot.QuickSlot.Consumable.First");
	UE_DEFINE_GAMEPLAY_TAG(ConsumableQuickSlot2, "Equipment.Slot.QuickSlot.Consumable.Second");
	UE_DEFINE_GAMEPLAY_TAG(ConsumableQuickSlot3, "Equipment.Slot.QuickSlot.Consumable.Third");
	UE_DEFINE_GAMEPLAY_TAG(WeaponQuickSlotCategory, "Equipment.Slot.QuickSlot.Weapon");
	UE_DEFINE_GAMEPLAY_TAG(WeaponQuickSlot1, "Equipment.Slot.QuickSlot.Weapon.Primary");
	UE_DEFINE_GAMEPLAY_TAG(WeaponQuickSlot2, "Equipment.Slot.QuickSlot.Weapon.Secondary");
}

namespace RPGGameplayTags::Input
{
	UE_DEFINE_GAMEPLAY_TAG(Inventory, "Input.Inventory");
	UE_DEFINE_GAMEPLAY_TAG(QuickSlot, "Equipment.Slot.QuickSlot");
	UE_DEFINE_GAMEPLAY_TAG(Ability, "Input.Ability");
	UE_DEFINE_GAMEPLAY_TAG(Attacks, "Input.Ability.Attacks");
	UE_DEFINE_GAMEPLAY_TAG(Skill, "Input.Ability.Attacks.Skill");
	UE_DEFINE_GAMEPLAY_TAG(SkillSlot1, "Input.Ability.Attacks.Skill.First");
	UE_DEFINE_GAMEPLAY_TAG(SkillSlot2, "Input.Ability.Attacks.Skill.Second");
	UE_DEFINE_GAMEPLAY_TAG(SkillSlot3, "Input.Ability.Attacks.Skill.Third");
}

namespace RPGGameplayTags::Event
{
	UE_DEFINE_GAMEPLAY_TAG(HitScanStart, "Event.HitScan.Start");
	UE_DEFINE_GAMEPLAY_TAG(HitScanEnd, "Event.HitScan.End");
	UE_DEFINE_GAMEPLAY_TAG(ContinueComboStart, "Event.ContinueCombo.Start");
	UE_DEFINE_GAMEPLAY_TAG(ContinueComboEnd, "Event.ContinueCombo.End");
}