// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "MKHGameState.generated.h"

class AMKHPlayerCharacter;

/**
 * High-level phase the 1v1 duel match is currently in.
 * Drives which timer is running and which gameplay rules are active.
 */
UENUM(BlueprintType)
enum class EMKHMatchPhase : uint8
{
	/** Match has not started yet (players still joining). */
	None,
	/** Initial equip phase: players prepare their loadout before the first duel. */
	InitialPreparation,
	/** Active duel: both players fight until one dies or the duel timer expires. */
	Duel,
	/** Sudden-death window after the duel timer expired: a drain effect ticks down health. */
	SuddenDeath,
	/** Short preparation phase played between two duels. */
	IntermissionPreparation,
	/** The match ended with a winner or a draw. */
	MatchOver
};

/**
 * Atomic snapshot of the replicated match/phase state.
 * Grouping phase + timing into a single replicated struct guarantees the fields arrive together
 * with one OnRep, so clients never observe a new phase paired with a stale countdown end time.
 */
USTRUCT(BlueprintType)
struct FMKHMatchStateData
{
	GENERATED_BODY()

	/** The current match phase. */
	UPROPERTY(BlueprintReadOnly, Category = "Duel")
	EMKHMatchPhase Phase = EMKHMatchPhase::None;

	/** Server world time (seconds) at which the current phase's countdown ends; 0 when the phase has no countdown. */
	UPROPERTY(BlueprintReadOnly, Category = "Duel")
	float PhaseEndServerTime = 0.f;

	/** Total duration (seconds) of the current phase; 0 when the phase has no countdown. Used for progress bars. */
	UPROPERTY(BlueprintReadOnly, Category = "Duel")
	float PhaseDuration = 0.f;
};

/**
 * Replicated per-player data for a single duel participant (character reference + score).
 * Held in a replicated array so every peer can render the live scoreboard.
 */
USTRUCT(BlueprintType)
struct FMKHDuelPlayerData
{
	GENERATED_BODY()

	/** The participant this entry refers to. */
	UPROPERTY(BlueprintReadOnly, Category = "Duel")
	TObjectPtr<AMKHPlayerCharacter> PlayerCharacter = nullptr;

	/** Rounds won by this participant. */
	UPROPERTY(BlueprintReadOnly, Category = "Duel")
	int32 Score = 0;
};

/**
 * Atomic snapshot of the post-match rematch ballot.
 * Both counters travel together so a client never renders "2/0" while the second field is stale.
 */
USTRUCT(BlueprintType)
struct FMKHRematchVoteData
{
	GENERATED_BODY()

	/** How many participants have asked for a rematch so far. */
	UPROPERTY(BlueprintReadOnly, Category = "Duel")
	int32 CurrentVotes = 0;

	/** How many votes are needed before the match restarts; 0 while no ballot is open. */
	UPROPERTY(BlueprintReadOnly, Category = "Duel")
	int32 RequiredVotes = 0;
};

// ============================================================
// Delegates (multicast, Blueprint-bindable — fire on every peer)
// ============================================================

/** Generic phase-change notification carrying the new phase. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMKHOnMatchPhaseChanged, EMKHMatchPhase, NewPhase);

/** Parameter-less phase notification reused by the granular per-phase events. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMKHDuelPhaseSignature);

/**
 * Broadcast when a participant is assigned to a scoreboard slot.
 * @param SlotIndex  The slot the participant occupies (0/1).
 * @param Character  The registered participant.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMKHOnParticipantRegistered, int32, SlotIndex, AMKHPlayerCharacter*, Character);

/**
 * Broadcast when a participant's score changes.
 * @param SlotIndex  The slot whose score changed (0/1).
 * @param NewScore   The updated score.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMKHOnScoreChanged, int32, SlotIndex, int32, NewScore);

/**
 * Broadcast on every peer when a player dies and a point is awarded.
 * @param DeadPlayer      The participant that died.
 * @param ScoringPlayer   The surviving participant that received the point.
 * @param ScoringScore    The updated score of the scoring player.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMKHOnPlayerDied, AMKHPlayerCharacter*, DeadPlayer, AMKHPlayerCharacter*, ScoringPlayer, int32, ScoringScore);

/**
 * Broadcast on every peer when the match ends.
 * @param WinningPlayer  The winner, or nullptr on a draw.
 * @param bIsDraw        True when both players reached the winning score together.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMKHOnMatchOver, AMKHPlayerCharacter*, WinningPlayer, bool, bIsDraw);

/**
 * Broadcast on every peer whenever the rematch ballot changes.
 * @param CurrentVotes   Participants that have asked for a rematch.
 * @param RequiredVotes  Votes needed before the match restarts.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMKHOnRematchVotesChanged, int32, CurrentVotes, int32, RequiredVotes);

/**
 * Replicated state hub for the 1v1 arena duel.
 *
 * The (server-only) game mode owns the match logic and pushes state here; this game state
 * replicates that state to every client and exposes Blueprint-bindable events so UI on all peers
 * reacts to phase changes and score updates. The live countdown is driven from a replicated
 * server-time deadline, letting each client compute the remaining time locally every frame
 * (smooth, no per-frame replication).
 */
