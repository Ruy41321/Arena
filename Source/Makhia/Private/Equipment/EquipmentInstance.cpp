// Fill out your copyright notice in the Description page of Project Settings.


#include "Equipment/EquipmentInstance.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Equipment/EquipmentActor.h"
#include "Equipment/EquipmentDefinition.h"
#include "Equipment/Weapon/MKHWeaponBase.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

void UEquipmentInstance::OnEquipped()
{
}

void UEquipmentInstance::OnUnEquipped()
{
}

void UEquipmentInstance::SpawnEquipmentActors(const TArray<FEquipmentActorToSpawn>& ActorsToSpawn, float WeaponDamage)
{
	ACharacter* OwningCharacter = GetCharacter();
	if (!IsValid(OwningCharacter))
	{
		return;
	}

	for (const FEquipmentActorToSpawn& ActorToSpawn : ActorsToSpawn)
	{
		if (!ActorToSpawn.EquipmentClass.ToSoftObjectPath().IsValid())
		{
			continue;
		}

		if (IsValid(ActorToSpawn.EquipmentClass.Get()))
		{
			SpawnActorFromSpecification(ActorToSpawn, OwningCharacter, WeaponDamage);
			continue;
		}

		RequestAsyncSpawn(ActorToSpawn, OwningCharacter, WeaponDamage);
	}
}

void UEquipmentInstance::SpawnActorFromSpecification(const FEquipmentActorToSpawn& ActorToSpawn, ACharacter* OwningCharacter, float WeaponDamage)
{
	if (!IsValid(OwningCharacter) || !IsValid(OwningCharacter->GetMesh()))
	{
		return;
	}

	UClass* EquipmentClass = ActorToSpawn.EquipmentClass.Get();
	if (!IsValid(EquipmentClass))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	AEquipmentActor* NewActor = World->SpawnActorDeferred<AEquipmentActor>(
		EquipmentClass,
		FTransform::Identity,
		OwningCharacter,
		OwningCharacter);

	if (!IsValid(NewActor))
	{
		return;
	}

	FinalizeSpawnedActor(NewActor, OwningCharacter, ActorToSpawn.AttachName, WeaponDamage);
	SpawnedActors.Emplace(NewActor);
}

void UEquipmentInstance::RequestAsyncSpawn(const FEquipmentActorToSpawn& ActorToSpawn, ACharacter* OwningCharacter, float WeaponDamage)
{
	if (!IsValid(OwningCharacter))
	{
		return;
	}

	FStreamableManager& Manager = UAssetManager::GetStreamableManager();
	TWeakObjectPtr<UEquipmentInstance> WeakThis(this);
	const TWeakObjectPtr<ACharacter> WeakCharacter(OwningCharacter);

	Manager.RequestAsyncLoad(
		ActorToSpawn.EquipmentClass.ToSoftObjectPath(),
		[WeakThis, WeakCharacter, ActorToSpawn, WeaponDamage]
		{
			if (!WeakThis.IsValid() || !WeakCharacter.IsValid())
			{
				return;
			}

			WeakThis->SpawnActorFromSpecification(ActorToSpawn, WeakCharacter.Get(), WeaponDamage);
		});
}

void UEquipmentInstance::FinalizeSpawnedActor(AEquipmentActor* SpawnedActor, ACharacter* OwningCharacter, const FName& AttachName, float WeaponDamage) const
{
	if (!IsValid(SpawnedActor) || !IsValid(OwningCharacter) || !IsValid(OwningCharacter->GetMesh()))
	{
		return;
	}

	SpawnedActor->FinishSpawning(FTransform::Identity);
	SpawnedActor->AttachToComponent(OwningCharacter->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, AttachName);
	ApplyWeaponDamageIfWeapon(SpawnedActor, WeaponDamage);
}

void UEquipmentInstance::ApplyWeaponDamageIfWeapon(AEquipmentActor* SpawnedActor, float WeaponDamage)
{
	if (AMKHWeaponBase* Weapon = Cast<AMKHWeaponBase>(SpawnedActor))
	{
		Weapon->SetWeaponDamage(WeaponDamage);
	}
}

void UEquipmentInstance::DestroySpawnedActors()
{
	for (AActor* Actor : SpawnedActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}

	SpawnedActors.Reset();
}

const TArray<TObjectPtr<AActor>>& UEquipmentInstance::GetSpawnedActors() const
{
	return SpawnedActors;
}

ACharacter* UEquipmentInstance::GetCharacter() const
{
	if (ACharacter* OuterCharacter = Cast<ACharacter>(GetOuter()))
	{
		return OuterCharacter;
	}

	if (APawn* OuterPawn = Cast<APawn>(GetOuter()))
	{
		return Cast<ACharacter>(OuterPawn);
	}

	if (AController* OuterController = Cast<AController>(GetOuter()))
	{
		return Cast<ACharacter>(OuterController->GetPawn());
	}

	if (const APlayerController* PlayerController = Cast<APlayerController>(GetOuter()))
	{
		return Cast<ACharacter>(PlayerController->GetPawn());
	}
	
	return nullptr;
}
