// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/MKHGameMode.h"

UCharacterClassInfo* AMKHGameMode::GetCharacterClassDefaultInfo() const
{
	return ClassDefaults;
}

UProjectileInfo* AMKHGameMode::GetProjectileInfo() const
{
	return ProjectileInfo;
}
