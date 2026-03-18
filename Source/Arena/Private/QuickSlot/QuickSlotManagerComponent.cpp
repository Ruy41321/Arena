// Fill out your copyright notice in the Description page of Project Settings.


#include "QuickSlot/QuickSlotManagerComponent.h"

#include "AbilitySystem/RPGGameplayTags.h"
#include "Input/RPGSystemInputComponent.h"
#include "Interfaces/InventoryInterface.h"
#include "Inventory/InventoryComponent.h"

UQuickSlotManagerComponent::UQuickSlotManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UQuickSlotManagerComponent::SetupInputActions(URPGInputConfig* InputConfig,
	URPGSystemInputComponent* InputComponent)
{
	InputComponent->BindAbilityActions(InputConfig, RPGGameplayTags::Input::QuickSlot, this, &UQuickSlotManagerComponent::UseQuickSlot, nullptr);
}

void UQuickSlotManagerComponent::AddQuickSlot(const FGameplayTag& QuickSlotTag, int64 ItemID)
{
	RemoveQuickSlot(ItemID);
	QuickSlotTagMap.Add(QuickSlotTag, ItemID);
	if (QuickSlotTag.MatchesTag(RPGGameplayTags::Equip::WeaponQuickSlotCategory) && !QuickEquipWeaponSlotTag.IsValid())
	{
		QuickEquipWeaponSlotTag = QuickSlotTag;
	}
}

void UQuickSlotManagerComponent::RemoveQuickSlot(int64 ItemID)
{
	// Find the key associated with the given ItemID and remove it from the map if it exists
	const FGameplayTag* ExistingKey = QuickSlotTagMap.FindKey(ItemID);
	if (ExistingKey)
	{
		bool bRemovedQuickEquipWeapon = false;
		if (ExistingKey->MatchesTagExact(QuickEquipWeaponSlotTag))
		{
			QuickEquipWeaponSlotTag = FGameplayTag();
			bRemovedQuickEquipWeapon = true;
		}
		QuickSlotTagMap.Remove(*ExistingKey);
		
		if (bRemovedQuickEquipWeapon)
		{
			if (QuickSlotTagMap.Contains(RPGGameplayTags::Equip::WeaponQuickSlot1))
			{
				QuickEquipWeaponSlotTag = RPGGameplayTags::Equip::WeaponQuickSlot1;
			}
			else if (QuickSlotTagMap.Contains(RPGGameplayTags::Equip::WeaponQuickSlot2))
			{
				QuickEquipWeaponSlotTag = RPGGameplayTags::Equip::WeaponQuickSlot2;
			}
		}
	}
}

void UQuickSlotManagerComponent::UseQuickSlot(const FGameplayTag& QuickSlotTag)
{
	// Dont trigger activation for sheathed use
	if (bCanTriggerActivation)
		QuickSlotActivatedDelegate.Execute(QuickSlotTag);
	else
		bCanTriggerActivation = true;
	
	const int64* ItemID = QuickSlotTagMap.Find(QuickSlotTag);
	if (!ItemID)
	{
		return;
	}
	
	UInventoryComponent* InventoryComponent = IInventoryInterface::Execute_GetInventoryComponent(GetOwner());
	if (!IsValid(InventoryComponent))
	{
		return;
	}
	
	// Assign the last weapon used as quick weapon if unarmed 
	if (QuickSlotTag.MatchesTag(RPGGameplayTags::Equip::WeaponQuickSlotCategory))
	{
		QuickEquipWeaponSlotTag = QuickSlotTag;
	}
	
	InventoryComponent->UseItem(*ItemID, 1);
}

int64 UQuickSlotManagerComponent::GetQuickSlotID(const FGameplayTag& QuickSlotTag) const
{
	return QuickSlotTagMap.Contains(QuickSlotTag) ? QuickSlotTagMap[QuickSlotTag] : 0;
}

bool UQuickSlotManagerComponent::TryEquipWeapon()
{
	if (!QuickEquipWeaponSlotTag.IsValid())
		return false;
	
	bCanTriggerActivation = false; // Dont trigger activation for sheathed use
	UseQuickSlot(QuickEquipWeaponSlotTag);
	
	return true;
}
