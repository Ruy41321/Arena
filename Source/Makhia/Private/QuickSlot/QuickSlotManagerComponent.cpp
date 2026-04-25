// Fill out your copyright notice in the Description page of Project Settings.


#include "QuickSlot/QuickSlotManagerComponent.h"

#include "AbilitySystem/MKHGameplayTags.h"
#include "Interfaces/InventoryInterface.h"
#include "Inventory/InventoryComponent.h"

UQuickSlotManagerComponent::UQuickSlotManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UQuickSlotManagerComponent::AddQuickSlot(const FGameplayTag& QuickSlotTag, int64 ItemID)
{
	RemoveQuickSlot(ItemID);
	QuickSlotTagMap.Add(QuickSlotTag, ItemID);

	if (QuickSlotTag.MatchesTag(MKHGameplayTags::Equip::WeaponQuickSlotCategory) && !QuickEquipWeaponSlotTag.IsValid())
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
		const bool bRemovedQuickEquipWeapon = ExistingKey->MatchesTagExact(QuickEquipWeaponSlotTag);
		if (bRemovedQuickEquipWeapon)
		{
			QuickEquipWeaponSlotTag = FGameplayTag();
		}

		QuickSlotTagMap.Remove(*ExistingKey);

		if (bRemovedQuickEquipWeapon)
		{
			RefreshQuickEquipWeaponSlotTag();
		}
	}
}

void UQuickSlotManagerComponent::UseQuickSlot(const FGameplayTag& QuickSlotTag)
{
	HandleQuickSlotActivation(QuickSlotTag);

	const int64* ItemID = FindItemIDForQuickSlot(QuickSlotTag);
	if (!ItemID)
	{
		return;
	}

	UInventoryComponent* InventoryComponent = ResolveInventoryComponent();
	if (!IsValid(InventoryComponent))
	{
		return;
	}

	UpdateQuickEquipWeaponSlotOnUse(QuickSlotTag);
	InventoryComponent->UseItem(*ItemID, 1);
}

int64 UQuickSlotManagerComponent::GetQuickSlotID(const FGameplayTag& QuickSlotTag) const
{
	return QuickSlotTagMap.Contains(QuickSlotTag) ? QuickSlotTagMap[QuickSlotTag] : 0;
}

bool UQuickSlotManagerComponent::TryEquipWeapon()
{
	if (!QuickEquipWeaponSlotTag.IsValid())
	{
		return false;
	}
	
	bCanTriggerActivation = false; // Dont trigger activation for sheathed use
	UseQuickSlot(QuickEquipWeaponSlotTag);
	
	return true;
}

void UQuickSlotManagerComponent::HandleQuickSlotActivation(const FGameplayTag& QuickSlotTag)
{
	if (bCanTriggerActivation)
	{
		QuickSlotActivatedDelegate.ExecuteIfBound(QuickSlotTag);
		return;
	}

	bCanTriggerActivation = true;
}

const int64* UQuickSlotManagerComponent::FindItemIDForQuickSlot(const FGameplayTag& QuickSlotTag) const
{
	return QuickSlotTagMap.Find(QuickSlotTag);
}

UInventoryComponent* UQuickSlotManagerComponent::ResolveInventoryComponent() const
{
	return IInventoryInterface::Execute_GetInventoryComponent(GetOwner());
}

void UQuickSlotManagerComponent::UpdateQuickEquipWeaponSlotOnUse(const FGameplayTag& QuickSlotTag)
{
	// Cache the latest used weapon slot for sheathed-attack quick equip flows.
	if (QuickSlotTag.MatchesTag(MKHGameplayTags::Equip::WeaponQuickSlotCategory))
	{
		QuickEquipWeaponSlotTag = QuickSlotTag;
	}
}

void UQuickSlotManagerComponent::RefreshQuickEquipWeaponSlotTag()
{
	if (QuickSlotTagMap.Contains(MKHGameplayTags::Equip::WeaponQuickSlot1))
	{
		QuickEquipWeaponSlotTag = MKHGameplayTags::Equip::WeaponQuickSlot1;
		return;
	}

	if (QuickSlotTagMap.Contains(MKHGameplayTags::Equip::WeaponQuickSlot2))
	{
		QuickEquipWeaponSlotTag = MKHGameplayTags::Equip::WeaponQuickSlot2;
	}
}

