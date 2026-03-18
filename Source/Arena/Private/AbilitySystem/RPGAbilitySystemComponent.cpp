// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/RPGAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/RPGGameplayAbility.h"
#include "AbilitySystem/Abilities/ProjectileAbility.h"
#include "Equipment/EquipmentManagerComponent.h"
#include "Equipment/EquipmentTypes.h"
#include "Interfaces/EquipmentInterface.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"

void URPGAbilitySystemComponent::AddCharacterAbilities(const TArray<TSubclassOf<class UGameplayAbility>>& AbilitiesToGrant)
{
	for (const TSubclassOf<UGameplayAbility>& Ability : AbilitiesToGrant) {
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Ability, 1.f);

		if (const URPGGameplayAbility* RPGAbility = Cast<URPGGameplayAbility>(AbilitySpec.Ability))
		{
			AbilitySpec.GetDynamicSpecSourceTags().AddTag(RPGAbility->InputTag);
			GiveAbility(AbilitySpec);
		}
	}
}

void URPGAbilitySystemComponent::AddCharacterPassiveAbilities(const TArray<TSubclassOf<class UGameplayAbility>>& PassivesToGrant)
{
	for (const TSubclassOf<UGameplayAbility>& Ability : PassivesToGrant) {
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Ability, 1.f);
		GiveAbilityAndActivateOnce(AbilitySpec);
	}
}

void URPGAbilitySystemComponent::InitializeDefaultAttributes(const TSubclassOf<class UGameplayEffect>& AttributeEffect)
{
	checkf(AttributeEffect, TEXT("No valid default attributes for this character %s"), *GetAvatarActor()->GetName());

	FGameplayEffectContextHandle ContextHandle = MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(AttributeEffect, 1.f, ContextHandle);
	ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	OnAttributesGiven.Broadcast();
}

void URPGAbilitySystemComponent::AbilityInputPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
		return;

	ABILITYLIST_SCOPE_LOCK();

	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			if (!Spec.IsActive())
			{
				TryActivateAbility(Spec.Handle);
			}
			else
			{	
				const TArray<UGameplayAbility*>& Instances = Spec.GetAbilityInstances();
				if (Instances.Num() > 0)
				{
					UGameplayAbility* AbilityInstance = Instances.Last();
					InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle,
						AbilityInstance->GetCurrentActivationInfo().GetActivationPredictionKey());
				}
			}
		}
	}
}

void URPGAbilitySystemComponent::AbilityInputReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
		return;

	ABILITYLIST_SCOPE_LOCK();

	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			const TArray<UGameplayAbility*>& Instances = Spec.GetAbilityInstances();
			if (Instances.Num() > 0)
			{
				UGameplayAbility* AbilityInstance = Instances.Last();
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle,
					AbilityInstance->GetCurrentActivationInfo().GetActivationPredictionKey());
			}
		}
	}
}

void URPGAbilitySystemComponent::SetDynamicProjectile(const FGameplayTag& ProjectileTag, int32 AbilityLevel)
{
	if (!ProjectileTag.IsValid())
		return;
	if (!GetAvatarActor()->HasAuthority())
	{
		ServerSetDynamicProjectile(ProjectileTag, AbilityLevel);
		return;
	}

	if (ActiveProjectileAbility.IsValid())
	{
		ClearAbility(ActiveProjectileAbility);
	}

	if (IsValid(DynamicProjectileAbility))
	{
		FGameplayAbilitySpec Spec = FGameplayAbilitySpec(DynamicProjectileAbility, AbilityLevel);
		if (UProjectileAbility* ProjectileAbility = Cast<UProjectileAbility>(Spec.Ability))
		{
			ProjectileAbility->ProjectileToSpawnTag = ProjectileTag;
			Spec.GetDynamicSpecSourceTags().AddTag(ProjectileAbility->InputTag);
			ActiveProjectileAbility = GiveAbility(Spec);
		}
	}
}

void URPGAbilitySystemComponent::AddEquipmentEffects(FRPGEquipmentEntry* EquipmentEntry)
{
	if (!EquipmentEntry)
		return;

	FStreamableManager& Manager = UAssetManager::GetStreamableManager();
	TWeakObjectPtr<URPGAbilitySystemComponent> WeakThis(this);
	UEquipmentManagerComponent* EquipmentManager = nullptr;
	if (AbilityActorInfo.IsValid())
	{
		EquipmentManager = IEquipmentInterface::Execute_GetEquipmentManagerComponent(AbilityActorInfo->PlayerController.Get());
	}
	TWeakObjectPtr<UEquipmentManagerComponent> WeakEquipmentManager(EquipmentManager);
	const int64 EntryItemID = EquipmentEntry->OriginalItemID;
	const FGameplayTag EntrySlotTag = EquipmentEntry->SlotTag;
	const FGameplayEffectContextHandle ContextHandle = MakeEffectContext();

	for (const FEquipmentStatEffectDefinition& StatEffect : EquipmentEntry->EffectPackage.StatEffects)
	{
		if (IsValid(StatEffect.EffectClass.Get()))
		{
			const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(StatEffect.EffectClass.Get(), StatEffect.CurrentValue, ContextHandle);
			const FActiveGameplayEffectHandle ActiveHandle = ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

			EquipmentEntry->GrantedHandles.AddEffectHandle(ActiveHandle);
		}
		else
		{
			Manager.RequestAsyncLoad(StatEffect.EffectClass.ToSoftObjectPath(),
				[WeakThis, WeakEquipmentManager, StatEffect, ContextHandle, EntryItemID, EntrySlotTag]
				{
					if (!WeakThis.IsValid())
						return;
					if (!WeakEquipmentManager.IsValid())
						return;

					FRPGEquipmentEntry* ResolvedEntry = WeakEquipmentManager->EquipmentList.FindEntryMutable(EntryItemID, EntrySlotTag);
					if (!ResolvedEntry)
						return;

					const FGameplayEffectSpecHandle SpecHandle = WeakThis->MakeOutgoingSpec(StatEffect.EffectClass.Get(), StatEffect.CurrentValue, ContextHandle);
					const FActiveGameplayEffectHandle ActiveHandle = WeakThis->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

					ResolvedEntry->GrantedHandles.AddEffectHandle(ActiveHandle);
				});
		}
	}
}

