// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/MKHGameplayAbility.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystem/MKHGameplayTags.h"
#include "Player/MKHPlayerCharacter.h"

UMKHGameplayAbility::UMKHGameplayAbility()
{
	SetAssetTags(FGameplayTagContainer(MKHGameplayTags::Ability::All));
	CancelAbilitiesWithTag.AddTag(MKHGameplayTags::Ability::All);
	ActivationOwnedTags.AddTag(MKHGameplayTags::Ability::AbilityActive);
}

void UMKHGameplayAbility::OnAbilityActivatedAgain_Implementation(float TimeWaited)
{
	BindInputPressEvent();
}

void UMKHGameplayAbility::OnAbilityReleased_Implementation(float TimeWaited)
{
	BindInputReleaseEvent();
}

void UMKHGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	BindInputPressEvent();
	BindInputReleaseEvent();
		
	CommitAbility(Handle, ActorInfo, ActivationInfo);
}

void UMKHGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);
	
	if (const UMKHGameplayAbility* CDO = Cast<UMKHGameplayAbility>(Spec.Ability))
	{
		InputTag = CDO->InputTag;
	}
}

void UMKHGameplayAbility::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	OnAbilityRemoved();
	
	Super::OnRemoveAbility(ActorInfo, Spec);
}

void UMKHGameplayAbility::SetCharacterOrientation(bool bFollowCamera)
{
	// Get the actor safely, using IsValid
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (IsValid(AvatarActor))
	{
		if (AMKHPlayerCharacter* Pl = Cast<AMKHPlayerCharacter>(AvatarActor))
		{
			Pl->bUseControllerRotationYaw = bFollowCamera;
		}
	}
}

void UMKHGameplayAbility::BindInputPressEvent()
{
	UAbilityTask_WaitInputPress* InputPressEvent = UAbilityTask_WaitInputPress::WaitInputPress(this);
	
	if (IsValid(InputPressEvent))
	{
		InputPressEvent->OnPress.AddDynamic(this, &UMKHGameplayAbility::OnAbilityActivatedAgain);
		InputPressEvent->ReadyForActivation();
	}
}

void UMKHGameplayAbility::BindInputReleaseEvent()
{
	UAbilityTask_WaitInputRelease* InputReleaseEvent = UAbilityTask_WaitInputRelease::WaitInputRelease(this);
	
	if (IsValid(InputReleaseEvent))
	{
		InputReleaseEvent->OnRelease.AddDynamic(this, &UMKHGameplayAbility::OnAbilityReleased);
		InputReleaseEvent->ReadyForActivation();
	}
}
