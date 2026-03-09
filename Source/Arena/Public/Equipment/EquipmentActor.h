// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EquipmentActor.generated.h"

UCLASS()
class ARENA_API AEquipmentActor : public AActor
{
	GENERATED_BODY()
	
public:	
	
	AEquipmentActor();

private:

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	TObjectPtr<UStaticMeshComponent> EquipmentMesh;

};
