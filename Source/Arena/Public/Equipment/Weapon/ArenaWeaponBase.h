// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/RPGAbilityTypes.h"
#include "Equipment/EquipmentActor.h"
#include "ArenaWeaponBase.generated.h"

struct FDamageEffectInfo;
/**
 * 
 */
UCLASS()
class ARENA_API AArenaWeaponBase : public AEquipmentActor
{
	GENERATED_BODY()
	
public:
	
	AArenaWeaponBase();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void HitScan();
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void HitScanStart(FDamageEffectInfo DamageEffectInfo);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void HitScanEnd();
	
protected:
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Custom Values | Hit Scan")
	TObjectPtr<USceneComponent> TraceStart;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Custom Values | Hit Scan")
	TObjectPtr<USceneComponent> TraceEnd;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Custom Values | Hit Scan")
	FDamageEffectInfo CachedDamageEffectInfo;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Custom Values | Hit Scan")
	TArray<AActor*> HitActors;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Custom Values | Hit Scan")
	FTimerHandle HitScanTimer;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Custom Values | Hit Scan")
	float HitScanRadius;
	
};
