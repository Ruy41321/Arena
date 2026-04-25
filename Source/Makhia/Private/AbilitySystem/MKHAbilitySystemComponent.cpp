// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/MKHAbilitySystemComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/MKHGameplayTags.h"
#include "AbilitySystem/Abilities/MKHGameplayAbility.h"
#include "AbilitySystem/Abilities/MKHDamageAbility.h"
#include "AbilitySystem/Abilities/MKHProjectileAbility.h"
#include "Equipment/EquipmentManagerComponent.h"
#include "Equipment/EquipmentTypes.h"
#include "Interfaces/EquipmentInterface.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"

void UMKHAbilitySystemComponent::AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& AbilitiesToGrant)
{
	for (const TSubclassOf<UGameplayAbility>& Ability : AbilitiesToGrant)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Ability, 1.f);

		if (const UMKHGameplayAbility* RPGAbility = Cast<UMKHGameplayAbility>(AbilitySpec.Ability))
		{
			AbilitySpec.GetDynamicSpecSourceTags().AddTag(RPGAbility->InputTag);
			GiveAbility(AbilitySpec);
		}
	}
}

void UMKHAbilitySystemComponent::AddCharacterPassiveAbilities(const TArray<TSubclassOf<UGameplayAbility>>& PassivesToGrant)
{
	for (const TSubclassOf<UGameplayAbility>& Ability : PassivesToGrant)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Ability, 1.f);
		GiveAbilityAndActivateOnce(AbilitySpec);
	}
}

void UMKHAbilitySystemComponent::InitializeDefaultAttributes(const TSubclassOf<UGameplayEffect>& AttributeEffect)
{
	checkf(AttributeEffect, TEXT("No valid default attributes for this character %s"), *GetNameSafe(GetAvatarActor()));

	FGameplayEffectContextHandle ContextHandle = MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(AttributeEffect, 1.f, ContextHandle);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	OnAttributesGiven.Broadcast();
}

void UMKHAbilitySystemComponent::AbilityInputPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
		return;

	ABILITYLIST_SCOPE_LOCK();

	const bool bIsQuickSlotInput = IsQuickSlotInput(InputTag);

	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (!DoesSpecMatchInput(Spec, InputTag, bIsQuickSlotInput))
			continue;

		if (bIsQuickSlotInput)
		{
			SendQuickSlotEvent(InputTag);
		}

		HandleAbilityInputPressedForSpec(Spec);
	}
}

void UMKHAbilitySystemComponent::AbilityInputReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
		return;

	ABILITYLIST_SCOPE_LOCK();

	const bool bIsQuickSlotInput = IsQuickSlotInput(InputTag);

	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (!DoesSpecMatchInput(Spec, InputTag, bIsQuickSlotInput))
			continue;

		if (bIsQuickSlotInput)
			continue;
		
		HandleAbilityInputReleasedForSpec(Spec);
	}
}

bool UMKHAbilitySystemComponent::IsQuickSlotInput(const FGameplayTag& InputTag) const
{
	return InputTag.MatchesTag(MKHGameplayTags::Input::QuickSlot);
}

bool UMKHAbilitySystemComponent::DoesSpecMatchInput(const FGameplayAbilitySpec& Spec, const FGameplayTag& InputTag,
	const bool bIsQuickSlotInput) const
{
	FGameplayTag TagToConfront = InputTag;
	if (bIsQuickSlotInput)
		TagToConfront = MKHGameplayTags::Input::QuickSlot;

	return Spec.GetDynamicSpecSourceTags().HasTagExact(TagToConfront);
}

void UMKHAbilitySystemComponent::SendQuickSlotEvent(const FGameplayTag& InputTag) const
{
	FGameplayEventData Payload;
	Payload.TargetTags.AddTag(InputTag);
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetAvatarActor(), MKHGameplayTags::Event::UseQuickSlot, Payload);
}

void UMKHAbilitySystemComponent::HandleAbilityInputPressedForSpec(const FGameplayAbilitySpec& Spec)
{
	if (!Spec.IsActive())
	{
		TryActivateAbility(Spec.Handle);
		return;
	}

	if (const UGameplayAbility* PrimaryInstance = Spec.GetPrimaryInstance(); IsValid(PrimaryInstance))
	{
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle,
			PrimaryInstance->GetCurrentActivationInfo().GetActivationPredictionKey());
	}
}

void UMKHAbilitySystemComponent::HandleAbilityInputReleasedForSpec(const FGameplayAbilitySpec& Spec)
{
	if (const UGameplayAbility* PrimaryInstance = Spec.GetPrimaryInstance(); IsValid(PrimaryInstance))
	{
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle,
			PrimaryInstance->GetCurrentActivationInfo().GetActivationPredictionKey());
	}
}

