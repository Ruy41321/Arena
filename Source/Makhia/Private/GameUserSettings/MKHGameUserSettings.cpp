// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "GameUserSettings/MKHGameUserSettings.h"

#include "Audio/MKHAudioSettings.h"
#include "AudioModulationStatics.h"
#include "Engine/Engine.h"
#include "MKHLogChannels.h"
#include "SoundControlBus.h"

UMKHGameUserSettings::UMKHGameUserSettings()
{
	bEnableCameraShake = true;
}

void UMKHGameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	Super::ApplySettings(bCheckForCommandLineOverrides);
	ApplyAudioSettings();
}

void UMKHGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();

	MasterVolume = 1.f;
	MusicVolume = 1.f;
	SFXVolume = 1.f;
	bEnableCameraShake = true;
}

UMKHGameUserSettings* UMKHGameUserSettings::GetMKHGameUserSettings()
{
	return Cast<UMKHGameUserSettings>(GetGameUserSettings());
}

void UMKHGameUserSettings::SetMasterVolume(float NewVolume)
{
	MasterVolume = FMath::Clamp(NewVolume, 0.f, 1.f);
	ApplyVolumeToControlBus(UMKHAudioSettings::Get()->MasterVolumeControlBus, MasterVolume);
}

void UMKHGameUserSettings::SetMusicVolume(float NewVolume)
{
	MusicVolume = FMath::Clamp(NewVolume, 0.f, 1.f);
	ApplyVolumeToControlBus(UMKHAudioSettings::Get()->MusicVolumeControlBus, MusicVolume);
}

void UMKHGameUserSettings::SetSFXVolume(float NewVolume)
{
	SFXVolume = FMath::Clamp(NewVolume, 0.f, 1.f);
	ApplyVolumeToControlBus(UMKHAudioSettings::Get()->SFXVolumeControlBus, SFXVolume);
}

void UMKHGameUserSettings::ApplyAudioSettings()
{
	const UMKHAudioSettings* AudioSettings = UMKHAudioSettings::Get();
	if (!IsValid(AudioSettings))
	{
		UE_LOG(LogMKHAudio, Warning, TEXT("UMKHGameUserSettings::ApplyAudioSettings: UMKHAudioSettings unavailable, cannot apply volumes."));
		return;
	}

	ApplyVolumeToControlBus(AudioSettings->MasterVolumeControlBus, MasterVolume);
	ApplyVolumeToControlBus(AudioSettings->MusicVolumeControlBus, MusicVolume);
	ApplyVolumeToControlBus(AudioSettings->SFXVolumeControlBus, SFXVolume);
}

void UMKHGameUserSettings::ApplyVolumeToControlBus(const TSoftObjectPtr<USoundControlBus>& BusPtr, float LinearVolume) const
{
	// Modulation is per audio device, so a live play world is required. When none is
	// active yet, UMKHAudioWorldSubsystem re-applies the settings on world begin play.
	UWorld* World = IsValid(GEngine) ? GEngine->GetCurrentPlayWorld() : nullptr;
	if (!IsValid(World))
	{
		UE_LOG(LogMKHAudio, Verbose, TEXT("UMKHGameUserSettings::ApplyVolumeToControlBus: No active play world, deferring volume application."));
		return;
	}

	USoundControlBus* Bus = BusPtr.LoadSynchronous();
	if (!IsValid(Bus))
	{
		UE_LOG(LogMKHAudio, Warning, TEXT("UMKHGameUserSettings::ApplyVolumeToControlBus: Control Bus '%s' is not assigned or failed to load. Check Project Settings -> Game -> Makhia Audio."), *BusPtr.ToString());
		return;
	}

	UAudioModulationStatics::SetGlobalBusMixValue(World, Bus, LinearVolume);
}
