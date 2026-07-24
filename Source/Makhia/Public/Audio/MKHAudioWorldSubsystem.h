// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MKHAudioWorldSubsystem.generated.h"

/**
 * Bootstraps the audio state for every gameplay world.
 *
 * On world begin play (initial boot, PIE session, level travel) it:
 *  1. Re-pushes the saved user volumes to the Control Buses — Audio Modulation
 *     state lives on the world's audio device, so it must be re-applied per world.
 *  2. Starts the background music configured for the level in
 *     UMKHAudioSettings::LevelMusicMap (or fades out music when the level has
 *     no entry). Running here means it executes locally on every client,
 *     independently of the server-only GameMode.
 */
UCLASS()
class MAKHIA_API UMKHAudioWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ============================================================
	// Lifecycle (Constructor, BeginPlay, Tick, EndPlay, etc.)
	// ============================================================

	//~ Begin USubsystem Interface

	/**
	 * Restricts subsystem creation to worlds that actually run gameplay
	 * (Game and PIE), skipping editor preview and inactive worlds.
	 *
	 * @param Outer The world being initialized.
	 * @return True if the subsystem should be created for the given world.
	 */
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	//~ End USubsystem Interface

	//~ Begin UWorldSubsystem Interface

	/**
	 * Applies the persisted user volumes to the Audio Modulation Control Buses
	 * and starts the level's configured background music once the world is
	 * ready to play sounds.
	 *
	 * @param InWorld The world that just began play.
	 */
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	//~ End UWorldSubsystem Interface

private:

	// ============================================================
	// Protected / Internal Logic
	// ============================================================

	/**
	 * Looks up the music track configured for the given world in
	 * UMKHAudioSettings::LevelMusicMap and asks UMKHMusicSubsystem to play it.
	 * Fades out any playing music when the level has no configured track.
	 *
	 * @param InWorld The world that just began play.
	 */
	void StartLevelMusic(UWorld& InWorld) const;
};