void UMKHAbilitySystemComponent::AddEquipmentEffects(FRPGEquipmentEntry* EquipmentEntry)
{
	if (!EquipmentEntry)
		return;

	const FGameplayEffectContextHandle ContextHandle = MakeEffectContext();

	for (const FEquipmentStatEffectDefinition& StatEffect : EquipmentEntry->EffectPackage.StatEffects)
	{
		GrantEquipmentStatEffect(*EquipmentEntry, StatEffect, ContextHandle);
	}
}

void UMKHAbilitySystemComponent::RemoveEquipmentEffects(FRPGEquipmentEntry* EquipmentEntry)
{
	if (!EquipmentEntry)
	{
		return;
	}

	for (auto HandleIt = EquipmentEntry->GrantedHandles.ActiveEffects.CreateIterator(); HandleIt; ++HandleIt)
	{
		RemoveActiveGameplayEffect(*HandleIt);
		HandleIt.RemoveCurrent();
	}
}

void UMKHAbilitySystemComponent::AddEquipmentAbility(FRPGEquipmentEntry* EquipmentEntry)
{
	if (!EquipmentEntry)
		return;

	for (const FEquipmentAbilityDefinition& AbilityDef : EquipmentEntry->EffectPackage.Abilities)
	{
		GrantEquipmentAbilityDefinition(*EquipmentEntry, AbilityDef);
	}
}

void UMKHAbilitySystemComponent::RemoveEquipmentAbility(FRPGEquipmentEntry* EquipmentEntry)
{
	if (!EquipmentEntry)
	{
		return;
	}

	for (auto HandleIt = EquipmentEntry->GrantedHandles.GrantedAbilities.CreateConstIterator(); HandleIt; ++HandleIt)
	{
		ClearAbility(*HandleIt);
	}
	EquipmentEntry->GrantedHandles.GrantedAbilities.Empty();
}

void UMKHAbilitySystemComponent::GetCooldownRemainingForTag(const FGameplayTag CooldownTag, float& TimeRemaining,
	float& CooldownDuration) const
{
	TimeRemaining = 0.f;
	CooldownDuration = 0.f;

	if (CooldownTag.IsValid())
	{
		// Creating a query to find the Effect that grants the CooldownTag
		FGameplayEffectQuery const Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(CooldownTag));
        
		// Gets the Remaining Times of all Effects found with the query
		TArray<float> Durations = GetActiveEffectsTimeRemaining(Query);
        
		if (Durations.Num() > 0)
		{
			TimeRemaining = FMath::Max(Durations[0], 0.f);
            
			// Get the Total Duration as well
			TArray<float> TotalDurations = GetActiveEffectsDuration(Query);
			CooldownDuration = TotalDurations[0];
		}
	}
}

TWeakObjectPtr<UEquipmentManagerComponent> UMKHAbilitySystemComponent::GetWeakEquipmentManager() const
{
	if (!AbilityActorInfo.IsValid())
	{
		return nullptr;
	}

	APlayerController* PlayerController = AbilityActorInfo->PlayerController.Get();
	if (!IsValid(PlayerController))
	{
		return nullptr;
	}

	if (!PlayerController->GetClass()->ImplementsInterface(UEquipmentInterface::StaticClass()))
	{
		return nullptr;
	}

	return IEquipmentInterface::Execute_GetEquipmentManagerComponent(PlayerController);
}

FRPGEquipmentEntry* UMKHAbilitySystemComponent::FindEquipmentEntry(UEquipmentManagerComponent* EquipmentManager, const int64 ItemId,
	const FGameplayTag& SlotTag) const
{
	if (!IsValid(EquipmentManager))
	{
		return nullptr;
	}

	return EquipmentManager->EquipmentList.FindEntryMutable(ItemId, SlotTag);
}

