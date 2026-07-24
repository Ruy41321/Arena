// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MKHAudioSettings.generated.h"

class USoundBase;
class USoundControlBus;

/**
 * Project-wide audio configuration exposed in Project Settings -> Game -> Makhia Audio.
 * Holds references to the Audio Modulation Control Buses that drive user-facing
 * volume categories. Gameplay code reads these instead of hardcoding asset paths,
 * so audio designers can re-route or swap buses without touching C++.
 *
 * Values are stored in Config/DefaultGame.ini under [/Script/Makhia.MKHAudioSettings].
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Makhia Audio"))
class MAKHIA_API UMKHAudioSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// ============================================================
	// Lifecycle (Constructor, BeginPlay, Tick, EndPlay, etc.)
	// ============================================================

	UMKHAudioSettings();

	// ============================================================
	// Public Interface  (BlueprintCallable / externally-facing API)
	// ============================================================

	/**
	 * Convenience accessor for the project audio settings.
	 *
	 * @return The CDO of UMKHAudioSettings holding the configured values.
	 */
	static const UMKHAudioSettings* Get();

	// ============================================================
	// Properties
	// ============================================================

	/** Control Bus driving the master volume. Attached to the output volume modulation of every top-level submix. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Volume Control Buses")
	TSoftObjectPtr<USoundControlBus> MasterVolumeControlBus;

	/** Control Bus driving the music volume. Attached to the output volume modulation of the music submix. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Volume Control Buses")
	TSoftObjectPtr<USoundControlBus> MusicVolumeControlBus;

	/** Control Bus driving the sound effects volume. Attached to the output volume modulation of the SFX submix. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Volume Control Buses")
	TSoftObjectPtr<USoundControlBus> SFXVolumeControlBus;

	/**
	 * Maps a level name (short name, without path or PIE prefix — e.g. "CastleArena")
	 * to the music track that starts when that level begins play.
	 * Levels without an entry fade out any currently playing music.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Music")
	TMap<FString, TSoftObjectPtr<USoundBase>> LevelMusicMap;

	/** Default fade duration (seconds) used for music fade-in, fade-out and crossfades. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Music", meta = (ClampMin = "0.0"))
	float DefaultMusicFadeSeconds = 2.f;
};
