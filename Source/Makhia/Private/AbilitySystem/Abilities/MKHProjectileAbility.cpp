// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/MKHProjectileAbility.h"

#include "MKHLogChannels.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_SpawnActor.h"
#include "AbilitySystem/MKHAbilityGrantPayload.h"
#include "AbilitySystem/MKHGameplayTags.h"
#include "Projectiles/MKHProjectileBase.h"
#include "Data/ProjectileInfo.h"
#include "Engine/World.h"
#include "Equipment/Weapon/MKHWeaponBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Libraries/MKHAbilitySystemLibrary.h"

void UMKHProjectileAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// Super may have ended the ability already (commit failure): reacting further would leave
	// stale state behind (e.g. an orientation set after EndAbility already reverted it).
	if (!IsActive() || ActorInfo == nullptr)
	{
		return;
	}

	SetCharacterOrientation(true);

	// The spawn notify is only reliable on the locally-controlled machine (owning client for
	// players, server for AI): that machine listens for the event and computes the aim point.
	if (ActorInfo->IsLocallyControlled())
	{
		BindSpawnProjectileEvent();
	}

	// The server proxy of a remote player receives the aim point as replicated target data
	// and performs the authoritative spawn: only the authority ever spawns the projectile,
	// which then replicates to every client.
	if (ActorInfo->IsNetAuthority() && !ActorInfo->IsLocallyControlled())
	{
		BindServerSpawnTargetDataDelegate();
	}
}

void UMKHProjectileAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// EndAbility re-enters through montage interruption callbacks and replicated ends: run
	// the target data cleanup only while the end is still valid (it is idempotent).
	if (IsEndAbilityValid(Handle, ActorInfo))
	{
		RemoveServerSpawnTargetDataDelegate();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	
	SetCharacterOrientation(false);
	
	if (ActorInfo != nullptr && ActorInfo->IsNetAuthority())
	{
		if (IsValid(OwningWeapon))
		{
			OwningWeapon->SetActorHiddenInGame(false);
		}
	}
}

void UMKHProjectileAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	// This instance already starts from the CDO default (InstancedPerActor). The per-grant
	// payload set by GrantEquipmentAbility overrides it with this weapon's projectile tag.
	if (const UMKHAbilityGrantPayload* Payload = GetGrantPayload(Spec);
		Payload && Payload->ProjectileToSpawnTag.IsValid())
	{
		ProjectileToSpawnTag = Payload->ProjectileToSpawnTag;
	}

	AvatarActorFromInfo = GetAvatarActorFromActorInfo();
	if (!ProjectileToSpawnTag.IsValid() || !IsValid(AvatarActorFromInfo)) return;

	if (UProjectileInfo* ProjectileInfo = UMKHAbilitySystemLibrary::GetProjectileInfo(AvatarActorFromInfo))
	{
		if (const FProjectileParams* FoundParams = ProjectileInfo->ProjectileInfoMap.Find(ProjectileToSpawnTag))
		{
			CurrentProjectileParams = *FoundParams;
		}
		else
		{
			UE_LOG(LogMKHAbility, Warning, TEXT("OnGiveAbility: projectile tag %s is not mapped in the ProjectileInfo data asset, %s will not spawn anything."), *ProjectileToSpawnTag.ToString(), *GetNameSafe(this));
		}
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
	HandleProjectileFiredReaction();

	const FVector AimPoint = ComputeAimPoint();

	if (CurrentActorInfo && CurrentActorInfo->IsNetAuthority())
	{
		// Listen-server host or AI: this machine is the authority, spawn directly.
		SpawnProjectile(AimPoint);
	}
	else
	{
		SendSpawnProjectileToServer(AimPoint);
	}
}

