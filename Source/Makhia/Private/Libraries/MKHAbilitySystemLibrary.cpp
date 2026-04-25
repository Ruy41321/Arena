// Fill out your copyright notice in the Description page of Project Settings.


#include "Libraries/MKHAbilitySystemLibrary.h"
#include "AbilitySystem/MKHAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "GameMode/MKHGameMode.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/MKHGameplayTags.h"
#include "Equipment/EquipmentTypes.h"
#include "Inventory/InventoryComponent.h"
#include "Kismet/GameplayStatics.h"

UCharacterClassInfo* UMKHAbilitySystemLibrary::GetCharacterClassDefaultInfo(const UObject* WorldContextObject)
{
	if (const AMKHGameMode* MKHGameMode = Cast<AMKHGameMode>(UGameplayStatics::GetGameMode(WorldContextObject)))
	{
		return MKHGameMode->GetCharacterClassDefaultInfo();
	}

	return nullptr;
}

UProjectileInfo* UMKHAbilitySystemLibrary::GetProjectileInfo(const UObject* WorldContextObject)
{
	if (const AMKHGameMode* MKHGameMode = Cast<AMKHGameMode>(UGameplayStatics::GetGameMode(WorldContextObject)))
	{
		return MKHGameMode->GetProjectileInfo();
	}

	return nullptr;
}

void UMKHAbilitySystemLibrary::ApplyDamageEffect(const FDamageEffectInfo& DamageEffectInfo)
{
	if (!IsValid(DamageEffectInfo.SourceASC) or !IsValid(DamageEffectInfo.TargetASC))
		return;
	
	FGameplayEffectContextHandle ContextHandle = DamageEffectInfo.SourceASC->MakeEffectContext();
	ContextHandle.AddSourceObject(DamageEffectInfo.AvatarActor);
	
	const FGameplayEffectSpecHandle SpecHandle = DamageEffectInfo.SourceASC->MakeOutgoingSpec(DamageEffectInfo.DamageEffect,
		1.f, ContextHandle);

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, MKHGameplayTags::Combat::Data_Damage, DamageEffectInfo.BaseDamage);

	DamageEffectInfo.TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void UMKHAbilitySystemLibrary::K2_SetLooseTagCountStatic(UAbilitySystemComponent* ASC, FGameplayTag Tag, int32 NewCount)
{
	if (IsValid(ASC))
	{
		ASC->SetLooseGameplayTagCount(Tag, NewCount);
	}
}

void UMKHAbilitySystemLibrary::AssignDynamicSkillInputTag(FRPGInventoryEntry& NewEntry)
{
	TArray<FGameplayTag> SkillInputTag = {
		MKHGameplayTags::Input::SkillSlot1, 
		MKHGameplayTags::Input::SkillSlot2, 
		MKHGameplayTags::Input::SkillSlot3
	};
	
	uint8 i = 0;
	for (FEquipmentAbilityDefinition& Ability : NewEntry.EffectPackage.Abilities)
	{
		// Assign a dynamic Input Tag to Skill Abilities in order of as
		if (Ability.bIsSkillAbility)
			Ability.SkillInputTag = SkillInputTag[i++];
			
		// Bind Maximum the num of SkillInputTag available
		if (i == SkillInputTag.Num())
			break;
	}
}