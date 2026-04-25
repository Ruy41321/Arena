// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/EnemyBase.h"
#include "AbilitySystem/MKHAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/MKHAttributeSet.h"
#include "Net/UnrealNetwork.h"

AEnemyBase::AEnemyBase()
{
	bReplicates = true;

	MKHAbilitySystemComponent = CreateDefaultSubobject<UMKHAbilitySystemComponent>(TEXT("MKHAbilitySystemComponent"));
	MKHAbilitySystemComponent->SetIsReplicated(true);
	MKHAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	MKHAttributeSet = CreateDefaultSubobject<UMKHAttributeSet>(TEXT("MKHAttributeSet"));

}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	BindCallbacksToDependencies();
	InitAbilityActorInfo();

}

void AEnemyBase::InitAbilityActorInfo()
{
	if (IsValid(MKHAbilitySystemComponent) && IsValid(MKHAttributeSet))
	{
		MKHAbilitySystemComponent->InitAbilityActorInfo(this, this);

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
	// La logica comune (AddCharacterAbilities, InitializeDefaultAttributes, ecc.) Ã¨ in CharacterBase::InitClassDefaults
	Super::InitClassDefaults();
}

void AEnemyBase::BindCallbacksToDependencies()
{
	Super::BindCallbacksToDependencies();

	if (IsValid(MKHAbilitySystemComponent))
	{
		if (HasAuthority())
		{
			MKHAbilitySystemComponent->OnAttributesGiven.AddLambda(
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
