// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MKHGameInstance.generated.h"

class UGenericClassReference;
enum class ERegionInfo : uint8;

/** Identifies which named level to travel to, resolved to a level name via UMKHGameInstance's level name properties. */
UENUM(BlueprintType)
enum class EMKHLevelType : uint8
{
	Lobby,
	Game
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReadyStateChangedDelegate, bool, bIsReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUsernameChangedDelegate, const FString&, NewUsername);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStartMatchDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStartMatchmakingDelegate);

/**
 * Custom GameInstance to hold session state and connectivity data.
 */
UCLASS()
class MAKHIA_API UMKHGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	// ============================================================
	// Lifecycle (Constructor, BeginPlay, Tick, EndPlay, etc.)
	// ============================================================

	virtual void Init() override;

	/**
	 * Called by the engine once a map finished loading.
	 * Clears the re-entrancy guard raised by BackToLobby so a later return can run.
	 *
	 * @param LoadTime  Seconds the map load took.
	 * @param MapName   Name of the map that finished loading.
	 */
	virtual void LoadComplete(const float LoadTime, const FString& MapName) override;

	/**
	 * Supplies the online session class the engine instantiates for this game instance.
	 * Returns UMKHOnlineSession so a lost connection routes through BackToLobby instead of
	 * the engine's default browse to the default map.
	 *
	 * @return The online session class to use.
	 */
	virtual TSubclassOf<UOnlineSession> GetOnlineSessionClass() override;

	// ============================================================
	// Delegates
	// ============================================================

	/** Triggered when the player's ready state changes. */
	UPROPERTY(BlueprintAssignable, Category = "GameInstance|Lobby")
	FOnReadyStateChangedDelegate OnReadyStateChanged;

	/** Triggered when the player's username changes. */
	UPROPERTY(BlueprintAssignable, Category = "GameInstance|Session")
	FOnUsernameChangedDelegate OnUsernameChanged;

	/** Triggered when all players are ready and there are enough players to start the match. */
	UPROPERTY(BlueprintAssignable, Category = "GameInstance|Session")
	FOnStartMatchDelegate OnStartMatchDelegate;

	/** Triggered when all players are ready but more players are needed, initiating matchmaking. */
	UPROPERTY(BlueprintAssignable, Category = "GameInstance|Session")
	FOnStartMatchmakingDelegate OnStartMatchmakingDelegate;

	// ============================================================
	// Public Interface  (BlueprintCallable / externally-facing API)
	// ============================================================

	/**
	 * Retrieves the current instance of UMKHGameInstance.
	 * 
	 * @param WorldContextObject Object to retrieve the world from.
	 * @return A pointer to the current game instance, or nullptr if cast fails.
	 */
	UFUNCTION(BlueprintPure, Category = "GameInstance", meta = (WorldContext = "WorldContextObject"))
	static UMKHGameInstance* GetMKHGameInstance(const UObject* WorldContextObject);

	/**
	 * Getter for the data asset containing references to blueprint class needed in c++
	 * @return UGenericClassReference
	 */
	TObjectPtr<UGenericClassReference> GetGenericClassReference() {return GenericClassReference;}
	
	/**
	 * Sets the player as logged in and assigns a session name.
	 * 
	 * @param UserID The ID of the user Logged in.
	 * @param NewUsername The username of the player.
	 */
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Session")
	void OnLogin(FString UserID, FString NewUsername);

	/**
	 * Logs the player out, clearing the session name and logged-in state.
	 */
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Session")
	void OnLogout();

	/**
	 * Marks the player as currently in a session.
	 */
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Session")
	void OnJoinSession();

	/**
	 * Marks the player as no longer in a session.
	 */
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Session")
	void OnLeaveSession();

	/** Gets the current session name. */
	UFUNCTION(BlueprintPure, Category = "GameInstance|Session")
	FName GetCurrentSessionName() const { return CurrentSessionName; }

	/** Sets the current session name manually. */
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Session")
	void SetCurrentSessionName(FName NewSessionName) { CurrentSessionName = NewSessionName; }

	/** Checks if the player is currently logged in. */
	UFUNCTION(BlueprintPure, Category = "GameInstance|Session")
	bool GetIsLoggedIn() const { return bIsLoggedIn; }

	/** Sets the logged-in state manually. */
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Session")
	void SetIsLoggedIn(bool bNewLoggedIn) { bIsLoggedIn = bNewLoggedIn; }

	/** Checks if the player is in an active session. */
	UFUNCTION(BlueprintPure, Category = "GameInstance|Session")
	bool GetIsInSession() const { return bIsInSession; }

	/** Sets the in-session state manually. */
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Session")
	void SetIsInSession(bool bNewInSession) { bIsInSession = bNewInSession; }

	/** Checks if the player is hosting a session. */
	UFUNCTION(BlueprintPure, Category = "GameInstance|Session")
	bool GetIsHost() const { return bIsHost; }

	/** Sets the host status. */
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Session")
	void SetIsHost(bool bNewHost) { bIsHost = bNewHost; }

	/** Checks if the player is a client. */
	UFUNCTION(BlueprintPure, Category = "GameInstance|Session")
	bool IsClient() const;

	/** Gets the player's username. */
	UFUNCTION(BlueprintPure, Category = "GameInstance|Session")
	FString GetUsername() const { return Username; }

	/** Sets the player's username. */
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Session")
	void SetUsername(const FString& NewUsername);

	/** Checks if the level is open as Listen Server. */
	UFUNCTION(BlueprintPure, Category = "GameInstance|Lobby")
	bool GetIsListenServer() const { return bIsListenServer; }
	
	/** Sets if the level is open as Listen Server. */
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Lobby")
	void SetIsListenServer(bool bNewListenServer) { bIsListenServer = bNewListenServer; }
	
	/** Gets the current number of players in the lobby. */
	UFUNCTION(BlueprintPure, Category = "GameInstance|Lobby")
	int32 GetPlayersInLobby() const { return PlayersInLobby; }

	/** Sets the current number of players in the lobby manually. */
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Lobby")
	void SetPlayersInLobby(int32 NewPlayersInLobby) { PlayersInLobby = NewPlayersInLobby; }

	/** Increments the number of players in the lobby by 1. */
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Lobby")
	int32 IncrementPlayersInLobby() { return ++PlayersInLobby; }

	/** Increments the number of players in the lobby by 1. */
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Lobby")
	int32 DecrementPlayersInLobby() { return --PlayersInLobby; }
	
	/** Checks if the player is currently matchmaking. */
	UFUNCTION(BlueprintPure, Category = "GameInstance|Lobby")
	bool GetIsMatchmaking() const { return bIsMatchmaking; }

	/** Sets the matchmaking state. */
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Lobby")
	void SetIsMatchmaking(bool bNewMatchmaking) { bIsMatchmaking = bNewMatchmaking; }

	/** Checks if the player is currently creating a lobby. */
	UFUNCTION(BlueprintPure, Category = "GameInstance|Lobby")
	bool GetIsCreatingLobby() const { return bIsCreatingLobby; }

	/** Sets the lobby creation state. */
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Lobby")
	void SetIsCreatingLobby(bool bNewCreatingLobby) { bIsCreatingLobby = bNewCreatingLobby; }

	/** Gets the required number of players to start the game. */
	UFUNCTION(BlueprintPure, Category = "GameInstance|Lobby")
	int32 GetPlayersToStart() const { return PlayersToStart; }

	/** Sets the required number of players to start the game. */
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Lobby")
	void SetPlayersToStart(int32 NewPlayersToStart) { PlayersToStart = NewPlayersToStart; }

	/** Gets the current number of players ready. */
	UFUNCTION(BlueprintPure, Category = "GameInstance|Lobby")
	int32 GetPlayersReady() const { return PlayersReady; }

	/** Sets the current number of players ready. */
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Lobby")
	void SetPlayersReady(int32 NewPlayersReady) { PlayersReady = NewPlayersReady; }

	/** Checks if the player is ready. */
	UFUNCTION(BlueprintPure, Category = "GameInstance|Lobby")
	bool GetIsReady() const { return bIsReady; }

	/** Modify the PlayersReady number and based on numbers Start the Match or Matchmaking. */ 
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Lobby")
	void HandlePlayerReady(bool bNewReady);
	
	/** Sets if the player is ready. */
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Lobby")
	void SetIsReady(bool bNewReady);

	/** Checks whether the game can be started. */
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Lobby")
	bool CanStartGame() const;

	/** Gets the selected region for sessions. */
	UFUNCTION(BlueprintPure, Category = "GameInstance|Session")
	ERegionInfo GetSelectedRegion() const { return SelectedRegion; }

	/** Sets the selected region for sessions. */
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Session")
	void SetSelectedRegion(ERegionInfo NewRegion) { SelectedRegion = NewRegion; }

	/** Called when a lobby is successfully created. */
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Lobby")
	void OnLobbyCreated();

	/**
	 * Returns to the lobby and restores the lobby state to a clean, coherent one.
	 *
	 * The online session is torn down first so the lobby is re-created from scratch on arrival
	 * (see UMKHGameInstance::TryDestroyCurrentSession); the level travel is deferred until the
	 * destroy completes, or until ReturnToLobbyTimeout elapses if the backend never answers.
	 * Repeated calls while a return is already in flight are ignored: AGameModeBase::Logout
	 * fires once per controller, so a disconnect triggers this several times in a row.
	 *
	 * @param bAsHost If true, the player will return to the lobby with server travel as listen.
	 */
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Lobby")
	void BackToLobby(bool bAsHost = false);
	
	/** Executes a server travel to the level identified by the given level type. */
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Session")
	void ServerTravel(EMKHLevelType LevelType);

	/** Opens the level identified by the given level type. */
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Session")
	void OpenLevel(EMKHLevelType LevelType, bool bAsListen = false);

	/** Gets the name of the lobby level. */
	UFUNCTION(BlueprintPure, Category = "GameInstance|Session")
	FString GetLobbyLevelName() const { return LobbyLevelName; }

	/** Gets the name of the game level. */
	UFUNCTION(BlueprintPure, Category = "GameInstance|Session")
	FString GetGameLevelName() const { return GameLevelName; }

