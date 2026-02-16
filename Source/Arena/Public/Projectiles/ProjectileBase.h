// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectileBase.generated.h"

struct FProjectileParams;
class UProjectileMovementComponent;

UCLASS()
class ARENA_API AProjectileBase : public AActor
{
	GENERATED_BODY()
	
public:	

	AProjectileBase();

	void SetProjectileParams(const FProjectileParams& NewParams);

private:

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

};
