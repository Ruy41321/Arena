// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/RPGDamageAbility.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystem/RPGAbilityTypes.h"
#include "Equipment/EquipmentManagerComponent.h"
#include "Player/PlayerCharacter.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

void URPGDamageAbility::CaptureDamageEffectInfo(AActor* TargetActor, FDamageEffectInfo& OutInfo, const float DamageMultiplier)
{
	if (AActor* AvatarActorFromInfo = GetAvatarActorFromActorInfo())
	{
		OutInfo.AvatarActor = AvatarActorFromInfo;
		OutInfo.AbilityLevel = GetAbilityLevel();
		OutInfo.BaseDamage = BaseDamage.GetValueAtLevel(GetAbilityLevel()) * (1 + DamageMultiplier);
		OutInfo.DamageEffect = DamageEffect;
		OutInfo.SourceASC = GetAbilitySystemComponentFromActorInfo();

		if (IsValid(TargetActor))
		{
				OutInfo.TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
		}
	}
}

void URPGDamageAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!PlayAnimation())
	{
		return EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
	OnMontageStarted();
}

void URPGDamageAbility::PreActivate(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,FOnGameplayAbilityEnded::FDelegate* OnGameplayAbilityEndedDelegate, 
	const FGameplayEventData* TriggerEventData)
{
	Super::PreActivate(Handle, ActorInfo, ActivationInfo, OnGameplayAbilityEndedDelegate, TriggerEventData);
	
	FaceCharacterTowardsAttack();
	
}

void URPGDamageAbility::OnMontageStarted_Implementation()
{
	// Does Nothing; To be Overriden
}

bool URPGDamageAbility::PlayAnimation()
{
	if (!IsValid(AttackMontageToPlay))
	{
		UE_LOG(LogTemp, Warning, TEXT("Not Valid Montage to Play"));
		return false;
	}
	
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		AttackMontageToPlay,
		1.0f,
		NAME_None,
		true,
		1.0f,
		0.0f,
		true
	);
	
	if (!MontageTask)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to create MontageTask!"));
		return false;
	}
	
	MontageTask->OnCompleted.AddDynamic(this, &URPGDamageAbility::OnMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &URPGDamageAbility::OnMontageFinished);
	MontageTask->OnCancelled.AddDynamic(this, &URPGDamageAbility::OnMontageFinished);
 
	MontageTask->ReadyForActivation();
	return true;
}

void URPGDamageAbility::FaceCharacterTowardsAttack()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	APlayerCharacter* Character = Cast<APlayerCharacter>(AvatarActor);
	
	FRotator ControlRotation = Character->GetControlRotation();
	FRotator TargetRotation = FRotator(0.f, ControlRotation.Yaw, 0.f);
	Character->SetActorRotation(TargetRotation);
}

void URPGDamageAbility::OnMontageFinished()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

