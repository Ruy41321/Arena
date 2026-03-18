// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/RPGGameplayAbility.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"

void URPGGameplayAbility::OnAbilityActivatedAgain_Implementation(float TimeWaited)
{
	BindInputPressEvent();
}

void URPGGameplayAbility::OnAbilityReleased_Implementation(float TimeWaited)
{
	BindInputReleaseEvent();
}

void URPGGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	BindInputPressEvent();
	BindInputReleaseEvent();
		
	CommitAbility(Handle, ActorInfo, ActivationInfo);
}

void URPGGameplayAbility::BindInputPressEvent()
{
	UAbilityTask_WaitInputPress* InputPressEvent = UAbilityTask_WaitInputPress::WaitInputPress(
		this
	);
	
	if (InputPressEvent)
	{
		InputPressEvent->OnPress.AddDynamic(this, &URPGGameplayAbility::OnAbilityActivatedAgain);
		InputPressEvent->ReadyForActivation();
	}
}

void URPGGameplayAbility::BindInputReleaseEvent()
{
	UAbilityTask_WaitInputRelease* InputReleaseEvent = UAbilityTask_WaitInputRelease::WaitInputRelease(
		this
	);
	
	if (InputReleaseEvent)
	{
		InputReleaseEvent->OnRelease.AddDynamic(this, &URPGGameplayAbility::OnAbilityReleased);
		InputReleaseEvent->ReadyForActivation();
	}
}
