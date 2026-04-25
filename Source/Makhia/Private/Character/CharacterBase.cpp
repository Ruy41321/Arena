// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/CharacterBase.h"
#include "AbilitySystem/MKHAbilitySystemComponent.h"
#include "AbilitySystem/MKHGameplayTags.h"
#include "AbilitySystem/Attributes/MKHAttributeSet.h"
#include "Libraries/MKHAbilitySystemLibrary.h"
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
	return MKHAbilitySystemComponent;
}

void ACharacterBase::InitAbilityActorInfo()
{
	// Blank for now
}

void ACharacterBase::BindCallbacksToDependencies()
{
	if (IsValid(MKHAbilitySystemComponent) && IsValid(MKHAttributeSet))
	{
		// Health
		MKHAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UMKHAttributeSet::GetHealthAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnHealthChanged.Broadcast(Data.OldValue, Data.NewValue, MKHAttributeSet->GetMaxHealth());
			});

		// Shield
		MKHAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UMKHAttributeSet::GetShieldAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnShieldChanged.Broadcast(Data.OldValue, Data.NewValue, MKHAttributeSet->GetMaxShield());
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

	UCharacterClassInfo* ClassInfo = UMKHAbilitySystemLibrary::GetCharacterClassDefaultInfo(this);
	if (!ClassInfo)
	{
		return;
	}

	if (const FCharacterClassDefaultInfo* SelectedClass = ClassInfo->ClassDefaultInfoMap.Find(CharacterTag))
	{
		if (IsValid(MKHAbilitySystemComponent))
		{
			MKHAbilitySystemComponent->AddCharacterAbilities(SelectedClass->StartingAbilities);
			MKHAbilitySystemComponent->AddCharacterPassiveAbilities(SelectedClass->StartingPassives);
			MKHAbilitySystemComponent->InitializeDefaultAttributes(SelectedClass->DefaultAttributes);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("CharacterBase: MKHAbilitySystemComponent is not valid for %s"), *GetNameSafe(this));
		}
	}
}

void ACharacterBase::BroadcastInitialValues()
{
	if (IsValid(MKHAttributeSet))
	{
		OnHealthChanged.Broadcast(MKHAttributeSet->GetHealth(), MKHAttributeSet->GetHealth(), MKHAttributeSet->GetMaxHealth());
		OnShieldChanged.Broadcast(MKHAttributeSet->GetShield(), MKHAttributeSet->GetShield(), MKHAttributeSet->GetMaxShield());
	}
}

void ACharacterBase::HandleStaminaChanged(float OldStamina, float CurrentStamina, float MaxStamina)
{
	if (!IsValid(MKHAbilitySystemComponent))
	{
		return;
	}
	
	const bool bIsOutOfStamina = MKHAbilitySystemComponent->HasMatchingGameplayTag(MKHGameplayTags::State::OutOfStamina);
	
	if (CurrentStamina <= 0.f and !bIsOutOfStamina)
	{
		MKHAbilitySystemComponent->AddLooseGameplayTag(MKHGameplayTags::State::OutOfStamina);
	}
	else if (CurrentStamina == MaxStamina and bIsOutOfStamina)
	{
		MKHAbilitySystemComponent->RemoveLooseGameplayTag(MKHGameplayTags::State::OutOfStamina);
	}
}

