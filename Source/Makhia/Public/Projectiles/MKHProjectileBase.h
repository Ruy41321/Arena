// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystem/MKHAbilityTypes.h"
#include "MKHProjectileBase.generated.h"

struct FProjectileParams;
class UProjectileMovementComponent;
class USphereComponent;

UCLASS()
class MAKHIA_API AMKHProjectileBase : public AActor
{
	GENERATED_BODY()
	
public:	

	AMKHProjectileBase();
	 
	// ==========================================
	// Configuration & Damage
	// ==========================================

	/**
	 * Configures the physical properties and visual mesh of the projectile dynamically.
	 * @param NewParams Structure containing initial speed, gravity, and bounce settings.
	 */
	UFUNCTION(BlueprintCallable, Category = "Projectile | Setup")
	void SetProjectileParams(const FProjectileParams& NewParams);
		
	/** Stored effect setup bound to this projectile specifically meant for damaging overlapped actors. */
	UPROPERTY(BlueprintReadWrite, Category = "Projectile | Damage")
	FDamageEffectInfo DamageEffectInfo;

	// ==========================================
	// Collision Events
	// ==========================================

	/**
	 * Natively handles sphere bounding volume overlaps to apply gameplay damage effects.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Projectile | Collision")
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	virtual void OnSphereBeginOverlap_Implementation(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	/**
	 * Natively handles blocking collisions leading normally to immediate projectile destruction.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Projectile | Collision")
	void OnSphereHit(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	virtual void OnSphereHit_Implementation(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

protected:

	virtual void BeginPlay() override;
		
private:

	// ==========================================
	// Components
	// ==========================================

	/** Core sphere collision component defining dynamic overlaps and block events. */
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"), Category = "Projectile | Components")
	TObjectPtr<USphereComponent> OverlapSphere;

	/** Pivot wrapper used potentially to apply rotating behaviors independently from movement direction. */
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"), Category = "Projectile | Components")
	TObjectPtr<USceneComponent> MeshPivot;
	
	/** The visual representation of the flying object assigned dynamically via SetProjectileParams. */
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"), Category = "Projectile | Components")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	/** Dedicated Unreal component managing flying path, velocity, drop rate, and bouncing logic. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Projectile | Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

};
