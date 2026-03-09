// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/CharacterBase.h"
#include "AbilitySystem/RPGAbilitySystemComponent.h"
#include "AbilitySystem/RPGGameplayTags.h"
#include "AbilitySystem/Attributes/RPGAttributeSet.h"
#include "Libraries/RPGAbilitySystemLibrary.h"
#include "Data/CharacterClassInfo.h"

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

UAbilitySystemComponent* ACharacterBase::GetAbilitySystemComponent() const
{
	return RPGAbilitySystemComponent;
}

void ACharacterBase::InitAbilityActorInfo()
{
	// Blank for now
}

void ACharacterBase::BindCallbacksToDependencies()
{
	if (IsValid(RPGAbilitySystemComponent) && IsValid(RPGAttributeSet))
	{
		//Health
		RPGAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(URPGAttributeSet::GetHealthAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnHealthChanged(Data.OldValue, Data.NewValue, RPGAttributeSet->GetMaxHealth());
			});

		//Shield
		RPGAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(URPGAttributeSet::GetShieldAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnShieldChanged(Data.OldValue, Data.NewValue, RPGAttributeSet->GetMaxShield());
			});
		}
}

void ACharacterBase::InitClassDefaults()
{
	if (!CharacterTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("CharacterBase: CharacterTag is not set for %s"), *GetNameSafe(this));
		return;
	}

	UCharacterClassInfo* ClassInfo = URPGAbilitySystemLibrary::GetCharacterClassDefaultInfo(this);
	if (!ClassInfo)
	{
		return;
	}

	if (const FCharacterClassDefaultInfo* SelectedClass = ClassInfo->ClassDefaultInfoMap.Find(CharacterTag))
	{
		if (IsValid(RPGAbilitySystemComponent))
		{
			RPGAbilitySystemComponent->AddCharacterAbilities(SelectedClass->StartingAbilities);
			RPGAbilitySystemComponent->AddCharacterPassiveAbilities(SelectedClass->StartingPassives);
			RPGAbilitySystemComponent->InitializeDefaultAttributes(SelectedClass->DefaultAttributes);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("CharacterBase: RPGAbilitySystemComponent is not valid for %s"), *GetNameSafe(this));
		}
	}
}

void ACharacterBase::BroadcastInitialValues()
{
	if (IsValid(RPGAttributeSet))
	{
		OnHealthChanged(RPGAttributeSet->GetHealth(), RPGAttributeSet->GetHealth(), RPGAttributeSet->GetMaxHealth());
		OnShieldChanged(RPGAttributeSet->GetShield(), RPGAttributeSet->GetShield(), RPGAttributeSet->GetMaxShield());
	}
}

void ACharacterBase::OnStaminaChanged_Implementation(float OldStamina, float CurrentStamina, float MaxStamina)
{
	if (!IsValid(RPGAbilitySystemComponent))
	{
		return;
	}
	
	const bool bIsOutOfStamina = RPGAbilitySystemComponent->HasMatchingGameplayTag(RPGGameplayTags::State::OutOfStamina);
	
	if (CurrentStamina <= 0.f and !bIsOutOfStamina)
	{
		RPGAbilitySystemComponent->AddLooseGameplayTag(RPGGameplayTags::State::OutOfStamina);
	}
	else if (CurrentStamina == MaxStamina and bIsOutOfStamina)
	{
		RPGAbilitySystemComponent->RemoveLooseGameplayTag(RPGGameplayTags::State::OutOfStamina);
	}
}

