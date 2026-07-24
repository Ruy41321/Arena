// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameplayEffectTypes.h"
#include "GameMode/MKHGameState.h"
#include "MKHGameMode.generated.h"

class UCharacterClassInfo;
class UProjectileInfo;
class AMKHPlayerCharacter;
class AMKHGameState;
class UGameplayEffect;
struct FGameplayEventData;

/**
 * Per-player runtime data tracked by the game mode for the duration of the match.
 * Holds the participant reference, their score and the spawn transform they respawn at
 * between rounds. Extend this struct with future per-player match data (streaks, stats, ...).
 *
 * This is the server-authoritative record; the replicated subset used by UI lives in the
 * game state (see FMKHDuelPlayerData).
 */
USTRUCT(BlueprintType)
struct FMKHPlayerData
{
	GENERATED_BODY()

	/** The player character participating in the duel. */
	UPROPERTY(BlueprintReadOnly, Category = "Duel")
	TObjectPtr<AMKHPlayerCharacter> PlayerCharacter = nullptr;

	/** Number of rounds this player has won so far. */
	UPROPERTY(BlueprintReadOnly, Category = "Duel")
	int32 Score = 0;

	/** World transform this player spawned at; used to reset position between rounds. */
	UPROPERTY(BlueprintReadOnly, Category = "Duel")
	FTransform SpawnTransform = FTransform::Identity;

	/** Handle of the sudden-death drain effect currently active on this player; invalid when no drain is applied. */
	FActiveGameplayEffectHandle SuddenDeathDrainHandle;

	/** True once this player's death has been counted in the current round; cleared on round reset. */
	bool bDiedThisRound = false;

	/** True once this slot has been assigned a valid participant. */
	bool IsValidPlayer() const { return PlayerCharacter != nullptr; }
};

/**
 * Game mode for the 1v1 arena duel (server-authoritative orchestrator).
 *
 * Match flow:
 *   InitialPreparation (equip) -> Duel -> [player dies OR duel timer -> SuddenDeath drain -> player dies]
 *   -> award point -> (death-resolve delay) -> reset positions -> IntermissionPreparation -> Duel -> ...
 *   until a player reaches ScoreToWin.
 *
 * The game mode owns the logic and timers; all replicated state and the Blueprint-bindable events
 * that UI on every peer reacts to live on AMKHGameState, which this class drives.
 */
UCLASS()
class MAKHIA_API AMKHGameMode : public AGameMode
{
	GENERATED_BODY()

	// ============================================================
	// Lifecycle
	// ============================================================

public:
	AMKHGameMode();

protected:
	/**
	 * Registers a participant once their pawn has been spawned and possessed,
	 * the first point where the controller's pawn is guaranteed valid on the server.
	 */
	virtual void RestartPlayer(AController* NewPlayer) override;

	/** Drops the leaving player's rematch vote so a stale ballot cannot restart the match. */
	virtual void Logout(AController* Exiting) override;

	// ============================================================
	// Public Interface (Class default data accessors)
	// ============================================================

public:
	/** Returns the character class default info data asset configured on this game mode. */
	UCharacterClassInfo* GetCharacterClassDefaultInfo() const;

	/** Returns the projectile info data asset configured on this game mode. */
	UProjectileInfo* GetProjectileInfo() const;

	// ============================================================
	// Public Interface (Rematch ballot — called by player controllers)
	// ============================================================

	/**
	 * Records a rematch vote for the given player and restarts the match once every participant
	 * has voted. Votes are per-player, so pressing the button twice cannot restart a duel on its own.
	 * Ignored outside the MatchOver phase.
	 *
	 * @param Voter  The controller asking for a rematch.
	 */
	void RequestRematch(APlayerController* Voter);

	// ============================================================
	// Protected / Internal Logic (Match flow)
	// ============================================================

protected:
	/** Registers a newly joined player character into the first free player slot and caches its spawn transform. */
	void RegisterParticipant(AController* NewPlayer);

	/** Starts the match once both participants have joined: enters the initial preparation phase. */
	void TryStartMatch();

	/** Enters the initial equip phase and schedules the transition to the first duel. */
	void StartInitialPreparationPhase();

	/** Enters the duel phase and schedules the sudden-death transition when the duel timer expires. */
	void StartDuelPhase();

	/** Enters the short intermission phase between duels and schedules the next duel. */
	void StartIntermissionPhase();

	/** Applies the sudden-death drain effect to both players and moves the match into the sudden-death phase. */
	void HandleDuelTimeOver();

	/**
	 * Awards a point to the opposite player on death (both deaths count on a double KO); the first
	 * death of the round stops the timers, removes the drain and schedules ResolveRound.
	 *
	 * @param Payload       The gameplay event payload (unused, required by the delegate signature).
	 * @param DeadCharacter The participant that triggered the death event.
	 */
	void HandlePlayerDeath(const FGameplayEventData* Payload, AMKHPlayerCharacter* DeadCharacter);

	/**
	 * Runs after the death-resolve delay. Ends the match once ScoreToWin is reached (equal scores
	 * mean a draw), otherwise resets both participants and starts the intermission.
	 */
	void ResolveRound();

