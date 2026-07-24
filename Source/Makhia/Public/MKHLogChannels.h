// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#pragma once

#include "CoreMinimal.h"

/** Log category for the Gameplay Ability System layer (abilities, attributes, damage pipeline, input queue). */
MAKHIA_API DECLARE_LOG_CATEGORY_EXTERN(LogMKHAbility, Log, All);

/** Log category for the audio layer (music playback, submix routing, volume modulation, user audio settings). */
MAKHIA_API DECLARE_LOG_CATEGORY_EXTERN(LogMKHAudio, Log, All);

/** Log category for the session layer (login, lobby lifecycle, matchmaking, level travel). */
MAKHIA_API DECLARE_LOG_CATEGORY_EXTERN(LogMKHSession, Log, All);
