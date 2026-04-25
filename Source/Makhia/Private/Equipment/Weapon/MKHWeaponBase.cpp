// Fill out your copyright notice in the Description page of Project Settings.


#include "Equipment/Weapon/MKHWeaponBase.h"

AMKHWeaponBase::AMKHWeaponBase()
{
	TraceStart = CreateDefaultSubobject<USceneComponent>(TEXT("TraceStart"));
	TraceStart->SetupAttachment(GetRootComponent());
	
	TraceEnd = CreateDefaultSubobject<USceneComponent>(TEXT("TraceEnd"));
	TraceEnd->SetupAttachment(GetRootComponent());
	
	ProjectileSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ProjectileSpawnPoint"));
	ProjectileSpawnPoint->SetupAttachment(GetRootComponent());
}

void AMKHWeaponBase::SetWeaponDamage(float InDamage)
{
	WeaponDamage = InDamage;
}

float AMKHWeaponBase::GetWeaponDamage() const
{
	return WeaponDamage;
}

FVector AMKHWeaponBase::GetProjectileSpawnLocation() const
{
	return ProjectileSpawnPoint ? ProjectileSpawnPoint->GetComponentLocation() : GetActorLocation();
}
