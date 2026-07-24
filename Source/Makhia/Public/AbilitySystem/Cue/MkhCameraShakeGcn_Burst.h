// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Burst.h"
#include "MkhCameraShakeGcn_Burst.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, Category = "GameplayCueNotify", Meta = (ShowWorldContextPin, DisplayName = "CameraShake GCN Burst", ShortTooltip = "Default Camera Shake which don't execute if the camera shake is disabled in the settings."))
class MAKHIA_API UMkhCameraShakeGcn_Burst : public UGameplayCueNotify_Burst
{
	GENERATED_BODY()
	
protected:
	
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};