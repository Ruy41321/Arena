// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EquipmentActor.generated.h"

UCLASS()
class MAKHIA_API AEquipmentActor : public AActor
{
	GENERATED_BODY()
	
public:	
	/** Creates the default scene and mesh hierarchy used by equipment visuals. */
	AEquipmentActor();

private:

	/** Root scene component used as stable attachment parent for all equipment visuals. */
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	TObjectPtr<USceneComponent> RootScene;

	/** Static mesh component that renders the equipped item in the world/character socket. */
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	TObjectPtr<UStaticMeshComponent> EquipmentMesh;

};