void URPGAbilitySystemComponent::RemoveEquipmentEffects(FRPGEquipmentEntry* EquipmentEntry)
{
	for (auto HandleIt = EquipmentEntry->GrantedHandles.ActiveEffects.CreateIterator(); HandleIt; ++HandleIt)
	{
		RemoveActiveGameplayEffect(*HandleIt);
		HandleIt.RemoveCurrent();
	}
}

void URPGAbilitySystemComponent::AddEquipmentAbility(FRPGEquipmentEntry* EquipmentEntry)
{
	if (!EquipmentEntry)
		return;

	FStreamableManager& Manager = UAssetManager::GetStreamableManager();
	TWeakObjectPtr<URPGAbilitySystemComponent> WeakThis(this);
	UEquipmentManagerComponent* EquipmentManager = nullptr;
	if (AbilityActorInfo.IsValid())
	{
		EquipmentManager = IEquipmentInterface::Execute_GetEquipmentManagerComponent(AbilityActorInfo->PlayerController.Get());
	}
	TWeakObjectPtr<UEquipmentManagerComponent> WeakEquipmentManager(EquipmentManager);
	const int64 EntryItemID = EquipmentEntry->OriginalItemID;
	const FGameplayTag EntrySlotTag = EquipmentEntry->SlotTag;

	for (const FEquipmentAbilityDefinition& AbilityDef : EquipmentEntry->EffectPackage.Abilities)
	{
		if (IsValid(AbilityDef.AbilityClass.Get()))
		{
			EquipmentEntry->GrantedHandles.AddAbilityHandle(GrantEquipmentAbility(AbilityDef));
		}
		else
		{
			Manager.RequestAsyncLoad(AbilityDef.AbilityClass.ToSoftObjectPath(),
				[WeakThis, WeakEquipmentManager, AbilityDef, EntryItemID, EntrySlotTag]
				{
					if (!WeakThis.IsValid())
						return;
					if (!WeakEquipmentManager.IsValid())
						return;

					FRPGEquipmentEntry* ResolvedEntry = WeakEquipmentManager->EquipmentList.FindEntryMutable(EntryItemID, EntrySlotTag);
					if (!ResolvedEntry)
						return;

					ResolvedEntry->GrantedHandles.AddAbilityHandle(WeakThis->GrantEquipmentAbility(AbilityDef));
				});
		}
	}
}

void URPGAbilitySystemComponent::RemoveEquipmentAbility(FRPGEquipmentEntry* EquipmentEntry)
{
	for (auto HandleIt = EquipmentEntry->GrantedHandles.GrantedAbilities.CreateConstIterator(); HandleIt; ++HandleIt)
	{
		ClearAbility(*HandleIt);
	}
	EquipmentEntry->GrantedHandles.GrantedAbilities.Empty();
}

FGameplayAbilitySpecHandle URPGAbilitySystemComponent::GrantEquipmentAbility(const FEquipmentAbilityDefinition& AbilityDef)
{
	FGameplayAbilitySpec Spec = FGameplayAbilitySpec(AbilityDef.AbilityClass.Get(), AbilityDef.AbilityLevel);

	if (URPGGameplayAbility* RPGAbility = Cast<URPGGameplayAbility>(Spec.Ability))
	{
		if (AbilityDef.bIsSkillAbility)
		{
			// Ovveride the input tag for skills because they are set dynamically
			RPGAbility->InputTag = AbilityDef.SkillInputTag;
		}
		Spec.GetDynamicSpecSourceTags().AddTag(RPGAbility->InputTag);
	}

	if (UProjectileAbility* ProjectileAbility = Cast<UProjectileAbility>(Spec.Ability))
	{
		ProjectileAbility->ProjectileToSpawnTag = AbilityDef.ContextTag;
	}

	return GiveAbility(Spec);
}

void URPGAbilitySystemComponent::ServerSetDynamicProjectile_Implementation(const FGameplayTag& ProjectileTag, int32 AbilityLevel)
{
	SetDynamicProjectile(ProjectileTag, AbilityLevel);
}
