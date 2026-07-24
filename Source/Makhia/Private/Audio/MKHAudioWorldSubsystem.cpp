// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Audio/MKHAudioWorldSubsystem.h"

#include "Audio/MKHAudioSettings.h"
#include "Audio/MKHMusicSubsystem.h"
#include "GameUserSettings/MKHGameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "MKHLogChannels.h"
#include "Sound/SoundBase.h"

bool UMKHAudioWorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	const UWorld* World = Cast<UWorld>(Outer);
	return IsValid(World) && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UMKHAudioWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// Dedicated servers have no audio device: volumes and music are client-side concerns only.
	if (InWorld.GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	UMKHGameUserSettings* UserSettings = UMKHGameUserSettings::GetMKHGameUserSettings();
	if (IsValid(UserSettings))
	{
		UserSettings->ApplyAudioSettings();
		UE_LOG(LogMKHAudio, Log, TEXT("UMKHAudioWorldSubsystem: User audio settings applied for world '%s'."), *InWorld.GetName());
	}
	else
	{
		UE_LOG(LogMKHAudio, Warning, TEXT("UMKHAudioWorldSubsystem::OnWorldBeginPlay: UMKHGameUserSettings unavailable, user volumes not applied."));
	}

	StartLevelMusic(InWorld);
}

void UMKHAudioWorldSubsystem::StartLevelMusic(UWorld& InWorld) const
{
	const UMKHAudioSettings* AudioSettings = UMKHAudioSettings::Get();
	UMKHMusicSubsystem* MusicSubsystem = IsValid(InWorld.GetGameInstance()) ? InWorld.GetGameInstance()->GetSubsystem<UMKHMusicSubsystem>() : nullptr;
	if (!IsValid(AudioSettings) || !IsValid(MusicSubsystem))
	{
		UE_LOG(LogMKHAudio, Warning, TEXT("UMKHAudioWorldSubsystem::StartLevelMusic: Audio settings or music subsystem unavailable."));
		return;
	}

	// Short level name without the PIE prefix, matching the LevelMusicMap keys.
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(&InWorld, /*bRemovePrefixString*/ true);

	const TSoftObjectPtr<USoundBase>* FoundTrack = AudioSettings->LevelMusicMap.Find(LevelName);
	if (FoundTrack == nullptr)
	{
		MusicSubsystem->StopMusic();
		return;
	}

	USoundBase* Track = FoundTrack->LoadSynchronous();
	if (!IsValid(Track))
	{
		UE_LOG(LogMKHAudio, Warning, TEXT("UMKHAudioWorldSubsystem::StartLevelMusic: Track for level '%s' failed to load ('%s')."), *LevelName, *FoundTrack->ToString());
		return;
	}

	MusicSubsystem->PlayMusicTrack(Track);
}
