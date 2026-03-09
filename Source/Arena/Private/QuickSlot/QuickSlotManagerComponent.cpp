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
	
}

void UQuickSlotManagerComponent::RemoveQuickSlot(int64 ItemID)
{
	// Find the key associated with the given ItemID and remove it from the map if it exists
	const FGameplayTag* ExistingKey = QuickSlotTagMap.FindKey(ItemID);
	if (ExistingKey)
	{
		QuickSlotTagMap.Remove(*ExistingKey);
	}
}

void UQuickSlotManagerComponent::UseQuickSlot(const FGameplayTag& QuickSlotTag)
{
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
	
	InventoryComponent->UseItem(*ItemID, 1);
}

int64 UQuickSlotManagerComponent::GetQuickSlotID(const FGameplayTag& QuickSlotTag) const
{
	return QuickSlotTagMap.Contains(QuickSlotTag) ? QuickSlotTagMap[QuickSlotTag] : 0;
}