FVector UMKHProjectileAbility::ComputeAimPoint() const
{
	const FVector StartLocation = IsValid(AvatarActorFromInfo) ? AvatarActorFromInfo->GetActorLocation() : FVector::ZeroVector;

	// Mirrors the Blueprint WaitTargetData single-line-trace setup (NoCollision trace profile,
	// aim pitch affecting): with no geometry to hit, the target point is simply the endpoint
	// AimTraceMaxRange units along the controller's view direction, re-projected so the
	// resulting direction originates from the avatar instead of the camera.
	if (const APlayerController* PlayerController = CurrentActorInfo ? CurrentActorInfo->PlayerController.Get() : nullptr)
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

		const FVector ViewEnd = ViewLocation + ViewRotation.Vector() * AimTraceMaxRange;
		const FVector AimDirection = (ViewEnd - StartLocation).GetSafeNormal();
		if (!AimDirection.IsNearlyZero())
		{
			return StartLocation + AimDirection * AimTraceMaxRange;
		}
	}

	// AI (or missing player controller): aim straight ahead of the avatar.
	const FVector ForwardDirection = IsValid(AvatarActorFromInfo) ? AvatarActorFromInfo->GetActorForwardVector() : FVector::ForwardVector;
	return StartLocation + ForwardDirection * AimTraceMaxRange;
}

void UMKHProjectileAbility::HandleProjectileFiredReaction()
{
	SetCharacterOrientation(false);

	// The owning client may not have resolved the weapon at activation: resolve it lazily
	// before hiding it until the ability ends (EndAbility un-hides it on the authority).
	if (!IsValid(OwningWeapon))
	{
		InitOwningWeapon();
	}

	if (IsValid(OwningWeapon))
	{
		OwningWeapon->SetActorHiddenInGame(true);
	}
}

// ============================================================
// Replicated Aim Point (client -> server)
// ============================================================

void UMKHProjectileAbility::SendSpawnProjectileToServer(const FVector& AimPoint)
{
	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}

	FGameplayAbilityTargetData_LocationInfo* LocationData = new FGameplayAbilityTargetData_LocationInfo();
	LocationData->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
	LocationData->TargetLocation.LiteralTransform = FTransform(AimPoint);

	FGameplayAbilityTargetDataHandle TargetDataHandle;
	TargetDataHandle.Add(LocationData);

	// The scoped prediction window lets the server accept target data sent outside the
	// original activation window (the spawn notify fires well after activation).
	FScopedPredictionWindow ScopedPrediction(AbilitySystemComponent);
	AbilitySystemComponent->CallServerSetReplicatedTargetData(
		CurrentSpecHandle,
		CurrentActivationInfo.GetActivationPredictionKey(),
		TargetDataHandle,
		FGameplayTag(),
		AbilitySystemComponent->ScopedPredictionKey
	);
}

void UMKHProjectileAbility::BindServerSpawnTargetDataDelegate()
{
	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}

	AbilitySystemComponent->AbilityTargetDataSetDelegate(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey())
		.AddUObject(this, &UMKHProjectileAbility::OnServerSpawnTargetDataReceived);
}

void UMKHProjectileAbility::RemoveServerSpawnTargetDataDelegate()
{
	if (CurrentActorInfo == nullptr || !CurrentActorInfo->IsNetAuthority() || CurrentActorInfo->IsLocallyControlled())
	{
		return;
	}

	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}

	AbilitySystemComponent->AbilityTargetDataSetDelegate(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey()).RemoveAll(this);
	AbilitySystemComponent->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
}

void UMKHProjectileAbility::OnServerSpawnTargetDataReceived(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ApplicationTag)
{
	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	if (IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
	}

	if (!IsActive())
	{
		return;
	}

	for (int32 DataIndex = 0; DataIndex < DataHandle.Num(); ++DataIndex)
	{
		const FGameplayAbilityTargetData* TargetData = DataHandle.Get(DataIndex);
		if (TargetData == nullptr || TargetData->GetScriptStruct() != FGameplayAbilityTargetData_LocationInfo::StaticStruct())
		{
			continue;
		}

		const FGameplayAbilityTargetData_LocationInfo* LocationData = static_cast<const FGameplayAbilityTargetData_LocationInfo*>(TargetData);
		const FVector AimPoint = LocationData->TargetLocation.GetTargetingTransform().GetLocation();

		// Mirror the owning client's reaction on the authority (weapon hide replicates to
		// every other client) and perform the authoritative spawn.
		HandleProjectileFiredReaction();
		SpawnProjectile(AimPoint);
	}
}

void UMKHProjectileAbility::OnProjectileDestroyed_Implementation(AActor* DestroyedActor)
{
	// To be overridden
}
