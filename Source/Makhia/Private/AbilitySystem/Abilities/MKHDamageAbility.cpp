// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/MKHDamageAbility.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystem/MKHAbilityTypes.h"
#include "AbilitySystem/MKHGameplayTags.h"
#include "Equipment/EquipmentInstance.h"
#include "Equipment/EquipmentManagerComponent.h"
#include "Equipment/Weapon/MKHWeaponBase.h"
#include "Interfaces/EquipmentInterface.h"
#include "Player/MKHPlayerCharacter.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Data/GenericClassReference.h"
#include "Player/PlayerController/MKHPlayerController.h"

UMKHDamageAbility::UMKHDamageAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UMKHDamageAbility::SetDamagePercent(float InDamagePercent)
{
	DamagePercent = FMath::Max(0.f, InDamagePercent);
}

void UMKHDamageAbility::SetCooldownData(float InCooldownTime, const FGameplayTagContainer& InCooldownTagContainer)
{
	CooldownTime = FMath::Max(0.f, InCooldownTime);
	CooldownTagContainer = InCooldownTagContainer;
}

void UMKHDamageAbility::SetIsSkillAbility(bool InIsSkillAbility)
{
	bIsSkillAbility = InIsSkillAbility;
}

void UMKHDamageAbility::CaptureDamageEffectInfo(AActor* TargetActor, FDamageEffectInfo& OutInfo)
{
	if (AActor* AvatarActorFromInfo = GetAvatarActorFromActorInfo())
	{
		const float WeaponBaseDamage = IsValid(OwningWeapon) ? OwningWeapon->GetWeaponDamage() : 0.f;

		OutInfo.AvatarActor = AvatarActorFromInfo;
		OutInfo.BaseDamage = GetBaseDamageValue(WeaponBaseDamage);
		OutInfo.DamageEffect = DamageEffect;
		OutInfo.SourceASC = GetAbilitySystemComponentFromActorInfo();
		// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Captured Damage Info: BaseDamage = %f , WeaponDamage = %f"), OutInfo.BaseDamage, WeaponBaseDamage));
		if (IsValid(TargetActor) && TargetActor->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
		{
				OutInfo.TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
		}
	}
}

AMKHWeaponBase* UMKHDamageAbility::InitOwningWeapon()
{
	OwningWeapon = nullptr;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	AMKHPlayerCharacter* MKHPlayerCharacter = Cast<AMKHPlayerCharacter>(AvatarActor);
	if (!MKHPlayerCharacter)
	{
		return nullptr;
	}

	AController* Controller = MKHPlayerCharacter->GetController();
	if (!IsValid(Controller) || !Controller->GetClass()->ImplementsInterface(UEquipmentInterface::StaticClass()))
	{
		return nullptr;
	}

	UEquipmentManagerComponent* EquipmentManager = IEquipmentInterface::Execute_GetEquipmentManagerComponent(Controller);
	if (!IsValid(EquipmentManager))
	{
		return nullptr;
	}

	UEquipmentInstance* WeaponInstance = EquipmentManager->GetEquipmentInstanceBySlot(MKHGameplayTags::Equip::WeaponSlot);
	if (!IsValid(WeaponInstance))
	{
		return nullptr;
	}

	const TArray<TObjectPtr<AActor>>& SpawnedActors = WeaponInstance->GetSpawnedActors();
	if (SpawnedActors.IsEmpty() || !IsValid(SpawnedActors[0]))
	{
		return nullptr;
	}

	OwningWeapon = Cast<AMKHWeaponBase>(SpawnedActors[0]);
	return OwningWeapon;
}

void UMKHDamageAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (ActorInfo && ActorInfo->IsNetAuthority())
	{
		if (!InitOwningWeapon())
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to initialize owning weapon for damage ability. Ending ability."));
			return EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		}
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!PlayAnimation())
	{
		return EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
	OnMontageStarted();
}

void UMKHDamageAbility::PreActivate(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,FOnGameplayAbilityEnded::FDelegate* OnGameplayAbilityEndedDelegate, 
	const FGameplayEventData* TriggerEventData)
{
	Super::PreActivate(Handle, ActorInfo, ActivationInfo, OnGameplayAbilityEndedDelegate, TriggerEventData);
	
	FaceCharacterTowardsAttack();
	
}

void UMKHDamageAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);
	
	// Settings the config value from GAS.GrantsAbility() to the specific instance (previously was set only the CDO)
	if(const UMKHDamageAbility* CDO = Cast<UMKHDamageAbility>(Spec.Ability))
	{
		SetDamagePercent(CDO->DamagePercent);
		SetIsSkillAbility(CDO->bIsSkillAbility);
		SetCooldownData(CDO->CooldownTime, CDO->CooldownTagContainer);
	}

	AssignBPClasses(ActorInfo);
}

void UMKHDamageAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!bIsSkillAbility)
	{
		return Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
	}
	
	if (CooldownEffect && CooldownTime > 0.f)
	{
		const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CooldownEffect, GetAbilityLevel());

		// Setting CD time through set by caller magnitude
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Combat.Data.AbilityCooldownTime")), CooldownTime);

		// Setting dynamically the tag of the cooldown to apply to the actor
		SpecHandle.Data.Get()->DynamicGrantedTags.Reset();
		for (FGameplayTag CooldownTag : CooldownTagContainer)
		{
			SpecHandle.Data.Get()->DynamicGrantedTags.AddTag(CooldownTag);
		}

		// Apply Custom Cooldown
		(void)ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
}

const FGameplayTagContainer* UMKHDamageAbility::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UMKHDamageAbility::OnMontageStarted_Implementation()
{
	// Does Nothing; To be Overriden
}

float UMKHDamageAbility::GetBaseDamageValue(float WeaponDamage)
{
	return WeaponDamage * DamagePercent;
}

bool UMKHDamageAbility::PlayAnimation()
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
	
	MontageTask->OnCompleted.AddDynamic(this, &UMKHDamageAbility::OnMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &UMKHDamageAbility::OnMontageFinished);
	MontageTask->OnCancelled.AddDynamic(this, &UMKHDamageAbility::OnMontageFinished);
 
	MontageTask->ReadyForActivation();
	return true;
}

void UMKHDamageAbility::FaceCharacterTowardsAttack()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	AMKHPlayerCharacter* Character = Cast<AMKHPlayerCharacter>(AvatarActor);
	
	FRotator ControlRotation = Character->GetControlRotation();
	FRotator TargetRotation = FRotator(0.f, ControlRotation.Yaw, 0.f);
	Character->SetActorRotation(TargetRotation);
}

void UMKHDamageAbility::OnMontageFinished_Implementation()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UMKHDamageAbility::AssignBPClasses(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (const AMKHPlayerCharacter* MKHPlayerCharacter = Cast<AMKHPlayerCharacter>(ActorInfo->AvatarActor.Get()))
	{
		if (const AMKHPlayerController* PlayerController = Cast<AMKHPlayerController>(MKHPlayerCharacter->GetController()))
		{
			if (UGenericClassReference* ClassDataAsset = PlayerController->GenericClassReference)
			{
				if (const TSubclassOf<UObject>* CooldownEffectPtr = ClassDataAsset->GenericClassMap.Find("GE_GenericSkillCooldown"))
				{
					CooldownEffect = *CooldownEffectPtr;
				}
				if (const TSubclassOf<UObject>* DamageEffectPtr = ClassDataAsset->GenericClassMap.Find("GE_Damage"))
				{
					DamageEffect = *DamageEffectPtr;
				}
			}
		}
	}
}
