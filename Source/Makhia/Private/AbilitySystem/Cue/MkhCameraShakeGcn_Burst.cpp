// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Cue/MkhCameraShakeGcn_Burst.h"

#include "GameUserSettings/MKHGameUserSettings.h"

bool UMkhCameraShakeGcn_Burst::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	UMKHGameUserSettings* UserSettings = UMKHGameUserSettings::GetMKHGameUserSettings();
	if (UserSettings && !UserSettings->IsCameraShakeEnabled())
	{
		return false; // Skip executing the camera shake if it's disabled in settings
	}
	return Super::OnExecute_Implementation(MyTarget, Parameters);
}
