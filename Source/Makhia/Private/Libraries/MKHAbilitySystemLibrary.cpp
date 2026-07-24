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

	if (FMKHGameplayEffectContext* MKHContext = FMKHGameplayEffectContext::GetEffectContext(ContextHandle))
	{
		MKHContext->SetTargetStatusEffects(DamageEffectInfo.TargetStatusEffects);
		MKHContext->SetSelfOnHitStatusEffects(DamageEffectInfo.SelfOnHitStatusEffects);
	}

	const FGameplayEffectSpecHandle DamageSpecHandle = DamageEffectInfo.SourceASC->MakeOutgoingSpec(DamageEffectInfo.DamageEffect,
		1.f, ContextHandle);

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(DamageSpecHandle, MKHGameplayTags::Combat::Data_Damage, DamageEffectInfo.DamageMultiplier);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(DamageSpecHandle, MKHGameplayTags::Combat::Data_StaminaDamage, DamageEffectInfo.StaminaDamage);

	DamageEffectInfo.TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpecHandle.Data.Get());
}

void UMKHAbilitySystemLibrary::ApplyStatusEffects(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC,
	const TArray<FStatusEffectData>& StatusEffects, const FGameplayEffectContextHandle& ContextHandle)
{
	if (!IsValid(SourceASC) || !IsValid(TargetASC))
	{
		return;
	}

	for (const auto& [EffectClass, EffectDuration, EffectValue] : StatusEffects)
	{
		if (!IsValid(EffectClass))
		{
			continue;
		}

		const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, 1.0f, ContextHandle);
		if (!SpecHandle.IsValid())
		{
			continue;
		}

		// The status effect duration and value travel as set-by-caller magnitudes.
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, MKHGameplayTags::Combat::Data_StatusEffectDuration, EffectDuration);
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, MKHGameplayTags::Combat::Data_StatusEffectValue, EffectValue);

		TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
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