// Fill out your copyright notice in the Description page of Project Settings.


#include "Equipment/Weapon/ArenaWeaponBase.h"

AArenaWeaponBase::AArenaWeaponBase()
{
	TraceStart = CreateDefaultSubobject<USceneComponent>("TraceStart");
	TraceStart->SetupAttachment(GetRootComponent());
	
	TraceEnd = CreateDefaultSubobject<USceneComponent>("TraceEnd");
	TraceEnd->SetupAttachment(GetRootComponent());
}
