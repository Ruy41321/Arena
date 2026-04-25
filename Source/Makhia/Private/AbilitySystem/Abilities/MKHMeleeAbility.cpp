// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/MKHMeleeAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/MKHGameplayTags.h"
#include "Equipment/Weapon/MKHWeaponBase.h"

void UMKHMeleeAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (ActorInfo && ActorInfo->IsNetAuthority())
	{
		BindHitScanEvents();
	}
}

void UMKHMeleeAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	
	OnHitScanEndReceived(FGameplayEventData());
}

void UMKHMeleeAbility::OnMontageStarted_Implementation()
{
	Super::OnMontageStarted_Implementation();
	
	if (!bIsComboAbility)
		return;
	
	ComboHitCounter = 1;
	ComboDamageMultiplier = 0.f;
	
	BindOnContinueComboEvents();
}

void UMKHMeleeAbility::OnAbilityActivatedAgain_Implementation(float TimeWaited)
{
	Super::OnAbilityActivatedAgain_Implementation(TimeWaited);
	
	if (bIsWithinComboWindow)
	{
		bContinueCombo = true;
	}
}

float UMKHMeleeAbility::GetBaseDamageValue(float WeaponDamage)
{
	return WeaponDamage * (DamagePercent + ComboDamageMultiplier);
}

void UMKHMeleeAbility::HitScanStart_Implementation()
{
	if (IsValid(OwningWeapon))
	{
		FDamageEffectInfo DamageEffectInfo;
		CaptureDamageEffectInfo(nullptr, DamageEffectInfo);
		OwningWeapon->HitScanStart(DamageEffectInfo);
	}
}

void UMKHMeleeAbility::HitScanEnd_Implementation()
{
	if (IsValid(OwningWeapon))
	{
		OwningWeapon->HitScanEnd();
	}
}

void UMKHMeleeAbility::BindHitScanEvents()
{
	UAbilityTask_WaitGameplayEvent* HitScanEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		MKHGameplayTags::Event::HitScanStart
	);
	
	if (IsValid(HitScanEvent))
	{
		HitScanEvent->EventReceived.AddDynamic(this, &UMKHMeleeAbility::OnHitScanStartReceived);
		HitScanEvent->ReadyForActivation();
	}
	
	HitScanEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		MKHGameplayTags::Event::HitScanEnd
	);
	
	if (IsValid(HitScanEvent))
	{
		HitScanEvent->EventReceived.AddDynamic(this, &UMKHMeleeAbility::OnHitScanEndReceived);
		HitScanEvent->ReadyForActivation();
	}
}

void UMKHMeleeAbility::BindOnContinueComboEvents()
{
	UAbilityTask_WaitGameplayEvent* WaitEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		MKHGameplayTags::Event::ContinueComboStart
	);
	
	if (IsValid(WaitEvent))
	{
		WaitEvent->EventReceived.AddDynamic(this, &UMKHMeleeAbility::OnContinueComboStartReceived);
		WaitEvent->ReadyForActivation();
	}
	
	WaitEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		MKHGameplayTags::Event::ContinueComboEnd
	);
	
	if (IsValid(WaitEvent))
	{
		WaitEvent->EventReceived.AddDynamic(this, &UMKHMeleeAbility::OnContinueComboEndReceived);
		WaitEvent->ReadyForActivation();
	}
}

void UMKHMeleeAbility::OnHitScanStartReceived(FGameplayEventData Payload)
{
	HitScanStart();
}

void UMKHMeleeAbility::OnHitScanEndReceived(FGameplayEventData Payload)
{
	HitScanEnd();
}

void UMKHMeleeAbility::OnContinueComboStartReceived(FGameplayEventData Payload)
{
	bIsWithinComboWindow = true;
	bContinueCombo = false;
}

void UMKHMeleeAbility::OnContinueComboEndReceived(FGameplayEventData Payload)
{
	bIsWithinComboWindow = false;
	if (bContinueCombo)
	{
		++ComboHitCounter;
		OnComboTriggered(ComboHitCounter);
	}
	else
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}
