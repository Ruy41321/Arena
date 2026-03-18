// Fill out your copyright notice in the Description page of Project Settings.


#include "Libraries/RPGAbilitySystemLibrary.h"
#include "AbilitySystem/RPGAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "GameMode/RPGGameMode.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/RPGGameplayTags.h"
#include "Equipment/EquipmentTypes.h"
#include "Inventory/InventoryComponent.h"
#include "Kismet/GameplayStatics.h"

UCharacterClassInfo* URPGAbilitySystemLibrary::GetCharacterClassDefaultInfo(const UObject* WorldContextObject)
{
	if (const ARPGGameMode* RPGGameMode = Cast<ARPGGameMode>(UGameplayStatics::GetGameMode(WorldContextObject)))
	{
		return RPGGameMode->GetCharacterClassDefaultInfo();
	}

	return nullptr;
}

UProjectileInfo* URPGAbilitySystemLibrary::GetProjectileInfo(const UObject* WorldContextObject)
{
	if (const ARPGGameMode* RPGGameMode = Cast<ARPGGameMode>(UGameplayStatics::GetGameMode(WorldContextObject)))
	{
		return RPGGameMode->GetProjectileInfo();
	}

	return nullptr;
}

void URPGAbilitySystemLibrary::ApplyDamageEffect(const FDamageEffectInfo& DamageEffectInfo)
{
	if (!IsValid(DamageEffectInfo.SourceASC) or !IsValid(DamageEffectInfo.TargetASC))
		return;
	
	FGameplayEffectContextHandle ContextHandle = DamageEffectInfo.SourceASC->MakeEffectContext();
	ContextHandle.AddSourceObject(DamageEffectInfo.AvatarActor);
	
	const FGameplayEffectSpecHandle SpecHandle = DamageEffectInfo.SourceASC->MakeOutgoingSpec(DamageEffectInfo.DamageEffect, 
		DamageEffectInfo.AbilityLevel, ContextHandle);

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, RPGGameplayTags::Combat::Data_Damage, DamageEffectInfo.BaseDamage);

	DamageEffectInfo.TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void URPGAbilitySystemLibrary::K2_SetLooseTagCountStatic(UAbilitySystemComponent* ASC, FGameplayTag Tag, int32 NewCount)
{
	if (IsValid(ASC))
	{
		ASC->SetLooseGameplayTagCount(Tag, NewCount);
	}
}

void URPGAbilitySystemLibrary::AssignDynamicSkillInputTag(FRPGInventoryEntry& NewEntry)
{
	TArray<FGameplayTag> SkillInputTag = {
		RPGGameplayTags::Input::SkillSlot1, 
		RPGGameplayTags::Input::SkillSlot2, 
		RPGGameplayTags::Input::SkillSlot3
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