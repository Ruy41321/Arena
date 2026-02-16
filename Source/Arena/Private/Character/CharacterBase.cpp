// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/CharacterBase.h"

ACharacterBase::ACharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

}

void ACharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
}

void ACharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void ACharacterBase::InitAbilityActorInfo()
{
	// Blank for now
}

void ACharacterBase::BindCallbacksToDependencies()
{
	// Blank for now
}

void ACharacterBase::InitClassDefaults()
{
	// Blank for now
}

void ACharacterBase::BroadcastInitialValues()
{
	// Blank for now
}