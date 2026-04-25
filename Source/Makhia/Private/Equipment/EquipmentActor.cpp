// Fill out your copyright notice in the Description page of Project Settings.


#include "Equipment/EquipmentActor.h"

AEquipmentActor::AEquipmentActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	EquipmentMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EquipmentMesh"));
	EquipmentMesh->SetupAttachment(GetRootComponent());
	EquipmentMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

}
