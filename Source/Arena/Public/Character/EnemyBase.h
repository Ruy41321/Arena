// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
#include "AbilitySystemInterface.h"
#include "EnemyBase.generated.h"

class URPGAttributeSet;
class URPGAbilitySystemComponent;
/**
 * 
 */
UCLASS()
class ARENA_API AEnemyBase : public ACharacterBase, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:

	AEnemyBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	virtual void BeginPlay() override;
	virtual void InitAbilityActorInfo() override;
	virtual void InitClassDefaults() override;
	virtual void BindCallbacksToDependencies() override;
	virtual void BroadcastInitialValues() override;

private:

	UPROPERTY(ReplicatedUsing=OnRep_InitAttributes)
	bool bInitAttributes = false;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<URPGAbilitySystemComponent> RPGAbilitySystemComponent;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<URPGAttributeSet> RPGAttributeSet;

	UFUNCTION()
	void OnRep_InitAttributes();

};
