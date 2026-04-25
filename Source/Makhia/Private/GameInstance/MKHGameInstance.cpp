// Fill out your copyright notice in the Description page of Project Settings.


#include "GameInstance/MKHGameInstance.h"
#include "Kismet/GameplayStatics.h"

void UMKHGameInstance::Init()
{
	Super::Init();

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
	OpenLevel("Lobby", true);
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
	PlayersReady = 0;
	bIsMatchmaking = false;
	if (bAsHost)
	{
		OpenLevel("Lobby", true);
	}
	else
	{
		OnLeaveSession();
		OpenLevel("Lobby", false);
	}
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

void UMKHGameInstance::ServerTravel(FName LevelName)
{
	if (UWorld* World = GetWorld())
	{
		FString URL = FString::Printf(TEXT("%s?listen"), *LevelName.ToString());
		World->ServerTravel(URL);
	}
}

void UMKHGameInstance::OpenLevel(FString LevelUrl, bool bAsListen)
{
	FString FinalUrl = LevelUrl;
	bIsListenServer = bAsListen;
	if (bAsListen)
	{
		FinalUrl += TEXT("?listen");
	}
	UGameplayStatics::OpenLevel(this, FName(*FinalUrl));
}