	/**
	 * Starts a fresh match without leaving the level: wipes the scoreboard, cancels anything the
	 * previous match left running, revives both participants at their spawns and re-enters the
	 * initial preparation phase.
	 *
	 * Preferred over AGameMode::RestartGame for a rematch: no server travel means no reconnection,
	 * no reloaded level and no rebuilt HUD, so the transition is a plain phase change every peer
	 * already knows how to react to.
	 */
	void RestartMatchInPlace();

	// ============================================================
	// Protected / Internal Logic (Helpers)
	// ============================================================

protected:
	/** Sets the local authoritative phase and mirrors it (with its countdown) onto the replicated game state. */
	void SetPhase(EMKHMatchPhase NewPhase, float PhaseDurationSeconds);

	/** Returns the typed duel game state, or nullptr if it is not available yet. */
	AMKHGameState* GetDuelGameState() const;

	/** Binds the game mode's death handler to the given participant's ability system death event. */
	void BindDeathEvent(AMKHPlayerCharacter* Character);

	/** Clears any pending phase timer so a new phase can be scheduled cleanly. */
	void ClearPhaseTimer();

	/** Resets both participants to their spawn transforms and revives them for a fresh duel. */
	void ResetParticipantsForNewRound();

	/** Applies the configured round-reset gameplay effect to a single participant, if one is set. */
	void ApplyRoundResetEffect(AMKHPlayerCharacter* Character) const;

	/**
	 * Applies the sudden-death drain effect to a participant and stores the active-effect handle
	 * in the player data for later removal.
	 */
	void ApplySuddenDeathDrain(FMKHPlayerData& PlayerData) const;

	/** Removes the active sudden-death drain from every participant and invalidates the stored handles. */
	void RemoveSuddenDeathDrainFromParticipants();

	/** Returns the index (0/1) of the slot holding the given character, or INDEX_NONE if not a participant. */
	int32 FindParticipantIndex(const AMKHPlayerCharacter* Character) const;

	/**
	 * Number of votes the rematch ballot needs: one per connected player.
	 * Recomputed on every vote so a disconnect cannot leave the ballot unreachable.
	 */
	int32 CountRematchVotersNeeded() const;

	/** Pushes the current ballot onto the game state so every peer's panel can render it. */
	void PublishRematchVotes() const;

	// ============================================================
	// Properties (Designer-tunable match rules)
	// ============================================================

protected:
	/** Duration in seconds of the initial equip preparation phase. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Duel|Rules", meta = (ClampMin = "0.0"))
	float InitialPreparationDuration = 60.f;

	/** Maximum duration in seconds of a duel before sudden death triggers. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Duel|Rules", meta = (ClampMin = "0.0"))
	float DuelDuration = 180.f;

	/** Duration in seconds of the short intermission preparation phase between duels. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Duel|Rules", meta = (ClampMin = "0.0"))
	float IntermissionPreparationDuration = 10.f;

	/** Delay in seconds between a death and the round reset, giving the death animation time to play. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Duel|Rules", meta = (ClampMin = "0.0"))
	float DeathResolveDelay = 3.f;

	/** Score a participant must reach to win the match. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Duel|Rules", meta = (ClampMin = "1"))
	int32 ScoreToWin = 3;

	/**
	 * Health-drain gameplay effect applied to both players when the duel timer expires.
	 * Should reduce health over time until one participant inevitably dies.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Duel|Effects")
	TSubclassOf<UGameplayEffect> SuddenDeathDrainEffect;

	/**
	 * Gameplay effect applied to each participant when reset for a new round.
	 * Typically restores health/stamina/shield to full so the next duel starts fresh.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Duel|Effects")
	TSubclassOf<UGameplayEffect> RoundResetEffect;

	// ============================================================
	// Properties (Runtime state)
	// ============================================================

protected:
	/** The two duel participants and their per-match data (index 0 and 1). */
	UPROPERTY(BlueprintReadOnly, Category = "Duel|State")
	TArray<FMKHPlayerData> Players;

	/** Authoritative current match phase (mirrored to the game state for replication). */
	UPROPERTY(BlueprintReadOnly, Category = "Duel|State")
	EMKHMatchPhase MatchPhase = EMKHMatchPhase::None;

	/**
	 * Controllers that have asked for a rematch since the match ended.
	 * A set rather than a counter: one player spamming the button must not restart the duel alone.
	 */
	UPROPERTY()
	TSet<TObjectPtr<APlayerController>> RematchVoters;

	/** True while a death is being resolved (between the death and the round reset); blocks re-entrant death handling. */
	bool bRoundResolving = false;

	/** Timer handle driving the current phase's duration and the death-resolve delay. */
	FTimerHandle PhaseTimerHandle;

	// ============================================================
	// Properties (Class defaults data assets)
	// ============================================================

private:
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Class Defaults")
	TObjectPtr<UCharacterClassInfo> ClassDefaults;

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Projectiles")
	TObjectPtr<UProjectileInfo> ProjectileInfo;
};