private:

	// ============================================================
	// Protected / Internal Logic
	// ============================================================

	/**
	 * Resolves a level type to the corresponding level name set on this instance.
	 *
	 * @param LevelType The level type to resolve.
	 * @return The level name associated with the given type.
	 */
	FString ResolveLevelName(EMKHLevelType LevelType) const;

	/**
	 * Asks the online subsystem to destroy the session named CurrentSessionName.
	 * Leaving the session registered would make the lobby re-creation fail on arrival and
	 * would strand an orphan lobby on the backend.
	 *
	 * @return True if the destroy request was accepted and HandleDestroySessionComplete will run.
	 */
	bool TryDestroyCurrentSession();

	/**
	 * Completion callback for TryDestroyCurrentSession. Clears the local session state and
	 * resumes the pending return to the lobby.
	 *
	 * @param SessionName     Name of the session that was destroyed.
	 * @param bWasSuccessful  True if the backend reported a clean destroy.
	 */
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	/** Unregisters the session destroy callback, if one is still pending. */
	void ClearPendingSessionDestroy();

	/** Clears the local session state and travels to the lobby using the pending return mode. */
	void FinishBackToLobby();

	/** Resets the per-match lobby counters and flags to their pre-lobby defaults. */
	void ResetLobbyState();

	// ============================================================
	// Properties
	// ============================================================
	
	/** Data Asset to allow c++ code to take references of Blueprint Classes */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Category = "GameInstance|ClassDataAsset")
	TObjectPtr<UGenericClassReference> GenericClassReference;
		
	/** Name of the current active session. */
	UPROPERTY(VisibleAnywhere, Category = "GameInstance|Session", meta = (AllowPrivateAccess = "true"))
	FName CurrentSessionName = NAME_None;
	
	/** Indicates if the player is successfully logged into online services. */
	UPROPERTY(VisibleAnywhere, Category = "GameInstance|Session", meta = (AllowPrivateAccess = "true"))
	bool bIsLoggedIn = false;
		
	/** Indicates if the player has joined an active game session. */
	UPROPERTY(VisibleAnywhere, Category = "GameInstance|Session", meta = (AllowPrivateAccess = "true"))
	bool bIsInSession = false;

	/** Indicates if the player is the host of the current session. */
	UPROPERTY(VisibleAnywhere, Category = "GameInstance|Session", meta = (AllowPrivateAccess = "true"))
	bool bIsHost = false;
	
	/** The username of the player. */
	UPROPERTY(VisibleAnywhere, Category = "GameInstance|Session", meta = (AllowPrivateAccess = "true"))
	FString Username = TEXT("");

	/** Indicates if the player is ready. */
	UPROPERTY(VisibleAnywhere, Category = "GameInstance|Lobby", meta = (AllowPrivateAccess = "true"))
	bool bIsReady = false;

	/** Indicates if the level is open as Listen Server. */
	UPROPERTY(VisibleAnywhere, Category = "GameInstance|Lobby", meta = (AllowPrivateAccess = "true"))
	bool bIsListenServer = false;
	
	/** The current count of players in the lobby. */
	UPROPERTY(VisibleAnywhere, Category = "GameInstance|Lobby", meta = (AllowPrivateAccess = "true"))
	int32 PlayersInLobby = 0;

	/** Indicates if the player is currently matchmaking. */
	UPROPERTY(VisibleAnywhere, Category = "GameInstance|Lobby", meta = (AllowPrivateAccess = "true"))
	bool bIsMatchmaking = false;

	/** Indicates if the player is currently creating a lobby. */
	UPROPERTY(VisibleAnywhere, Category = "GameInstance|Lobby", meta = (AllowPrivateAccess = "true"))
	bool bIsCreatingLobby = false;

	/** The required number of players to start the match. */
	UPROPERTY(VisibleAnywhere, Category = "GameInstance|Lobby", meta = (AllowPrivateAccess = "true"))
	int32 PlayersToStart = 2;

	/** The current number of players marked as ready. */
	UPROPERTY(VisibleAnywhere, Category = "GameInstance|Lobby", meta = (AllowPrivateAccess = "true"))
	int32 PlayersReady = 0;

	/** The currently selected matchmaking region. */
	UPROPERTY(VisibleAnywhere, Category = "GameInstance|Session", meta = (AllowPrivateAccess = "true"))
	ERegionInfo SelectedRegion;

	/** Name of the lobby level to travel to. */
	UPROPERTY(EditDefaultsOnly, Category = "GameInstance|Session", meta = (AllowPrivateAccess = "true"))
	FString LobbyLevelName;

	/** Name of the game level to travel to. */
	UPROPERTY(EditDefaultsOnly, Category = "GameInstance|Session", meta = (AllowPrivateAccess = "true"))
	FString GameLevelName;

	/** Seconds to wait for the session destroy callback before travelling to the lobby anyway. */
	UPROPERTY(EditDefaultsOnly, Category = "GameInstance|Lobby", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float ReturnToLobbyTimeout = 5.f;

	/** True while a return to the lobby is in flight; used to ignore duplicate requests. */
	UPROPERTY(VisibleAnywhere, Category = "GameInstance|Lobby", meta = (AllowPrivateAccess = "true"))
	bool bReturningToLobby = false;

	/** Return mode requested by the in-flight BackToLobby call. */
	UPROPERTY(VisibleAnywhere, Category = "GameInstance|Lobby", meta = (AllowPrivateAccess = "true"))
	bool bPendingReturnAsHost = false;

	/** Handle for the session destroy completion delegate registered by TryDestroyCurrentSession. */
	FDelegateHandle DestroySessionCompleteHandle;

	/** Timer guarding against a session destroy callback that never fires. */
	FTimerHandle ReturnToLobbyTimeoutHandle;

};
