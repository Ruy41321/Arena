// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "CharacterBase.generated.h"

class URPGAbilitySystemComponent;
class URPGAttributeSet;

UCLASS()
class ARENA_API ACharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACharacterBase();

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintImplementableEvent)
	void OnHealthChanged(float OldHealth, float CurrentHealth, float MaxHealth);

	UFUNCTION(BlueprintNativeEvent)
	void OnStaminaChanged(float OldStamina, float CurrentStamina, float MaxStamina);
	virtual void OnStaminaChanged_Implementation(float OldStamina, float CurrentStamina, float MaxStamina);

	UFUNCTION(BlueprintImplementableEvent)
	void OnShieldChanged(float OldShield, float CurrentShield, float MaxShield);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float StaminaRegenDelay = 1.0f;
	
protected:
	virtual void BeginPlay() override;
	virtual void InitAbilityActorInfo();
	virtual void BindCallbacksToDependencies();
	virtual void InitClassDefaults();

	UFUNCTION(BlueprintCallable)
	virtual void BroadcastInitialValues();

	UPROPERTY(EditAnywhere, Category = "Custom Values | Character Info")
	FGameplayTag CharacterTag;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<URPGAbilitySystemComponent> RPGAbilitySystemComponent;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<URPGAttributeSet> RPGAttributeSet;
	
};