UCLASS()
class MAKHIA_API AMKHGameState : public AGameState
{
	GENERATED_BODY()

	// ============================================================
	// Lifecycle
	// ============================================================

public:
	AMKHGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ============================================================
	// Public Interface (Server authority — called by the game mode)
	// ============================================================

	/**
	 * Sets the current phase and (re)starts its countdown. Authority only.
	 * @param NewPhase             The phase to enter.
	 * @param PhaseDurationSeconds Countdown length; pass <= 0 for a phase without a countdown.
	 */
	void SetMatchPhase(EMKHMatchPhase NewPhase, float PhaseDurationSeconds);

	/** Registers a participant in the given scoreboard slot (0/1) with a zeroed score. Authority only. */
	void RegisterParticipant(int32 SlotIndex, AMKHPlayerCharacter* Character);

	/** Sets the score of the given scoreboard slot (0/1). Authority only. */
	void SetScore(int32 SlotIndex, int32 NewScore);

	/** Fires the "player died" event on every peer. Authority only. */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayerDied(AMKHPlayerCharacter* DeadPlayer, AMKHPlayerCharacter* ScoringPlayer, int32 ScoringScore);

	/** Fires the match-over event on every peer; WinningPlayer is null on a draw. Authority only. */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_MatchOver(AMKHPlayerCharacter* WinningPlayer, bool bIsDraw);

	/**
	 * Publishes the current rematch ballot to every peer. Authority only.
	 *
	 * @param CurrentVotes   Participants that have asked for a rematch.
	 * @param RequiredVotes  Votes needed before the match restarts.
	 */
	void SetRematchVotes(int32 CurrentVotes, int32 RequiredVotes);

	// ============================================================
	// Public Interface (Read-only queries — usable on any peer)
	// ============================================================

	/** Returns the current match phase. */
	UFUNCTION(BlueprintPure, Category = "Duel|State")
	EMKHMatchPhase GetMatchPhase() const { return DuelState.Phase; }

	/** Returns the whole duel state snapshot (phase + timing). */
	UFUNCTION(BlueprintPure, Category = "Duel|State")
	const FMKHMatchStateData& GetDuelState() const { return DuelState; }

	/** Returns the live scoreboard. */
	UFUNCTION(BlueprintPure, Category = "Duel|State")
	const TArray<FMKHDuelPlayerData>& GetPlayerDatas() const { return PlayerDatas; }

	/** Returns the current rematch ballot (votes cast / votes needed). */
	UFUNCTION(BlueprintPure, Category = "Duel|State")
	const FMKHRematchVoteData& GetRematchVotes() const { return RematchVotes; }

	/**
	 * Re-fires OnParticipantRegistered and OnScoreChanged for every filled slot. Call it right after
	 * binding so a listener created after registration still receives the current state.
	 */
	UFUNCTION(BlueprintCallable, Category = "Duel|Events")
	void BroadcastExistingPlayerDatas();

	/**
	 * Re-fires OnMatchPhaseChanged with the current phase so a listener created after the phase was
	 * already set still reflects it. On the listen-server host the initial phase can be broadcast in
	 * the same tick the host is (re)started, before its HUD binds, so it would otherwise miss the
	 * transition; call this right after such a listener is put on screen. Safe on any peer.
	 */
	UFUNCTION(BlueprintCallable, Category = "Duel|Events")
	void BroadcastCurrentPhase();

	/** True while the current phase has an active countdown. */
	UFUNCTION(BlueprintPure, Category = "Duel|Timer")
	bool IsPhaseTimerActive() const;

