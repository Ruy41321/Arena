// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Audio/MKHMusicSubsystem.h"

#include "Audio/MKHAudioSettings.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MKHLogChannels.h"

void UMKHMusicSubsystem::PlayMusicTrack(USoundBase* Track, float FadeInSeconds)
{
	if (!IsValid(Track))
	{
		UE_LOG(LogMKHAudio, Warning, TEXT("UMKHMusicSubsystem::PlayMusicTrack: Invalid track, nothing to play."));
		return;
	}

	// Same track already playing: keep it running seamlessly (e.g. level travel between maps sharing the theme).
	if (CurrentTrack == Track && IsMusicPlaying())
	{
		return;
	}

	const float FadeSeconds = ResolveFadeSeconds(FadeInSeconds);

	// Crossfade: let the old component fade itself out and auto-destroy.
	if (IsMusicPlaying())
	{
		MusicComponent->FadeOut(FadeSeconds, 0.f);
	}

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!IsValid(World))
	{
		UE_LOG(LogMKHAudio, Warning, TEXT("UMKHMusicSubsystem::PlayMusicTrack: No valid world available, cannot play '%s'."), *Track->GetName());
		return;
	}

	MusicComponent = UGameplayStatics::CreateSound2D(
		World, Track,
		/*VolumeMultiplier*/ 1.f, /*PitchMultiplier*/ 1.f, /*StartTime*/ 0.f,
		/*ConcurrencySettings*/ nullptr,
		/*bPersistAcrossLevelTransition*/ true,
		/*bAutoDestroy*/ true);

	if (!IsValid(MusicComponent))
	{
		UE_LOG(LogMKHAudio, Warning, TEXT("UMKHMusicSubsystem::PlayMusicTrack: Failed to create audio component for '%s'."), *Track->GetName());
		return;
	}

	MusicComponent->FadeIn(FadeSeconds, 1.f);
	CurrentTrack = Track;
	UE_LOG(LogMKHAudio, Log, TEXT("UMKHMusicSubsystem: Now playing '%s' (fade %.1fs)."), *Track->GetName(), FadeSeconds);
}

void UMKHMusicSubsystem::StopMusic(float FadeOutSeconds)
{
	if (!IsMusicPlaying())
	{
		CurrentTrack = nullptr;
		return;
	}

	MusicComponent->FadeOut(ResolveFadeSeconds(FadeOutSeconds), 0.f);
	MusicComponent = nullptr;
	CurrentTrack = nullptr;
	UE_LOG(LogMKHAudio, Log, TEXT("UMKHMusicSubsystem: Music stopped."));
}

bool UMKHMusicSubsystem::IsMusicPlaying() const
{
	return IsValid(MusicComponent) && MusicComponent->IsPlaying();
}

float UMKHMusicSubsystem::ResolveFadeSeconds(float RequestedSeconds) const
{
	if (RequestedSeconds >= 0.f)
	{
		return RequestedSeconds;
	}

	const UMKHAudioSettings* AudioSettings = UMKHAudioSettings::Get();
	return IsValid(AudioSettings) ? FMath::Max(AudioSettings->DefaultMusicFadeSeconds, 0.f) : 2.f;
}
