// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Audio/MKHAudioSettings.h"

UMKHAudioSettings::UMKHAudioSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("Makhia Audio");
}

const UMKHAudioSettings* UMKHAudioSettings::Get()
{
	return GetDefault<UMKHAudioSettings>();
}
