// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "UI/HUD/Settings/MKHAudioSettingsWidget.h"

#include "Components/Slider.h"
#include "GameUserSettings/MKHGameUserSettings.h"
#include "MKHLogChannels.h"

void UMKHAudioSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	InitializeSliderValues();
	BindSliderEvents();
}

void UMKHAudioSettingsWidget::NativeDestruct()
{
	// Safety net: persist any change made without releasing the slider (e.g. menu closed mid-drag).
	HandleVolumeCaptureEnd();
	UnbindSliderEvents();

	Super::NativeDestruct();
}

void UMKHAudioSettingsWidget::HandleMasterVolumeChanged(float NewValue)
{
	if (UMKHGameUserSettings* UserSettings = UMKHGameUserSettings::GetMKHGameUserSettings(); IsValid(UserSettings))
	{
		UserSettings->SetMasterVolume(NewValue);
	}
}

void UMKHAudioSettingsWidget::HandleMusicVolumeChanged(float NewValue)
{
	if (UMKHGameUserSettings* UserSettings = UMKHGameUserSettings::GetMKHGameUserSettings(); IsValid(UserSettings))
	{
		UserSettings->SetMusicVolume(NewValue);
	}
}

void UMKHAudioSettingsWidget::HandleSFXVolumeChanged(float NewValue)
{
	if (UMKHGameUserSettings* UserSettings = UMKHGameUserSettings::GetMKHGameUserSettings(); IsValid(UserSettings))
	{
		UserSettings->SetSFXVolume(NewValue);
	}
}

void UMKHAudioSettingsWidget::HandleVolumeCaptureEnd()
{
	if (UMKHGameUserSettings* UserSettings = UMKHGameUserSettings::GetMKHGameUserSettings(); IsValid(UserSettings))
	{
		UserSettings->SaveSettings();
	}
}

void UMKHAudioSettingsWidget::InitializeSliderValues() const
{
	const UMKHGameUserSettings* UserSettings = UMKHGameUserSettings::GetMKHGameUserSettings();
	if (!IsValid(UserSettings))
	{
		UE_LOG(LogMKHAudio, Warning, TEXT("UMKHAudioSettingsWidget::InitializeSliderValues: UMKHGameUserSettings unavailable."));
		return;
	}

	if (IsValid(MasterVolumeSlider))
	{
		MasterVolumeSlider->SetValue(UserSettings->GetMasterVolume());
	}
	if (IsValid(MusicVolumeSlider))
	{
		MusicVolumeSlider->SetValue(UserSettings->GetMusicVolume());
	}
	if (IsValid(SFXVolumeSlider))
	{
		SFXVolumeSlider->SetValue(UserSettings->GetSFXVolume());
	}
}

void UMKHAudioSettingsWidget::BindSliderEvents()
{
	if (IsValid(MasterVolumeSlider))
	{
		MasterVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UMKHAudioSettingsWidget::HandleMasterVolumeChanged);
		MasterVolumeSlider->OnMouseCaptureEnd.AddUniqueDynamic(this, &UMKHAudioSettingsWidget::HandleVolumeCaptureEnd);
		MasterVolumeSlider->OnControllerCaptureEnd.AddUniqueDynamic(this, &UMKHAudioSettingsWidget::HandleVolumeCaptureEnd);
	}
	if (IsValid(MusicVolumeSlider))
	{
		MusicVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UMKHAudioSettingsWidget::HandleMusicVolumeChanged);
		MusicVolumeSlider->OnMouseCaptureEnd.AddUniqueDynamic(this, &UMKHAudioSettingsWidget::HandleVolumeCaptureEnd);
		MusicVolumeSlider->OnControllerCaptureEnd.AddUniqueDynamic(this, &UMKHAudioSettingsWidget::HandleVolumeCaptureEnd);
	}
	if (IsValid(SFXVolumeSlider))
	{
		SFXVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UMKHAudioSettingsWidget::HandleSFXVolumeChanged);
		SFXVolumeSlider->OnMouseCaptureEnd.AddUniqueDynamic(this, &UMKHAudioSettingsWidget::HandleVolumeCaptureEnd);
		SFXVolumeSlider->OnControllerCaptureEnd.AddUniqueDynamic(this, &UMKHAudioSettingsWidget::HandleVolumeCaptureEnd);
	}
}

void UMKHAudioSettingsWidget::UnbindSliderEvents()
{
	if (IsValid(MasterVolumeSlider))
	{
		MasterVolumeSlider->OnValueChanged.RemoveDynamic(this, &UMKHAudioSettingsWidget::HandleMasterVolumeChanged);
		MasterVolumeSlider->OnMouseCaptureEnd.RemoveDynamic(this, &UMKHAudioSettingsWidget::HandleVolumeCaptureEnd);
		MasterVolumeSlider->OnControllerCaptureEnd.RemoveDynamic(this, &UMKHAudioSettingsWidget::HandleVolumeCaptureEnd);
	}
	if (IsValid(MusicVolumeSlider))
	{
		MusicVolumeSlider->OnValueChanged.RemoveDynamic(this, &UMKHAudioSettingsWidget::HandleMusicVolumeChanged);
		MusicVolumeSlider->OnMouseCaptureEnd.RemoveDynamic(this, &UMKHAudioSettingsWidget::HandleVolumeCaptureEnd);
		MusicVolumeSlider->OnControllerCaptureEnd.RemoveDynamic(this, &UMKHAudioSettingsWidget::HandleVolumeCaptureEnd);
	}
	if (IsValid(SFXVolumeSlider))
	{
		SFXVolumeSlider->OnValueChanged.RemoveDynamic(this, &UMKHAudioSettingsWidget::HandleSFXVolumeChanged);
		SFXVolumeSlider->OnMouseCaptureEnd.RemoveDynamic(this, &UMKHAudioSettingsWidget::HandleVolumeCaptureEnd);
		SFXVolumeSlider->OnControllerCaptureEnd.RemoveDynamic(this, &UMKHAudioSettingsWidget::HandleVolumeCaptureEnd);
	}
}
