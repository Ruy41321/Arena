// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "RPGAbilityTypes.generated.h"

class AProjectileBase;
class UGameplayEffect;
class UAbilitySystemComponent;

USTRUCT()
struct FRPGGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

	bool IsCriticalHit() const { return bCriticalHit; }
	void SetIsCriticalHit(bool bInIsCriticalHit) { bCriticalHit = bInIsCriticalHit; }

	static ARENA_API
	FRPGGameplayEffectContext* GetEffectContext(FGameplayEffectContextHandle Handle);

	virtual UScriptStruct* GetScriptStruct() const override { return FRPGGameplayEffectContext::StaticStruct(); }

	virtual FRPGGameplayEffectContext* Duplicate() const override
	{
		FRPGGameplayEffectContext* NewContext = new FRPGGameplayEffectContext();
		*NewContext = *this;

		if (GetHitResult())
		{
			NewContext->AddHitResult(*GetHitResult(), true);
		}

		return NewContext;
	}

	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;

private:

	UPROPERTY()
	bool bCriticalHit = false;

};

template<>
struct TStructOpsTypeTraits<FRPGGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FRPGGameplayEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true,
	};
};

USTRUCT()
struct FProjectileParams
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AProjectileBase> ProjectileClass;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMesh> ProjectileMesh;

	UPROPERTY(EditDefaultsOnly)
	float InitialSpeed = 1000.f;

	UPROPERTY(EditDefaultsOnly)
	float GravityScale = 1.f;

	UPROPERTY(EditDefaultsOnly)
	bool bShouldBounce = false;

	UPROPERTY(EditDefaultsOnly)
	float Bounciness = 0.6f;
};

USTRUCT(BlueprintType)
struct FDamageEffectInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	float BaseDamage = 0.f;

	UPROPERTY(BlueprintReadWrite)
	float AbilityLevel = 1.f;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<AActor> AvatarActor = nullptr;

	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> DamageEffect = nullptr;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UAbilitySystemComponent> SourceASC = nullptr;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UAbilitySystemComponent> TargetASC = nullptr;

	//UPROPERTY(BlueprintReadWrite)
	//TSubclassOf<UDamageType> DamageTypeClass;
};