void UMKHAbilitySystemComponent::ApplyAndTrackStatEffect(FRPGEquipmentEntry& EquipmentEntry,
	const FEquipmentStatEffectDefinition& StatEffect, const FGameplayEffectContextHandle& ContextHandle)
{
	if (!IsValid(StatEffect.EffectClass.Get()))
	{
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(StatEffect.EffectClass.Get(), StatEffect.CurrentValue, ContextHandle);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	const FActiveGameplayEffectHandle ActiveHandle = ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	EquipmentEntry.GrantedHandles.AddEffectHandle(ActiveHandle);
}

void UMKHAbilitySystemComponent::GrantEquipmentStatEffect(FRPGEquipmentEntry& EquipmentEntry,
	const FEquipmentStatEffectDefinition& StatEffect, const FGameplayEffectContextHandle& ContextHandle)
{
	if (IsValid(StatEffect.EffectClass.Get()))
	{
		ApplyAndTrackStatEffect(EquipmentEntry, StatEffect, ContextHandle);
		return;
	}

	FStreamableManager& Manager = UAssetManager::GetStreamableManager();
	const TWeakObjectPtr<UMKHAbilitySystemComponent> WeakThis(this);
	const TWeakObjectPtr<UEquipmentManagerComponent> WeakEquipmentManager = GetWeakEquipmentManager();
	const int64 EntryItemID = EquipmentEntry.OriginalItemID;
	const FGameplayTag EntrySlotTag = EquipmentEntry.SlotTag;

	Manager.RequestAsyncLoad(StatEffect.EffectClass.ToSoftObjectPath(),
		[WeakThis, WeakEquipmentManager, StatEffect, ContextHandle, EntryItemID, EntrySlotTag]
		{
			if (!WeakThis.IsValid() || !WeakEquipmentManager.IsValid()) return;

			if (FRPGEquipmentEntry* ResolvedEntry = WeakThis->FindEquipmentEntry(WeakEquipmentManager.Get(), EntryItemID, EntrySlotTag))
			{
				WeakThis->ApplyAndTrackStatEffect(*ResolvedEntry, StatEffect, ContextHandle);
			}
		});
}

void UMKHAbilitySystemComponent::ApplyAndTrackEquipmentAbility(FRPGEquipmentEntry& EquipmentEntry,
	const FEquipmentAbilityDefinition& AbilityDef)
{
	if (!IsValid(AbilityDef.AbilityClass.Get()))
	{
		return;
	}

	EquipmentEntry.GrantedHandles.AddAbilityHandle(GrantEquipmentAbility(AbilityDef));
}

void UMKHAbilitySystemComponent::GrantEquipmentAbilityDefinition(FRPGEquipmentEntry& EquipmentEntry,
	const FEquipmentAbilityDefinition& AbilityDef)
{
	if (IsValid(AbilityDef.AbilityClass.Get()))
	{
		ApplyAndTrackEquipmentAbility(EquipmentEntry, AbilityDef);
		return;
	}

	FStreamableManager& Manager = UAssetManager::GetStreamableManager();
	const TWeakObjectPtr<UMKHAbilitySystemComponent> WeakThis(this);
	const TWeakObjectPtr<UEquipmentManagerComponent> WeakEquipmentManager = GetWeakEquipmentManager();
	const int64 EntryItemID = EquipmentEntry.OriginalItemID;
	const FGameplayTag EntrySlotTag = EquipmentEntry.SlotTag;

	Manager.RequestAsyncLoad(AbilityDef.AbilityClass.ToSoftObjectPath(),
		[WeakThis, WeakEquipmentManager, AbilityDef, EntryItemID, EntrySlotTag]
		{
			if (!WeakThis.IsValid() || !WeakEquipmentManager.IsValid()) return;

			if (FRPGEquipmentEntry* ResolvedEntry = WeakThis->FindEquipmentEntry(WeakEquipmentManager.Get(), EntryItemID, EntrySlotTag))
			{
				WeakThis->ApplyAndTrackEquipmentAbility(*ResolvedEntry, AbilityDef);
			}
		});
}

FGameplayAbilitySpecHandle UMKHAbilitySystemComponent::GrantEquipmentAbility(const FEquipmentAbilityDefinition& AbilityDef)
{
	FGameplayAbilitySpec Spec = FGameplayAbilitySpec(AbilityDef.AbilityClass.Get(), 1.f);

	if (UMKHGameplayAbility* RPGAbility = Cast<UMKHGameplayAbility>(Spec.Ability))
	{
		if (AbilityDef.bIsSkillAbility)
		{
			// Ovveride the input tag for skills because they are set dynamically
			RPGAbility->InputTag = AbilityDef.SkillInputTag; // AbilityDef.SkillInputTag is set when the skill is rolled
		}
		Spec.GetDynamicSpecSourceTags().AddTag(RPGAbility->InputTag);
	}

	if (UMKHProjectileAbility* ProjectileAbility = Cast<UMKHProjectileAbility>(Spec.Ability))
	{
		ProjectileAbility->ProjectileToSpawnTag = AbilityDef.ContextTag;
	}

	if (UMKHDamageAbility* DamageAbility = Cast<UMKHDamageAbility>(Spec.Ability))
	{
		DamageAbility->SetDamagePercent(AbilityDef.DamagePercent);
		DamageAbility->SetIsSkillAbility(AbilityDef.bIsSkillAbility);
		if (AbilityDef.bIsSkillAbility)
		{
			DamageAbility->SetCooldownData(AbilityDef.CooldownTime, AbilityDef.CooldownTag.GetSingleTagContainer());
		}
	}

	return GiveAbility(Spec);
}
