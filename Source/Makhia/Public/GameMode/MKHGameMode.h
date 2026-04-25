// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MKHGameMode.generated.h"

class UCharacterClassInfo;
class UProjectileInfo;
/**
 * 
 */
UCLASS()
class MAKHIA_API AMKHGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:

	UCharacterClassInfo* GetCharacterClassDefaultInfo() const;
	UProjectileInfo* GetProjectileInfo() const;

private:

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Class Defaults")
	TObjectPtr<UCharacterClassInfo> ClassDefaults;
	
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Projectiles")
	TObjectPtr<UProjectileInfo> ProjectileInfo;
};
