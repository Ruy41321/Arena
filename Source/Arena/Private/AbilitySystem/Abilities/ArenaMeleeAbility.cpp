// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/ArenaMeleeAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/RPGGameplayTags.h"
#include "Equipment/EquipmentInstance.h"
#include "Equipment/EquipmentManagerComponent.h"
#include "Equipment/Weapon/ArenaWeaponBase.h"
#include "Interfaces/EquipmentInterface.h"
#include "Player/PlayerCharacter.h"

void UArenaMeleeAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (ActorInfo && ActorInfo->IsNetAuthority())
	{
		if (!InitOwningWeapon())
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to initialize owning weapon for melee ability. Ending ability."));
			return EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		}
	}
	
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (ActorInfo && ActorInfo->IsNetAuthority())
	{
		BindHitScanEvents();
	}
}

void UArenaMeleeAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	
	OnHitScanEndReceived(FGameplayEventData());
}

void UArenaMeleeAbility::OnMontageStarted_Implementation()
{
	Super::OnMontageStarted_Implementation();
	
	if (!bIsComboAbility)
		return;
	
	ComboHitCounter = 1;
	ComboDamageMultiplier = 0.f;
	
	BindOnContinueComboEvents();
}

void UArenaMeleeAbility::OnAbilityActivatedAgain_Implementation(float TimeWaited)
{
	Super::OnAbilityActivatedAgain_Implementation(TimeWaited);
	
	if (bIsWithinComboWindow)
	{
		bContinueCombo = true;
	}
}

AArenaWeaponBase* UArenaMeleeAbility::InitOwningWeapon()
{
	OwningWeapon = nullptr;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(AvatarActor);
	if (!PlayerCharacter)
	{
		return nullptr;
	}

	AController* Controller = PlayerCharacter->GetController();
	if (!IsValid(Controller) || !Controller->GetClass()->ImplementsInterface(UEquipmentInterface::StaticClass()))
	{
		return nullptr;
	}

	UEquipmentManagerComponent* EquipmentManager = IEquipmentInterface::Execute_GetEquipmentManagerComponent(Controller);
	if (!IsValid(EquipmentManager))
	{
		return nullptr;
	}

	UEquipmentInstance* WeaponInstance = EquipmentManager->GetEquipmentInstanceBySlot(RPGGameplayTags::Equip::WeaponSlot);
	if (!IsValid(WeaponInstance))
	{
		return nullptr;
	}

	const TArray<AActor*>& SpawnedActors = WeaponInstance->GetSpawnedActors();
	if (SpawnedActors.IsEmpty() || !IsValid(SpawnedActors[0]))
	{
		return nullptr;
	}

	OwningWeapon = Cast<AArenaWeaponBase>(SpawnedActors[0]);
	return OwningWeapon;
}

void UArenaMeleeAbility::BindHitScanEvents()
{
	UAbilityTask_WaitGameplayEvent* HitScanEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		RPGGameplayTags::Event::HitScanStart
	);
	
	if (HitScanEvent)
	{
		HitScanEvent->EventReceived.AddDynamic(this, &UArenaMeleeAbility::OnHitScanStartReceived);
		HitScanEvent->ReadyForActivation();
	}
	
	HitScanEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		RPGGameplayTags::Event::HitScanEnd
	);
	
	if (HitScanEvent)
	{
		HitScanEvent->EventReceived.AddDynamic(this, &UArenaMeleeAbility::OnHitScanEndReceived);
		HitScanEvent->ReadyForActivation();
	}
}

void UArenaMeleeAbility::BindOnContinueComboEvents()
{
	UAbilityTask_WaitGameplayEvent* WaitEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		RPGGameplayTags::Event::ContinueComboStart
	);
	if (WaitEvent)
	{
		WaitEvent->EventReceived.AddDynamic(this, &UArenaMeleeAbility::OnContinueComboStartReceived);
		WaitEvent->ReadyForActivation();
	}
	
	WaitEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		RPGGameplayTags::Event::ContinueComboEnd
	);
	if (WaitEvent)
	{
		WaitEvent->EventReceived.AddDynamic(this, &UArenaMeleeAbility::OnContinueComboEndReceived);
		WaitEvent->ReadyForActivation();
	}
}

void UArenaMeleeAbility::OnHitScanStartReceived(FGameplayEventData Payload)
{
	if (IsValid(OwningWeapon))
	{
		FDamageEffectInfo DamageEffectInfo;
		CaptureDamageEffectInfo(nullptr, DamageEffectInfo, ComboDamageMultiplier);
		OwningWeapon->HitScanStart(DamageEffectInfo);
	}
}

void UArenaMeleeAbility::OnHitScanEndReceived(FGameplayEventData Payload)
{
	if (IsValid(OwningWeapon))
	{
		OwningWeapon->HitScanEnd();
	}
}

void UArenaMeleeAbility::OnContinueComboStartReceived(FGameplayEventData Payload)
{
	bIsWithinComboWindow = true;
	bContinueCombo = false;
}

void UArenaMeleeAbility::OnContinueComboEndReceived(FGameplayEventData Payload)
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
