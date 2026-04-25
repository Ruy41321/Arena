// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerState/MKHPlayerState.h"
#include "AbilitySystem/MKHAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/MKHAttributeSet.h"

AMKHPlayerState::AMKHPlayerState()
{
	SetNetUpdateFrequency(100.f);
	SetMinNetUpdateFrequency(66.f);

	MKHAbilitySystemComponent = CreateDefaultSubobject<UMKHAbilitySystemComponent>("AbilitySystemComp");
	MKHAbilitySystemComponent->SetIsReplicated(true);
	MKHAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	MKHAttributeSet = CreateDefaultSubobject<UMKHAttributeSet>("AttributeSet");
}

UAbilitySystemComponent* AMKHPlayerState::GetAbilitySystemComponent() const
{
	return MKHAbilitySystemComponent;
}

UMKHAbilitySystemComponent* AMKHPlayerState::GetRPGAbilitySystemComponent() const
{
	return MKHAbilitySystemComponent;
}

UMKHAttributeSet* AMKHPlayerState::GetRPGAttributeSet() const
{
	return MKHAttributeSet;
}
