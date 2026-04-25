// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/MKHProjectileAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_SpawnActor.h"
#include "AbilitySystem/MKHGameplayTags.h"
#include "Projectiles/MKHProjectileBase.h"
#include "Data/ProjectileInfo.h"
#include "Equipment/Weapon/MKHWeaponBase.h"
#include "Libraries/MKHAbilitySystemLibrary.h"

void UMKHProjectileAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	BindSpawnProjectileEvent();
	
}

void UMKHProjectileAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	// If primary instance (instanced per actor) it doesn't have the tag set in GrantAbility
	// because CDO does , so we get the tag from CDO to set it in this instance
	if (const UMKHProjectileAbility* CDO = Cast<UMKHProjectileAbility>(Spec.Ability))
	{
		ProjectileToSpawnTag = CDO->ProjectileToSpawnTag;
	}
	
	AvatarActorFromInfo = GetAvatarActorFromActorInfo();
	if (!ProjectileToSpawnTag.IsValid() || !IsValid(AvatarActorFromInfo)) return;

	if (UProjectileInfo* ProjectileInfo = UMKHAbilitySystemLibrary::GetProjectileInfo(AvatarActorFromInfo))
	{
		CurrentProjectileParams = *ProjectileInfo->ProjectileInfoMap.Find(ProjectileToSpawnTag);
	}
}

void UMKHProjectileAbility::SpawnProjectile(const FVector& TargetLocation)
{
	if (!IsValid(CurrentProjectileParams.ProjectileClass)) return;
	if (!IsValid(AvatarActorFromInfo)) return;

	const FVector SpawnPoint = GetSpawnLocation();
	const FRotator TargetRotation = (TargetLocation - SpawnPoint).Rotation();

	FTransform SpawnTransform;
	SpawnTransform.SetLocation(SpawnPoint);
	SpawnTransform.SetRotation(TargetRotation.Quaternion());

	SpawnedProjectile = GetWorld()->SpawnActorDeferred<AMKHProjectileBase>(
		CurrentProjectileParams.ProjectileClass, 
		SpawnTransform, 
		AvatarActorFromInfo, 
		Cast<APawn>(AvatarActorFromInfo), 
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);

	if (IsValid(SpawnedProjectile))
	{
		SpawnedProjectile->SetProjectileParams(CurrentProjectileParams);

		FDamageEffectInfo DamageEffectInfo;
		CaptureDamageEffectInfo(nullptr, DamageEffectInfo);

		SpawnedProjectile->DamageEffectInfo = DamageEffectInfo;

		// Bind Destruction Event
		SpawnedProjectile->OnDestroyed.AddDynamic(this, &UMKHProjectileAbility::OnProjectileDestroyed);
		
		SpawnedProjectile->FinishSpawning(SpawnTransform);
	}
}

void UMKHProjectileAbility::BindSpawnProjectileEvent()
{
	UAbilityTask_WaitGameplayEvent* SpawnProjectileEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		MKHGameplayTags::Event::SpawnProjectile
	);
	
	if (IsValid(SpawnProjectileEvent))
	{
		SpawnProjectileEvent->EventReceived.AddDynamic(this, &UMKHProjectileAbility::OnSpawnProjectileEvent);
		SpawnProjectileEvent->ReadyForActivation();
	}
}

FVector UMKHProjectileAbility::GetSpawnLocation() const
{
	if (IsValid(OwningWeapon))
	{
		return OwningWeapon->GetProjectileSpawnLocation();
	}
	else if (IsValid(AvatarActorFromInfo))
	{
		return AvatarActorFromInfo->GetActorLocation();
	}
	return FVector::ZeroVector;
}

void UMKHProjectileAbility::OnSpawnProjectileEvent_Implementation(FGameplayEventData Payload)
{
	if (IsValid(AvatarActorFromInfo))
	{
		SpawnProjectile(AvatarActorFromInfo->GetActorForwardVector() * 10000.f);
	}
}

void UMKHProjectileAbility::OnProjectileDestroyed_Implementation(AActor* DestroyedActor)
{
	// To be overridden
}
