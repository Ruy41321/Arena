// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/EnemyBase.h"
#include "AbilitySystem/RPGAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/RPGAttributeSet.h"
#include "Libraries/RPGAbilitySystemLibrary.h"
#include <Data/CharacterClassInfo.h>
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

UAbilitySystemComponent* AEnemyBase::GetAbilitySystemComponent() const
{
	return RPGAbilitySystemComponent;
}

void AEnemyBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEnemyBase, bInitAttributes);
}

void AEnemyBase::InitClassDefaults()
{
	if (!CharacterTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyBase: CharacterTag is not set for %s"), *GetNameSafe(this));
	}
	else if (UCharacterClassInfo* ClassInfo = URPGAbilitySystemLibrary::GetCharacterClassDefaultInfo(this))
	{
		if (const FCharacterClassDefaultInfo* SelectedClass = ClassInfo->ClassDefaultInfoMap.Find(CharacterTag))
		{
			if (IsValid(RPGAbilitySystemComponent))
			{
				RPGAbilitySystemComponent->AddCharacterAbilities(SelectedClass->StartingAbilities);
				RPGAbilitySystemComponent->AddCharacterPassiveAbilities(SelectedClass->StartingPassives);
				RPGAbilitySystemComponent->initializeDefaultAttributes(SelectedClass->DefaultAttributes);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("EnemyBase: RPGAbilitySystemComponent is not valid for %s"), *GetNameSafe(this));
			}
		}
	}
}

void AEnemyBase::BindCallbacksToDependencies()
{
	if (IsValid(RPGAbilitySystemComponent) && IsValid(RPGAttributeSet))
	{
		//Health
		RPGAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(URPGAttributeSet::GetHealthAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnHealthChanged(Data.NewValue, RPGAttributeSet->GetMaxHealth());
			});

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
	if (IsValid(RPGAttributeSet))
	{
		OnHealthChanged(RPGAttributeSet->GetHealth(), RPGAttributeSet->GetMaxHealth());
	}
}

void AEnemyBase::OnRep_InitAttributes()
{
	BroadcastInitialValues();
}
