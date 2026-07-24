// Fill out your copyright notice in the Description page of Project Settings.


#include "GameInstance/MKHGameInstance.h"
#include "GameInstance/MKHOnlineSession.h"
#include "Kismet/GameplayStatics.h"
#include "MKHLogChannels.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "TimerManager.h"

void UMKHGameInstance::Init()
{
	Super::Init();

}

void UMKHGameInstance::LoadComplete(const float LoadTime, const FString& MapName)
{
	Super::LoadComplete(LoadTime, MapName);

	bReturningToLobby = false;
}

TSubclassOf<UOnlineSession> UMKHGameInstance::GetOnlineSessionClass()
{
	return UMKHOnlineSession::StaticClass();
}

UMKHGameInstance* UMKHGameInstance::GetMKHGameInstance(const UObject* WorldContextObject)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (UMKHGameInstance* MKHGameInstance = Cast<UMKHGameInstance>(World->GetGameInstance()))
		{
			return MKHGameInstance;
		}
	}
	return nullptr;
}

void UMKHGameInstance::OnLogin(FString UserID, FString NewUsername)
{
	// If UserID is empty print on screen a debug message as warning
	if (UserID.IsEmpty())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Warning: UserID is empty! The Session Name will be a non unique one causing problems"));
	}
	UserID += "_LOBBY";
	CurrentSessionName = FName(UserID);
	SetUsername(NewUsername);
	bIsLoggedIn = true;
	OpenLevel(EMKHLevelType::Lobby, true);
}

void UMKHGameInstance::OnLogout()
{
	CurrentSessionName = NAME_None;
	bIsLoggedIn = false;
	bIsInSession = false;
	bIsHost = false;
}

void UMKHGameInstance::OnJoinSession()
{
	bIsInSession = true;
	bIsListenServer = false;
}

void UMKHGameInstance::OnLeaveSession()
{
	bIsInSession = false;
	bIsHost = false;
	PlayersInLobby = 0;
	PlayersReady = 0;
}

bool UMKHGameInstance::IsClient() const
{
	return bIsInSession && !bIsHost;
}

void UMKHGameInstance::SetUsername(const FString& NewUsername)
{
	if (Username != NewUsername)
	{
		Username = NewUsername;
		OnUsernameChanged.Broadcast(Username);
	}
}

void UMKHGameInstance::HandlePlayerReady(bool bNewReady)
{
	if (bNewReady)
	{
		PlayersReady++;
		if (PlayersReady == PlayersInLobby)
		{
			if (PlayersReady >= PlayersToStart)
			{
				OnStartMatchDelegate.Broadcast();
			}
			else
			{
				OnStartMatchmakingDelegate.Broadcast();	
			}
		}
	}
	else
	{
		PlayersReady--;
	}
}

void UMKHGameInstance::SetIsReady(bool bNewReady)
{
	if (bIsReady != bNewReady)
	{
		bIsReady = bNewReady;
		OnReadyStateChanged.Broadcast(bIsReady);
	}
}

void UMKHGameInstance::OnLobbyCreated()
{
	PlayersInLobby = 1;
	bIsHost = true;
	bIsInSession = true;
}

void UMKHGameInstance::BackToLobby(bool bAsHost)
{
	// AGameModeBase::Logout fires once per controller, so a disconnect lands here repeatedly.
	if (bReturningToLobby)
	{
		return;
	}

	bReturningToLobby = true;
	bPendingReturnAsHost = bAsHost;
	ResetLobbyState();

	if (!bIsInSession || !TryDestroyCurrentSession())
	{
		FinishBackToLobby();
		return;
	}

	// A subsystem that answered inline already travelled and cleared the handle: no fallback needed.
	if (!DestroySessionCompleteHandle.IsValid())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ReturnToLobbyTimeoutHandle, this, &UMKHGameInstance::FinishBackToLobby,
			FMath::Max(ReturnToLobbyTimeout, 0.1f), false);
	}
}

void UMKHGameInstance::ResetLobbyState()
{
	PlayersReady = 0;
	PlayersInLobby = 0;
	bIsReady = false;
	bIsMatchmaking = false;
}

bool UMKHGameInstance::TryDestroyCurrentSession()
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	const IOnlineSessionPtr Sessions = Subsystem ? Subsystem->GetSessionInterface() : nullptr;
	if (!Sessions.IsValid() || CurrentSessionName == NAME_None)
	{
		UE_LOG(LogMKHSession, Warning,
			TEXT("BackToLobby: no session interface or no session name, returning to the lobby without a destroy."));
		return false;
	}

	DestroySessionCompleteHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &UMKHGameInstance::HandleDestroySessionComplete));

	if (!Sessions->DestroySession(CurrentSessionName))
	{
		ClearPendingSessionDestroy();
		UE_LOG(LogMKHSession, Warning, TEXT("BackToLobby: DestroySession('%s') was refused."), *CurrentSessionName.ToString());
		return false;
	}

	UE_LOG(LogMKHSession, Log, TEXT("BackToLobby: destroying session '%s' before returning."), *CurrentSessionName.ToString());
	return true;
}

void UMKHGameInstance::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	// The delegate is global to the session interface, so ignore destroys we did not request.
	if (SessionName != CurrentSessionName)
	{
		return;
	}

	UE_LOG(LogMKHSession, Log, TEXT("BackToLobby: session '%s' destroy completed (success: %d)."),
		*SessionName.ToString(), bWasSuccessful ? 1 : 0);

	FinishBackToLobby();
}

void UMKHGameInstance::ClearPendingSessionDestroy()
{
	if (!DestroySessionCompleteHandle.IsValid())
	{
		return;
	}

	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		if (const IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface(); Sessions.IsValid())
		{
			Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
		}
	}
	DestroySessionCompleteHandle.Reset();
}

void UMKHGameInstance::FinishBackToLobby()
{
	// Whichever of the two resume paths runs first disarms the other, so this runs once per return.
	ClearPendingSessionDestroy();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReturnToLobbyTimeoutHandle);
	}

	// Clears bIsInSession/bIsHost so the lobby re-hosts itself on arrival.
	OnLeaveSession();
	OpenLevel(EMKHLevelType::Lobby, bPendingReturnAsHost);
}

bool UMKHGameInstance::CanStartGame() const
{
	if (!bIsHost)
		return false;
	
	if (PlayersInLobby < PlayersToStart)
		return false;
	
	if (!bIsMatchmaking && PlayersReady < PlayersInLobby)
		return false;
	
	return true;
}

void UMKHGameInstance::ServerTravel(EMKHLevelType LevelType)
{
	if (UWorld* World = GetWorld())
	{
		FString URL = FString::Printf(TEXT("%s?listen"), *ResolveLevelName(LevelType));
		World->ServerTravel(URL);
	}
}

void UMKHGameInstance::OpenLevel(EMKHLevelType LevelType, bool bAsListen)
{
	FString FinalUrl = ResolveLevelName(LevelType);
	bIsListenServer = bAsListen;
	if (bAsListen)
	{
		FinalUrl += TEXT("?listen");
	}
	UGameplayStatics::OpenLevel(this, FName(*FinalUrl));
}

FString UMKHGameInstance::ResolveLevelName(EMKHLevelType LevelType) const
{
	switch (LevelType)
	{
	case EMKHLevelType::Lobby:
		return LobbyLevelName;
	case EMKHLevelType::Game:
		return GameLevelName;
	default:
		UE_LOG(LogTemp, Warning, TEXT("UMKHGameInstance::ResolveLevelName: Unhandled EMKHLevelType, returning empty level name."));
		return FString();
	}
}
