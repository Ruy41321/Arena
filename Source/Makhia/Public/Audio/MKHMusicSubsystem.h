// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MKHMusicSubsystem.generated.h"

class UAudioComponent;
class USoundBase;

/**
 * Owns background music playback for the whole game session.
 *
 * Lives on the GameInstance so the music component survives level travel
 * (created with bPersistAcrossLevelTransition): requesting the track that is
 * already playing is a no-op, which makes music continue seamlessly across
 * map changes that share the same theme. Requesting a different track
 * performs a crossfade; stopping fades out.
 *
 * Track selection per level is data-driven via UMKHAudioSettings::LevelMusicMap
 * and triggered by UMKHAudioWorldSubsystem on world begin play, so it runs
 * locally on every client (music is never a replicated concern).
 */
UCLASS()
class MAKHIA_API UMKHMusicSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ============================================================
	// Public Interface  (BlueprintCallable / externally-facing API)
	// ============================================================

	/**
	 * Starts (or crossfades to) the given music track.
	 * If the track is already the one playing, does nothing so playback
	 * continues seamlessly. Any previously playing track is faded out.
	 *
	 * @param Track          The track to play. Must be routed to the music submix (asset's Submix property).
	 * @param FadeInSeconds  Fade-in duration; negative values use UMKHAudioSettings::DefaultMusicFadeSeconds.
	 */
	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void PlayMusicTrack(USoundBase* Track, float FadeInSeconds = -1.f);

	/**
	 * Fades out and stops the currently playing music, if any.
	 *
	 * @param FadeOutSeconds Fade-out duration; negative values use UMKHAudioSettings::DefaultMusicFadeSeconds.
	 */
	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void StopMusic(float FadeOutSeconds = -1.f);

	/** Gets the track currently playing, or nullptr when no music is active. */
	UFUNCTION(BlueprintPure, Category = "Audio|Music")
	USoundBase* GetCurrentTrack() const { return CurrentTrack; }

	/** Checks whether a music track is currently playing. */
	UFUNCTION(BlueprintPure, Category = "Audio|Music")
	bool IsMusicPlaying() const;

private:

	// ============================================================
	// Protected / Internal Logic
	// ============================================================

	/**
	 * Resolves a requested fade duration, falling back to the project default.
	 *
	 * @param RequestedSeconds Caller-provided duration; negative means "use default".
	 * @return The fade duration to use, never negative.
	 */
	float ResolveFadeSeconds(float RequestedSeconds) const;

	// ============================================================
	// Properties
	// ============================================================

	/** Audio component playing the current music track. Persists across level transitions. */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MusicComponent;

	/** The track currently playing (or fading in). Null when music is stopped. */
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> CurrentTrack;
};
