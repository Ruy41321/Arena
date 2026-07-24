// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/MKHSystemWidget.h"
#include "MKHAudioSettingsWidget.generated.h"

class USlider;

/**
 * Audio settings panel with one slider per volume category (master / music / SFX).
 *
 * The Blueprint subclass only needs to place three USlider widgets named exactly
 * MasterVolumeSlider, MusicVolumeSlider and SFXVolumeSlider (BindWidget): all the
 * wiring is handled natively — sliders are initialized from UMKHGameUserSettings,
 * dragging applies the volume live to the Control Buses (audible feedback while
 * dragging), and releasing the slider persists the values to GameUserSettings.ini.
 *
 * No WidgetController is involved on purpose: user settings are a global,
 * engine-owned model with a single writer (this panel), so the Model->View
 * observer indirection used for gameplay data would add nothing here.
 */
UCLASS(Abstract)
class MAKHIA_API UMKHAudioSettingsWidget : public UMKHSystemWidget
{
	GENERATED_BODY()

protected:
	// ============================================================
	// Lifecycle (Constructor, BeginPlay, Tick, EndPlay, etc.)
	// ============================================================

	//~ Begin UUserWidget Interface

	/** Initializes the sliders from the saved user settings and binds their events. */
	virtual void NativeConstruct() override;

	/** Persists any pending changes and unbinds slider events. */
	virtual void NativeDestruct() override;
	//~ End UUserWidget Interface

	// ============================================================
	// Protected / Internal Logic
	// ============================================================

	/**
	 * Applies the master volume live while the slider is being dragged.
	 *
	 * @param NewValue Normalized slider value in [0, 1].
	 */
	UFUNCTION()
	void HandleMasterVolumeChanged(float NewValue);

	/**
	 * Applies the music volume live while the slider is being dragged.
	 *
	 * @param NewValue Normalized slider value in [0, 1].
	 */
	UFUNCTION()
	void HandleMusicVolumeChanged(float NewValue);

	/**
	 * Applies the SFX volume live while the slider is being dragged.
	 *
	 * @param NewValue Normalized slider value in [0, 1].
	 */
	UFUNCTION()
	void HandleSFXVolumeChanged(float NewValue);

	/** Persists the current volumes to GameUserSettings.ini when a slider is released. */
	UFUNCTION()
	void HandleVolumeCaptureEnd();

private:

	/** Pushes the saved volume values into the three sliders without triggering change events. */
	void InitializeSliderValues() const;

	/** Binds value-changed and capture-end events on the three sliders. */
	void BindSliderEvents();

	/** Unbinds every event this widget registered on the sliders. */
	void UnbindSliderEvents();

	// ============================================================
	// Properties
	// ============================================================

	/** Slider controlling the master volume. Must exist in the Blueprint subclass with this exact name. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<USlider> MasterVolumeSlider;

	/** Slider controlling the music volume. Must exist in the Blueprint subclass with this exact name. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<USlider> MusicVolumeSlider;

	/** Slider controlling the sound effects volume. Must exist in the Blueprint subclass with this exact name. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<USlider> SFXVolumeSlider;
};
