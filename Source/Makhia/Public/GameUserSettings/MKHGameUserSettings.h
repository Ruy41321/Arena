// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "MKHGameUserSettings.generated.h"

class USoundControlBus;

/**
 * Project-specific user settings persisted to GameUserSettings.ini.
 * Extends the engine settings with accessibility options and user-facing
 * audio volume controls (master / music / SFX).
 *
 * Volumes are stored as normalized sliders in [0, 1] and pushed to the
 * Audio Modulation Control Buses configured in UMKHAudioSettings. The buses
 * use a Volume modulation parameter, which maps the normalized value to
 * decibels internally — this gives a perceptually linear slider without
 * manual dB conversion in game code.
 */
UCLASS()
class MAKHIA_API UMKHGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	// ============================================================
	// Lifecycle (Constructor, BeginPlay, Tick, EndPlay, etc.)
	// ============================================================

	UMKHGameUserSettings();

	//~ Begin UGameUserSettings Interface
	virtual void ApplySettings(bool bCheckForCommandLineOverrides) override;
	virtual void SetToDefaults() override;
	//~ End UGameUserSettings Interface

	// ============================================================
	// Public Interface  (BlueprintCallable / externally-facing API)
	// ============================================================

	/**
	 * Convenience accessor for the active game user settings.
	 *
	 * @return The engine's UGameUserSettings instance cast to the project type, or nullptr if unavailable.
	 */
	UFUNCTION(BlueprintPure, Category = "Settings")
	static UMKHGameUserSettings* GetMKHGameUserSettings();

	// --- Audio ---

	/**
	 * Sets the master volume and applies it immediately to the master Control Bus.
	 * Call SaveSettings() (or ApplySettings) afterwards to persist the value to disk.
	 *
	 * @param NewVolume Normalized volume in [0, 1]; values outside the range are clamped.
	 */
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMasterVolume(float NewVolume);

	/** Gets the normalized master volume in [0, 1]. */
	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetMasterVolume() const { return MasterVolume; }

	/**
	 * Sets the music volume and applies it immediately to the music Control Bus.
	 * Call SaveSettings() (or ApplySettings) afterwards to persist the value to disk.
	 *
	 * @param NewVolume Normalized volume in [0, 1]; values outside the range are clamped.
	 */
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMusicVolume(float NewVolume);

	/** Gets the normalized music volume in [0, 1]. */
	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetMusicVolume() const { return MusicVolume; }

	/**
	 * Sets the sound effects volume and applies it immediately to the SFX Control Bus.
	 * Call SaveSettings() (or ApplySettings) afterwards to persist the value to disk.
	 *
	 * @param NewVolume Normalized volume in [0, 1]; values outside the range are clamped.
	 */
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetSFXVolume(float NewVolume);

	/** Gets the normalized sound effects volume in [0, 1]. */
	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetSFXVolume() const { return SFXVolume; }

	/**
	 * Pushes every stored volume to its Control Bus on the currently active play world.
	 * Safe to call when no play world exists (e.g. before the first level is up):
	 * in that case it is a no-op and UMKHAudioWorldSubsystem will re-apply the
	 * values when the world begins play.
	 */
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void ApplyAudioSettings();

	// --- Accessibility ---

	/** Enables or disables gameplay camera shake effects. */
	UFUNCTION(BlueprintCallable, Category = "Settings|Accessibility")
	void SetCameraShakeEnabled(bool bEnable) { bEnableCameraShake = bEnable; }

	/** Checks whether gameplay camera shake effects are enabled. */
	UFUNCTION(BlueprintPure, Category = "Settings|Accessibility")
	bool IsCameraShakeEnabled() const { return bEnableCameraShake; }

private:

	// ============================================================
	// Protected / Internal Logic
	// ============================================================

	/**
	 * Applies a single normalized volume to a Control Bus through a global bus mix value.
	 * No-op (with a verbose log) when no play world is active; logs a warning when the
	 * bus asset is not assigned in UMKHAudioSettings.
	 *
	 * @param BusPtr       Soft reference to the target Control Bus (from UMKHAudioSettings).
	 * @param LinearVolume Normalized volume in [0, 1] to set on the bus.
	 */
	void ApplyVolumeToControlBus(const TSoftObjectPtr<USoundControlBus>& BusPtr, float LinearVolume) const;

	// ============================================================
	// Properties
	// ============================================================

	/** Normalized master volume in [0, 1]. Drives the master Control Bus (affects every submix). */
	UPROPERTY(Config)
	float MasterVolume = 1.f;

	/** Normalized music volume in [0, 1]. Drives the music Control Bus. */
	UPROPERTY(Config)
	float MusicVolume = 1.f;

	/** Normalized sound effects volume in [0, 1]. Drives the SFX Control Bus. */
	UPROPERTY(Config)
	float SFXVolume = 1.f;

	/** Whether gameplay camera shake effects are allowed to play. */
	UPROPERTY(Config)
	bool bEnableCameraShake;
};
