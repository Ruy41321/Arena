// Copyright (c) 2025 Luigi Pennisi. All rights reserved.


#include "GameInstance/MKHOnlineSession.h"
#include "GameInstance/MKHGameInstance.h"
#include "MKHLogChannels.h"

void UMKHOnlineSession::HandleDisconnect(UWorld* World, UNetDriver* NetDriver)
{
	UMKHGameInstance* GameInstance = Cast<UMKHGameInstance>(GetOuter());
	if (!IsValid(GameInstance))
	{
		UE_LOG(LogMKHSession, Warning, TEXT("HandleDisconnect: no MKH game instance, falling back to the default handling."));
		Super::HandleDisconnect(World, NetDriver);
		return;
	}

	UE_LOG(LogMKHSession, Log, TEXT("HandleDisconnect: connection lost, returning to the lobby as host."));

	// Returns as host so the lobby is re-created on arrival, mirroring the post-login flow.
	GameInstance->BackToLobby(true);
}
