// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/EnemyBase.h"
#include "AbilitySystem/RPGAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/RPGAttributeSet.h"
#include "Net/UnrealNetwork.h"

AEnemyBase::AEnemyBase()
{
	bReplicates = true;

	RPGAbilitySystemComponent = CreateDefaultSubobject<URPGAbilitySystemComponent>(TEXT("RPGAbilitySystemComponent"));
	RPGAbilitySystemComponent->SetIsReplicated(true);
	RPGAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	RPGAttributeSet = CreateDefaultSubobject<URPGAttributeSet>(TEXT("RPGAttributeSet"));

}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	BindCallbacksToDependencies();
	InitAbilityActorInfo();

}

void AEnemyBase::InitAbilityActorInfo()
{
	if (IsValid(RPGAbilitySystemComponent) && IsValid(RPGAttributeSet))
	{
		RPGAbilitySystemComponent->InitAbilityActorInfo(this, this);

		if (HasAuthority())
		{
			InitClassDefaults();
			BroadcastInitialValues();
		}
	}
}

void AEnemyBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEnemyBase, bInitAttributes);
}

void AEnemyBase::InitClassDefaults()
{
	// La logica comune (AddCharacterAbilities, InitializeDefaultAttributes, ecc.) è in CharacterBase::InitClassDefaults
	Super::InitClassDefaults();
}

void AEnemyBase::BindCallbacksToDependencies()
{
	Super::BindCallbacksToDependencies();

	if (IsValid(RPGAbilitySystemComponent))
	{
		if (HasAuthority())
		{
			RPGAbilitySystemComponent->OnAttributesGiven.AddLambda(
				[this]
				{
					bInitAttributes = true;
				});
		}
	}
}

void AEnemyBase::BroadcastInitialValues()
{
	Super::BroadcastInitialValues();

}

void AEnemyBase::OnRep_InitAttributes()
{
	BroadcastInitialValues();
}