	/**
	 * Seconds remaining in the current phase, computed locally from the replicated server-time
	 * deadline. Call every frame from UI for a smooth real-time countdown. Returns 0 when no timer is active.
	 */
	UFUNCTION(BlueprintPure, Category = "Duel|Timer")
	float GetPhaseTimeRemaining() const;

	/** Normalized [0,1] elapsed fraction of the current phase, for progress bars. Returns 0 when no timer is active. */
	UFUNCTION(BlueprintPure, Category = "Duel|Timer")
	float GetPhaseElapsedFraction() const;

	// ============================================================
	// Protected / Internal Logic
	// ============================================================

protected:
	/** Broadcasts the generic and granular phase events for the current phase (runs on every peer). */
	void HandlePhaseChanged();

	/** Replication callback: mirrors a server phase change onto clients. */
	UFUNCTION()
	void OnRep_DuelState();

	/** Replication callback: mirrors a server scoreboard change onto clients. */
	UFUNCTION()
	void OnRep_PlayerDatas();

	/** Replication callback: mirrors a server rematch-ballot change onto clients. */
	UFUNCTION()
	void OnRep_RematchVotes();

	// ============================================================
	// Blueprint Events (bindable on every peer)
	// ============================================================

public:
	/** Fired on any phase change, carrying the new phase. */
	UPROPERTY(BlueprintAssignable, Category = "Duel|Events")
	FMKHOnMatchPhaseChanged OnMatchPhaseChanged;

	/** Fired when the initial equip preparation phase begins. */
	UPROPERTY(BlueprintAssignable, Category = "Duel|Events")
	FMKHDuelPhaseSignature OnInitialPreparationStarted;

	/** Fired when a duel phase begins (the first duel and every subsequent one). */
	UPROPERTY(BlueprintAssignable, Category = "Duel|Events")
	FMKHDuelPhaseSignature OnDuelStarted;

	/** Fired when the duel timer expires and the sudden-death drain begins. */
	UPROPERTY(BlueprintAssignable, Category = "Duel|Events")
	FMKHDuelPhaseSignature OnDuelTimeOver;

	/** Fired when the intermission preparation phase between two duels begins. */
	UPROPERTY(BlueprintAssignable, Category = "Duel|Events")
	FMKHDuelPhaseSignature OnIntermissionPreparationStarted;

	/** Fired when a participant is registered into a scoreboard slot. */
	UPROPERTY(BlueprintAssignable, Category = "Duel|Events")
	FMKHOnParticipantRegistered OnParticipantRegistered;

	/** Fired when a participant's score changes. */
	UPROPERTY(BlueprintAssignable, Category = "Duel|Events")
	FMKHOnScoreChanged OnScoreChanged;

	/** Fired when a player dies and a point is awarded to the survivor. */
	UPROPERTY(BlueprintAssignable, Category = "Duel|Events")
	FMKHOnPlayerDied OnPlayerDied;

	/** Fired when the match ends, with the winner or a draw flag. */
	UPROPERTY(BlueprintAssignable, Category = "Duel|Events")
	FMKHOnMatchOver OnMatchOver;

	/** Fired whenever the post-match rematch ballot changes. */
	UPROPERTY(BlueprintAssignable, Category = "Duel|Events")
	FMKHOnRematchVotesChanged OnRematchVotesChanged;

	// ============================================================
	// Properties (Replicated state)
	// ============================================================

protected:
	/** Replicated phase + countdown snapshot. */
	UPROPERTY(ReplicatedUsing = OnRep_DuelState, BlueprintReadOnly, Category = "Duel|State")
	FMKHMatchStateData DuelState;

	/** Replicated scoreboard (one entry per participant). */
	UPROPERTY(ReplicatedUsing = OnRep_PlayerDatas, BlueprintReadOnly, Category = "Duel|State")
	TArray<FMKHDuelPlayerData> PlayerDatas;

	/** Replicated rematch ballot, driven by the game mode once the match is over. */
	UPROPERTY(ReplicatedUsing = OnRep_RematchVotes, BlueprintReadOnly, Category = "Duel|State")
	FMKHRematchVoteData RematchVotes;

	/** Last replicated snapshot, diffed in OnRep_PlayerDatas to tell registrations from score updates. */
	UPROPERTY()
	TArray<FMKHDuelPlayerData> PreviousPlayerDatas;
};
